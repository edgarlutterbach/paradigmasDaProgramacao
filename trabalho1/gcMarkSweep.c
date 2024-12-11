#include<stdio.h>
#include<stdlib.h>

#define STACK_MAX 256
#define INIT_OBJ_NUM_MAX 8

// Para termos algum lixo para coletar, vamos primeiro iniciar "criando" um
// interpretador para uma nova linguagem, que possui dois tipos: ints e pairs.
// Um pair pode ser um par de qualquer coisa, dois ints, dois pairs, um int e
// um pair.
typedef enum {
    INT,
    PAIR
} tipoDoObjeto;


// Aqui definimos um objeto, que possui um tipo, uma espécie de tag que será
// marcada na hora de realizar a coleta, um ponteiro para o próximo objeto e
// uma tagged union, usada para guardar o conteúdo para o tipo int ou pair.
typedef struct objeto {
    tipoDoObjeto tipo;
    unsigned char marca;

    struct objeto *prox;

    union {
        int valor;

        struct {
            struct objeto *cabeca;
            struct objeto *cauda;
        };
    };
} objeto;


// Agora que críamos nosso interpretador para uma linguagem também criada, 
// vamos criar uma máquina virtual para armazenar a pilha de variáveis que
// estão sendo usadas no escopo atual. 
typedef struct {
    objeto *stack[STACK_MAX];
    int stackTam;
    objeto *primeiroObjeto;
    int numObjetos;
    int maxObjetos;
} maqVirtual;


// Primeiramente, precisamos inicializar nossa máquina virtual...
maqVirtual *novaMV() {
    maqVirtual *mv = malloc(sizeof(maqVirtual));
    mv->stackTam = 0;
    mv->primeiroObjeto = NULL;
    mv->numObjetos = 0;
    mv->maxObjetos = INIT_OBJ_NUM_MAX;
    return mv;
}

// E quando inicializada, precisamos de funções para manipular a pilha dentro
// da nossa máquina virtual...
void push(maqVirtual *mv, objeto *valor) {
    if(mv->stackTam > STACK_MAX) {
        printf("Stack overflow!\n");
    }
    mv->stack[mv->stackTam++] = valor;
}

objeto *pop(maqVirtual *mv) {
    if(mv->stackTam < 0) {
        printf("Stack underflow!\n");
    }
    return mv->stack[--mv->stackTam];
}

// A primeira fase da coleta de lixo é o "marking". Precisamos percorrer todos
// os objetos e marcá-los...
void mark(objeto *obj) {
    if(obj->marca) {
        return;
    }

    obj->marca = 1;
    if(obj->tipo == PAIR) {
        mark(obj->cabeca);
        mark(obj->cauda);
    }
}

void markTodos(maqVirtual *mv) {
    int i;
    for(i = 0; i < mv->stackTam; i++) {
        mark(mv->stack[i]);
    }
}

// A segunda fase é o "sweeping", onde verificamos todos os objetos e deletamos
// os que não estão marcados...
void sweep(maqVirtual *mv) {
    objeto **obj = &mv->primeiroObjeto;
    while(*obj) {
        if(!(*obj)->marca) {
            objeto *objInacessivel = *obj;
            *obj = objInacessivel->prox;
            free(objInacessivel);
            mv->numObjetos--;
        } else {
            (*obj)->marca = 0;
            obj = &(*obj)->prox;
        }
    }
}

// Então, podemos inicializar nosso coletor de lixo.
void gc(maqVirtual *mv) {
    int numObjetos = mv->numObjetos;

    markTodos(mv);
    sweep(mv);

    mv->maxObjetos = mv->numObjetos == 0 ? INIT_OBJ_NUM_MAX : mv->numObjetos * 2;
    printf("Coletado %d objetos, %d restantes.\n", numObjetos - mv->numObjetos, mv->numObjetos);
}

// Para coletar o lixo, precisamos inicializar também um objeto.
objeto *novoObjeto(maqVirtual *mv, tipoDoObjeto tipo) {
    if(mv->numObjetos == mv->maxObjetos) {
        gc(mv);
    }

    objeto *obj = malloc(sizeof(objeto));
    obj->tipo = tipo;
    obj->prox = mv->primeiroObjeto;
    mv->primeiroObjeto = obj;
    obj->marca = 0;

    mv->numObjetos++;
    return obj;
}

// Uma função auxiliar para inserir um inteiro...
void pushInt(maqVirtual *mv, int intValor) {
  objeto *obj = novoObjeto(mv, INT);
  obj->valor = intValor;

  push(mv, obj);
}

// E outra para inserir um pair.
objeto *pushPair(maqVirtual *mv) {
  objeto *obj = novoObjeto(mv, PAIR);
  obj->cauda = pop(mv);
  obj->cabeca = pop(mv);

  push(mv, obj);
  return obj;
}

// Por fim, podemos dar um free na máquina virtual
void freeMV(maqVirtual *mv) {
  mv->stackTam = 0;
  gc(mv);
  free(mv);
}

void teste1() {
  printf("Teste 1: Objetos na pilha estao preservados.\n");
  maqVirtual *mv = novaMV();
  pushInt(mv, 1);
  pushInt(mv, 2);

  gc(mv);
  if(mv->numObjetos != 2) {
    printf("Deveria ter preservado os objetos.\n");
  }
  freeMV(mv);
}

void teste2() {
  printf("Teste 2: Objetos inacessiveis sao coletados.\n");
  maqVirtual *mv = novaMV();
  pushInt(mv, 1);
  pushInt(mv, 2);
  pop(mv);
  pop(mv);

  gc(mv);  
  if(mv->numObjetos != 0) {
    printf("Deveria ter coletado objetos.\n");
  }
  freeMV(mv);
}

void teste3() {
  printf("Teste 3: Alcanca objetos aninhados.\n");
  maqVirtual *mv = novaMV();
  pushInt(mv, 1);
  pushInt(mv, 2);
  pushPair(mv);
  pushInt(mv, 3);
  pushInt(mv, 4);
  pushPair(mv);
  pushPair(mv);

  gc(mv);
  if(mv->numObjetos != 7) {
    printf("Deveria ter alcancado objetos.\n");
  }
  freeMV(mv);
}

void teste4() {
  printf("Teste 4: Lidando com ciclos.\n");
  maqVirtual *mv = novaMV();
  pushInt(mv, 1);
  pushInt(mv, 2);
  objeto *a = pushPair(mv);
  pushInt(mv, 3);
  pushInt(mv, 4);
  objeto *b = pushPair(mv);

  a->cauda = b;
  b->cauda = a;

  gc(mv);
  if(mv->numObjetos != 4) {
    printf("Deveria ter coletado objetos.\n");
  }
  freeMV(mv);
}


int main() {
  teste1();
  teste2();
  teste3();
  teste4();

  return 0;
}