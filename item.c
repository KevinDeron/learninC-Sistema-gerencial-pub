#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "item.h"

int totalItens = 0;
struct item *itens;

int adicionarItem(char *nome, float preco){
    int indiceN;
    if (strlen(nome) > 14){
        printf("Nome do item maior que %zu caracteres", sizeof(itens->nome) + 1);
        return -1;
    }
    if(totalItens != 0){
        for(indiceN = 0;indiceN < totalItens;indiceN++){
            if(strcmp(itens[indiceN].nome, nome) == 0){
                printf("Item ja existe");
                return -1;
            }
        }
    }
    totalItens++;
    struct item *new_p = realloc(itens, totalItens * sizeof(struct item));
    if(new_p == NULL){
        printf("Falha ao alocar memoria![Item]");
        totalItens--;
        return -1;
    }
    itens = new_p;
    indiceN = totalItens - 1;
    strcpy(itens[indiceN].nome, nome);
    itens[indiceN].preco = preco;
    return indiceN;
}