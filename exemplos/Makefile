#
# Makefile para programas exemplo
# 

CC=gcc
LIB_DIR=../lib

# all: exemplo teste_vetor
all: exemplo
# all: exemplo mandel teste_vetor series

exemplo: exemplo.c $(LIB_DIR)/libcthread.a
	$(CC) -o exemplo exemplo.c -L$(LIB_DIR) -lcthread -Wall

# teste_vetor: teste_vetor.c $(LIB_DIR)/libcthread.a
# 	$(CC) -o teste_vetor teste_vetor.c -L$(LIB_DIR) -lcthread -Wall

# mandel: mandel.c $(LIB_DIR)/libcthread.a
# 	$(CC) -o mandel mandel.c -L$(LIB_DIR) -lcthread -Wall

# series: series.c $(LIB_DIR)/libcthread.a
# 	$(CC) -o series series.c -L$(LIB_DIR) -lcthread -Wall

clean:
	rm -rf exemplo *.o *~

# clean:
# 	rm -rf exemplo mandel teste_vetor series *.o *~