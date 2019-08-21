CC	:= gcc
OPT	:= -g -O0 -Wall
LIB	:= -lm
INC	:= -Isrc							\
       -Isrc/notes						\
       -Isrc/moog						\
       -Isrc/moog/low_pass				\
       -Isrc/moog/enveloppe				\
       -Isrc/moog/generators			\
       -Isrc/parsing					\
       -Isrc/wav_writer
SRC	:= src/lilymoog.c					\
       src/notes/notes.c				\
       src/moog/moog.c					\
       src/moog/low_pass/low_pass.c		\
       src/moog/enveloppe/adsr.c		\
       src/moog/generators/saw_gen.c	\
       src/moog/generators/sine_gen.c	\
       src/moog/generators/square_gen.c	\
       src/moog/generators/wave_gen.c	\
       src/parsing/seq_parser.c			\
       src/parsing/cfg_parser.c			\
       src/wav_writer/wav_writer.c
OUT	:= lilymoog

all:
	@$(CC) $(OPT) $(INC) $(SRC) $(LIB) -o $(OUT)

clean:
	@if [ -f $(OUT) ]; then rm -rf $(OUT); fi
