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

int totalComandas = 6; //numero inicial de comandas a serem alocadas em memoria
struct comanda *comandas;
struct item *itensComanda;

int setIniciarComandas(){
    if((comandas = calloc(totalComandas, sizeof(struct comanda))) == NULL){
        printf("Erro ao alocar memoria![setIniciarComandas[comanda]]\n");
        return -1;
    }
    for (int i = 0; i < totalComandas; i++){
        comandas[i].isLivre = 1;
    }
    // itens = realloc(NULL,sizeof(struct item) * totalComandas);
    return 0;
}

int criarComanda(char *mesa, char *cliente){
    int indiceN;
    for (indiceN = 0; indiceN < totalComandas; indiceN++){
        if(comandas[indiceN].isLivre){
            if(renomearComanda(&comandas[indiceN], mesa, cliente) == 0){
                return -1;
            };
            comandas[indiceN].isLivre = 0;
            //DB ABRIR COMANDA TIMESTAMP;
            if(sqlite3_prepare_v2(db,"INSERT INTO comandas (mesa,cliente,aberta_em) VALUES(?,?,?)",-1,&stmt,NULL)){
                printf("Erro SQL(%s) em: %s\n",sqlite3_errmsg(db),__func__);
                sqlite3_finalize(stmt);
                return -1;
            }
            sqlite3_bind_text(stmt,1,mesa,-1,NULL);
            sqlite3_bind_text(stmt,2,cliente,-1,NULL);
            sqlite3_bind_null(stmt,3); //TIMESTAMP
            sqlite3_step(stmt);
            sqlite3_reset(stmt);
            return indiceN;
        }
    }
    totalComandas++;
    struct comanda *new_p = realloc(comandas,totalComandas * sizeof(struct comanda));
    if(new_p == NULL) {
        printf("Erro ao realocar memoria![Comanda]\n");
        totalComandas--;
        return -1;
    }
    comandas = new_p;
    comandas[indiceN].quantidadeItens = 0;
    resetarComanda(&comandas[indiceN]);
    if(renomearComanda(&comandas[indiceN], mesa, cliente) == 0){
        return -1;
    };
    comandas[indiceN].isLivre = 0;
    //DB ABRIR COMANDA TIMESTAMP;
    if(sqlite3_prepare_v2(db,"INSERT INTO comandas (mesa,cliente,aberta_em) VALUES(?,?,?)",-1,&stmt,NULL)){
        printf("Erro SQL(%s) em: %s\n",sqlite3_errmsg(db),__func__);
        sqlite3_finalize(stmt);
        return -1;
    }
    sqlite3_bind_text(stmt,1,mesa,-1,NULL);
    sqlite3_bind_text(stmt,2,cliente,-1,NULL);
    sqlite3_bind_null(stmt,3); //TIMESTAMP
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return indiceN;
};

float calculaValorTotal(struct comanda *comanda){
    float total = 0;
    for(int i = 0;i < comanda->quantidadeItens;i++){
        total += (comanda->itensComanda[i].preco * comanda->itensComanda[i].quant);
    }
    comanda->valorTotal = total;
    return total;
};

int adicionarItemComanda(struct comanda *comanda, int quant, int id_item,float preco){
    if(comanda->isLivre){
        printf("Comanda esta livre!\n");
        return -1;
    }
    if(comanda->quantidadeItens > 0){
        for(int indiceN = 0;indiceN < comanda->quantidadeItens; indiceN++){
            if(strcmp(comanda->itensComanda[indiceN].nome,cardapio[id_item].nome) == 0){
                comanda->itensComanda[indiceN].quant += quant;
                //DB TIMESTAMP
                return 0;
            }
        }


        struct item *new_p = realloc(comanda->itensComanda, (comanda->quantidadeItens + 1) * sizeof(struct item));
        if(new_p == NULL){
            printf("Falha ao alocar memoria %s\n",__func__);
            return -1;
        }
        comanda->itensComanda = new_p;
    }else{
        if((comanda->itensComanda = calloc(1,sizeof(struct item))) == NULL){
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
    comanda->itensComanda[comanda->quantidadeItens] = cardapio[id_item];
    if(QUANT_NAO_INFORMADO != quant){
        comanda->itensComanda[comanda->quantidadeItens].quant = quant;
    }else{
        comanda->itensComanda[comanda->quantidadeItens].quant = 1;
    }
    if(PRECO_NAO_INFORMADO != preco){
        comanda->itensComanda[comanda->quantidadeItens].preco = preco;
    }
    comanda->quantidadeItens++;
    return 0;
};

void resetarComanda(struct comanda *comanda){
    comanda->isLivre = 1;
    comanda->valorTotal = 0;
    strcpy(comanda->mesa,"");
    strcpy(comanda->cliente,"");
    free(comanda->itensComanda);
    comanda->quantidadeItens = 0;
};

void fecharComanda(struct comanda *comanda){
    if(comanda->isLivre){
        printf("Comanda esta livre\n");
        return;
    }
       
    printf("Comanda: %s(%s)\n", comanda->mesa, comanda->cliente);
    for (int i = 0; i < comanda->quantidadeItens; i++){
        printf("%d | %s | %0.2f\n", 
            comanda->itensComanda[i].quant, comanda->itensComanda[i].nome, comanda->itensComanda[i].preco);
    }
    calculaValorTotal(comanda);
    printf("Valor total: %0.2f\n", comanda->valorTotal);
    printf("Deseja fechar[s/n?[placeholder]\n");
    char r;
    scanf(" %c", &r);
    if(r == 's'){
        resetarComanda(comanda);
        printf("Comanda fechada[test]\n");
        return;
    }
    printf("Comanda nao fechada[test]\n");
};

int renomearComanda(struct comanda *comanda, char *novoNomeMesa, char *novoNomeCliente){
    if(strlen(novoNomeMesa) <= 14 && strlen(novoNomeCliente) <= 14){
        strcpy(comanda->mesa, novoNomeMesa);
        strcpy(comanda->cliente, novoNomeCliente);
        printf("Comanda renomeada para %s(%s)\n",comanda->mesa, comanda->cliente);
        return 1;
    }
    printf("Nome maior que %zu(-1) caracteres!\n", sizeof(comanda->cliente));
    return 0;
};