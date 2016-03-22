/*********************************************************************
* 		MP3 FILE
*********************************************************************/
#ifndef ___MP3FILE_H
#define ___MP3FILE_H

#ifdef __cplusplus
extern "C" {
#endif


/*--------------------------------------------------------------------
* 	MP3HEADER構造体
*-------------------------------------------------------------------*/
typedef struct {
	int  ID;       	// 0: MPEG1, 1: MPEG2, 2: MPEG2.5
	int  layer;    	// ここでは3のみ

	int  crc;      	// 1:有  0:無

	long bitrate;  	// (bit per sec)
	long samplfreq;	// サンプリング周波数(Hz)

	int  padding;  	// 1:有  0:無
	int  channel;  	// 1:Monoral 2:Stereo or DualMonoral
	int  mode;     	// チャンネルモード
	               	// MODE_MONORAL :  ch=1:Monoral , ch=2:DualMonoral

	int  copy;     	// COPY_COPYRIGHT: 著作権フラグ
	               	// COPY_ORIGINAL:  オリジナルフラグ

	int  emphasis; 	// 強調(0-3)
	               	// 0:強調 1:50/15ms 2:予約済み 3:CCIT J.17
} MP3HEADER;

#define MP3HDR_ID_MPEG1         0
#define MP3HDR_ID_MPEG2         1
#define MP3HDR_ID_MPEG25        2
#define MP3HDR_MODE_MONORAL     0
#define MP3HDR_MODE_STEREO      1
#define MP3HDR_MODE_JOINT       2
#define MP3HDR_MODE_INTENSITY   4
#define MP3HDR_MODE_MIDDLESIDE  8
#define MP3HDR_MODE_JSTE        (MP3HDR_MODE_STEREO|MP3HDR_MODE_JOINT)
#define MP3HDR_MODE_ISTE        (MP3HDR_MODE_STEREO|MP3HDR_MODE_JOINT|MP3HDR_MODE_INTENSITY)
#define MP3HDR_MODE_MSSTE       (MP3HDR_MODE_STEREO|MP3HDR_MODE_JOINT|MP3HDR_MODE_MIDDLESIDE)
#define MP3HDR_MODE_IMSSTE      (MP3HDR_MODE_STEREO|MP3HDR_MODE_JOINT|MP3HDR_MODE_INTENSITY|MP3HDR_MODE_MIDDLESIDE)
#define MP3HDR_COPY_COPYRIGHT   1
#define MP3HDR_COPY_ORIGINAL    2


/*--------------------------------------------------------------------
* 	MP3のフレームのヘッダを分解する
* 
* 	header(in)  : MP3のヘッダ(32bit)
* 	mp3hdr(out) : 結果を受け取るアドレス
* 
* 	戻り値：フレームのサイズ
* 			0のときはフリーフォーマット(サイズ不明)
* 
* 	エラー: 不の値
* 		ERR_MP3HEADER_INVALID  無効またはサポートしていない形式
*-------------------------------------------------------------------*/
extern int mp3_decompose_header(unsigned long header,MP3HEADER* mp3hdr);

#define ERR_MP3HEADER_INVALID  -1


/*--------------------------------------------------------------------
* 	RIFF/IDv2ヘッダをスキップする
* 		fdのファイルポインタをヘッダの後ろにセットする
* 		RIFFヘッダおよびID3v2ヘッダがない場合は移動せず0を返す
* 
* 	fd(in)  : ファイルディスクリプタ
* 
* 	戻り値：スキップしたバイト数
* 	エラー：-1
*-------------------------------------------------------------------*/
extern int mp3_skip_fileheader(int fd);


#ifdef __cplusplus
}
#endif

#endif
