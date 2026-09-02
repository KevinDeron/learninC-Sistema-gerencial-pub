#ifndef COMANDA_H
#define COMANDA_H
#include "item.h"
extern int totalComandas;

struct comanda
{
    char nome[15];
    char apelido[15];
    float valorTotal;
    int isLivre;
    int quantidadeItens;
    struct item itens[10];
};

void setIniciarComandas();
int criarComanda();
float calculaValorTotal(struct comanda *comanda);
void adicionarItemComanda(struct comanda *comanda,int,struct item *novoItem,float);
void fecharComanda(struct comanda *comanda);
void renomearComanda(struct comanda *comanda, char *novoNome);

extern struct comanda *comandas;

#endif