#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "item.h"
#include "cardapio.h"

int totalItensCardapio = 0;

int adicionarItemCardapio(char *nome, float preco){
    int indiceN;
    if (strlen(nome) >= 14){
        printf("Nome do item maior que %zu caracteres", sizeof(cardapio->nome) + 1);
        return 1;
    }    
    for(indiceN = 0;indiceN <= totalItensCardapio;indiceN++){
        if(strcmp(cardapio[indiceN].nome, nome) == 0){
            printf("Item ja existe");
            return 1;
        }
    }
    struct item *new_p = realloc(cardapio, totalItensCardapio * sizeof(struct item));
    if(new_p == NULL){
        printf("Falha ao alocar memoria![Item]");
        return 0;
    }
    cardapio = new_p;
    strcpy(cardapio[totalItensCardapio].nome, nome);
    cardapio[totalItensCardapio].preco = preco;
    totalItensCardapio++;
    return 0;
}