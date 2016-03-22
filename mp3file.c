/*********************************************************************
* 		MP3 File
* 
* [2004]
* 	06/20:	�w�b�_�����֐��쐬(mp3_decompose_header())
* 	06/24:	RIFF/ID3v2�w�b�_�X�L�b�v�֐��쐬(mp3file_skip_fileheader())
*********************************************************************/
#include <io.h>
#include <stdio.h>
#include <fcntl.h>
#include "mp3file.h"

#define SPEEDUP 0

/*--------------------------------------------------------------------
* 	MP3�̃t���[���̃w�b�_�𕪉�����
* 
* 	header(in)  : MP3�̃w�b�_(32bit)
* 	mp3hdr(out) : ���ʂ��󂯎��A�h���X
* 
* 	�߂�l�F�t���[���̃T�C�Y
* 			0�̂Ƃ��̓t���[�t�H�[�}�b�g(�T�C�Y�s��)
* 
* 	�G���[: �s�̒l
* 		ERR_MP3HEADER_INVALID �����Ȍ`��
*-------------------------------------------------------------------*/
int mp3_decompose_header(unsigned long header,MP3HEADER* mp3hdr)
{

	static const int mp3_bitrate[3][16] = { 		// �r�b�g���[�g�e�[�u��
		// MPEG1 Layer3
		{ 0,32000,40000,48000,56000,64000,80000,96000,112000,128000,160000,192000,224000,256000,320000, -1 },
		// MPEG2 Layer3
		{ 0, 8000,16000,24000,32000,40000,48000,56000, 64000, 80000, 96000,112000,128000,144000,160000, -1 },
		// MPEG2.5 Layer3
		{ 0, 8000,16000,24000,32000,40000,48000,56000, 64000, 80000, 96000,112000,128000,144000,160000, -1 },
	};
	static const int mp3_samplefreq[3][4] = { 		// �T���v�����O���g���e�[�u��(Hz)
		{ 44100, 48000, 32000, -1 },	// MPEG1
		{ 22050, 24000, 16000, -1 },	// MPEG2
		{ 11025, 12000,  8000, -1 } 	// MPEG2.5
	};
	static const int mp3_channel[4] = {		// �`�����l����
		2/*Stereo*/, 2/*JointSte*/, 2/*DualMono*/, 1/*Monoral*/
	};

#if !SPEEDUP
	static const int mp3_exmode[4][4] = {		// �`�����l�����[�h
		{ MP3HDR_MODE_STEREO,  MP3HDR_MODE_STEREO,  MP3HDR_MODE_STEREO,  MP3HDR_MODE_STEREO  },	// Stereo
		{ MP3HDR_MODE_JSTE,    MP3HDR_MODE_ISTE,    MP3HDR_MODE_MSSTE,   MP3HDR_MODE_IMSSTE  },	// JointStereo
		{ MP3HDR_MODE_MONORAL, MP3HDR_MODE_MONORAL, MP3HDR_MODE_MONORAL, MP3HDR_MODE_MONORAL },	// DualMonoral
		{ MP3HDR_MODE_MONORAL, MP3HDR_MODE_MONORAL, MP3HDR_MODE_MONORAL, MP3HDR_MODE_MONORAL } 	// Monoral
	};
#endif

	unsigned int idx;

	// �o�[�W��������C���[
	switch(header & 0x0000feff){
		case 0xfaff:	// MPEG1 Layer3
			mp3hdr->ID    = MP3HDR_ID_MPEG1;
			mp3hdr->layer = 3;
			break;

		case 0xf2ff:	// MPEG2 Layer3
			mp3hdr->ID    = MP3HDR_ID_MPEG2;
			mp3hdr->layer = 3;
			break;

		case 0xe2ff:	// MPEG2.5 Layer3
			mp3hdr->ID    = MP3HDR_ID_MPEG25;
			mp3hdr->layer = 3;
			break;

		default:
			return ERR_MP3HEADER_INVALID;
	}

	// CRC�ی�
	mp3hdr->crc = !(header & 0x00000100);

	// �r�b�g���[�g
	idx = (header & 0x00f00000) >>20;
	mp3hdr->bitrate = mp3_bitrate[mp3hdr->ID][idx];
	if(mp3hdr->bitrate<0)
		return ERR_MP3HEADER_INVALID;

	// �T���v�����O���g��
	idx = (header & 0x000C0000) >>18;
	mp3hdr->samplfreq = mp3_samplefreq[mp3hdr->ID][idx];
	if(mp3hdr->samplfreq<0)
		return ERR_MP3HEADER_INVALID;

	// �p�f�B���O
	mp3hdr->padding = (header & 0x00020000) >>17;

	// �`�����l����
	idx = (header & 0xc0000000)>>30;
	mp3hdr->channel = mp3_channel[idx];

#if !SPEEDUP

	//�g�����[�h
	mp3hdr->mode = mp3_exmode[idx][(header & 0x20000000)>>28];

	// ���쌠
	mp3hdr->copy = (header & 0x08000000) ? MP3HDR_COPY_COPYRIGHT : 0;

	// �I���W�i����R�s�[
	mp3hdr->copy |= (header & 0x04000000) ? MP3HDR_COPY_ORIGINAL : 0;

	// ����
	mp3hdr->emphasis = (header & 0x02000000) >>24;

#endif

	return (mp3hdr->ID?72:144)*mp3hdr->bitrate/mp3hdr->samplfreq + mp3hdr->padding;
}


#undef SPEEDUP


/*---------------------------------
* 	ID3v2�̃V���N�Z�[�t�����p
*--------------------------------*/
#define SinkSafeInt(p)	\
	(((((char *)p)[0]&0x7f)<<21)+((((char *)p)[1]&0x7f)<<14)+((((char *)p)[2]&0x7f)<<7)+(((char *)p)[3]&0x7f))

/*--------------------------------------------------------------------
* 	RIFF/IDv2�w�b�_���X�L�b�v����
* 		fd�̃t�@�C���|�C���^���w�b�_�̌��ɃZ�b�g����
* 		RIFF�w�b�_�����ID3v2�w�b�_���Ȃ��ꍇ�͉������Ȃ�
* 
* 	fd(in)  : �t�@�C���f�B�X�N���v�^
* 
* 	�߂�l�F�X�L�b�v�����o�C�g��
* 	�G���[�F-1
*-------------------------------------------------------------------*/
int mp3_skip_fileheader(int fd)
{
	unsigned long t;	// �ŏ��̃o�b�t�@

	// �����ăt�@�C���擪�Ɉړ����Ȃ�
	// lseek(fd,0,SEEK_SET);

	if(read(fd,&t,sizeof(t))<0) return -1;

	// RIFF�w�b�_��ID3v2�͋����ł��Ȃ�
	// �܂��A���݂��Ă��Ȃ����Ƃ�O��

	if(t=='RIFF'){	// RIFF�w�b�_
		unsigned short ftag;
		struct {
			unsigned long id;
			unsigned long size;
		} chunk;

		if(lseek(fd,8,SEEK_CUR)<0 || 	// seek error
			read(fd,&chunk,sizeof(chunk))<0)	// read error
			return -1;

		// �t�H�[�}�b�g�^�O�m�F(MP3:0x55)

		while(chunk.id!='fmt '){
			if( lseek(fd,chunk.size,SEEK_CUR) <0 ||
				read(fd,&chunk,sizeof(chunk)) <0 )
					return -1;
		}
		if(read(fd,&ftag,sizeof(ftag))<0)
			return -1;

		if(ftag!=0x55) return -1;	// MP3�łȂ�
		if(lseek(fd,0-sizeof(ftag),SEEK_CUR) < 0) return -1;

		// data�`�����N��T��
		while(chunk.id!='data'){
			if( lseek(fd,chunk.size,SEEK_CUR) <0 ||
				read(fd,&chunk,sizeof(chunk)) <0 )
					return -1;
		}
	}
	else if((t&0x00ffffff) == 'ID3'){	// ID3v2�^�O

		struct {
			unsigned char id3[3];
			unsigned char ver[2];
			unsigned char flag;
			unsigned char size[4];
		} id3h;

		int s;

		if(read(fd,&id3h.ver[1],sizeof(id3h)-4)<0)	// read error
			return -1;

		s = 10 + SinkSafeInt(id3h.size);

		if(id3h.flag & 0x10) s += 10;	// �t�b�_�L��

		if(lseek(fd,s-sizeof(id3h),SEEK_CUR)<0)
			return -1;
	}
	else{
		if(lseek(fd,0-sizeof(t),SEEK_CUR)<0)
			return -1;
	}

	return tell(fd);
}


/*----test*/
#if 1
#include <stdio.h>
#include <io.h>
int main(int argc,char *argv[])
{
	FILE      *fp;
	MP3HEADER mp3h;
	int       num,pos,size,skip;
	union {
		unsigned long  l;
		unsigned short s[2];
		unsigned char  c[4];
	} buff;

	if(argc<2){
		fputs("USEAGE: MP3FILE <MP3FileName>\n",stderr);
		return 0;
	}

	if((fp=fopen(argv[1],"rb"))==NULL){
		fputs("file open error.",stderr);
		return 0;
	}

	num  = 0;
	skip = 0;
	pos  = mp3_skip_fileheader(fp->fd);

	fprintf(stdout,"mp3 data start : %p\n",pos);
//	fseek(fp,pos,SEEK_SET);

	fprintf(stdout,"  NUM      POS SIZE V L c bps   kHz p C jisd C O E\n");
	do{
		if(fread(&buff,4,1,fp)!=1)
			continue;

		size = mp3_decompose_header(buff.l,&mp3h);
		if(size<0){
			fseek(fp,-3,SEEK_CUR);
			skip++;
			continue;
		}

		if(skip){
			fprintf(stdout,"skip %d byte\n",skip);
			pos += skip;
			skip = 0;
		}

		++num;

		fprintf(stdout,"%5d %08lx %4d %1d %1d %d %3d %5.1lf %1d %1d %c%c%c%c %1d %1d %1d\n",
			num,pos,size,mp3h.ID+1,mp3h.layer,mp3h.crc,
			mp3h.bitrate/1000,mp3h.samplfreq/1000.0,
			mp3h.padding,mp3h.channel,
			(mp3h.mode&MP3HDR_MODE_JOINT)?'j':'-',(mp3h.mode&MP3HDR_MODE_INTENSITY)?'i':'-',
			(mp3h.mode&MP3HDR_MODE_MIDDLESIDE)?'m':'-',(mp3h.channel==2&&mp3h.mode==MP3HDR_MODE_MONORAL)?'d':'-',
			mp3h.copy&MP3HDR_COPY_COPYRIGHT,mp3h.copy&MP3HDR_COPY_ORIGINAL,
			mp3h.emphasis
		);

		fseek(fp,size-4,SEEK_CUR);
		pos += size;

	}while(!feof(fp) && !ferror(fp));

	fclose(fp);

	if(num==0)
		fputs("MP3Frame NotFound.",stderr);

	return 0;
}

#endif

