#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sqlite3.h>
#include "database.h"
#include "item.h"
#include "cardapio.h"

char *getItensCardapioJSON(){
    sqlite3_stmt *stmt;
    if(sqlite3_prepare_v2(db,"SELECT json_Group_array(json_object('ID',ID,'nome',nome,'categoria',categoria,'preco',preco)) FROM itens WHERE isAtivo == 1;",-1,&stmt,NULL)){
        printf("Erro SQL(%s) em: %s\n",sqlite3_errmsg(db),__func__);
        sqlite3_finalize(stmt);
        return 0;
    }
    sqlite3_step(stmt);
    char *cardapioJSON = sqlite3_mprintf("%s",(const char *)sqlite3_column_text(stmt,0));
    printf("%s\n",cardapioJSON); //debug
    sqlite3_finalize(stmt);
    return cardapioJSON;
}