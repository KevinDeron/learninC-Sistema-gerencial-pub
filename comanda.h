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
    struct item *itensComanda;
};

int setIniciarComandas();
int criarComanda(char *mesa, char *cliente);
float calculaValorTotal(struct comanda *comanda);
int adicionarItemComanda(struct comanda *comanda,int,int novoItem,float);
void resetarComanda(struct comanda *comanda);
void fecharComanda(struct comanda *comanda);
int renomearComanda(struct comanda *comanda, char *novoNomeMesa, char *novoNomeCliente);

extern struct comanda *comandas;
extern struct item *itensComanda;

#endif