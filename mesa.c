#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "mesa.h"
#define PRECO_NAO_INFORMADO -1
#define QUANT_NAO_INFORMADO -1

int totalMesas = 6;
struct mesa *mesas;


void setIniciarMesas(){
    if((mesas  = calloc(totalMesas, sizeof(struct mesa))) == NULL){
        printf("Erro ao alocar memoria!\n");
        return;
    }
    for (int i = 0; i < totalMesas; i++){
        mesas[i].isLivre = 1;
        // char numeroMesa[3];
        // snprintf(numeroMesa,2,"%d",i+1);
        // strcpy(mesas[i].nome,"Mesa ");
        // strcat(mesas[i].nome,numeroMesa);
        // printf("%s\n",mesas[i].nome);
    }
}

int getMesaLivre(){
    int mesaN;
    for (mesaN = 0; mesaN < totalMesas; mesaN++){
        if(mesas[mesaN].isLivre){
            mesas[mesaN].isLivre = 0;
            return mesaN;
        }
    }
    totalMesas++;
    struct mesa *new_p = realloc(mesas,totalMesas * sizeof(struct mesa));
    if(new_p == NULL) {
        printf("Erro ao realocar memoria!\n");
        totalMesas--;
        return -1;
    }
    mesas = new_p;
    mesas[mesaN].quantidadeItens = 0;
    resetarMesa(&mesas[mesaN]);
    mesas[mesaN].isLivre = 0;
    return mesaN;
};

float calculaValorTotal(struct mesa *mesa){
    float total = 0;
    for(int i = 0;i < mesa->quantidadeItens;i++){
        total += (mesa->itens[i].preco * mesa->itens[i].quant);
    }
    mesa->valorTotal = total;
    return total;
};

void adicionarItemMesa(struct mesa *mesa, int quant, struct item *novoItem,float preco){
    if(mesa->isLivre){
        printf("Mesa esta livre!\n");
        return;
    }
    if(mesa->quantidadeItens >= 10){
        printf("Mesa %s: numero de itens excedido(%d)\n",mesa->nome, mesa->quantidadeItens);
        return;
    }
    for(int i = 0;i < mesa->quantidadeItens; i++){
        if(!strcmp(mesa->itens[i].nome,novoItem->nome)){
            mesa->itens[i].quant += quant;
            return;
        }
    }
    mesa->itens[mesa->quantidadeItens] = *novoItem;
    if(QUANT_NAO_INFORMADO != quant){
        mesa->itens[mesa->quantidadeItens].quant = quant;
    }else{
        mesa->itens[mesa->quantidadeItens].quant = 1;
    }
    if(PRECO_NAO_INFORMADO != preco){
        mesa->itens[mesa->quantidadeItens].preco = preco;
    }
    mesa->quantidadeItens++;
};

void resetarMesa(struct mesa *mesa){
    mesa->isLivre = 1;
    mesa->valorTotal = 0;
    for (int i = 0; i < mesa->quantidadeItens; i++){
        strcpy(mesa->itens[i].nome,"");
        mesa->itens[i].preco = 0;
        mesa->itens[i].quant = 0;
    }
    strcpy(mesa->apelido,"");
    mesa->quantidadeItens = 0;
    
};

void fecharMesa(struct mesa *mesa){
    if(mesa->isLivre){
        printf("Mesa esta livre\n");
        return;
    }
       
    printf("Mesa: %s(%s)\n", mesa->nome, mesa->apelido);
    for (int i = 0; i < mesa->quantidadeItens; i++){
        printf("%d | %s | %0.2f\n", 
            mesa->itens[i].quant, mesa->itens[i].nome, mesa->itens[i].preco);
    }
    calculaValorTotal(mesa);
    printf("Valor total: %0.2f\n", mesa->valorTotal);
    printf("Deseja fechar[s/n?[placeholder]\n");
    char r;
    scanf("%c", &r);
    if(r == 's'){
        resetarMesa(mesa);
        printf("mesa fechada[test]\n");
        return;
    }
    printf("mesa nao fechada[test]\n");
};

void renomearMesa(struct mesa *mesa, char *novoNome){
    if(strlen(novoNome) <= 14){
        strcpy(mesa->apelido, novoNome);
        printf("Mesa renomeada para %s\n", novoNome);
        return;
    }
    printf("Nome maior que %zu(-1) caracteres!\n", sizeof(mesa->nome));
};