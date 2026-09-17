#include <stdio.h>
#include <sqlite3.h>

sqlite3 *db;
sqlite3_stmt *stmt;
int rc;
char *zErrMsg;
char *sql;

static int callback(void *NotUsed, int argc, char **argv, char **azColName) {
   for(int i = 0; i<argc; i++) {
      printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
   }
   printf("\n");
   return 0;
}

int initDB(){
    zErrMsg = NULL;
    rc = sqlite3_open("data.db",&db);
    if(rc){
        fprintf(stderr, "Nao foi possivel abrir database: %s\n",sqlite3_errmsg(db));
        return 0;
    }else{
        fprintf(stderr,"Database aberta com sucesso!\n");
    }
    if(!db){
        fprintf(stderr ,"NULL pointer passado para %s\n", __func__);
        return 1;
    }
    //======================DEBUG======================
    // sql = "DROP TABLE IF EXISTS itens";
    // rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);
    // if(rc != SQLITE_OK){
    //     fprintf(stderr, "Erro SQL: %s\n",zErrMsg);
    //     sqlite3_free(zErrMsg);
    //     return 0;
    // }
    // sql = "DROP TABLE IF EXISTS comandas";
    // rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);
    // if(rc != SQLITE_OK){
    //     fprintf(stderr, "Erro SQL: %s\n",zErrMsg);
    //     sqlite3_free(zErrMsg);
    //     return 0;
    // }
    // sql = "DROP TABLE IF EXISTS comanda_itens";
    // rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);
    // if(rc != SQLITE_OK){
    //     fprintf(stderr, "Erro SQL: %s\n",zErrMsg);
    //     sqlite3_free(zErrMsg);
    //     return 0;
    // }
    //=================================================
    //Tabela comandas
    sql =   "CREATE TABLE IF NOT EXISTS comandas(" \
            "ID INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL," \
            "mesa       TEXT," \
            "cliente    TEXT," \
            "aberta_em  TIMESTAMP," \
            "fechada_em TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP);";
    rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);

    if(rc == SQLITE_OK){
        printf("Tabela(s) inicializada(s)!\n");
        // return 1;
    }else {
        printf("Erro ao criar tabela[%s]\n", zErrMsg);
        sqlite3_free(zErrMsg);
        return 0;
    }
    //Tabela itens
    sql =   "CREATE TABLE IF NOT EXISTS itens(" \
            "ID INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL," \
            "nome           TEXT," \
            "categoria      TEXT," \
            "preco          REAL NOT NULL," \
            "isAtivo        INTEGER NOT NULL DEFAULT 1);"; //1=ativo , 0=inativo

    rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);

    if(rc == SQLITE_OK){
        printf("Tabela(s) inicializada(s)!\n");
        // return 1;
    }else {
        printf("Erro ao criar tabela[%s]\n", zErrMsg);
        sqlite3_free(zErrMsg);
        return 0;
    }
    //tabela comanda_itens

    sql =   "CREATE TABLE IF NOT EXISTS comanda_itens(" \
            "ID             INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL," \
            "comanda_id     INTEGER NOT NULL REFERENCES comandas(id)," \
            "item_id        INTEGER NOT NULL REFERENCES itens(id)," \
            "quantidade     INTEGER NOT NULL," \
            "preco_unitario REAL NOT NULL," \
            "adicionado_em  TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP," \
            "removido_em    TIMESTAMP);";

    rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);

    if(rc == SQLITE_OK){
        printf("Tabela(s) inicializada(s)!\n");
        return 1;
    }else {
        printf("Erro ao criar tabela[%s]\n", zErrMsg);
        sqlite3_free(zErrMsg);
        return 0;
    }
}