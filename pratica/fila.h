struct fila
{
    int elementos[10];
    int inicio;
    int fim;
    int nroElementos;
};

typedef struct fila Fila; 

Fila criaFila();
int insereFila(Fila *fila, int n);
int removeFila(Fila *fila);
int filaVazia(Fila *fila);
int filaCheia(Fila *fila);