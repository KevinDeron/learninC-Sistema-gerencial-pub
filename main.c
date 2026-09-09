#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include "database.h"
#include "item.h"
#include "cardapio.h"
#include "comanda.h"

int comandaN1, comandaN2, test;

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

int main (int argc, char *argv[]){

    rc = sqlite3_open("data.db",&db);
    if(rc){
        fprintf(stderr, "Nao foi possivel abrir database: %s\n",sqlite3_errmsg(db));
        return 0;
    }else{
        fprintf(stderr,"Database aberta com sucesso!\n");
    }
    int dbInicializada = initDB(db);
    if(dbInicializada == 0){
        fprintf(stderr, "Nao foi possivel inicializar a database!\n");
        return 0;
    }

    if(setIniciarComandas() == -1){return 0;}
    //==================DEBUG=======================
    //Sera removido com implementacao de python(ui)

    int chopp500 = adicionarItem("Chopp 500","Chopp",15.0);
    if(chopp500 == -1){return 0;}
    int fritas = adicionarItem("Fritas","Porcoes",25.0);
    if(fritas == -1){return 0;}
    int chopp400 = adicionarItem("Chopp 400","Chopp",13.0);
    if(chopp400 == -1){return 0;}
    chopp400 = adicionarItem("Chopp 400","Chopp",13.0);
    if(chopp400 == -1){return 0;}

    if(getItensCardapio() == NULL){return 0;}

    comandaN1 = criarComanda("Mesa 1", "");
    if (comandaN1 == -1){return 0;}
    test = adicionarItemComanda(&comandas[comandaN1], 2, chopp500, 13);
    if(test == -1){return 0;}
    test = adicionarItemComanda(&comandas[comandaN1], 10, chopp500, -1);
    if(test == -1){return 0;}
    test = adicionarItemComanda(&comandas[comandaN1], 1, fritas, -1);
    if(test == -1){return 0;}
    test = adicionarItemComanda(&comandas[comandaN1], 1, chopp400, -1);
    if(test == -1){return 0;}
    calculaValorTotal(&comandas[comandaN1]);
    
    comandaN2 = criarComanda("", "123test");
    if(comandaN2 == -1){return 0;}

    test = adicionarItemComanda(&comandas[comandaN2], 2,chopp500, -1);
    if(test == -1){return 0;}
    test = adicionarItemComanda(&comandas[comandaN2], -1,fritas, 20);
    if(test == -1){return 0;}
    test = adicionarItemComanda(&comandas[comandaN2], 1, chopp400, -1);
    if(test == -1){return 0;}
    test = calculaValorTotal(&comandas[comandaN2]);
    if(test == -1){return 0;}

    imprime();
    
    fecharComanda(&comandas[comandaN1]);
    fecharComanda(&comandas[comandaN2]);
    imprime();

    //============FIM DEBUG=================
    //=============CLEANUP==================
    free(comandas);
    // free(itens);
    free(cardapio);

    printf("Fechando database!\n");
    sqlite3_close(db);
    db = NULL;
    return 0;
};