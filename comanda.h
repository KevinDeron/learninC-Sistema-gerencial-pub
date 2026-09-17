#ifndef COMANDA_H
#define COMANDA_H
#include "item.h"
extern int totalComandas;

struct comanda
{
    int id_database_comanda;
    char mesa[15];
    char cliente[15];
    float valorTotal;
    int isLivre;
    int quantidadeItens;
    struct item *itensComanda;
};

int setIniciarComandas();
int criarComanda(char *mesa, char *cliente);
float calculaValorTotal(int id_database_comanda);
int getItemCardapioIndice(int id_database_item);
int adicionarItemComanda(int id_database_comanda, int quant, int id_database_item, float preco);
void resetarComanda(struct comanda *comanda);
int fecharComanda(int id_database_comanda);
int renomearComanda(struct comanda *comanda, char *novoNomeMesa, char *novoNomeCliente);
int getComandaIndice(int id_database_comanda);
void encerrarSistema();

extern struct comanda *comandas;
extern struct item *itensComanda;

#endif