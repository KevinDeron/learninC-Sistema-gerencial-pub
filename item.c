#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sqlite3.h>
#include "database.h"
#include "item.h"

int adicionarItem(char *nome, char *categoria, float preco){
    sqlite3_stmt *stmt = NULL;
    int id_database_item;
    if (strlen(nome) > 14){
        printf("Nome do item maior que 15 caracteres\n");
        return 0;
    }    
    if(sqlite3_prepare_v2(db,"SELECT nome FROM itens WHERE EXISTS (SELECT 1 WHERE nome = ?)",-1,&stmt,NULL)){
        printf("Erro SQL(%s) em: %s\n",sqlite3_errmsg(db),__func__);
        sqlite3_finalize(stmt);
        return 0;
    }
    sqlite3_bind_text(stmt,1,nome,-1,NULL);
    sqlite3_step(stmt);
    char *c = sqlite3_mprintf("%s",(const char *)sqlite3_column_text(stmt,0));
    if(strcmp(c,nome) == 0){
        printf("Item (%s) ja existe!\n",nome);
        sqlite3_finalize(stmt);
        return 0;
    }
    sqlite3_finalize(stmt);
    
    if(sqlite3_prepare_v2(db,"INSERT INTO itens(nome, categoria, preco) VALUES (?, ?, ?)",-1,&stmt,NULL)){
        printf("Erro SQL(%s) em: %s\n",sqlite3_errmsg(db),__func__);
        sqlite3_finalize(stmt);
        return 0;
    }
    sqlite3_bind_text   (stmt,1,nome,-1,NULL);
    sqlite3_bind_text   (stmt,2,categoria,-1,NULL);
    sqlite3_bind_double (stmt,3,preco);
    sqlite3_step(stmt);
    if(sqlite3_changes(db) == 0){
        printf("Erro ao criar item");
        return 0;
    }
    id_database_item = sqlite3_last_insert_rowid(db);
    sqlite3_finalize(stmt);
    return id_database_item;
}

int setItemAtivo(int id_database_item, int isAtivo){
    sqlite3_stmt *stmt = NULL;
    if(sqlite3_prepare_v2(db,"UPDATE itens SET isAtivo = ? WHERE ID == ?;",-1,&stmt,NULL)){
        printf("Erro SQL(%s) em: %s\n",sqlite3_errmsg(db),__func__);
        sqlite3_finalize(stmt);
        return 0;
    }
    sqlite3_bind_int(stmt,1,isAtivo);
    sqlite3_bind_int(stmt,2,id_database_item);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return sqlite3_changes(db);
}