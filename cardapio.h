#ifndef CARDAPIO_H
#define CARDAPIO_H
#include "item.h"

extern int totalItensCardapio;
extern struct item *cardapio;

int adicionarItemCardapio(char *nome, float preco);

#endif