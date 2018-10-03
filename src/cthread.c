#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../include/support.h"
#include "../include/cthread.h"
#include "../include/cdata.h"

#define stackSize SIGSTKSZ

// Estrutura que define a relação de espera entre duas threads (cJoin)
typedef struct threadEsperando{
	int tidEsperando;
	int tidSendoEsperada;
} threadEsperando;


int inicializaGeral();
void inicializaPrincipal();
void inicializaDispatcher();
void inicializaThreadEnd();
void dispatcher();
void threadEnd();
TCB_t *proximaExecucao();
int	Insert(PFILA2 pfila, TCB_t *tcb);
int trocarEstado(PFILA2 fila, TCB_t *thread);
int checkJoin(int tid);
int procuraTidEsperando(int tid);
int tidExiste(int tid);
void printFilas();
TCB_t * procuraTidFila(PFILA2 pfila, int tid, int del);
void avisaJoin(int tid);
int retornaTidEsperando(int tid, int del);

int inicializado = 0;
int ultimo_tid = 1;

TCB_t *threadExecutando;
TCB_t *mainThread;

FILA2 filaAptos[3];
FILA2 filaBloqueados;
FILA2 filaTerminados;
FILA2 filaJoin;

ucontext_t threadEnd_ctx, dispatcher_ctx;

int ccreate(void* (*start)(void*), void *arg, int prio){
	if(inicializaGeral() == -1){
		return -1;
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

	printf("Thread Criada - TID: %d\n", threadCriada->tid);

	ultimo_tid++;

	// swapcontext(&mainThread->context, &dispatcher_ctx);

	return threadCriada->tid;
}

int cyield(){
	if(inicializaGeral() == -1){
		return -1;
	}

	printf("tid %d esta liberando voluntariamente, parabens pela humildade\n", threadExecutando->tid);
	threadExecutando->state = PROCST_APTO;

	if(trocarEstado(&filaAptos[threadExecutando->prio], threadExecutando) != 0)
		return -1;

	swapcontext(&threadExecutando->context, &dispatcher_ctx);

	return 0;	
}

int csetprio(int tid, int prio) {
	if(inicializaGeral() == -1){
		return -1;
	}

	if(prio < 0 || prio > 2){
		return -1;
	}

	printf("Alterando prioridade da thread executando (%d) %d -> %d\n", threadExecutando->tid, threadExecutando->prio, prio);
	threadExecutando->prio = prio;
	return 0;
}

int cjoin(int tid) {
	int flagEsperando;
	int existe;
	threadEsperando *w;

	if(inicializaGeral() == -1){
		return -1;
	}

	printf("Requisição de cjoin para tid %d\n", tid);

	existe = tidExiste(tid);
	// Verifica se tid existe 
	if(existe == 0){
		printf("tid %d não existe\n", tid);
		return -1;
	}

	// Se a tid já tá em estado terminado
	if(existe == 2){
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
	if(inicializaGeral() == -1){
		return -1;
	}

	sem->count = count;
	sem->fila  = (FILA2 *)malloc(sizeof(FILA2));

	if(sem->fila == NULL){
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
	if(inicializaGeral() == -1){
		return -1;
	}

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
	if(inicializaGeral() == -1){
		return -1;
	}

	sem->count++;

	if(FirstFila2(sem->fila) == 0){
		
		TCB_t *t_des = (TCB_t *) GetAtIteratorFila2(sem->fila);
		t_des->state = PROCST_APTO;
		
		if(trocarEstado(&filaAptos[t_des->prio], t_des) == -1){
			return -1;
		}
		
		if(DeleteAtIteratorFila2(sem->fila) != 0){
			printf("Nao foi deletada da fila do semaforo em csignal()\n");
			return -1;
		}
	}
		
	return 0;	
}

int cidentify(char *name, int size){
	if(name == NULL){
		return -1;
	}
	strncpy(name, "JULIO CESAR DE AZEREDO - 00285682\nJUAN SUZANO DA FONSECA - 00285689\nPEDRO MARTINS BASSO - 00285683", size);
	return 0;
}





/* ################### Funções Auxiliares ################### */

/*-------------------------------------------------------------------
Função:	Verifica se é a primeira inicialização, se não for, inicializa tudo.
Ret:	-1 = Erro | 0 = Sucesso
-------------------------------------------------------------------*/
int inicializaGeral(){
	if(!inicializado){
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
		return 0;
	}
	return 0;
}

/*-------------------------------------------------------------------
Função:	Inicializa thread principal (mainthread).
-------------------------------------------------------------------*/
void inicializaPrincipal(){
	mainThread = (TCB_t*) malloc(sizeof(TCB_t));
		
	mainThread->tid = 0;	
	mainThread->prio = 2;
	mainThread->state = PROCST_EXEC;	
	
	getcontext(&mainThread->context);
	threadExecutando = mainThread;
}

/*-------------------------------------------------------------------
Função:	Inicializa contexto do dispatcher
-------------------------------------------------------------------*/
void inicializaDispatcher(){
	getcontext(&dispatcher_ctx);

	dispatcher_ctx.uc_link = 0;
	dispatcher_ctx.uc_stack.ss_sp = (char*) malloc(stackSize);
	dispatcher_ctx.uc_stack.ss_size = stackSize;

	makecontext(&dispatcher_ctx, (void(*)(void))dispatcher, 0);
}

/*-------------------------------------------------------------------
Função:	Inicializa thread de tratamento de fim de thread.
-------------------------------------------------------------------*/
void inicializaThreadEnd(){
	getcontext(&threadEnd_ctx);

	threadEnd_ctx.uc_link = 0;
	threadEnd_ctx.uc_stack.ss_sp = (char*) malloc(stackSize);
	threadEnd_ctx.uc_stack.ss_size = stackSize;

	makecontext(&threadEnd_ctx, (void(*)(void))threadEnd, 0);
}

/*-------------------------------------------------------------------
Função:	Verifica qual é a próxima thread para executar e executa.
-------------------------------------------------------------------*/
void dispatcher(){
	// Remover
	printf("Função dispatcher Iniciada\n");

	// Verifica qual próxima thread pra executar
	threadExecutando = proximaExecucao();
	
	if(threadExecutando == NULL){
		printf("Voltando a executar mainthread\n\n");
		threadExecutando = mainThread;
		setcontext(&mainThread->context);
	}
	else{
		if (FirstFila2(&filaAptos[threadExecutando->prio]) == 0 && DeleteAtIteratorFila2(&filaAptos[threadExecutando->prio]) == 0){
			printFilas();
			setcontext(&threadExecutando->context);
		}
	}
}

/*-------------------------------------------------------------------
Função:	Método de tratamento para fim de uma thread. Troca estado para terminado
e checa se alguém estava esperando que ela terminasse.
-------------------------------------------------------------------*/
void threadEnd(){
	printf("thread tid %d finalizando\n", threadExecutando->tid);
	
	trocarEstado(&filaTerminados, threadExecutando);

	avisaJoin(threadExecutando->tid);

	setcontext(&dispatcher_ctx);
}

/*-------------------------------------------------------------------
Função:	Procura próxima thread a ser executada.
Ret:	NULL = Nenhuma Thread pra executar | Ponteiro TCB_t = Sucesso
-------------------------------------------------------------------*/
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

/*-------------------------------------------------------------------
Função:	Insere thread na fila informada.
Ret:	!= 0 => Erro | 0 = Sucesso
-------------------------------------------------------------------*/
int	Insert(PFILA2 pfila, TCB_t *tcb) {
	if (LastFila2(pfila) == 0) {
		return InsertAfterIteratorFila2(pfila, tcb);
	}

	return AppendFila2(pfila, (void *)tcb);
}

/*-------------------------------------------------------------------
Função:	Troca fila da thread.
Ret:	-1 = Erro | 0 = Sucesso
-------------------------------------------------------------------*/
int trocarEstado(PFILA2 fila, TCB_t *thread){
	if(threadExecutando != NULL){
		if(Insert(fila, thread) != 0){
			printf("trocarEstado() nao conseguiu inserir na fila!\n");
			return -1;
		}
		return 0;
	}
	return -1;
}

/*-------------------------------------------------------------------
Função:	Procura se existe alguém esperando a tid informada.
Ret:	0 = Erro/Ninguém está esperando | 1 = Tid sendo esperada.
-------------------------------------------------------------------*/
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

/*-------------------------------------------------------------------
Função:	Procura se existe alguém esperando a tid informada,
e exclui a relação de espera caso del = 1.
Ret:	-1 = Erro | Tid de quem está esperando = Sucesso
-------------------------------------------------------------------*/
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

/*-------------------------------------------------------------------
Função:	Procura se tid existe em alguma fila.
Ret:	0 = Não Existe | 1 = Existe | 2 = Existe e já terminou
-------------------------------------------------------------------*/
int tidExiste(int tid){
	if(procuraTidFila(&filaAptos[0], tid, 0) != NULL){
		return 1;
	}
	else if(procuraTidFila(&filaAptos[0], tid, 0) != NULL){
		return 1;
	}
	else if(procuraTidFila(&filaAptos[1], tid, 0) != NULL){
		return 1;
	}
	else if(procuraTidFila(&filaAptos[2], tid, 0) != NULL){
		return 1;
	}
	else if(procuraTidFila(&filaBloqueados, tid, 0) != NULL){
		return 1;
	}
	else if(procuraTidFila(&filaTerminados, tid, 0) != NULL){
		return 2;
	}

	return 0;
}

/*-------------------------------------------------------------------
Função:	Procura uma tid em uma fila informada. Se del = 1, remove a thread da fila (caso encontrada).
Ret:	Ponteiro TCB_t = Achou | NULL = Não achou, erro.
-------------------------------------------------------------------*/
TCB_t * procuraTidFila(PFILA2 pfila, int tid, int del){
	TCB_t *thread_i;

	if(FirstFila2(pfila) == 0){
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

/*-------------------------------------------------------------------
Função:	Avisa que a tid passada por argumento está em estado de terminado
e que pode liberar a thread que estava eperando por ela (caso houver).
-------------------------------------------------------------------*/
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

/*-------------------------------------------------------------------
Função:	Printa o que existe nas filas. (Não utilizado no código final)
-------------------------------------------------------------------*/
void printFilas(){
	TCB_t *thread_i;
	int i;

	// Printa fila de aptos
	for(i = 0; i < 3; i++){
		if(FirstFila2(&filaAptos[i]) == 0){
			printf("\n###### Fila de Aptos [%d] ######\n", i);
			do{
				thread_i = (TCB_t *)GetAtIteratorFila2(&filaAptos[i]);
				printf("\tTID: %d\n", thread_i->tid);
				printf("\tEstado: %d\n\n", thread_i->state);

				thread_i = (TCB_t *)NextFila2(&filaAptos[i]);
			}while(thread_i == 0);
			printf("-------------------------\n");
		}
	}


	// Printa fila de bloqueados
	if(FirstFila2(&filaBloqueados) == 0){
		printf("\n###### Fila de Bloqueados######\n");
		do{
			thread_i = (TCB_t *)GetAtIteratorFila2(&filaBloqueados);

			printf("\tTID: %d\n", thread_i->tid);
			printf("\tEstado: %d\n\n", thread_i->state);
		
			thread_i = (TCB_t *)NextFila2(&filaBloqueados);
		}while(thread_i == 0);
		printf("-------------------------\n");
	}

	

	// Printa fila de terminados
	if(FirstFila2(&filaTerminados) == 0){
		printf("\n###### Fila de Terminados ######\n");
		do{
			thread_i = (TCB_t *)GetAtIteratorFila2(&filaTerminados);

			printf("\tTID: %d\n", thread_i->tid);
			printf("\tEstado: %d\n\n", thread_i->state);
		
			thread_i = (TCB_t *)NextFila2(&filaTerminados);
		}while(thread_i == 0);
		printf("-------------------------\n");
	}

	// Printa thread a ser executada
	printf("\n###### Próxima Thread a Executar ######\n");
	printf("\tTID: %d\n", threadExecutando->tid);
	printf("\tEstado: %d\n\n", threadExecutando->state);
	printf("-------------------------\n\n");
} 