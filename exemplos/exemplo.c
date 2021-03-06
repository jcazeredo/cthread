
/*
 *	Programa de exemplo de uso da biblioteca cthread
 *
 *	Vers�o 1.0 - 14/04/2016
 *
 *	Sistemas Operacionais I - www.inf.ufrgs.br
 *
 */

#include "../include/support.h"
#include "../include/cthread.h"
#include <stdio.h>
#include <stdlib.h>

void* func0(void *arg) {
	printf("\n------Eu sou a thread ID1 imprimindo %d------\n\n", *((int *)arg));
	return;
}

void* func1(void *arg) {
	printf("\n------Eu sou a thread ID2 imprimindo %d------\n\n", *((int *)arg));
	return;
}

int main(int argc, char *argv[]) {

	int	id1, id2;
	int i;

	//1
	id1 = ccreate(func0, (void *)&i, 0);
	// printf("ID: %d\n", id1);
	//2
	id2 = ccreate(func1, (void *)&i, 1);
	// printf("ID: %d\n", id1)
	printf("\n------Eu sou a main apos a criacao de ID1 e ID2------\n\n");

	cjoin(id1);
	cyield();

	printf("\n------Eu sou a main apos cJoin(%d)------\n\n", id1);

	cjoin(id2);
	printf("\n------Eu sou a main apos cJoin(%d)------\n\n", id2);
	csetprio(1,1);

	printf("\n------Eu sou a main voltando para terminar o programa------\n");

	int size = 100;
	char *name = (char *)malloc(size*sizeof(char));
	cidentify(name, size);
	printf("%s\n", name);
}