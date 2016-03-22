/*********************************************************************
* 		MP3 FILE
*********************************************************************/
#ifndef ___MP3FILE_H
#define ___MP3FILE_H

#ifdef __cplusplus
extern "C" {
#endif


/*--------------------------------------------------------------------
* 	MP3HEADER�\����
*-------------------------------------------------------------------*/
typedef struct {
	int  ID;       	// 0: MPEG1, 1: MPEG2, 2: MPEG2.5
	int  layer;    	// �����ł�3�̂�

	int  crc;      	// 1:�L  0:��

	long bitrate;  	// (bit per sec)
	long samplfreq;	// �T���v�����O���g��(Hz)

	int  padding;  	// 1:�L  0:��
	int  channel;  	// 1:Monoral 2:Stereo or DualMonoral
	int  mode;     	// �`�����l�����[�h
	               	// MODE_MONORAL :  ch=1:Monoral , ch=2:DualMonoral

	int  copy;     	// COPY_COPYRIGHT: ���쌠�t���O
	               	// COPY_ORIGINAL:  �I���W�i���t���O

	int  emphasis; 	// ����(0-3)
	               	// 0:���� 1:50/15ms 2:�\��ς� 3:CCIT J.17
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
* 	MP3�̃t���[���̃w�b�_�𕪉�����
* 
* 	header(in)  : MP3�̃w�b�_(32bit)
* 	mp3hdr(out) : ���ʂ��󂯎��A�h���X
* 
* 	�߂�l�F�t���[���̃T�C�Y
* 			0�̂Ƃ��̓t���[�t�H�[�}�b�g(�T�C�Y�s��)
* 
* 	�G���[: �s�̒l
* 		ERR_MP3HEADER_INVALID  �����܂��̓T�|�[�g���Ă��Ȃ��`��
*-------------------------------------------------------------------*/
extern int mp3_decompose_header(unsigned long header,MP3HEADER* mp3hdr);

#define ERR_MP3HEADER_INVALID  -1


/*--------------------------------------------------------------------
* 	RIFF/IDv2�w�b�_���X�L�b�v����
* 		fd�̃t�@�C���|�C���^���w�b�_�̌��ɃZ�b�g����
* 		RIFF�w�b�_�����ID3v2�w�b�_���Ȃ��ꍇ�͈ړ�����0��Ԃ�
* 
* 	fd(in)  : �t�@�C���f�B�X�N���v�^
* 
* 	�߂�l�F�X�L�b�v�����o�C�g��
* 	�G���[�F-1
*-------------------------------------------------------------------*/
extern int mp3_skip_fileheader(int fd);


#ifdef __cplusplus
}
#endif

#endif
