#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include "database.h"
#include "item.h"
#include "cardapio.h"
#include "comanda.h"
#define PRECO_NAO_INFORMADO -1
#define QUANT_NAO_INFORMADO -1

int totalComandas; //numero total de comandas alocadas em memoria
struct comanda *comandas;
struct item *itensComanda;

// int setIniciarComandas(){
//     if((comandas = calloc(totalComandas, sizeof(struct comanda))) == NULL){
//         printf("Erro ao alocar memoria![setIniciarComandas[comanda]]\n");
//         return -1;
//     }
//     for (int i = 0; i < totalComandas; i++){
//         comandas[i].isLivre = 1;
//     }
//     itens = realloc(NULL,sizeof(struct item) * totalComandas);
//     return 0;
// }

int criarComanda(char *mesa, char *cliente){//YEAH, I KNOW, ITS UGLY, THIS HAS TO BE FIXED '-' someday(tm)
    int indiceN = 0;
    int id_database_comanda;
    if(sqlite3_prepare_v2(db,"INSERT INTO comandas (mesa,cliente) VALUES(?,?)",-1,&stmt,NULL)){
        printf("Erro SQL(%s) em: %s\n",sqlite3_errmsg(db),__func__);
        sqlite3_finalize(stmt);
        return -1;
    }
    sqlite3_bind_text(stmt,1,mesa,-1,NULL);
    sqlite3_bind_text(stmt,2,cliente,-1,NULL);
    if(totalComandas > 0){
        for(indiceN = 0;indiceN < totalComandas;indiceN++){
            if(comandas[indiceN].isLivre == 1){
                if(renomearComanda(&comandas[indiceN], mesa, cliente) == -1){return -1;};
                comandas[indiceN].isLivre = 0;
                sqlite3_step(stmt);
                id_database_comanda = sqlite3_last_insert_rowid(db);
                comandas[indiceN].id_database_comanda = id_database_comanda;
                sqlite3_finalize(stmt);
                return id_database_comanda;
            }
        }
        struct comanda *new_p = realloc(comandas,(totalComandas + 1) * sizeof(struct comanda));
        if(new_p == NULL){
            printf("Erro ao realocar memoria em: %s\n",__func__);
            return -1;
        }
        comandas =  new_p;
        comandas[indiceN].quantidadeItens = 0;
        resetarComanda(&comandas[indiceN]);
        if(renomearComanda(&comandas[indiceN], mesa, cliente) == -1){return -1;};
        comandas[indiceN].isLivre = 0;
        totalComandas++;
        sqlite3_step(stmt);
        id_database_comanda = sqlite3_last_insert_rowid(db);
        comandas[indiceN].id_database_comanda = id_database_comanda;
        sqlite3_finalize(stmt);
        return id_database_comanda;
    }
    struct comanda *new_p = realloc(comandas,(totalComandas + 1) * sizeof(struct comanda));
    if(new_p == NULL){
        printf("Erro ao alocar memoria em: %s\n",__func__);
        return -1;
    }
    comandas =  new_p;
    comandas[indiceN].quantidadeItens = 0;
    resetarComanda(&comandas[indiceN]);
    if(renomearComanda(&comandas[indiceN], mesa, cliente) == -1){return -1;};
    comandas[indiceN].isLivre = 0;
    totalComandas++;
    sqlite3_step(stmt);
    id_database_comanda = sqlite3_last_insert_rowid(db);
    comandas[indiceN].id_database_comanda = id_database_comanda;
    sqlite3_finalize(stmt);
    return id_database_comanda;
};

float calculaValorTotal(int id_database_comanda){
    float total = 0;
    int id_comanda = getComandaIndice(id_database_comanda);
    for(int i = 0;i < comandas[id_comanda].quantidadeItens;i++){
        total += (comandas[id_comanda].itensComanda[i].preco * comandas[id_comanda].itensComanda[i].quant);
    }
    comandas[id_comanda].valorTotal = total;
    return total;
};

int adicionarItemComanda(int id_database_comanda, int quant, int id_database_item,float preco){
    int indiceN = 0;
    int indiceCardapio = -1;
    int id_comanda = getComandaIndice(id_database_comanda);
    if(id_comanda == -1){return -1;};
    if(comandas[id_comanda].isLivre){
        printf("Comanda esta livre![%s]\n",__func__);
        return -1;
    }
    indiceCardapio = getItemCardapioIndice(id_database_item);
    if(indiceCardapio == -1){return -1;};
    sql =   "INSERT INTO comanda_itens " \
            "(comanda_id, item_id, quantidade, preco_unitario) " \
            "VALUES (?,?,?,?);";
    // "(comanda_id = 1, item_id = 2, quantidade = 3, preco_unitario = 4) "
    if(sqlite3_prepare_v2(db,sql,-1,&stmt,NULL)){
        printf("Erro SQL(%s) em: %s\n",sqlite3_errmsg(db),__func__);
        return -1;
    }
    sqlite3_bind_int(stmt,1,id_database_comanda);
    sqlite3_bind_int(stmt,2,id_database_item);

    if(comandas[id_comanda].quantidadeItens > 0){
        for(indiceN = 0;indiceN < comandas[id_comanda].quantidadeItens; indiceN++){
            if(strcmp(comandas[id_comanda].itensComanda[indiceN].nome, cardapio[indiceCardapio].nome) == 0){
                if(comandas[id_comanda].itensComanda[indiceN].preco == preco){
                    if(QUANT_NAO_INFORMADO == quant){
                        comandas[id_comanda].itensComanda[indiceN].quant += 1;
                        sqlite3_bind_int(stmt,3,1);
                        sqlite3_bind_double(stmt,4,preco);
                        sqlite3_step(stmt);
                        sqlite3_finalize(stmt);
                        return 0;
                    }
                    comandas[id_comanda].itensComanda[indiceN].quant += quant;
                    sqlite3_bind_int(stmt,3,quant);
                    sqlite3_bind_double(stmt,4,preco);
                    sqlite3_step(stmt);
                    sqlite3_finalize(stmt);
                    return 0;
                }
            }
        }

        struct item *new_p = realloc(comandas[id_comanda].itensComanda,(comandas[id_comanda].quantidadeItens + 1) * sizeof(struct item));
        if(new_p == NULL){
            printf("Falha ao alocar memoria %s\n",__func__);
            return -1;
        }
        comandas[id_comanda].itensComanda = new_p;
    }else{
        if((comandas[id_comanda].itensComanda = calloc(1,sizeof(struct item))) == NULL){
            printf("Erro ao alocar memoria em: %s\n",__func__);
            return -1;
        }
    }

    comandas[id_comanda].itensComanda[indiceN] = cardapio[indiceCardapio];
    if(QUANT_NAO_INFORMADO != quant){
        comandas[id_comanda].itensComanda[indiceN].quant = quant;
        sqlite3_bind_int(stmt,3,quant);
    }else{
        comandas[id_comanda].itensComanda[indiceN].quant = 1;
        sqlite3_bind_int(stmt,3,1);
    }
    if(PRECO_NAO_INFORMADO != preco){
        comandas[id_comanda].itensComanda[indiceN].preco = preco;
        sqlite3_bind_double(stmt,4,preco);
    }else{
        sqlite3_bind_double(stmt,4,comandas[id_comanda].itensComanda[indiceN].preco);
    }
    comandas[id_comanda].quantidadeItens++;
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return 0;
};

void resetarComanda(struct comanda *comanda){
    comanda->isLivre = 1;
    comanda->valorTotal = 0;
    strcpy(comanda->mesa," ");
    strcpy(comanda->cliente," ");
    if(comanda->quantidadeItens > 0){
        free(comanda->itensComanda);
    }
    comanda->quantidadeItens = 0;
    comanda->id_database_comanda = 0;
};

int fecharComanda(int id_database_comanda){
    int id_comanda = getComandaIndice(id_database_comanda);
    if(comandas[id_comanda].isLivre){
        printf("Falha ao fechar mesa[Mesa Livre]\n");
        return -1;
    }
       
    printf("Comanda: %s(%s)\n", comandas[id_comanda].mesa, comandas[id_comanda].cliente);
    for (int i = 0; i < comandas[id_comanda].quantidadeItens; i++){
        printf("%d | %s | %0.2f\n", 
            comandas[id_comanda].itensComanda[i].quant, comandas[id_comanda].itensComanda[i].nome, comandas[id_comanda].itensComanda[i].preco);
    }
    calculaValorTotal(id_database_comanda);
    printf("Valor total: %0.2f\n", comandas[id_comanda].valorTotal);
    printf("Deseja fechar[s/n?[placeholder]\n");
    char r;
    scanf(" %c", &r);
    if(r == 's'){
        if(sqlite3_prepare_v2(db,"UPDATE comandas SET fechada_em = CURRENT_TIMESTAMP WHERE ID = ?",-1,&stmt,NULL)){
            printf("Erro SQL(%s) em:%s\n",sqlite3_errmsg(db),__func__);
            return -1;
        }
        sqlite3_bind_int(stmt,1,id_database_comanda);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        resetarComanda(&comandas[id_comanda]);
        return 0;
    }
    return 1;
};

int renomearComanda(struct comanda *comanda, char *novoNomeMesa, char *novoNomeCliente){
    // int id_comanda = getComandaIndice(id_database_comanda);
    if(strlen(novoNomeMesa) <= 14 && strlen(novoNomeCliente) <= 14){
        if(strcmp(novoNomeMesa, " ") == 0) {
            strcpy(comanda->mesa, " ");
        }else {
            strcpy(comanda->mesa, novoNomeMesa);
        }
        if(strcmp(novoNomeCliente, " ") == 0){
            strcpy(comanda->cliente, " ");
        }else {
            strcpy(comanda->cliente, novoNomeCliente);
        }
        printf("Comanda renomeada para %s|Cliente: %s\n",comanda->mesa, comanda->cliente);
        return 1;
    }
    printf("Nome maior que %zu(-1) caracteres!\n", sizeof(comandas->cliente));
    return -1;
};

int getComandaIndice(int id_database_comanda){
        for (int indiceN = 0;indiceN < totalComandas; indiceN++){
            if(comandas[indiceN].id_database_comanda == id_database_comanda){
                return indiceN;
        }
    }
    printf("Comanda nao existe![%s]\n",__func__);
    return -1;
}

void encerrarSistema(){
    printf("Livrando memoria\n");
    free(comandas);
    free(cardapio);
    printf("Fechando database\n");
    sqlite3_close(db);
    db = NULL;
}