#include <stdio.h>
#include "item.h"
#include "cardapio.h"
#include "mesa.h"

int mesaN1, mesaN2;

void imprime(){
    for(int j = 0;j < totalMesas;j++){
        if(!mesas[j].isLivre){
            printf("Mesa: %s(%s)\nLivre?: %d\nQuantidaItens: %d\nValor Total: %.2f\n",
                mesas[j].nome, mesas[j].apelido, mesas[j].isLivre, mesas[j].quantidadeItens, mesas[j].valorTotal);
        }
    }
    for(int j = 0;j < totalMesas;j++){
        if(!mesas[j].isLivre){
            printf("Mesa: %s(%s)\n",
                mesas[j].nome, mesas[j].apelido);
        }
        for(int i = 0;i < mesas[j].quantidadeItens; i++){
            printf("iNome: %s\niPreco: %.2f\niQuantidade: %d\n",
            mesas[j].itens[i].nome, mesas[j].itens[i].preco, mesas[j].itens[i].quant);
        }
    }
};

int main (void){
    setIniciarMesas();


    mesaN1 = getMesaLivre();
    renomearMesa(&mesas[mesaN1],"test");
    adicionarItemMesa(&mesas[mesaN1], 2, &chopp500, -1);
    calculaValorTotal(&mesas[mesaN1]);
    
    mesaN2 = getMesaLivre();
    renomearMesa(&mesas[mesaN2], "123test");
    adicionarItemMesa(&mesas[mesaN2], 2,&chopp500, -1);
    adicionarItemMesa(&mesas[mesaN2], -1,&fritas, 20);
    adicionarItemMesa(&mesas[mesaN2], 1, &chopp500, -1);
    calculaValorTotal(&mesas[mesaN2]);

    imprime();
    
    fecharMesa(&mesas[mesaN1]);
    imprime();
    return 0;
};