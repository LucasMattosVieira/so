#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "fila.h"

#define MEM_SZ 4096

struct shared_area1 {
    sem_t escrita;
    sem_t leitura;
    pid_t processos[7];
    int pipe1[2];
    int pipe2[2];
    int busy_wait_vez;
    int contador;
    int contador_P5;
    int contador_P6;
    Fila fila1;
    Fila fila2;
    struct timespec clock_start;
    struct timespec clock_end;
    int menor;
    int maior;  
};

struct shared_area2 {
    int valores[1000];
};

struct shared_area1 *shared_area1_ptr;
struct shared_area2 *shared_area2_ptr;

// prototipos
void handler_p4(int p);
void * p4(void *arg);
void * p7(void *arg);
int escolheVez();
void relatorio();
int maisRepetido(int array[], int size);

int main()
{
    struct timespec start_p1;
    clock_gettime(CLOCK_REALTIME, &start_p1);

    key_t key1=1234, key2=5678;
	void *shared_memory1 = (void *)0;
    void *shared_memory2 = (void *)0;
	int shmid1, shmid2;

    shmid1 = shmget(key1,MEM_SZ,0666|IPC_CREAT);
	if ( shmid1 == -1 )
	{
		printf("shmget falhou\n");
		exit(-1);
	}

    shmid2 = shmget(key2,MEM_SZ,0666|IPC_CREAT);
	if ( shmid2 == -1 )
	{
		printf("shmget falhou\n");
		exit(-1);
	}
	
	//printf("shmid1=%d\n",shmid1);
    //printf("shmid2=%d\n",shmid2);
	
	shared_memory1 = shmat(shmid1,(void*)0,0);
    shared_memory2 = shmat(shmid2,(void*)0,0);
	
	if (shared_memory1 == (void *) -1 )
	{
		printf("shmat falhou\n");
		exit(-1);
	}

    if (shared_memory2 == (void *) -1 )
	{
		printf("shmat falhou\n");
		exit(-1);
	}
		
	//printf("\nMemoria compartilhada no endereco=%p\n\n", shared_memory1);
    //printf("\nMemoria compartilhada no endereco=%p\n\n", shared_memory2);

    shared_area1_ptr = (struct shared_area1 *) shared_memory1;
    shared_area2_ptr = (struct shared_area2 *) shared_memory2;
    
    shared_area1_ptr->clock_start = start_p1;

    if ( pipe(shared_area1_ptr->pipe1) == -1 ) {
        printf("Erro pipe1"); 
        return -1; 
    }

    if ( pipe(shared_area1_ptr->pipe2) == -1 ) {
        printf("Erro pipe2"); 
        return -1; 
    }

    shared_area1_ptr->fila1 = criaFila();
    shared_area1_ptr->fila2 = criaFila();

    for (int i = 0; i < 7; i++) {
        shared_area1_ptr->processos[i] = 0;     // preenche os pids salvos na shared_memory1 com 0 ate que sejam criados
    }
    
    shared_area1_ptr->processos[0] = getpid();

    if (sem_init((sem_t*)&shared_area1_ptr->escrita, 1, 1) != 0) {
        printf("sem_init falhou\n");
        exit(-1);
    }

    if (sem_init((sem_t*)&shared_area1_ptr->leitura, 1, 0) != 0) {
        printf("sem_init falhou\n");
        exit(-1);
    }

    shared_area1_ptr->busy_wait_vez = escolheVez();

    shared_area1_ptr->contador = 0;
    shared_area1_ptr->contador_P5 = 0;
    shared_area1_ptr->contador_P6 = 0;

    shared_area1_ptr->menor = 1000;
    shared_area1_ptr->maior = 1;

    for (int i = 0; i < 1000; i++) {
        shared_area2_ptr->valores[i] = 0;
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
            shared_area1_ptr->processos[i+1] = getpid();
            break;
        }

    }

    if (idProcesso <= 3) {          // P1, P2 e P3

        while (1) {

            sem_wait((sem_t*)&shared_area1_ptr->escrita);

            if (filaCheia(&shared_area1_ptr->fila1) == 0) {              // enquanto F1 tiver espaco

                int valor_123 = rand() % 1000 + 1;                              // gera um inteiro entre 1 e 1000
                insereFila(&shared_area1_ptr->fila1, valor_123);

                if (filaCheia(&shared_area1_ptr->fila1) == 1) {
                    if (shared_area1_ptr->processos[3] == 0) {
                        sleep(1);                                       // espera P4 ser criado, caso F1 encha antes
                    }        

                    if (shared_area1_ptr->processos[3] > 0) {
                        if (kill(shared_area1_ptr->processos[3],SIGUSR1) == -1) {    // manda P4 esvaziar F1
                            printf("P4 nao recebeu o sinal.\n");        
                        }
                        continue;                                       // vai para a proxima iteracao sem liberar o semaforo de escrita
                    }
                }
            }

            sem_post((sem_t*)&shared_area1_ptr->escrita);
        }
        
    } else if (idProcesso == 4) {       // P4
        
        signal(SIGUSR1, handler_p4);                // define um handler do sinal SIGUSR1 para P4
        
        pthread_t p4_thread2;
        int p4_qual_thread1 = 1, p4_qual_thread2 = 2;
        void *p4_thread1_ptr, *p4_thread2_ptr;
        p4_thread1_ptr = &p4_qual_thread1;
        p4_thread2_ptr = &p4_qual_thread2;

        // cria thread 2:
        pthread_create(&p4_thread2, NULL, p4, p4_thread2_ptr);

        // thread 1:
        p4(p4_thread1_ptr);
    
    } else if (idProcesso == 5) {       // P5
        
        int valor_5;
        while (1) {

            // le valores do pipe1
            read(shared_area1_ptr->pipe1[0], &valor_5, sizeof(int));

            // espera a vez de escrever em F2
            while (1) {
                if (shared_area1_ptr->busy_wait_vez == 1) {
                     if (filaCheia(&shared_area1_ptr->fila2) == 0) {
                        insereFila(&shared_area1_ptr->fila2, valor_5);
                        shared_area1_ptr->contador_P5++;
                        break;
                    } else {
                        shared_area1_ptr->busy_wait_vez = escolheVez();
                    }
                }
            }

        }
        
        

    } else if (idProcesso == 6) {       // P6
        
        int valor_6;
        while (1) {

            // le valores do pipe2
            read(shared_area1_ptr->pipe2[0], &valor_6, sizeof(int));

            // espera a vez de escrever em F2
            while (1) {
                if (shared_area1_ptr->busy_wait_vez == 2) {
                     if (filaCheia(&shared_area1_ptr->fila2) == 0) {
                        insereFila(&shared_area1_ptr->fila2, valor_6);
                        shared_area1_ptr->contador_P6++;
                        break;
                    } else {
                        shared_area1_ptr->busy_wait_vez = escolheVez();
                    }
                }
            }
        }

    } else {            // P7

        pthread_t p7_thread2, p7_thread3;

        int p7_thread1_vez = 3, p7_thread2_vez = 4, p7_thread3_vez = 5;
        void *p7_thread1_ptr, *p7_thread2_ptr, *p7_thread3_ptr;
        p7_thread1_ptr = &p7_thread1_vez;
        p7_thread2_ptr = &p7_thread2_vez;
        p7_thread3_ptr = &p7_thread3_vez;

        // cria thread 2 e 3
        pthread_create(&p7_thread2, NULL, p7, p7_thread2_ptr);
        pthread_create(&p7_thread3, NULL, p7, p7_thread3_ptr);

        // thread 1
        p7(p7_thread1_ptr);

    }

    
    return 0;
}

void handler_p4(int signum) {    
    //printf("\n\nRecebi o sinal\n\n");
    sem_post((sem_t*)&shared_area1_ptr->leitura);        // libera o semaforo de leitura
}

void * p4(void *arg) {
    
    int p4_qual_thread = *(int*)arg;
    int valor_4;
    while (1) {
            
        sem_wait((sem_t*)&shared_area1_ptr->leitura);

        if (filaVazia(&shared_area1_ptr->fila1) == 0) {

            // remove 1 valor de F1 e manda pelo pipe
            valor_4 = removeFila(&shared_area1_ptr->fila1);

            if (shared_area1_ptr->processos[p4_qual_thread+4] == 0) {
                sleep(1);                                       // espera P5 ou P6 serem criados, caso P4 leia F1 antes
            }

            if (shared_area1_ptr->processos[p4_qual_thread+4] > 0) {
                if (p4_qual_thread == 1) {
                    write(shared_area1_ptr->pipe1[1],&valor_4,sizeof(int));
                } else {
                    write(shared_area1_ptr->pipe2[1],&valor_4,sizeof(int));
                }
                
            }

        } else {
            sem_post((sem_t*)&shared_area1_ptr->escrita);    // libera o semaforo de escrita
            continue;                                       // vai para a proxima iteracao sem liberar o semaforo de leitura
        }

        sem_post((sem_t*)&shared_area1_ptr->leitura);

    }

}

void * p7(void *arg) {
    int minhaVez = *(int*)arg;
    int valor_7;
    while (1) {
        if (shared_area1_ptr->busy_wait_vez == minhaVez) {
            if (filaVazia(&shared_area1_ptr->fila2) == 0) {

                valor_7 = removeFila(&shared_area1_ptr->fila2);

                if (valor_7 > shared_area1_ptr->maior) {
                    shared_area1_ptr->maior = valor_7;
                }

                if (valor_7 < shared_area1_ptr->menor) {
                    shared_area1_ptr->menor = valor_7;
                }

                shared_area2_ptr->valores[valor_7-1]++;

                printf("%d\n",valor_7);

                shared_area1_ptr->contador++;
                if (shared_area1_ptr->contador == 10000) {
                    relatorio();
                }
            }
            shared_area1_ptr->busy_wait_vez = escolheVez();
        }
    }

}

int escolheVez() {
    // gera o proximo busy_wait_vez
    int vez;
    if (shared_area1_ptr->processos[6] > 0) {
        vez = rand() % 5 + 1;
    } else {
        vez = rand() % 2 + 1;
    }
    return vez;
}

void relatorio() {
    clock_gettime(CLOCK_REALTIME, &shared_area1_ptr->clock_end);

    for (int i = 0; i < 6; i++) {
        kill(shared_area1_ptr->processos[i], SIGTERM);
    }

    double tempo_total = shared_area1_ptr->clock_end.tv_sec - shared_area1_ptr->clock_start.tv_sec;
    printf("\n\nP7 imprimiu 10000 valores em %.2f segundos\n",tempo_total);
    printf("P5 processou %d valores\n",shared_area1_ptr->contador_P5);
    printf("P6 processou %d valores\n",shared_area1_ptr->contador_P6);
    printf("Valor que mais se repetiu: %d\n",(maisRepetido(shared_area2_ptr->valores,1000)+1));
    printf("Menor valor impresso: %d\n",shared_area1_ptr->menor);
    printf("Maior valor impresso: %d\n",shared_area1_ptr->maior);
    printf("\n\n");

    exit(0);
}

int maisRepetido(int array[], int size) {
    int maior_ocorrencia = 0;
    int indice = 0;
    for (int i = 0; i < size; i++) {
        if (array[i] > maior_ocorrencia) {
            maior_ocorrencia = array[i];
            indice = i;
        }
    }
    return indice;
}