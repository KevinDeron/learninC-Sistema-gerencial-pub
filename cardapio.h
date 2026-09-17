#ifndef CARDAPIO_H
#define CARDAPIO_H
#include "item.h"

extern int totalItensCardapio;

struct item *getItensCardapio();
int getItemCardapioIndice(int id_database_item);

extern struct item *cardapio;

#endif