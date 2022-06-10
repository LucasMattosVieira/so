#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <unistd.h>
#include <semaphore.h>

#include "fila.h"

#define MEM_SZ 4096
// #define BUFF_SZ MEM_SZ-sizeof(sem_t)-sizeof(int)

struct shared_area {
    sem_t mutex;
    Fila fila1;
    Fila fila2;
    //int busy_wait_vez;
};

int main()
{
    key_t key=5678;
	struct shared_area *shared_area_ptr;
	void *shared_memory = (void *)0;
	int shmid;

    shmid = shmget(key,MEM_SZ,0666|IPC_CREAT);
	if ( shmid == -1 )
	{
		printf("shmget falhou\n");
		exit(-1);
	}
	
	printf("shmid=%d\n",shmid);
	
	shared_memory = shmat(shmid,(void*)0,0);
	
	if (shared_memory == (void *) -1 )
	{
		printf("shmat falhou\n");
		exit(-1);
	}
		
	printf("Memoria compartilhada no endereco=%p\n", shared_memory);

    shared_area_ptr = (struct shared_area *) shared_memory;

    pid_t processos[6];
    int idProcesso = 1;

    for (int i = 0; i < 6; i++) {

        processos[i] = fork();

        if (processos[i] < 0) {
            printf("Erro ao criar processo.\n");
            exit(-1);
        } else if (processos[i] == 0) {
            idProcesso = i+2;
            break;
        }

    }

    if (idProcesso <= 3) {
        printf("Meu id: %d. Meu pid: %d\nVou gerar numeros para F1\n\n",idProcesso, getpid());
    } else if (idProcesso == 4) {
        printf("Meu id: %d. Meu pid: %d\nVou ler numeros de F1 e manda-los para 5 e 6\n\n",idProcesso, getpid());
    } else if (idProcesso == 5) {
        printf("Meu id: %d. Meu pid: %d\nVou receber numeros de 4 por pipe1 e mandar pra 7\n\n",idProcesso, getpid());
    } else if (idProcesso == 6) {
        printf("Meu id: %d. Meu pid: %d\nVou receber numeros de 4 por pipe2 e mandar pra 7\n\n",idProcesso, getpid());
    } else {
        printf("Meu id: %d. Meu pid: %d\nRecebo numeros de 5,6 e imprimo\n\n",idProcesso, getpid());
    }

    
    return 0;
}
