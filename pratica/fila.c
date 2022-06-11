#include <stdio.h> 

#include "fila.h"

Fila criaFila() {
    Fila fila;
    fila.inicio = 0;
    fila.fim = 0;
    fila.nroElementos = 0;
    return fila;
}

int insereFila(Fila *fila, int n) {
    if (filaCheia(fila)) {
        printf("Fila cheia!\n\n");
        return -1;
    } else {
        fila->elementos[fila->fim] = n;
        fila->fim++;
        if (fila->fim == 10) {
            fila->fim = 0;
        }
        fila->nroElementos++;
        return 0;
    }
}

int removeFila(Fila *fila) {
    int n = -1;
    if (filaVazia(fila)) {
        printf("Fila vazia!\n\n");
    } else {
        n = fila->elementos[fila->inicio];
        fila->inicio++;
        if (fila->inicio == 10) {
            fila->inicio = 0;
        }
        fila->nroElementos--;
    }
    return n;
}

int filaVazia(Fila *fila) {
    return (fila->nroElementos == 0) ? 1 : 0; 
}

int filaCheia(Fila *fila) {
    return (fila->nroElementos == 10) ? 1 : 0; 
}