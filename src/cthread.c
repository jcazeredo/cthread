#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../include/support.h"
#include "../include/cthread.h"
#include "../include/cdata.h"

#define stackSize SIGSTKSZ
#define MAX_STU_CHAR 100
#define STU1 "Julio - 000 \n"
#define STU2 "Basso - 000 \n"
#define STU3 "Juan - 000 \n"

/*
TO-DO:
cidentify()
csetprio()
cJoin():
	- verificar quando termina
lookForTidinBlockedQueue()
*/

/* STUCTURE */
typedef struct threadEsperando{
	int tidEsperando;
	int tidSendoEsperada;
} threadEsperando;

int inicializado = 0;
int ultimo_tid = 1;

TCB_t *threadExecutando;
TCB_t *mainThread;

FILA2 filaAptos[3];
FILA2 filaBloqueados;
FILA2 filaTerminados;
FILA2 filaJoin;

ucontext_t threadEnd_ctx, dispatcher_ctx;

void inicializaPrincipal();
void inicializaDispatcher();
void inicializaThreadEnd();
void dispatcher();
void threadEnd();
TCB_t *proximaExecucao();
int	Insert(PFILA2 pfila, TCB_t *tcb);
void trocarEstado(PFILA2 fila, TCB_t *thread);
int checkJoin(int tid);
int procuraTidEsperando(int tid);
int tidExiste(int tid);
void printFilas();
TCB_t * procuraTidFila(PFILA2 pfila, int tid, int del);
void avisaJoin(int tid);
int retornaTidEsperando(int tid, int del);

// Retorna o TID da thread criada
int ccreate (void* (*start)(void*), void *arg, int prio) {
	

	if(!inicializado){
		//remover
		system("clear");

		inicializaPrincipal();
		inicializaDispatcher();
		inicializaThreadEnd();

		if(CreateFila2(&filaAptos[0]) != 0){
			printf("Fila de aptos de prioridade alta não foi criada\n");
			return -1;
		}

		if(CreateFila2(&filaAptos[1]) != 0){
			printf("Fila de aptos de prioridade média não foi criada\n");
			return -1;
		}

		if(CreateFila2(&filaAptos[2]) != 0){
			printf("Fila de aptos de prioridade baixa não foi criada\n");
			return -1;
		}
		
		if(CreateFila2(&filaBloqueados) != 0){
			printf("Fila de bloqueados não foi criada\n");
			return -1;
		}

		if(CreateFila2(&filaTerminados) != 0){
			printf("Fila de bloqueados não foi criada\n");
			return -1;
		}
		
		if(CreateFila2(&filaJoin) != 0){
			printf("Fila Cjoin não foi criada\n");
			return -1;
		}	

		inicializado = 1;	
	}

	TCB_t *threadCriada;
	threadCriada = (TCB_t*) malloc(sizeof(TCB_t));

	threadCriada->tid = ultimo_tid;
	threadCriada->prio = prio;
	threadCriada->state = PROCST_APTO;
	
	getcontext(&(threadCriada->context));

	threadCriada->context.uc_link = &threadEnd_ctx;
	threadCriada->context.uc_stack.ss_sp = (char*) malloc(stackSize);
	threadCriada->context.uc_stack.ss_size = stackSize;
	
	makecontext(&threadCriada->context, (void (*) (void))start, 1, arg);
	
	if(Insert(&filaAptos[prio], threadCriada) != 0){
		return -1;
	}

	printf("Thread Criada - ID: %d\n", threadCriada->tid);

	ultimo_tid++;

	// ???
	// swapcontext(&mainThread->context, &dispatcher_ctx);

	return threadCriada->tid;
}

int cyield(){
	threadExecutando->state = PROCST_APTO;

	trocarEstado(&filaAptos[0], threadExecutando);
	swapcontext(&threadExecutando->context, &dispatcher_ctx);

	return 0;	
}

int csetprio(int tid, int prio) {
	printf("Alterando prioridade da thread executando (%d) %d -> %d\n", threadExecutando->tid, threadExecutando->prio, prio);
	threadExecutando->prio = prio;
	return 0;
}

int cjoin(int tid) {
	int flagEsperando;
	threadEsperando *w;

	printf("Requisição de cjoin para tid %d\n", tid);

	// Verifica se tid existe
	if(!tidExiste(tid)){
		printf("tid %d não existe\n", tid);
		return -1;
	}

	// Procura se a tid já tá em estado terminado
	if(procuraTidFila(&filaTerminados, tid, 0) != NULL){
		printf("tid %d já ta em estado de terminado, n faz nada\n", tid);
		return 0;
	}

	flagEsperando = procuraTidEsperando(tid);

	if(!flagEsperando){
		w = (threadEsperando*) malloc(sizeof(threadEsperando));
		w->tidEsperando = threadExecutando->tid;
		w->tidSendoEsperada = tid;
		printf("cjoin (%d, %d) adicionado a fila de cjoin\n", w->tidEsperando, w->tidSendoEsperada);
		// Adiciona relação de espera
		AppendFila2(&filaJoin, w);

		// Bloqueia e chama dispatcher
		threadExecutando->state = PROCST_BLOQ;
		trocarEstado(&filaBloqueados, threadExecutando);
		swapcontext(&threadExecutando->context, &dispatcher_ctx);
		return 0;
	}

	return -1;
}

int csem_init(csem_t *sem, int count){

	sem->count = count;
	sem->fila  = (FILA2 *)malloc(sizeof(FILA2));

	if(sem->fila != NULL){
		printf("Não foi possível alocar memória\n");
		return -1;
	}

	if(CreateFila2(sem->fila) != 0){
		printf("Fila em csem_init não foi criada!\n");
		return -1;
	}

	return 0;
}

int cwait(csem_t *sem){
	sem->count--;

	if(sem->count < 0){

		threadExecutando->state = PROCST_BLOQ;

		if(AppendFila2(sem->fila, (void *) threadExecutando) != 0){
			printf("Nao foi colocada no fim de sem->fila em cwait()\n");
			return -1;
		}
		swapcontext(&threadExecutando->context, &dispatcher_ctx);	
	} 
	
	return 0;
}

int csignal(csem_t *sem) {
	sem->count++;

	if(FirstFila2(sem->fila) == 0){
		
		TCB_t *t_des = (TCB_t *) GetAtIteratorFila2(sem->fila);
		t_des->state = PROCST_APTO;
		
		trocarEstado(&filaAptos[0], t_des);
		
		if(DeleteAtIteratorFila2(sem->fila) != 0){
			printf("Nao foi deletada da fila do semaforo em csignal()\n");
			return -1;
		}
	}
		
	return 0;	
}

int cidentify(char *name, int size){
	strncpy(name, "Julio - 000 \nBasso - 000 \nJuan - 000 ", size);
	return 0;
}

/* ################### Funções Auxiliares ################### */

void inicializaPrincipal(){
	mainThread = (TCB_t*) malloc(sizeof(TCB_t));
		
	mainThread->tid = 0;	
	mainThread->prio = 2;
	mainThread->state = PROCST_EXEC;	
	
	getcontext(&mainThread->context);
	threadExecutando = mainThread;
}

void inicializaDispatcher(){
	getcontext(&dispatcher_ctx);

	dispatcher_ctx.uc_link = 0;
	dispatcher_ctx.uc_stack.ss_sp = (char*) malloc(stackSize);
	dispatcher_ctx.uc_stack.ss_size = stackSize;

	makecontext(&dispatcher_ctx, (void(*)(void))dispatcher, 0);
}

void inicializaThreadEnd(){
	getcontext(&threadEnd_ctx);

	threadEnd_ctx.uc_link = 0;
	threadEnd_ctx.uc_stack.ss_sp = (char*) malloc(stackSize);
	threadEnd_ctx.uc_stack.ss_size = stackSize;

	makecontext(&threadEnd_ctx, (void(*)(void))threadEnd, 0);
}

void dispatcher(){
	// Remover
	printf("Função dispatcher Iniciada\n");

	// Verifica qual próxima thread pra executar
	threadExecutando = proximaExecucao();
	printf("Proxima Thread: %p\n", threadExecutando);
	printFilas();
	// Remove a thread escolhida da fila de aptos
	if (threadExecutando != NULL){
		if (FirstFila2(&filaAptos[threadExecutando->prio]) == 0 && DeleteAtIteratorFila2(&filaAptos[threadExecutando->prio]) == 0)
			setcontext(&threadExecutando->context);
	}
	else{
		printf("Acabaram as threads para serem executadas\n");

		// Precisa???
		// setcontext(&mainThread->context);
	}
}

void threadEnd(){
	printf("thread tid %d finalizando\n", threadExecutando->tid);
	
	trocarEstado(&filaTerminados, threadExecutando);

	avisaJoin(threadExecutando->tid);
	// Avisa que tem thread terminada (para cjoin)

	// ???
	// free(threadExecutando);
	// threadExecutando = NULL;

	setcontext(&dispatcher_ctx);
}

TCB_t *proximaExecucao(){
	if (FirstFila2(&filaAptos[0]) == 0 && GetAtIteratorFila2(&filaAptos[0]) != NULL)
		return (TCB_t *)GetAtIteratorFila2(&filaAptos[0]);
	else if (FirstFila2(&filaAptos[1]) == 0 && GetAtIteratorFila2(&filaAptos[1]) != NULL)
		return (TCB_t *)GetAtIteratorFila2(&filaAptos[1]);
	else if (FirstFila2(&filaAptos[2]) == 0 && GetAtIteratorFila2(&filaAptos[2]) != NULL)
		return (TCB_t *)GetAtIteratorFila2(&filaAptos[2]);
	else
		return NULL;
}

int	Insert(PFILA2 pfila, TCB_t *tcb) {

	if (LastFila2(pfila) == 0) {
		return InsertAfterIteratorFila2(pfila, tcb);
	}	
	return AppendFila2(pfila, (void *)tcb);
}

void trocarEstado(PFILA2 fila, TCB_t *thread){
	if(threadExecutando != NULL){
		if(Insert(fila, thread) != 0)
			printf("trocarEstado() nao conseguiu inserir na fila!\n");
	}
}

// 
int procuraTidEsperando(int tid){
	threadEsperando *espera_i;

	// Verifica se há um elemento na fila de Cjoin
	if(FirstFila2(&filaJoin) == 0){
		// Itera sobre as relações de espera
		do{
			espera_i = (threadEsperando *)GetAtIteratorFila2(&filaJoin);

			if(espera_i->tidSendoEsperada == tid){
				printf("Já tá esperando o %d\n", tid);
				return 1;
			}
		
			espera_i = (threadEsperando *)NextFila2(&filaJoin);
		}while(espera_i == 0);
	}

	printf("Ninguém está esperando a tid %d\n", tid);

	return 0;
}

// Retorna o tid de quem está esperando a tid do parametro, e pode excluir. -1 = erro
int retornaTidEsperando(int tid, int del){
	threadEsperando *espera_i;
	int tid_esperando;

	// Verifica se há um elemento na fila de Cjoin
	if(FirstFila2(&filaJoin) == 0){
		// Itera sobre as relações de espera
		do{
			espera_i = (threadEsperando *)GetAtIteratorFila2(&filaJoin);
			tid_esperando = espera_i->tidEsperando;
			if(espera_i->tidSendoEsperada == tid){
				printf("%d tá esperando o %d\n", espera_i->tidEsperando, tid);
				if(del == 1){
					if(DeleteAtIteratorFila2(&filaJoin) != 0){
						return -1;
					}
				}
				return tid_esperando;
			}
		
			espera_i = (threadEsperando *)NextFila2(&filaJoin);
		}while(espera_i == 0);
	}

	printf("Ninguém está esperando a tid %d\n", tid);

	return -1;
}

// Procura se tid existe em alguma fila, 0 = não existe, 1 = existe
int tidExiste(int tid){
	TCB_t *thread_i;
	int i;

	// Procura na fila de aptos
	// printf("Procurando na Fila de Aptos\n");
	for(i = 0; i < 3; i++){
		if(FirstFila2(&filaAptos[i]) == 0){
			// printf("\nFila %d\n", i);
			do{
				thread_i = (TCB_t *)GetAtIteratorFila2(&filaAptos[i]);

				if(thread_i->tid == tid){
					printf("tid %d existe \n", tid);
					return 1;
				}
			
				thread_i = (TCB_t *)NextFila2(&filaAptos[i]);
			}while(thread_i == 0);
		}
	}

	// Procura na fila de bloqueados
	// printf("Procurando na Fila de Bloqueados\n");
	if(FirstFila2(&filaBloqueados) == 0){
		do{
			thread_i = (TCB_t *)GetAtIteratorFila2(&filaBloqueados);

			if(thread_i->tid == tid){
				printf("tid %d existe \n", tid);
				return 1;
			}
		
			thread_i = (TCB_t *)NextFila2(&filaBloqueados);
		}while(thread_i == 0);
	}

	// Procura na fila de terminados
	// printf("Procurando na Fila de Terminados\n");
	if(FirstFila2(&filaTerminados) == 0){
		do{
			thread_i = (TCB_t *)GetAtIteratorFila2(&filaTerminados);

			if(thread_i->tid == tid){
				printf("tid %d existe \n", tid);
				return 1;
			}
		
			thread_i = (TCB_t *)NextFila2(&filaTerminados);
		}while(thread_i == 0);
	}

	return 0;
}

void printFilas(){
	TCB_t *thread_i;
	int i;

	printf("\n\nPrintando o que existe nas filas");
	// Procura na fila de aptos
	
	for(i = 0; i < 3; i++){
		printf("\n## Fila de Aptos %d ##\n", i);
		if(FirstFila2(&filaAptos[i]) == 0){
			do{
				thread_i = (TCB_t *)GetAtIteratorFila2(&filaAptos[i]);
				printf("TID: %d\n", thread_i->tid);
				printf("Estado: %d\n\n", thread_i->state);

				thread_i = (TCB_t *)NextFila2(&filaAptos[i]);
			}while(thread_i == 0);
		}
	}

	// Procura na fila de bloqueados
	printf("\n## Fila de Bloqueados ##\n");
	if(FirstFila2(&filaBloqueados) == 0){
		do{
			thread_i = (TCB_t *)GetAtIteratorFila2(&filaBloqueados);

			printf("TID: %d\n", thread_i->tid);
			printf("Estado: %d\n\n", thread_i->state);
		
			thread_i = (TCB_t *)NextFila2(&filaBloqueados);
		}while(thread_i == 0);
	}

	// Procura na fila de terminados
	printf("\n## Fila de Terminados ##\n");
	if(FirstFila2(&filaTerminados) == 0){
		do{
			thread_i = (TCB_t *)GetAtIteratorFila2(&filaTerminados);

			printf("TID: %d\n", thread_i->tid);
			printf("Estado: %d\n\n", thread_i->state);
		
			thread_i = (TCB_t *)NextFila2(&filaTerminados);
		}while(thread_i == 0);
	}

	printf("## Executando ##\n");
	printf("TID: %d\n", threadExecutando->tid);
	printf("Estado: %d\n\n", threadExecutando->state);
} 

// NULL = não achou ou de erro, procura Tid numa fila específica e pode remover se quiser
TCB_t * procuraTidFila(PFILA2 pfila, int tid, int del){
	TCB_t *thread_i;

	// Verifica se há um elemento na fila de Cjoin
	if(FirstFila2(pfila) == 0){
		// Itera sobre as relações de espera
		do{
			thread_i = (TCB_t *)GetAtIteratorFila2(pfila);

			if(thread_i->tid == tid){
				if(del == 1){
					if(DeleteAtIteratorFila2(pfila) != 0){
						return NULL;
					}
				}
				return thread_i;
			}
		
			thread_i = (TCB_t *)NextFila2(pfila);
		}while(thread_i == 0);
	}

	return NULL;
}

// Avisa que a tid terminou e que pode liberar se alguem tava esperando
void avisaJoin(int tid){
	int tid_esperando;
	TCB_t *threadEsperando;

	tid_esperando = retornaTidEsperando(tid, 1);

	if(tid_esperando == -1){
		return;
	}

	threadEsperando = procuraTidFila(&filaBloqueados, tid_esperando, 1);
	if(threadEsperando != NULL){
		printf("Trocando %d para apto, pois %d terminou de executar (cjoin)\n", threadEsperando->tid, tid);
		threadEsperando->state = PROCST_APTO;
		trocarEstado(&filaAptos[threadEsperando->prio], threadEsperando);
	}
}