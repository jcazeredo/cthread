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
checkJoin()
csetprio()
cJoin()s
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
TCB_t *threadPrincipal;

FILA2 filaAptos[3];
FILA2 filaBloqueados;
// FILA2 filaAptosSuspensos;
// FILA2 filaBloqueadosSusp;
FILA2 filaCjoin;

ucontext_t threadEnd_ctx, dispatcher_ctx;

void inicializaPrincipal();
void inicializaDispatcher();
void inicializaThreadEnd();
void dispatcher();
void threadEnd();
TCB_t *proximaExecucao();
int	Insert(PFILA2 pfila, TCB_t *tcb);
void changeState(PFILA2 fila, TCB_t *proxThread);
int checkJoin(int tid);

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
		
		
		if(CreateFila2(&filaCjoin) != 0){
			printf("Fila Cjoin não foi criada\n");
			return -1;
		}	
		// ISSO NAO TEM NO NOSSO - basso 

		// if(CreateFila2(&filaAptosSuspensos) != 0){
		// 	printf("Fila de aptos-suspensos não foi criada\n");
		// 	return -1;
		// }
		
		// if(CreateFila2(&filaBloqueadosSusp) != 0){
		// 	printf("Fila de bloqueados-suspensos não foi criada\n");
		// 	return -1;
		// }
		
		inicializado = 1;	
	}

	TCB_t *threadCriada;
	threadCriada = (TCB_t*) malloc(sizeof(TCB_t));

	threadCriada->tid = ultimo_tid;
	threadCriada->prio = prio;
	


	threadCriada->state = PROCST_APTO;
	// threadCriada->state = 0; ?? estado 0 é criado, não deve ser apto = 1? 

	getcontext(&(threadCriada->context));

	threadCriada->context.uc_link = &threadEnd_ctx;
	threadCriada->context.uc_stack.ss_sp = (char*) malloc(stackSize);
	threadCriada->context.uc_stack.ss_size = stackSize;
	
	makecontext(&(threadCriada->context), (void (*) (void))start, 1, arg);

	
	if (Insert(&filaAptos[0], threadCriada) == 0)
		printf("Nova Thread foi inserida na fila de Aptos\n");
	else{
		printf("Erro ao inserir na fila de Aptos\n");
		return -1;
	}

	printf("Thread Criada - ID: %d\n", threadCriada->tid);

	ultimo_tid++;

	return threadCriada->tid;
}

int cyield(){
	threadExecutando->state = PROCST_APTO;

	changeState(&filaAptos[0], threadExecutando);
	swapcontext(&threadExecutando->context, &dispatcher_ctx);
	
	// returnThread = 0; inutil?
	return 0;	
}

int csetprio(int tid, int prio) {
	return -1;
}

int cjoin(int tid) {
	int flagEsperando;	
	
	if(filaCjoin.it == NULL){
		if(CreateFila2(&filaCjoin) != 0){
			printf("Fila cJoin não foi criada :(\n");
			return -1;
		}
	}
	//basso modificou
	if ((FirstFila2(&filaAptos[0]) == 0) || (FirstFila2(&filaBloqueados) == 0)){
		

		flagEsperando = ((verifyCjoin(tid, filaAptos[0])) || (verifyCjoin(tid, filaBloqueados)));

		if (flagEsperando == 0)
			return -1;

		// flagEsperando = checkJoin(tid);
		// Remover 
		//chamar aqui a função pra testar se tem mais de uma thread esperando pela mesma thread,
		// comparar lista FILA2 filaCjoin

		flagEsperando = 0; 

		if (flagEsperando == 0){
			threadExecutando->state = PROCST_BLOQ;
			changeState(&filaBloqueados, threadExecutando);
			swapcontext(&threadExecutando->context, &dispatcher_ctx);

			return 0;
		}
	}
	// tem que chamar o dispatcher aqui se pá - pra testar prio
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
		
		changeState(&filaAptos[0], t_des);
		
		if(DeleteAtIteratorFila2(sem->fila) != 0){
			printf("Nao foi deletada da fila do semaforo em csignal()\n");
			return -1;
		}
	}
		
	return 0;	
}

int cidentify(char *name, int size){
	char student[MAX_STU_CHAR] = "";

	int i;
	int st2 = strlen(STU1);
	int st3 = st2 + strlen(STU2);
	int letters;

	strcat(student, STU1);
	strcat(student, STU2);
	strcat(student, STU3);
	
	if(size >= MAX_STU_CHAR){
		for( i = 0; i < MAX_STU_CHAR;i++)
		name[i] = student[i];
		return 0;
	}

	else if( size >= 9){
		letters = size/3 - 2;
		
		for(i = 0; i < letters; i++){
			name[i] = student[i];
			name[i + letters + 1] = student[i + st2];
			name[i + 2*(letters + 1)] = student[i + st3];
		}
		
		name[letters] = '\n';
		name[2*(letters) +1]= '\n';
		name[i + 2*(letters + 1) + 1] = '\0';
	}

	return -1;
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

void changeState(PFILA2 fila, TCB_t *proxThread){

	//inserts in the fila

	if (threadExecutando != NULL)
		if(Insert(fila, proxThread) != 0)
			printf("changeState() nao conseguiu inserir na fila!\n");
}

//this function checks if there isnt more than one thread waiting for the same tid
//if positive-> 0 (Error); if negative, 1 (Success)

int checkJoin(int tid){

	return -1;
}