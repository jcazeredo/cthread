#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../include/support.h"
#include "../include/cthread.h"
#include "../include/cdata.h"
#define stackSize SIGSTKSZ

int firstExecution = 1;
int numbersOfTID = 1;
int returnThread = 0;

TCB_t *exec;
TCB_t *mainThread;


FILA2 aptos;
FILA2 bloqueados;
FILA2 aptossusp;
FILA2 bloqueadossusp;
FILA2 cjoinQueue;

ucontext_t endThread, dispatch_ctx;

//teste
// Retorna o TID da thread criada
int ccreate (void* (*start)(void*), void *arg, int prio) {


	TCB_t *novaThread;
	novaThread = (TCB_t*) malloc(sizeof(TCB_t));

	novaThread->tid = 0;
	novaThread->state = 0;
	novaThread->prio = prio;

	getcontext(&(novaThread->context));

	novaThread->context.uc_link = &endThread;
	novaThread->context.uc_stack.ss_sp = (char*) malloc(stackSize);
	novaThread->context.uc_stack.ss_size = stackSize;

	makecontext(&(novaThread->context), (void (*) (void))start, 1, arg);

	setcontext(&exec->context);

	return novaThread->tid;
}

int csetprio(int tid, int prio) {
	return -1;
}

int cyield(void) {
	return -1;
}

int cjoin(int tid) {
	return -1;
}

int csem_init(csem_t *sem, int count) {
	return -1;
}

int cwait(csem_t *sem) {
	return -1;
}

int csignal(csem_t *sem) {
	return -1;
}

int cidentify (char *name, int size) {
	strncpy (name, "Sergio Cechin - 2017/1 - Teste de compilacao.", size);
	return 0;
}
