######################################################################
#	MakeFile for Borland C/C++ Compiler
#

CC  = bcc32
LN  = bcc32
RL  = brc32
RC  = brcc32

CFLAG = -c -O1 -O2 -Oc -Oi -Ov
LFLAG = -tWD -O1 -O2
RFLAG = 

EXE = mp3_input.aui
OBJ = mp3_input.obj mp3file.obj


all: $(EXE)

$(EXE): $(OBJ)
	$(LN) $(LFLAG) -e$(EXE) $(OBJ)


mp3_input.obj: mp3_input.cpp input.h mp3file.h
	$(CC) $(CFLAG) mp3_input.cpp

mp3file.obj: mp3file.c mp3file.h
	$(CC) $(CFLAG) mp3file.c

