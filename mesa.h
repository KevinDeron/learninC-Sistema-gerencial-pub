#ifndef MESA_H
#define MESA_H
#include "item.h"
extern int totalMesas;

void setIniciarMesas();
int getMesaLivre();
float calculaValorTotal(struct mesa *mesa);
void adicionarItemMesa(struct mesa *mesa,int,struct item *novoItem,float);
void fecharMesa(struct mesa *mesa);
void renomearMesa(struct mesa *mesa, char *novoNome);

struct mesa
{
    char nome[15];
    char apelido[15];
    float valorTotal;
    int isLivre;
    int quantidadeItens;
    struct item itens[10];
};

extern struct mesa *mesas;

#endif