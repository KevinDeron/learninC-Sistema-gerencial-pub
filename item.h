#ifndef ITEM_H
#define ITEM_H

extern int totalItens;

struct item
{
    int id_database;
    char nome[15];
    char categoria[15];
    int quant;
    float preco;
    int ativo;//bool
};

int adicionarItem(char *nome, char *categoria, float preco);

extern struct item *itens;

#endif