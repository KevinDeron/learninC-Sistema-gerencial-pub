#ifndef COMANDA_H
#define COMANDA_H
#include "item.h"
// extern int totalComandas;

// struct comanda
// {
//     int id_database_comanda;
//     char mesa[15];
//     char cliente[15];
//     float valorTotal;
//     int isLivre;
//     int quantidadeItens;
//     struct item *itensComanda;
// };

int criarComanda(char *mesa, char *cliente);
// double calculaValorTotal(int id_database_comanda);
int adicionarItemComanda(int id_database_comanda,int id_database_item, int quant, double preco);
int fecharComandaPorId(int id_database_comanda);
int renomearComandaPorId(int id_database_comanda, char *novoNomeMesa, char *novoNomeCliente);
char *getComandasAbertasJSON();
char *getItensComandaJSON(int id_database_comanda);
int atualizarItemLancamento(int id_lancamento, int quantidade, double preco_unitario);
int removerItemLancamento(int id_lancamento);
void encerrarSistema();

// extern struct comanda *comandas;
// extern struct item *itensComanda;

#endif