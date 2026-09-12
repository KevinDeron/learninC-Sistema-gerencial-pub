#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sqlite3.h>
#include "database.h"
#include "item.h"

int totalItens;

int adicionarItem(char *nome, char *categoria, float preco){
    int id_database_item;
    if (strlen(nome) > 14){
        printf("Nome do item maior que 15 caracteres");
        return -1;
    }
    // if(sqlite3_prepare_v2(db,"SELECT ID from itens",-1,&stmt,NULL)){
    //     printf("Erro SQL(%s) em: %s\n",sqlite3_errmsg(db),__func__);
    //     sqlite3_finalize(stmt);
    //     return -1;
    // }
    // while (sqlite3_step(stmt) == SQLITE_ROW){
    //     totalItens++;
    // }
    // printf("totalItens(%d) [%s][test]\n",totalItens,__func__);
    // sqlite3_reset(stmt);
    
    if(totalItens > 0){
        char *c;
        rc = sqlite3_prepare_v2(db,"SELECT nome FROM itens WHERE EXISTS (SELECT 1 WHERE nome = ?)",-1,&stmt,NULL);
        if(rc){
            printf("Erro SQL(%s) em: %s\n",sqlite3_errmsg(db),__func__);
            sqlite3_finalize(stmt);
            return -1;
        }
        sqlite3_bind_text(stmt,1,nome,-1,NULL);
        while(sqlite3_step(stmt) == SQLITE_ROW){
            c = sqlite3_column_text(stmt,0);
            if(strcmp(nome,c) == 0){
                printf("Item (%s) ja existe!\n",nome);
                sqlite3_finalize(stmt);
                return -1;
            }
        }
        sqlite3_reset(stmt);
    }
    if( sqlite3_prepare_v2(db,"INSERT INTO itens(nome, categoria, preco) VALUES (?, ?, ?)",-1,&stmt,NULL)){
        printf("Erro SQL(%s) em: %s\n",sqlite3_errmsg(db),__func__);
        sqlite3_finalize(stmt);
        return -1;
    }
    sqlite3_bind_text   (stmt,1,nome,-1,NULL);
    sqlite3_bind_text   (stmt,2,categoria,-1,NULL);
    sqlite3_bind_double (stmt,3,preco);
    sqlite3_step(stmt);
    id_database_item = sqlite3_last_insert_rowid(db);
    sqlite3_finalize(stmt);
    // printf("ITEM ID TEST %d\n",id_item);
    return id_database_item;
}

int setItemAtivo(int id_database_item, int isAtivo){
    if(sqlite3_prepare_v2(db,"INSERT itens SET isAtivo = ? WHERE id == ?",-1,&stmt,NULL)){
        printf("Erro SQL em: %s\n",__func__);
        sqlite3_finalize(stmt);
        return -1;
    }
    sqlite3_bind_int(stmt,1,isAtivo);
    sqlite3_bind_int(stmt,2,id_database_item);
    sqlite3_finalize(stmt);
    return 0;
}