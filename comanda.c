#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include "database.h"
#include "item.h"
#include "cardapio.h"
#include "comanda.h"

int criarComanda(char *mesa,char *cliente){
    sqlite3_stmt *stmt = NULL;
    int id_database_comanda;
    if(sqlite3_prepare_v2(db,"INSERT INTO comandas (mesa,cliente) VALUES(?,?)",-1,&stmt,NULL)){
        printf("Erro SQL(%s) em: %s\n",sqlite3_errmsg(db),__func__);
        sqlite3_finalize(stmt);
        return -1;
    }
    sqlite3_bind_text(stmt,1,mesa,-1,NULL);
    sqlite3_bind_text(stmt,2,cliente,-1,NULL);
    sqlite3_step(stmt);
    if(sqlite3_changes(db) == 0){
        return -1;
    }
    id_database_comanda = sqlite3_last_insert_rowid(db);
    sqlite3_finalize(stmt);
    return id_database_comanda;
}

// double calculaValorTotal(int id_database_comanda){
//     double total = 0;
//     if(sqlite3_prepare_v2)
//     return total;
// };

int adicionarItemComanda(int id_database_comanda,int id_database_item,int quant, double preco){
    sqlite3_stmt *stmt = NULL;
    if(sqlite3_prepare_v2(db,   "INSERT INTO comanda_itens " \
                                "(comanda_id, item_id, quantidade, preco_unitario) " \
                                "VALUES (?,?,?,?);",-1,&stmt,NULL)){
        printf("Erro SQL(%s) em: %s\n",sqlite3_errmsg(db),__func__);
        sqlite3_finalize(stmt);
        return 0;
    }
    sqlite3_bind_int(stmt,1,id_database_comanda);
    sqlite3_bind_int(stmt,2,id_database_item);
    sqlite3_bind_int(stmt,3,quant);
    sqlite3_bind_double(stmt,4,preco);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return sqlite3_changes(db);
}

int fecharComandaPorId(int id_database_comanda){
    sqlite3_stmt *stmt = NULL;
    const char *sql = "UPDATE comandas SET fechada_em = CURRENT_TIMESTAMP WHERE ID = ? AND fechada_em ISNULL;";
    if(sqlite3_prepare_v2(db,sql,-1,&stmt,NULL)){
        printf("Erro SQL(%s) em:%s\n",sqlite3_errmsg(db),__func__);
        return 0;
    }
    sqlite3_bind_int(stmt,1,id_database_comanda);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return sqlite3_changes(db);
}

int renomearComandaPorId(int id_database_comanda, char *novoNomeMesa, char *novoNomeCliente){
    sqlite3_stmt *stmt = NULL;
    const char *sql = "UPDATE comandas SET mesa = ?, cliente = ? WHERE ID = ? AND fechada_em ISNULL;";
    if(sqlite3_prepare_v2(db,sql,-1,&stmt,NULL)){
        printf("Erro SQL(%s) em:%s\n",sqlite3_errmsg(db),__func__);
        sqlite3_finalize(stmt);
        return 0;
    }
    sqlite3_bind_text(stmt,1,novoNomeMesa,-1,NULL);
    sqlite3_bind_text(stmt,2,novoNomeCliente,-1,NULL);
    sqlite3_bind_int(stmt,3,id_database_comanda);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return sqlite3_changes(db);
}

char *getComandasAbertasJSON(){
    sqlite3_stmt *stmt = NULL;
    if(sqlite3_prepare_v2(db,   "SELECT json_group_array(json_object('ID',ID,'mesa',mesa,'cliente',cliente))" \
                                "FROM comandas WHERE fechada_em ISNULL",-1,&stmt,NULL)){
        printf("Erro SQL(%s) em:%s\n",sqlite3_errmsg(db),__func__);
        sqlite3_finalize(stmt);
        return 0;
    }
    sqlite3_step(stmt);
    char *completeJSON = sqlite3_mprintf("%s",(const char *)sqlite3_column_text(stmt,0));
    printf("%s\n",completeJSON); //debug
    sqlite3_finalize(stmt);
    return completeJSON;
}

char *getItensComandaJSON(int id_database_comanda){
    sqlite3_stmt *stmt = NULL;
    const char *sql =   "SELECT json_group_array(json_object('id_lancamento',ci.ID,'quantidade',ci.quantidade,'item',i.nome,'preco_unitario',ci.preco_unitario))"
            "FROM comanda_itens ci " \
            "LEFT JOIN comandas c ON ci.comanda_id = c.id " \
            "LEFT JOIN itens i ON ci.item_id = i.id " \
            "WHERE fechada_em ISNULL AND comanda_id = ? AND ci.removido_em ISNULL;";
    if(sqlite3_prepare_v2(db,sql,-1,&stmt,NULL)){
        printf("Erro SQL(%s) em:%s\n",sqlite3_errmsg(db),__func__);
        sqlite3_finalize(stmt);
        return 0;
    }
    sqlite3_bind_int(stmt,1,id_database_comanda);
    sqlite3_step(stmt);
    char *completeJSON = sqlite3_mprintf("%s",(const char *)sqlite3_column_text(stmt,0));
    printf("%s\n",completeJSON); //debug
    sqlite3_finalize(stmt);
    return completeJSON;
}

int atualizarItemLancamento(int id_lancamento, int quatidade, double preco_unitario){
    sqlite3_stmt *stmt = NULL;
    const char *sql = "UPDATE comanda_itens SET quantidade = ?, preco_unitario = ? WHERE ID = ?;";
    if(sqlite3_prepare_v2(db,sql,-1,&stmt,NULL)){
        printf("Erro SQL(%s) em:%s\n",sqlite3_errmsg(db),__func__);
        sqlite3_finalize(stmt);
        return 0;
    }
    sqlite3_bind_int(   stmt,1,quatidade);
    sqlite3_bind_double(stmt,2,preco_unitario);
    sqlite3_bind_int(   stmt,3,id_lancamento);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return sqlite3_changes(db);
}

int removerItemLancamento(int id_lancamento){
    sqlite3_stmt *stmt = NULL;
    const char *sql = "UPDATE comanda_itens SET removido_em = CURRENT_TIMESTAMP WHERE id = ?;";
    if(sqlite3_prepare_v2(db,sql,-1,&stmt,NULL)){
        printf("Erro SQL(%s) em:%s\n",sqlite3_errmsg(db),__func__);
        sqlite3_finalize(stmt);
        return 0;
    }
    sqlite3_bind_int(stmt,1,id_lancamento);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return sqlite3_changes(db);
}

void encerrarSistema(){
    printf("Fechando database\n");
    sqlite3_close(db);
    db = NULL;
}