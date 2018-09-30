#
# Makefile de EXEMPLO
#
# OBRIGATÓRIO ter uma regra "all" para geração da biblioteca e de uma
# regra "clean" para remover todos os objetos gerados.
#
# É NECESSARIO ADAPTAR ESSE ARQUIVO de makefile para suas necessidades.
#  1. Cuidado com a regra "clean" para não apagar o "support.o"
#
# OBSERVAR que as variáveis de ambiente consideram que o Makefile está no diretótio "cthread"
# 

CC=gcc
LIB_DIR=./lib/
INC_DIR=./include/
BIN_DIR=./bin/
SRC_DIR=./src/
CFLAGS=-m32 -c

all: directory lib

directory:
	mkdir lib -p -v

lib: cthread.o
	ar crs $(LIB_DIR)libcthread.a $(BIN_DIR)support.o $(BIN_DIR)cthread.o

cthread.o: $(SRC_DIR)cthread.c $(INC_DIR)cdata.h $(INC_DIR)cthread.h $(INC_DIR)support.h
	$(CC) $(CFLAGS) $(SRC_DIR)cthread.c -Wall -o $(BIN_DIR)cthread.o

clean:
	find $(BIN_DIR) $(LIB_DIR) -type f ! -name 'support.o' -delete

