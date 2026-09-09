#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sqlite3.h>
#include "database.h"
#include "item.h"
#include "cardapio.h"

int totalItensCardapio = 0;
struct item *cardapio;

struct item *getItensCardapio(){
    int indiceN = 0;
    char *c;
    rc = sqlite3_prepare_v2(db,"SELECT ID,nome,categoria,preco FROM itens WHERE isAtivo == 1",-1,&stmt,NULL);
    if(rc){
        printf("Erro SQL(%s) em: %s\n",sqlite3_errmsg(db),__func__);
        sqlite3_finalize(stmt);
        return NULL;
    }
    while(sqlite3_step(stmt) == SQLITE_ROW){
        struct item *new_p = realloc(cardapio,(indiceN + 1) * sizeof(struct item));
        if(new_p == NULL){
            printf("Falha ao alocar memoria[%s]\n", __func__);
            sqlite3_finalize(stmt);
            return NULL;
        }
        cardapio = new_p;
        cardapio[indiceN].id_database = sqlite3_column_int(stmt,0);
        c = sqlite3_column_text(stmt,1);
            strcpy(cardapio[indiceN].nome,c);
        c = sqlite3_column_text(stmt,2);
            strcpy(cardapio[indiceN].categoria,c);
        cardapio[indiceN].preco = sqlite3_column_double(stmt,3);
        indiceN++;
    }
    totalItensCardapio = indiceN;
    sqlite3_finalize(stmt);
    return cardapio;
}