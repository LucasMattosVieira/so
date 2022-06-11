#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <unistd.h>

#include "fila.h"

#define MEM_SZ 4096
// #define BUFF_SZ MEM_SZ-sizeof(sem_t)-sizeof(int)

struct shared_area {
    sem_t escrita;
    sem_t leitura;
    pid_t processos[7];
    int pipe1[2];
    int pipe2[2];
    //int busy_wait_vez;
    Fila fila1;
    Fila fila2;
};

struct shared_area *shared_area_ptr;

void handler_p4(int p);
void * p4_thread2(void *arg);

int main()
{
    key_t key=5678;
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
		
	printf("Memoria compartilhada no endereco=%p\n\n", shared_memory);

    shared_area_ptr = (struct shared_area *) shared_memory;

    if ( pipe(shared_area_ptr->pipe1) == -1 ) {
        printf("Erro pipe1"); 
        return -1; 
    }

    if ( pipe(shared_area_ptr->pipe2) == -1 ) {
        printf("Erro pipe2"); 
        return -1; 
    }

    shared_area_ptr->fila1 = criaFila();
    shared_area_ptr->fila2 = criaFila();

    for (int i = 0; i < 7; i++) {
        shared_area_ptr->processos[i] = 0;     // preenche os pids salvos na shared_memory com 0 ate que sejam criados
    }
    
    shared_area_ptr->processos[0] = getpid();

    if (sem_init((sem_t*)&shared_area_ptr->escrita, 1, 1) != 0) {
        printf("sem_init falhou\n");
        exit(-1);
    }

    if (sem_init((sem_t*)&shared_area_ptr->leitura, 1, 0) != 0) {
        printf("sem_init falhou\n");
        exit(-1);
    }

    pid_t processoCriado;
    int idProcesso = 1;

    for (int i = 0; i < 6; i++) {

        processoCriado = fork();

        if (processoCriado < 0) {
            printf("Erro ao criar processo.\n");
            exit(-1);
        } else if (processoCriado == 0) {
            idProcesso = i+2;
            shared_area_ptr->processos[i+1] = getpid();
            break;
        }

    }

    if (idProcesso <= 3) {

        printf("Meu id: %d. Meu pid: %d\nVou gerar numeros para F1\n\n",idProcesso, getpid());

        while (1) {

            sem_wait((sem_t*)&shared_area_ptr->escrita);

            if (filaCheia(&shared_area_ptr->fila1) == 0) {              // enquanto F1 tiver espaco

                int n = rand() % 1000 + 1;                              // gera um inteiro entre 1 e 1000
                insereFila(&shared_area_ptr->fila1, n);

                printf("Processo %d, pid %d, inseriu %d em F1.\n\n", idProcesso, getpid(), n);
                printf("estado fila: %d elem\n\n",shared_area_ptr->fila1.nroElementos);

                if (filaCheia(&shared_area_ptr->fila1) == 1) {
                    if (shared_area_ptr->processos[3] == 0) {
                        sleep(1);                                       // espera P4 ser criado, caso F1 encha antes
                    }        

                    if (shared_area_ptr->processos[3] > 0) {
                        if (kill(shared_area_ptr->processos[3],SIGUSR1) == -1) {    // manda P4 esvaziar F1
                            printf("P4 nao recebeu o sinal.\n");        
                        }
                        continue;                                       // vai para a proxima iteracao sem liberar o semaforo de escrita
                    }
                }
            }

            sem_post((sem_t*)&shared_area_ptr->escrita);
        }
        
    } else if (idProcesso == 4) {
        signal(SIGUSR1, handler_p4);                // define um handler do sinal SIGUSR1 para P4

        printf("\nMeu id: %d. Meu pid: %d\nVou ler numeros de F1 e manda-los para 5 e 6\n",idProcesso, getpid());

        // cria thread 2:
        pthread_t p4_t2;
        pthread_create(&p4_t2, NULL, p4_thread2, NULL);

        // thread 1:
        int n;
        while (1) {
            
            sem_wait((sem_t*)&shared_area_ptr->leitura);

            if (filaVazia(&shared_area_ptr->fila1) == 0) {

                // remove 1 valor de F1 e manda pra P5 pelo pipe1
                n = removeFila(&shared_area_ptr->fila1);
                printf("\nthread 1, leu: %d\n",n);

                if (shared_area_ptr->processos[4] == 0) {
                    sleep(1);                                       // espera P5 ser criado, caso P4 leia F1 antes
                }

                if (shared_area_ptr->processos[4] > 0) {
                    write(shared_area_ptr->pipe1[1],&n,sizeof(int));
                    printf("\nthread 1 mandou %d no pipe1\n",n);
                }

            } else {
                sem_post((sem_t*)&shared_area_ptr->escrita);    // libera o semaforo de escrita
                continue;                                       // vai para a proxima iteracao sem liberar o semaforo de leitura
            }

            sem_post((sem_t*)&shared_area_ptr->leitura);

        }
    
    } else if (idProcesso == 5) {
        printf("\nMeu id: %d. Meu pid: %d\nVou receber numeros de 4 por pipe1 e mandar pra 7\n",idProcesso, getpid());

        int n;
        while (1) {
            // le valores do pipe1
            read(shared_area_ptr->pipe1[0], &n, sizeof(int));
            printf("\nP5 recebeu %d do pipe1\n",n);
        }
        
        

    } else if (idProcesso == 6) {
        printf("\nMeu id: %d. Meu pid: %d\nVou receber numeros de 4 por pipe2 e mandar pra 7\n",idProcesso, getpid());

        int n;
        while (1) {
            // le valores do pipe2
            read(shared_area_ptr->pipe2[0], &n, sizeof(int));
            printf("\nP6 recebeu %d do pipe2\n",n);
        }

    } else {
        printf("\nMeu id: %d. Meu pid: %d\nRecebo numeros de 5,6 e imprimo\n",idProcesso, getpid());

    }

    
    return 0;
}

void handler_p4(int signum) {    
    printf("\n\nRecebi o sinal\n\n");
    sem_post((sem_t*)&shared_area_ptr->leitura);        // libera o semaforo de leitura
}

void * p4_thread2(void *arg) {
    
    int n;
    while (1) {
            
        sem_wait((sem_t*)&shared_area_ptr->leitura);

        if (filaVazia(&shared_area_ptr->fila1) == 0) {

            // remove 1 valor de F1 e manda pra P6 pelo pipe2
            n = removeFila(&shared_area_ptr->fila1);
            printf("\nthread 2, leu: %d\n",n);

            if (shared_area_ptr->processos[5] == 0) {
                sleep(1);                                       // espera P6 ser criado, caso P4 leia F1 antes
            }

            if (shared_area_ptr->processos[5] > 0) {
                write(shared_area_ptr->pipe2[1],&n,sizeof(int));
                printf("\nthread 2 mandou %d no pipe2\n",n);
            }

        } else {
            sem_post((sem_t*)&shared_area_ptr->escrita);    // libera o semaforo de escrita
            continue;                                       // vai para a proxima iteracao sem liberar o semaforo de leitura
        }

        sem_post((sem_t*)&shared_area_ptr->leitura);

    }

}