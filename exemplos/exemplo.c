
/*
 *	Programa de exemplo de uso da biblioteca cthread
 *
 *	Versão 1.0 - 14/04/2016
 *
 *	Sistemas Operacionais I - www.inf.ufrgs.br
 *
 */

#include "../include/support.h"
#include "../include/cthread.h"
#include <stdio.h>

void* func0(void *arg) {
	printf("\nEu sou a thread ID1 imprimindo %d\n\n", *((int *)arg));
	return;
}

void* func1(void *arg) {
	printf("\nEu sou a thread ID2 imprimindo %d\n\n", *((int *)arg));
	return;
}

int main(int argc, char *argv[]) {

	int	id1, id2;
	int i;

	//1
	id1 = ccreate(func0, (void *)&i, 0);
	//2
	id2 = ccreate(func1, (void *)&i, 1);
	//3

	printf("Eu sou a main apos a criacao de ID1 e ID2\n");

	// cjoin(id1);
	// csetprio(1,1);
	// cjoin(id2);

	printf("Eu sou a main voltando para terminar o programa\n");
}