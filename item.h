#ifndef ITEM_H
#define ITEM_H

extern int totalItens;

struct item
{
    int id_database_item;
    char nome[15];
    char categoria[15];
    int quant;
    float preco;
    int isAtivo;//bool
};

int adicionarItem(char *nome, char *categoria, float preco);
int setItemAtivo(int id_database_item, int isAtivo);

#endif