#ifndef COMANDA_H
#define COMANDA_H
#include "item.h"
extern int totalComandas;

struct comanda
{
    char mesa[15];
    char cliente[15];
    float valorTotal;
    int isLivre;
    int quantidadeItens;
    struct item itens[10];
};

void setIniciarComandas();
int criarComanda(char *mesa, char *cliente);
float calculaValorTotal(struct comanda *comanda);
void adicionarItemComanda(struct comanda *comanda,int,struct item *novoItem,float);
void resetarComanda(struct comanda *comanda);
void fecharComanda(struct comanda *comanda);
void renomearComanda(struct comanda *comanda, char *novoNomeMesa, char *novoNomeCliente);

extern struct comanda *comandas;

#endif