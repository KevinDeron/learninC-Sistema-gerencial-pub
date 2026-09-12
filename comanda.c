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

int criarComanda(char *mesa, char *cliente){
    // printf("TOTAL COMANDAS: %d\n",totalComandas);
    int indiceN = 0;
    int id_database_comanda;
    if(totalComandas > 0){
        for(indiceN = 0;indiceN < totalComandas;indiceN++){
            if(comandas[indiceN].isLivre == 1){
                if(renomearComanda(&comandas[indiceN], mesa, cliente) == -1){return -1;};
                comandas[indiceN].isLivre = 0;
                //DB ABRIR COMANDA TIMESTAMP;
                if(sqlite3_prepare_v2(db,"INSERT INTO comandas (mesa,cliente,aberta_em) VALUES(?,?,?)",-1,&stmt,NULL)){
                    printf("Erro SQL(%s) em: %s\n",sqlite3_errmsg(db),__func__);
                    comandas[indiceN].isLivre = 1;
                    sqlite3_finalize(stmt);
                    return -1;
                }
                sqlite3_bind_text(stmt,1,mesa,-1,NULL);
                sqlite3_bind_text(stmt,2,cliente,-1,NULL);
                sqlite3_bind_null(stmt,3); //TIMESTAMP
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
        resetarComanda(&comandas[indiceN]);
        if(renomearComanda(&comandas[indiceN], mesa, cliente) == -1){return -1;};
        comandas[indiceN].isLivre = 0;
        //DB ABRIR COMANDA TIMESTAMP;
        if(sqlite3_prepare_v2(db,"INSERT INTO comandas (mesa,cliente,aberta_em) VALUES(?,?,?)",-1,&stmt,NULL)){
            printf("Erro SQL(%s) em: %s\n",sqlite3_errmsg(db),__func__);
            comandas[indiceN].isLivre = 1;
            sqlite3_finalize(stmt);
            return -1;
        }
        totalComandas++;
        sqlite3_bind_text(stmt,1,mesa,-1,NULL);
        sqlite3_bind_text(stmt,2,cliente,-1,NULL);
        sqlite3_bind_null(stmt,3); //TIMESTAMP
        sqlite3_step(stmt);
        id_database_comanda = sqlite3_last_insert_rowid(db);
        comandas[indiceN].id_database_comanda = id_database_comanda;
        sqlite3_finalize(stmt);
        return id_database_comanda;
    }else{
        struct comanda *new_p = realloc(comandas,(totalComandas + 1) * sizeof(struct comanda));
        if(new_p == NULL){
            printf("Erro ao alocar memoria em: %s\n",__func__);
            return -1;
        }
        comandas =  new_p;
        resetarComanda(&comandas[indiceN]);
        if(renomearComanda(&comandas[indiceN], mesa, cliente) == -1){return -1;};
        comandas[indiceN].isLivre = 0;
        //DB ABRIR COMANDA TIMESTAMP;
        if(sqlite3_prepare_v2(db,"INSERT INTO comandas (mesa,cliente,aberta_em) VALUES(?,?,?)",-1,&stmt,NULL)){
            printf("Erro SQL(%s) em: %s\n",sqlite3_errmsg(db),__func__);
            comandas[indiceN].isLivre = 1;
            sqlite3_finalize(stmt);
            return -1;
        }
        totalComandas++;
        sqlite3_bind_text(stmt,1,mesa,-1,NULL);
        sqlite3_bind_text(stmt,2,cliente,-1,NULL);
        sqlite3_bind_null(stmt,3); //TIMESTAMP
        sqlite3_step(stmt);
        id_database_comanda = sqlite3_last_insert_rowid(db);
        comandas[indiceN].id_database_comanda = id_database_comanda;
        sqlite3_finalize(stmt);
        return id_database_comanda;
    }
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

int getItemCardapioIndice(int id_database_item){
    // int id_comanda = getComandaIndice(id_database_comanda);
    for (int indiceN = 0;indiceN < totalItensCardapio; indiceN++){
        // printf("TEST %d\n",comandas[id_comanda].itensComanda[indiceN].id_database);
        if(cardapio[indiceN].id_database_item == id_database_item){
            return indiceN;
        }
    }
    printf("Item nao existe![%s]\n",__func__);
    return -1;
}

int adicionarItemComanda(int id_database_comanda, int quant, int id_database_item,float preco){
    int indiceCardapio = -1;
    int id_comanda = getComandaIndice(id_database_comanda);
    if(comandas[id_comanda].isLivre){
        printf("Comanda esta livre![%s]\n",__func__);
        return -1;
    }
    printf("TEST id_database_item(%d) %s\n",id_database_item,__func__);
    printf("TEST id_database_comanda(%d) %s\n",id_database_comanda,__func__);
    indiceCardapio = getItemCardapioIndice(id_database_item);
    printf("TEST indiceCardapio(%d) %s\n",indiceCardapio,__func__);
    // printf("TEST id_database_comanda(%d) %s\n",id_database_comanda,__func__);
    if(indiceCardapio == -1){return -1;};
    if(comandas[id_comanda].quantidadeItens > 0){
        for(int indiceN = 0;indiceN < comandas[id_comanda].quantidadeItens; indiceN++){
            if(strcmp(comandas[id_comanda].itensComanda[indiceN].nome, cardapio[indiceCardapio].nome) == 0){
                if(comandas[id_comanda].itensComanda[indiceN].preco == preco){
                    if(QUANT_NAO_INFORMADO == quant){
                        comandas[id_comanda].itensComanda[indiceN].quant += 1;
                        // sqlite3_finalize(stmt);
                        return 0;
                    }
                    comandas[id_comanda].itensComanda[indiceN].quant += quant;
                    //DB TIMESTAMP
                    // sqlite3_finalize(stmt);
                    return 0;
                }
            }
        }


        struct item *new_p = realloc(comandas[id_comanda].itensComanda, 
                (comandas[id_comanda].quantidadeItens + 1) * sizeof(struct item));
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
    // if(sqlite3_prepare_v2(db,"SELECT ",-1,&stmt,NULL)){
    //     printf("Erro SQL(%s) em: %s",sqlite3_errmsg(db),__func__);
    //     sqlite3_finalize(stmt);
    //     return -1;
    // }

    //DB TIMESTAMP
    comandas[id_comanda].itensComanda[comandas[id_comanda].quantidadeItens] = cardapio[indiceCardapio];
    if(QUANT_NAO_INFORMADO != quant){
        comandas[id_comanda].itensComanda[comandas[id_comanda].quantidadeItens].quant = quant;
    }else{
        comandas[id_comanda].itensComanda[comandas[id_comanda].quantidadeItens].quant = 1;
    }
    if(PRECO_NAO_INFORMADO != preco){
        comandas[id_comanda].itensComanda[comandas[id_comanda].quantidadeItens].preco = preco;
    }
    comandas[id_comanda].quantidadeItens++;
    // sqlite3_finalize(stmt);
    return 0;
};

void resetarComanda(struct comanda *comanda){
    // int id_comanda = getComandaIndice(id_database_comanda);
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
        //DB FECHAR COMANDA
        if(sqlite3_prepare_v2(db,"UPDATE comandas SET fechada_em = ? WHERE ID == ?",-1,&stmt,NULL)){
            printf("Erro SQL(%s) em:%s\n",sqlite3_errmsg(db),__func__);
            return -1;
        }
        sqlite3_bind_text(stmt,1,NULL,-1,NULL);//TIMESTAMP
        sqlite3_bind_int(stmt,2,id_database_comanda);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        resetarComanda(&comandas[id_comanda]);
        printf("Comanda fechada[test]\n");
        return 0;
    }
    printf("Comanda nao fechada[test]\n");
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
            printf("TEST %d\n",comandas[indiceN].id_database_comanda);
            if(comandas[indiceN].id_database_comanda == id_database_comanda){
                return indiceN;
        }
    }
    printf("Comanda nao existe![%s]\n",__func__);
    return -1;
}