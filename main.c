#include <stdio.h>
#include <stdlib.h>
#include "item.h"
#include "cardapio.h"
#include "comanda.h"

int comandaN1, comandaN2;
void imprime(){
    for(int j = 0;j < totalComandas;j++){
        if(!comandas[j].isLivre){
            printf("Comanda: %s(%s)\nLivre?: %d\nQuantidaItens: %d\nValor Total: %.2f\n",
                comandas[j].mesa, comandas[j].cliente, comandas[j].isLivre, comandas[j].quantidadeItens, comandas[j].valorTotal);
        }
    }
    for(int j = 0;j < totalComandas;j++){
        if(!comandas[j].isLivre){
            printf("Comanda: %s(%s)\n",
                comandas[j].mesa, comandas[j].cliente);
        }
        for(int i = 0;i < comandas[j].quantidadeItens; i++){
            printf("iNome: %s\niPreco: %.2f\niQuantidade: %d\n",
            comandas[j].itens[i].nome, comandas[j].itens[i].preco, comandas[j].itens[i].quant);
        }
    }
};

int main (void){
    setIniciarComandas();


    comandaN1 = criarComanda("Mesa 1", "");
    if (comandaN1 == -1)
    {
        return 0;
    }
    adicionarItemComanda(&comandas[comandaN1], 2, &chopp500, -1);
    calculaValorTotal(&comandas[comandaN1]);
    
    comandaN2 = criarComanda("", "123test");
    if(comandaN2 == -1){
        return 0;
    }
    adicionarItemComanda(&comandas[comandaN2], 2,&chopp500, -1);
    adicionarItemComanda(&comandas[comandaN2], -1,&fritas, 20);
    adicionarItemComanda(&comandas[comandaN2], 1, &chopp500, -1);
    calculaValorTotal(&comandas[comandaN2]);

    imprime();
    
    fecharComanda(&comandas[comandaN1]);
    imprime();
    free(comandas);
    return 0;
};