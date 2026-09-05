#include <stdio.h>
#include <stdlib.h>
#include "item.h"
#include "cardapio.h"
#include "comanda.h"

int comandaN1, comandaN2, t;
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
            comandas[j].itensComanda[i].nome, comandas[j].itensComanda[i].preco, comandas[j].itensComanda[i].quant);
        }
    }
};

int main (void){
    if(setIniciarComandas() == -1){
        return 0;
    };

    int chopp500 = adicionarItem("Chopp 500",15);
    if(chopp500 == -1){return 0;}
    int fritas = adicionarItem("Fritas",25);
    if(fritas == -1){return 0;}
    int chopp400 = adicionarItem("Chopp 400",13);
    if(chopp400 == -1){return 0;}

    comandaN1 = criarComanda("Mesa 1", "");
    if (comandaN1 == -1){return 0;}
    t = adicionarItemComanda(&comandas[comandaN1], 2, chopp500, -1);
    if(t == -1){return 0;}
    t = adicionarItemComanda(&comandas[comandaN1], 10, chopp500, -1);
    if(t == -1){return 0;}
    t = adicionarItemComanda(&comandas[comandaN1], 1, fritas, -1);
    if(t == -1){return 0;}
    t = adicionarItemComanda(&comandas[comandaN1], 1, chopp400, -1);
    if(t == -1){return 0;}
    calculaValorTotal(&comandas[comandaN1]);
    
    comandaN2 = criarComanda("", "123test");
    if(comandaN2 == -1){return 0;}

    t = adicionarItemComanda(&comandas[comandaN2], 2,chopp500, -1);
    if(t == -1){return 0;}
    t = adicionarItemComanda(&comandas[comandaN2], -1,fritas, 20);
    if(t == -1){return 0;}
    t = adicionarItemComanda(&comandas[comandaN2], 1, chopp400, -1);
    if(t == -1){return 0;}
    t = calculaValorTotal(&comandas[comandaN2]);
    if(t == -1){return 0;}

    imprime();
    
    fecharComanda(&comandas[comandaN1]);
    fecharComanda(&comandas[comandaN2]);
    imprime();
    free(comandas);
    free(itens);
    free(cardapio);
    return 0;
};