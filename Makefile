# CC = qual compilador usar
CC = gcc

# CFLAGS = flags de compilacao aplicadas a TODO arquivo .c
CFLAGS = -Wall -Wextra

# LDFLAGS = flags usadas so na hora de LINKAR (juntar tudo no executavel final)
LDFLAGS = -lsqlite3

# Lista dos seus arquivos fonte
SRCS = main.c comanda.c cardapio.c item.c database.c

# Troca a extensao .c por .o em cada nome da lista acima (main.c -> main.o, etc)
OBJS = $(SRCS:.c=.o)

TARGET = pub

# Fontes da biblioteca compartilhada: tudo MENOS main.c.
# Uma lib nao tem main() proprio -- quem vai "dirigir" e o Python, chamando as funcoes direto.
LIB_SRCS = comanda.c cardapio.c item.c database.c

# Objetos separados (.pic.o, nao .o) para nao colidir com os objetos do executavel normal.
# Bibliotecas compartilhadas precisam ser compiladas com -fPIC (Position Independent Code) --
# codigo que funciona non-importa em que endereco de memoria for carregado, exigido pelo
# jeito que .so sao carregados dinamicamente (diferente de um executavel, que sempre carrega
# num endereco mais previsivel).
LIB_OBJS = $(LIB_SRCS:.c=.pic.o)
LIB_TARGET = libpub.so

# .PHONY diz que esses "alvos" nao sao nomes de arquivo de verdade,
# sao so comandos (sem isso, se um dia existir um arquivo chamado "clean", o make se confunde)
.PHONY: all clean debug lib

# Alvo padrao (roda quando voce so digita "make", sem argumento nenhum)
all: $(TARGET)

# Como construir o executavel final: precisa de todos os .o prontos primeiro
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

# Regra generica: qualquer arquivo.o depende do arquivo.c de mesmo nome.
# %  e um "coringa" -- essa regra unica serve pra main.o, comanda.o, etc, sem repetir.
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Regra irma da de cima, so que gerando objetos .pic.o (com -fPIC) para a biblioteca.
%.pic.o: %.c
	$(CC) $(CFLAGS) -fPIC -c $< -o $@

# "make lib" gera libpub.so
lib: $(LIB_TARGET)

$(LIB_TARGET): $(LIB_OBJS)
	$(CC) -shared -o $(LIB_TARGET) $(LIB_OBJS) $(LDFLAGS)

# "make debug" compila com AddressSanitizer + simbolos de debug, e forca recompilar tudo
debug: CFLAGS += -fsanitize=address -g
debug: clean all

# "make clean" apaga os arquivos gerados, pra build do zero
clean:
	rm -f $(OBJS) $(LIB_OBJS) $(TARGET) $(LIB_TARGET)