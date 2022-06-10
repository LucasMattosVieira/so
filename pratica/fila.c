# include "fila.h"

Fila criaFila() {
    Fila fila;
    fila.inicio = 0;
    fila.fim = 0;
    fila.nroElementos = 0;
    return fila;
}

void insereFila(int n, Fila fila) {
    if (filaCheia(fila) == 0) {
        fila.elementos[fila.fim] = n;
        fila.fim++;
        if (fila.fim == 10) {
            fila.fim = 0;
        }
        fila.nroElementos++;
    } else {
        printf("Fila cheia!\n\n");
    }
}

int removeFila(Fila fila) {
    int n = -1;
    if (filaVazia(fila) == 0) {
        n = fila.elementos[fila.inicio];
        fila.inicio++;
        if (fila.inicio == 10) {
            fila.inicio = 0;
        }
        fila.nroElementos--;
    } else {
        printf("Fila vazia!\n\n");
    }
    return n;
}

int filaVazia(Fila fila) {
    return (fila.nroElementos == 0) ? 1 : 0; 
}

int filaCheia(Fila fila) {
    return (fila.nroElementos == 10) ? 1 : 0; 
}