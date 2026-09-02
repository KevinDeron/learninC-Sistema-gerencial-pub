#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "comanda.h"
#define PRECO_NAO_INFORMADO -1
#define QUANT_NAO_INFORMADO -1

int totalComandas = 6; //numero inicial de comandas a serem alocadas em memoria
struct comanda *comandas;

void setIniciarComandas(){
    if((comandas = calloc(totalComandas, sizeof(struct comanda))) == NULL){
        printf("Erro ao alocar memoria!\n");
        return;
    }
    for (int i = 0; i < totalComandas; i++){
        comandas[i].isLivre = 1;
    }
}

int criarComanda(char *mesa, char *cliente){
    int indiceN;
    for (indiceN = 0; indiceN < totalComandas; indiceN++){
        if(comandas[indiceN].isLivre){
            if(renomearComanda(&comandas[indiceN], mesa, cliente)){
                return -1;
            };
            comandas[indiceN].isLivre = 0;
            return indiceN;
        }
    }
    totalComandas++;
    struct comanda *new_p = realloc(comandas,totalComandas * sizeof(struct comanda));
    if(new_p == NULL) {
        printf("Erro ao realocar memoria!\n");
        totalComandas--;
        return -1;
    }
    comandas = new_p;
    comandas[indiceN].quantidadeItens = 0;
    resetarComanda(&comandas[indiceN]);
    if(renomearComanda(&comandas[indiceN], mesa, cliente)){
        return -1;
    };
    comandas[indiceN].isLivre = 0;
    return indiceN;
};

float calculaValorTotal(struct comanda *comanda){
    float total = 0;
    for(int i = 0;i < comanda->quantidadeItens;i++){
        total += (comanda->itens[i].preco * comanda->itens[i].quant);
    }
    comanda->valorTotal = total;
    return total;
};

void adicionarItemComanda(struct comanda *comanda, int quant, struct item *novoItem,float preco){
    if(comanda->isLivre){
        printf("Comanda esta livre!\n");
        return;
    }
    if(comanda->quantidadeItens >= 10){
        printf("Comanda %s(%s) | Numero de itens excedido(%d)\n",
            comanda->cliente ,comanda->mesa, comanda->quantidadeItens);
        return;
    }
    for(int i = 0;i < comanda->quantidadeItens; i++){
        if(!strcmp(comanda->itens[i].nome,novoItem->nome)){
            comanda->itens[i].quant += quant;
            return;
        }
    }
    comanda->itens[comanda->quantidadeItens] = *novoItem;
    if(QUANT_NAO_INFORMADO != quant){
        comanda->itens[comanda->quantidadeItens].quant = quant;
    }else{
        comanda->itens[comanda->quantidadeItens].quant = 1;
    }
    if(PRECO_NAO_INFORMADO != preco){
        comanda->itens[comanda->quantidadeItens].preco = preco;
    }
    comanda->quantidadeItens++;
};

void resetarComanda(struct comanda *comanda){
    comanda->isLivre = 1;
    comanda->valorTotal = 0;
    for (int i = 0; i < comanda->quantidadeItens; i++){
        strcpy(comanda->itens[i].nome,"");
        comanda->itens[i].preco = 0;
        comanda->itens[i].quant = 0;
    }
    strcpy(comanda->mesa,"");
    strcpy(comanda->cliente,"");
    comanda->quantidadeItens = 0;
    
};

void fecharComanda(struct comanda *comanda){
    if(comanda->isLivre){
        printf("Comanda esta livre\n");
        return;
    }
       
    printf("Comanda: %s(%s)\n", comanda->mesa, comanda->cliente);
    for (int i = 0; i < comanda->quantidadeItens; i++){
        printf("%d | %s | %0.2f\n", 
            comanda->itens[i].quant, comanda->itens[i].nome, comanda->itens[i].preco);
    }
    calculaValorTotal(comanda);
    printf("Valor total: %0.2f\n", comanda->valorTotal);
    printf("Deseja fechar[s/n?[placeholder]\n");
    char r;
    scanf("%c", &r);
    if(r == 's'){
        resetarComanda(comanda);
        printf("Comanda fechada[test]\n");
        return;
    }
    printf("Comanda nao fechada[test]\n");
};

int renomearComanda(struct comanda *comanda, char *novoNomeMesa, char *novoNomeCliente){
    if(strlen(novoNomeMesa) <= 14 && strlen(novoNomeCliente) <= 14){
        strcpy(comanda->mesa, novoNomeMesa);
        strcpy(comanda->cliente, novoNomeCliente);
        printf("Comanda renomeada para %s(%s)\n",comanda->cliente, comanda->mesa);
        return 1;
    }
    printf("Nome maior que %zu(-1) caracteres!\n", sizeof(comanda->cliente));
    return 0;
};