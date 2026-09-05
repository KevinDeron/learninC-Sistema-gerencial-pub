#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "item.h"
#include "cardapio.h"

int totalItensCardapio = 0;
struct item *cardapio;

struct item *getItensCardapio(){
    int indiceItens;
    int indiceN = 0;
    for(indiceItens = 0;indiceItens < totalItens;indiceItens++){
        if(itens[indiceItens].ativo){
            struct item *new_p = realloc(cardapio,(indiceN + 1) * sizeof(struct item));
            if(new_p == NULL){
                printf("Falha ao alocar memoria[Cardapio]\n");
                return NULL;
            }
            cardapio[indiceN] = itens[indiceItens];
            indiceN++;
        }
    }
    totalItensCardapio = indiceN + 1;
    return cardapio;
}