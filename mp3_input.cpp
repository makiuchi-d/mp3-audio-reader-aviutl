/*****************************************************************************/
/*		MP3 Audio File Reader  for AviUtl ver0.98 or later
 *
 *[2004]
 *	06/20:	製作開始
 *	06/29:	とりあえず書き上げた。これでバグ無しならしーぽん
 *			なんてうまくいくわけないか。まともに動かねぇ
 *			wfx.nAvgBytesPerSecの計算結果が手計算と合わない…
 *			…オーバーフローしてただけか…orz
 *			微妙にVirtualDubModの出力の数値と合わないが(･∀･)ｷﾆｼﾅｲ!!
 *			wfx.nBlockAlignは1じゃないとまずいなぁ。
 *			(Nandub,VirtualDubModだとVBRの時は1152入れられてるが)
 *			とりあえず動くようにはなった。
 *			出力バイナリ見る限り期待通りに動いてるみたい。
 *			でもD&Dしたとき拡張子wav以外だと(つまり*.mp3)
 *			すでに読みこんである映像を破棄しやがる。
 *			音声読み込みから選べば問題ないが、Kenkunの仕様は謎だ。
 *			某企画ではその辺も考慮しておかないと。
 *	06/30:	やっぱりうまくいかない。WAVEFORMATEXのほうだけじゃなくて
 *			AVISTREAMINFOの方も調節してやる必要があるのか…
 *			Utlに渡すパラメタだけでどこまでできるか…
 *[2007]
 *	01/12:	何時の間にかいろいろ手を加えてあったみたい。
 *			nAvgBytesPerSecの計算方法が改善されてたりとか。
 *			あとConfigウィンドウとかのテストもやってる。
 *			とりあえずコンパイルしなおした。
 */
/*****************************************************************************/
/*  TODO:
 * 		壊れたストリームがあっても大丈夫なようにする
 * 		AVIに格納されたMP3にも対応
 * 		フッダにもきちんと対応
 * 		設定ダイアログで、MP3にWAVヘッダをつける機能
 */
/*	MEMO:
 *
 * 	Nandub/VirtualDubModだとチャンクにフレームを一つしか入れないが
 * 	Utlだとそれを再現するのは無理ぽ
 *
 *
 * 	AVIファイルにも対応させるべき？
 * 	映像無視しても次以降のプラグインが読んでくれるから可能ではある
 *
 *
 */

#include <windows.h>
#include <mmreg.h>
#include <stdio.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>
#include "input.h"
#include "mp3file.h"


/*--------------------------------------------------------------------
* 		入力プラグイン構造体定義
*-------------------------------------------------------------------*/
static char plugin_name[] = "MP3 Audio File Reader";
static char plugin_info[] = "MP3 Audio File Reader version α001 by MakKi";
static char file_filter[] = "MP3AudioFile (*.mp3;*.wav)\0*.mp3;*.wav\0";

INPUT_PLUGIN_TABLE input_plugin_table = {
	INPUT_PLUGIN_FLAG_AUDIO,	//	音声をサポートする
	plugin_name,            	//	プラグインの名前
	file_filter,            	//	入力ファイルフィルタ
	plugin_info,            	//	プラグインの情報
	NULL,                   	//	DLL開始時に呼ばれる関数へのポインタ (NULLなら呼ばれません)
	NULL,                   	//	DLL終了時に呼ばれる関数へのポインタ (NULLなら呼ばれません)
	func_open,              	//	入力ファイルをオープンする関数へのポインタ
	func_close,             	//	入力ファイルをクローズする関数へのポインタ
	func_info_get,          	//	入力ファイルの情報を取得する関数へのポインタ
	NULL,                   	//	画像データを読み込む関数へのポインタ
	func_read_audio,        	//	音声データを読み込む関数へのポインタ
	NULL,                   	//	キーフレームか調べる関数へのポインタ (NULLなら全てキーフレーム)
	func_config,                   	//	入力設定のダイアログを要求された時に呼ばれる関数へのポインタ (NULLなら呼ばれません)
};


/*====================================================================
* 		入力プラグイン構造体のポインタを渡す関数
*===================================================================*/
EXTERN_C INPUT_PLUGIN_TABLE __declspec(dllexport) * __stdcall GetInputPluginTable( void )
{
	return &input_plugin_table;
}


/*--------------------------------------------------------------------
* 		ファイルハンドル構造体
*-------------------------------------------------------------------*/
typedef struct {
	int                  fd;    	// ファイルディスクリプタ
	MPEGLAYER3WAVEFORMAT wfm;   	// MPEG Layer3 WAVEFORMATEX structure

	long          flag;         	// FH_FLAG_VBR
	unsigned long totalsize;    	// データ全体のサイズ(Byte)
	unsigned long totalframe;   	// フレーム数

	struct FRAMEINDEX{
		unsigned long pos;      	// フレーム先頭アドレス
		unsigned long size;     	// フレームのサイズ
	} *frameidx;
} FILE_HANDLE;

#define FH_FLAG_VBR		1


/*--------------------------------------------------------------------
* 		ファイルオープン
*-------------------------------------------------------------------*/
INPUT_HANDLE func_open( LPSTR file )
{
	FILE_HANDLE  *fh;

	try{
		fh = (FILE_HANDLE*) calloc(1,sizeof(FILE_HANDLE));
		if(fh==NULL) throw;

		fh->fd = open(file,O_RDONLY|O_BINARY);
		if(fh->fd<0) throw;

		if(mp3_skip_fileheader(fh->fd)<0)	// RIFF/ID3v2ヘッダをスキップ
			throw;

		// 以下、MP3のフレームは必ず連続していることを前提とする。
		// 読み取れないフレームヘッダが現れたら終端と見なす。
		// まぁ'TAG'も'LYRICS'もあるいは'LIST'も誤認されないから大丈夫でしょ。
		// 壊れたMP3は投げ込むなって事で。
		// フリービットレートも考慮しない。ﾏﾝﾄﾞｸｻｲから。

		MP3HEADER mp3h;
		int pos;
		int size;
		unsigned long hdr;

		pos = tell(fh->fd);
		if(read(fh->fd,&hdr,sizeof(hdr))<0) throw;

		size = mp3_decompose_header(hdr,&mp3h);
		if(size <= 0)	throw;

		// 最初のフレーム情報
		fh->totalsize  = size;
		fh->totalframe = 1;
		fh->flag       = 0;

		// frameidx buff確保
		unsigned int idxbuffsize = 75000;	// とりあえず48000kHz30min分
		fh->frameidx = (FILE_HANDLE::FRAMEINDEX*) malloc(idxbuffsize * sizeof(FILE_HANDLE::FRAMEINDEX));
		if(fh->frameidx==NULL) throw;

		fh->frameidx[0].pos  = pos;
		fh->frameidx[0].size = size;

		// 次のフレームへ
		pos = lseek(fh->fd,size-sizeof(hdr),SEEK_CUR);
		read(fh->fd,&hdr,sizeof(hdr));

		while(!eof(fh->fd)){
			MP3HEADER mp3h2;

			if((size=mp3_decompose_header(hdr,&mp3h2))<0)
				break;

			if(fh->totalframe >= idxbuffsize){
				idxbuffsize += 50000;					// とりあえず
				fh->frameidx = (FILE_HANDLE::FRAMEINDEX*) realloc(fh->frameidx,idxbuffsize*sizeof(FILE_HANDLE::FRAMEINDEX));
				if(fh->frameidx==NULL) throw;
			}

			if(mp3h.samplfreq != mp3h2.samplfreq) throw;	// 可変サンプリングレートは放置

			if(mp3h.bitrate != mp3h2.bitrate)
				fh->flag |= FH_FLAG_VBR;

			fh->frameidx[fh->totalframe].pos  = pos;
			fh->frameidx[fh->totalframe].size = size;// - mp3h.padding;	//とりあえずパディングは放置
			fh->totalsize += size;

			pos = lseek(fh->fd,size-sizeof(hdr),SEEK_CUR);
			if(pos<0) break;
			if(read(fh->fd,&hdr,sizeof(hdr))<=0){
				hdr = 0;
				break;
			}

			fh->totalframe++;
		}

		// MPEGLAYER3WAVEFORMAT設定
		fh->wfm.wfx.wFormatTag      = WAVE_FORMAT_MPEGLAYER3;	// (固定)
		fh->wfm.wfx.nChannels       = mp3h.channel;
		fh->wfm.wfx.nSamplesPerSec  = mp3h.samplfreq;
		fh->wfm.wfx.nAvgBytesPerSec = (unsigned __int64)fh->totalsize * mp3h.samplfreq / fh->totalframe / 1152;
		fh->wfm.wfx.nBlockAlign     = 1;//(fh->flag&FH_FLAG_VBR) ? fh->totalsize / fh->totalframe : 1;
		fh->wfm.wfx.wBitsPerSample  = 0;	// (固定)
		fh->wfm.wfx.cbSize          = MPEGLAYER3_WFX_EXTRA_BYTES; // (固定)
		fh->wfm.wID                 = MPEGLAYER3_ID_MPEG;	// (固定)
		fh->wfm.fdwFlags            = MPEGLAYER3_FLAG_PADDING_OFF;
		fh->wfm.nBlockSize          = fh->totalsize / fh->totalframe;
		fh->wfm.nFramesPerBlock     = 1;	// 固定?
		fh->wfm.nCodecDelay         = 0;

/*
char str[2048];
wsprintf(str,"totalsize    \t%d\ntotalframe  \t%d\n\n"
"[MP3HEADER]\nID       \t%d\nlayer    \t%d\nbitrate  \t%d\nsamplfreq\t%d\npadding  \t%d\n\n"
"[MPEGLAYER3WAVEFORMAT]\nwFormatTag     \t%04X\nnChannels      \t%d\n"
"nSamplesPerSec \t%d\nnAvgBytesPerSec\t%d\nnBlockAlign    \t%d\nwBitsPerSample \t%d\ncbSize         \t%d\n"
"[EXTRA]\nwID            \t%d\nfdwFlags       \t%d\nnBlockSize     \t%d\nnFramesPerBlock\t%d\nnCodecDelay    \t%d\n",
fh->totalsize,fh->totalframe,
mp3h.ID,mp3h.layer,mp3h.bitrate,mp3h.samplfreq,mp3h.padding,
fh->wfm.wfx.wFormatTag,fh->wfm.wfx.nChannels,fh->wfm.wfx.nSamplesPerSec,fh->wfm.wfx.nAvgBytesPerSec,fh->wfm.wfx.nBlockAlign,
fh->wfm.wfx.wBitsPerSample,fh->wfm.wfx.cbSize,fh->wfm.wID,fh->wfm.fdwFlags,fh->wfm.nBlockSize,fh->wfm.nFramesPerBlock,fh->wfm.nCodecDelay);
MessageBox(NULL,str,"MP3AudioReader",MB_OK);
*/
	}
	catch(...){
		func_close(fh);
		return NULL;
	}

	return fh;
}


/*--------------------------------------------------------------------
* 		ファイルクローズ
*-------------------------------------------------------------------*/
BOOL func_close( INPUT_HANDLE ih )
{
	FILE_HANDLE	*fh = (FILE_HANDLE *)ih;

	if(fh){
		if(fh->fd>=0) close(fh->fd);
		if(fh->frameidx) free(fh->frameidx);
		free(fh);
	}

	return TRUE;
}


/*--------------------------------------------------------------------
* 		ファイルの情報
*-------------------------------------------------------------------*/
BOOL func_info_get( INPUT_HANDLE ih,INPUT_INFO *iip )
{
//MessageBox(NULL,"func_info_get","MP3Reader",MB_OK);

	FILE_HANDLE& fh = *(FILE_HANDLE *)ih;
	memset(iip,0,sizeof(*iip));
	iip->flag = INPUT_INFO_FLAG_AUDIO;
	iip->audio_n = fh.totalsize;// (fh.flag&FH_FLAG_VBR)?fh.totalframe:fh.totalsize;
	iip->audio_format = (WAVEFORMATEX*) &fh.wfm;
	iip->audio_format_size = sizeof(fh.wfm);

/*
char str[2048];
wsprintf(str,"totalsize(naudio_n)\t%d\ntotalframe         \t%d\n"
"[MPEGLAYER3WAVEFORMAT]\nwFormatTag     \t%04X\nnChannels      \t%d\n"
"nSamplesPerSec \t%d\nnAvgBytesPerSec\t%d\nnBlockAlign    \t%d\nwBitsPerSample \t%d\ncbSize         \t%d\n"
"[EXTRA]\nwID            \t%d\nfdwFlags       \t%d\nnBlockSize     \t%d\nnFramesPerBlock\t%d\nnCodecDelay    \t%d\n",
fh.totalsize,fh.totalframe,
fh.wfm.wfx.wFormatTag,fh.wfm.wfx.nChannels,fh.wfm.wfx.nSamplesPerSec,fh.wfm.wfx.nAvgBytesPerSec,fh.wfm.wfx.nBlockAlign,
fh.wfm.wfx.wBitsPerSample,fh.wfm.wfx.cbSize,fh.wfm.wID,fh.wfm.fdwFlags,fh.wfm.nBlockSize,fh.wfm.nFramesPerBlock,fh.wfm.nCodecDelay);
MessageBox(NULL,str,"MP3AudioReader",MB_OK);
*/

	return TRUE;
}


/*--------------------------------------------------------------------
* 		音声読み込み
*-------------------------------------------------------------------*/
int func_read_audio( INPUT_HANDLE ih,int start,int length,void *buf)
{
	FILE_HANDLE& fh = *(FILE_HANDLE *)ih;
	int r;

//	if(fh.flag & FH_FLAG_VBR){
		char *p = (char*)buf;
		unsigned int i;
		unsigned int st,ed;

		st = (unsigned __int64)start * fh.totalframe / fh.totalsize;
		ed = (unsigned __int64)(start+length) * fh.totalframe / fh.totalsize;

		for(i=st;i<ed;i++){
			if(i>=fh.totalframe) break;
			lseek(fh.fd,fh.frameidx[i].pos,SEEK_SET);
			read(fh.fd,p,fh.frameidx[i].size);
			p += fh.frameidx[i].size;
		}

		r = (unsigned long)p - (unsigned long)buf;

/*
static int show = ~IDCANCEL;
if(show!=IDCANCEL){
char str[256];
wsprintf(str,"start:%d\nlength:%d\nst:%d\ned:%d\nreturn:%d",start,length,st,ed,r);
show=MessageBox(NULL,str,"MP3AudioReader",MB_OKCANCEL);
}
*/

//	}
//	else {	// CBR
//		lseek(fh.fd,start + fh.frameidx[0].pos,SEEK_SET);
//		r = read(fh.fd,buf,length);
//	}

	return r;
}


/*--------------------------------------------------------------------
* 		設定画面
*-------------------------------------------------------------------*/
BOOL func_config( HWND hwnd,HINSTANCE dll_hinst )
{
	HANDLE h;
	char str[MAX_PATH];
	char *p;

	GetModuleFileName(NULL,str,MAX_PATH-5);
	p = strrchr(str,'.');
	if(p) strcpy(p,".ini");
	else  strcat(str,".ini");

	MessageBox(hwnd,str,"MP3 INPUT CONFIG",MB_OK);

	return TRUE;
}

