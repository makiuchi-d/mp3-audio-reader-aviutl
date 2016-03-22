/*****************************************************************************/
/*		MP3 Audio File Reader  for AviUtl ver0.98 or later
 *
 *[2004]
 *	06/20:	����J�n
 *	06/29:	�Ƃ肠���������グ���B����Ńo�O�����Ȃ炵�[�ۂ�
 *			�Ȃ�Ă��܂������킯�Ȃ����B�܂Ƃ��ɓ����˂�
 *			wfx.nAvgBytesPerSec�̌v�Z���ʂ���v�Z�ƍ���Ȃ��c
 *			�c�I�[�o�[�t���[���Ă��������corz
 *			������VirtualDubMod�̏o�͂̐��l�ƍ���Ȃ���(��ͥ)�ƼŲ!!
 *			wfx.nBlockAlign��1����Ȃ��Ƃ܂����Ȃ��B
 *			(Nandub,VirtualDubMod����VBR�̎���1152������Ă邪)
 *			�Ƃ肠���������悤�ɂ͂Ȃ����B
 *			�o�̓o�C�i�����������Ғʂ�ɓ����Ă�݂����B
 *			�ł�D&D�����Ƃ��g���qwav�ȊO����(�܂�*.mp3)
 *			���łɓǂ݂���ł���f����j�����₪��B
 *			�����ǂݍ��݂���I�ׂΖ��Ȃ����AKenkun�̎d�l�͓䂾�B
 *			�^���ł͂��̕ӂ��l�����Ă����Ȃ��ƁB
 *	06/30:	����ς肤�܂������Ȃ��BWAVEFORMATEX�̂ق���������Ȃ���
 *			AVISTREAMINFO�̕������߂��Ă��K�v������̂��c
 *			Utl�ɓn���p�����^�����łǂ��܂łł��邩�c
 *[2007]
 *	01/12:	�����̊Ԃɂ����낢���������Ă������݂����B
 *			nAvgBytesPerSec�̌v�Z���@�����P����Ă���Ƃ��B
 *			����Config�E�B���h�E�Ƃ��̃e�X�g������Ă�B
 *			�Ƃ肠�����R���p�C�����Ȃ������B
 */
/*****************************************************************************/
/*  TODO:
 * 		��ꂽ�X�g���[���������Ă����v�Ȃ悤�ɂ���
 * 		AVI�Ɋi�[���ꂽMP3�ɂ��Ή�
 * 		�t�b�_�ɂ�������ƑΉ�
 * 		�ݒ�_�C�A���O�ŁAMP3��WAV�w�b�_������@�\
 */
/*	MEMO:
 *
 * 	Nandub/VirtualDubMod���ƃ`�����N�Ƀt���[�������������Ȃ���
 * 	Utl���Ƃ�����Č�����͖̂�����
 *
 *
 * 	AVI�t�@�C���ɂ��Ή�������ׂ��H
 * 	�f���������Ă����ȍ~�̃v���O�C�����ǂ�ł���邩��\�ł͂���
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
* 		���̓v���O�C���\���̒�`
*-------------------------------------------------------------------*/
static char plugin_name[] = "MP3 Audio File Reader";
static char plugin_info[] = "MP3 Audio File Reader version ��001 by MakKi";
static char file_filter[] = "MP3AudioFile (*.mp3;*.wav)\0*.mp3;*.wav\0";

INPUT_PLUGIN_TABLE input_plugin_table = {
	INPUT_PLUGIN_FLAG_AUDIO,	//	�������T�|�[�g����
	plugin_name,            	//	�v���O�C���̖��O
	file_filter,            	//	���̓t�@�C���t�B���^
	plugin_info,            	//	�v���O�C���̏��
	NULL,                   	//	DLL�J�n���ɌĂ΂��֐��ւ̃|�C���^ (NULL�Ȃ�Ă΂�܂���)
	NULL,                   	//	DLL�I�����ɌĂ΂��֐��ւ̃|�C���^ (NULL�Ȃ�Ă΂�܂���)
	func_open,              	//	���̓t�@�C�����I�[�v������֐��ւ̃|�C���^
	func_close,             	//	���̓t�@�C�����N���[�Y����֐��ւ̃|�C���^
	func_info_get,          	//	���̓t�@�C���̏����擾����֐��ւ̃|�C���^
	NULL,                   	//	�摜�f�[�^��ǂݍ��ފ֐��ւ̃|�C���^
	func_read_audio,        	//	�����f�[�^��ǂݍ��ފ֐��ւ̃|�C���^
	NULL,                   	//	�L�[�t���[�������ׂ�֐��ւ̃|�C���^ (NULL�Ȃ�S�ăL�[�t���[��)
	func_config,                   	//	���͐ݒ�̃_�C�A���O��v�����ꂽ���ɌĂ΂��֐��ւ̃|�C���^ (NULL�Ȃ�Ă΂�܂���)
};


/*====================================================================
* 		���̓v���O�C���\���̂̃|�C���^��n���֐�
*===================================================================*/
EXTERN_C INPUT_PLUGIN_TABLE __declspec(dllexport) * __stdcall GetInputPluginTable( void )
{
	return &input_plugin_table;
}


/*--------------------------------------------------------------------
* 		�t�@�C���n���h���\����
*-------------------------------------------------------------------*/
typedef struct {
	int                  fd;    	// �t�@�C���f�B�X�N���v�^
	MPEGLAYER3WAVEFORMAT wfm;   	// MPEG Layer3 WAVEFORMATEX structure

	long          flag;         	// FH_FLAG_VBR
	unsigned long totalsize;    	// �f�[�^�S�̂̃T�C�Y(Byte)
	unsigned long totalframe;   	// �t���[����

	struct FRAMEINDEX{
		unsigned long pos;      	// �t���[���擪�A�h���X
		unsigned long size;     	// �t���[���̃T�C�Y
	} *frameidx;
} FILE_HANDLE;

#define FH_FLAG_VBR		1


/*--------------------------------------------------------------------
* 		�t�@�C���I�[�v��
*-------------------------------------------------------------------*/
INPUT_HANDLE func_open( LPSTR file )
{
	FILE_HANDLE  *fh;

	try{
		fh = (FILE_HANDLE*) calloc(1,sizeof(FILE_HANDLE));
		if(fh==NULL) throw;

		fh->fd = open(file,O_RDONLY|O_BINARY);
		if(fh->fd<0) throw;

		if(mp3_skip_fileheader(fh->fd)<0)	// RIFF/ID3v2�w�b�_���X�L�b�v
			throw;

		// �ȉ��AMP3�̃t���[���͕K���A�����Ă��邱�Ƃ�O��Ƃ���B
		// �ǂݎ��Ȃ��t���[���w�b�_�����ꂽ��I�[�ƌ��Ȃ��B
		// �܂�'TAG'��'LYRICS'�����邢��'LIST'����F����Ȃ�������v�ł���B
		// ��ꂽMP3�͓������ނȂ��Ď��ŁB
		// �t���[�r�b�g���[�g���l�����Ȃ��B���޸������B

		MP3HEADER mp3h;
		int pos;
		int size;
		unsigned long hdr;

		pos = tell(fh->fd);
		if(read(fh->fd,&hdr,sizeof(hdr))<0) throw;

		size = mp3_decompose_header(hdr,&mp3h);
		if(size <= 0)	throw;

		// �ŏ��̃t���[�����
		fh->totalsize  = size;
		fh->totalframe = 1;
		fh->flag       = 0;

		// frameidx buff�m��
		unsigned int idxbuffsize = 75000;	// �Ƃ肠����48000kHz30min��
		fh->frameidx = (FILE_HANDLE::FRAMEINDEX*) malloc(idxbuffsize * sizeof(FILE_HANDLE::FRAMEINDEX));
		if(fh->frameidx==NULL) throw;

		fh->frameidx[0].pos  = pos;
		fh->frameidx[0].size = size;

		// ���̃t���[����
		pos = lseek(fh->fd,size-sizeof(hdr),SEEK_CUR);
		read(fh->fd,&hdr,sizeof(hdr));

		while(!eof(fh->fd)){
			MP3HEADER mp3h2;

			if((size=mp3_decompose_header(hdr,&mp3h2))<0)
				break;

			if(fh->totalframe >= idxbuffsize){
				idxbuffsize += 50000;					// �Ƃ肠����
				fh->frameidx = (FILE_HANDLE::FRAMEINDEX*) realloc(fh->frameidx,idxbuffsize*sizeof(FILE_HANDLE::FRAMEINDEX));
				if(fh->frameidx==NULL) throw;
			}

			if(mp3h.samplfreq != mp3h2.samplfreq) throw;	// �σT���v�����O���[�g�͕��u

			if(mp3h.bitrate != mp3h2.bitrate)
				fh->flag |= FH_FLAG_VBR;

			fh->frameidx[fh->totalframe].pos  = pos;
			fh->frameidx[fh->totalframe].size = size;// - mp3h.padding;	//�Ƃ肠�����p�f�B���O�͕��u
			fh->totalsize += size;

			pos = lseek(fh->fd,size-sizeof(hdr),SEEK_CUR);
			if(pos<0) break;
			if(read(fh->fd,&hdr,sizeof(hdr))<=0){
				hdr = 0;
				break;
			}

			fh->totalframe++;
		}

		// MPEGLAYER3WAVEFORMAT�ݒ�
		fh->wfm.wfx.wFormatTag      = WAVE_FORMAT_MPEGLAYER3;	// (�Œ�)
		fh->wfm.wfx.nChannels       = mp3h.channel;
		fh->wfm.wfx.nSamplesPerSec  = mp3h.samplfreq;
		fh->wfm.wfx.nAvgBytesPerSec = (unsigned __int64)fh->totalsize * mp3h.samplfreq / fh->totalframe / 1152;
		fh->wfm.wfx.nBlockAlign     = 1;//(fh->flag&FH_FLAG_VBR) ? fh->totalsize / fh->totalframe : 1;
		fh->wfm.wfx.wBitsPerSample  = 0;	// (�Œ�)
		fh->wfm.wfx.cbSize          = MPEGLAYER3_WFX_EXTRA_BYTES; // (�Œ�)
		fh->wfm.wID                 = MPEGLAYER3_ID_MPEG;	// (�Œ�)
		fh->wfm.fdwFlags            = MPEGLAYER3_FLAG_PADDING_OFF;
		fh->wfm.nBlockSize          = fh->totalsize / fh->totalframe;
		fh->wfm.nFramesPerBlock     = 1;	// �Œ�?
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
* 		�t�@�C���N���[�Y
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
* 		�t�@�C���̏��
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
* 		�����ǂݍ���
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
* 		�ݒ���
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

