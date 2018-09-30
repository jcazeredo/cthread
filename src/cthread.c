#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../include/support.h"
#include "../include/cthread.h"
#include "../include/cdata.h"
#define stackSize SIGSTKSZ
#define tidPadrao -1

/*

*/

int inicializado = 0;
int ultimo_tid = 1;
// int returnThread = 0;

TCB_t *threadExecutando;
TCB_t *threadPrincipal;

FILA2 filaAptos[3];
FILA2 filaBloqueados;
FILA2 filaAptosSuspensos;
FILA2 filaBloqueadosSusp;
FILA2 filaCjoin;

ucontext_t threadEnd_ctx, dispatcher_ctx;

void inicializaPrincipal();
void inicializaDispatcher();
void inicializaThreadEnd();
void dispatcher();
void threadEnd();
TCB_t *proximaExecucao();
int	Insert(PFILA2 pfila, TCB_t *tcb);

// Retorna o TID da thread criada
int ccreate (void* (*start)(void*), void *arg, int prio) {
	// Remover
	printf("Função ccreate Iniciada\n");

	if(!inicializado){
		// Remover
		printf("Função ccreate Iniciada - Entrou if\n");
		
		inicializaPrincipal();
		inicializaDispatcher();
		inicializaThreadEnd();

		if(CreateFila2(&filaAptos[0]) != 0){
			printf("Fila de aptos não foi criada\n");
			return -1;
		}

		if(CreateFila2(&filaAptos[1]) != 0){
			printf("Fila de aptos não foi criada\n");
			return -1;
		}

		if(CreateFila2(&filaAptos[2]) != 0){
			printf("Fila de aptos não foi criada\n");
			return -1;
		}
		
		if(CreateFila2(&filaBloqueados) != 0){
			printf("Fila de bloqueados não foi criada\n");
			return -1;
		}	
		
		if(CreateFila2(&filaAptosSuspensos) != 0){
			printf("Fila de aptos-suspensos não foi criada\n");
			return -1;
		}
		
		if(CreateFila2(&filaBloqueadosSusp) != 0){
			printf("Fila de bloqueados-suspensos não foi criada\n");
			return -1;
		}
		
		inicializado = 1;	
	}

	TCB_t *thread;
	thread = (TCB_t*) malloc(sizeof(TCB_t));

	thread->tid = ultimo_tid;
	thread->prio = prio;
	thread->state = 0;

	getcontext(&(thread->context));

	thread->context.uc_link = &threadEnd_ctx;
	thread->context.uc_stack.ss_sp = (char*) malloc(stackSize);
	thread->context.uc_stack.ss_size = stackSize;
	
	makecontext(&(thread->context), (void (*) (void))start, 1, arg);

	if (Insert(&filaAptos[0], thread) == 0)
		printf("Nova Thread foi inserida na fila de Aptos\n");
	else{
		printf("Erro ao inserir na fila de Aptos\n");
		return -1;
	}

	printf("Thread Criada - ID: %d\n", thread->tid);

	ultimo_tid++;
	
	return thread->tid;
}

// 
int csetprio(int tid, int prio) {
	return -1;
}

//changes the executings thread state to APTO
//inserts it in the end of the queue
//calls the dispatcher to execute the next thread, saving the mains context
//variable that points the return to the mains thread after the cyield (0 -> SUCCESS; -1 -> ERROR)

int cyield(){
	exec->state = PROCST_APTO;

	changeState(&aptos, exec);
	swapcontext(&exec->context, &dispatch_ctx);

	changeState(&aptos, exec);
	swapcontext(&exec->context, &dispatch_ctx);
	
//fhudshfusdfsufsd
//dsjfosidjfsodifjs

	returnThread = 0;
	return 0;	
}

int cjoin(int tid) {
	//testeeeeeeeeeee
	return -1;
}

int csem_init(csem_t *sem, int count) {
	//nova função etcetcetc
	return -1;
}

int cwait(csem_t *sem) {
	return -1;
}

int csignal(csem_t *sem) {
	return -1;
}

int cidentify (char *name, int size) {
	strncpy (name, "Sergio Cechin - 2018/2 - Teste de compilacao.", size);
	return 0;
}


/* ################### Funções Auxiliares ################### */

void inicializaPrincipal(){
	// Remover
	printf("Função inicializaPrincipal Iniciada\n");

	threadPrincipal = (TCB_t*) malloc(sizeof(TCB_t));
		
	threadPrincipal->tid = 0;	
	threadPrincipal->prio = 0;	
	threadPrincipal->state = PROCST_EXEC;	
	
	getcontext(&threadPrincipal->context);
	threadExecutando = threadPrincipal;
}

void inicializaDispatcher(){
	// Remover
	printf("Função inicializaDispatcher Iniciada\n");

	getcontext(&dispatcher_ctx);

	dispatcher_ctx.uc_link = 0;
	dispatcher_ctx.uc_stack.ss_sp = (char*) malloc(stackSize);
	dispatcher_ctx.uc_stack.ss_size = stackSize;

	makecontext(&dispatcher_ctx, (void(*)(void))dispatcher, 0);
}

void inicializaThreadEnd(){
	// Remover
	printf("Função inicializaThreadEnd Iniciada\n");

	getcontext(&threadEnd_ctx);

	threadEnd_ctx.uc_link = 0;
	threadEnd_ctx.uc_stack.ss_sp = (char*) malloc(stackSize);
	threadEnd_ctx.uc_stack.ss_size = stackSize;

	makecontext(&threadEnd_ctx, (void(*)(void))threadEnd, 0);
}

void dispatcher(){
	// Remover
	printf("Função dispatcher Iniciada\n");

	threadExecutando = proximaExecucao();

	// threadExecutando = NULL;
	if (threadExecutando != NULL){
		if (FirstFila2(&filaAptos[0]) == 0 && DeleteAtIteratorFila2(&filaAptos[0]) == 0)
			setcontext(&threadExecutando->context);
	}
	else
		printf("Acabaram as threads para serem executadas\n");
}

void threadEnd(){
	// Remover
	printf("Função threadEnd Iniciada\n");
	
	printf("thread finalizando = %d\n", threadExecutando->tid);
		
	// lookForTidinBlockedQueue(); //serachs in the blocked queue a thread that is waiting for one that has already ended

	free(threadExecutando);
	threadExecutando = NULL;

	dispatcher();
}

TCB_t *proximaExecucao(){
	// Remover
	printf("Função proximaExecucao Iniciada\n");

	if (FirstFila2(&filaAptos[0]) == 0 && GetAtIteratorFila2(&filaAptos[0]) != NULL)
		return (TCB_t *)GetAtIteratorFila2(&filaAptos[0]);
	else
		return NULL;
}

int	Insert(PFILA2 pfila, TCB_t *tcb) {

	// Remover
	printf("Função Insert Iniciada\n");

	if (LastFila2(pfila) == 0) {
		return InsertAfterIteratorFila2(pfila, tcb);
	}	
	return AppendFila2(pfila, (void *)tcb);
}