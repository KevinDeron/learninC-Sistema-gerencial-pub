from ctypes import *
import atexit
import json
import threading
from flask import Flask, render_template, request, Response, jsonify, redirect, url_for

libpub = CDLL('./libpub.so')
app = Flask(__name__)

# CTYPES ARGTYPES DATABASE
libpub.initDB.argtypes = [c_void_p]

# CTYPES ARGTYPES COMANDA
libpub.criarComanda.argtypes = [c_char_p, c_char_p]
libpub.criarComanda.restype = c_int
libpub.calculaValorTotal.argtypes = [c_int]
libpub.calculaValorTotal.restype = c_double
libpub.adicionarItemComanda.argtypes = [c_int, c_int, c_int, c_double]
libpub.resetarComanda.argtypes = [c_void_p]
libpub.resetarComanda.restype = None
# libpub.fecharComanda.argtypes = [c_int]
libpub.renomearComanda.argtypes = [c_void_p, c_char_p, c_char_p]
libpub.getComandaIndice.argtypes = [c_int]
libpub.getComandasAbertasJSON.restype = c_void_p
libpub.getItensComandaJSON.argtypes = [c_int]
libpub.getItensComandaJSON.restype = c_void_p
libpub.atualizarItemLancamento.argtypes = [c_int, c_int, c_double]
libpub.atualizarItemLancamento.restype = c_int
libpub.removerItemLancamento.argtypes = [c_int]
libpub.removerItemLancamento.restype = c_int

# OPCIONAIS - ainda nao existem no seu C. O app sobe sem elas.
#   void liberarString(void *p)        -> { sqlite3_free(p); }
#   int  fecharComandaPorId(int id)    -> UPDATE comandas SET fechada_em = CURRENT_TIMESTAMP
#                                         WHERE ID = ? AND fechada_em ISNULL; 1 = ok, 0 = falha
# (o fecharComanda atual nao serve pro Flask: pede [s/n] no terminal via
# scanf e depende do array em memoria.)
try:
    libpub.liberarString.argtypes = [c_void_p]
    libpub.liberarString.restype = None
except AttributeError:
    pass
try:
    libpub.fecharComandaPorId.argtypes = [c_int]
    libpub.fecharComandaPorId.restype = c_int
except AttributeError:
    pass
try:
    libpub.renomearComandaPorId.argtypes = [c_int, c_char_p, c_char_p]
    libpub.renomearComandaPorId.restype = c_int
except AttributeError:
    pass

# CTYPES ARGTYPES COMANDA > encerrarSistema() | limpar memoria
libpub.encerrarSistema.restype = None

# CTYPES ARGTYPES CARDAPIO
libpub.getItensCardapio.restype = c_void_p
libpub.getItemCardapioIndice.argtypes = [c_int]

# CTYPES ARGTYPES ITEM
libpub.adicionarItem.argtypes = [c_char_p, c_char_p, c_float]
libpub.adicionarItem.restype = c_int  # id do item novo, -1 = falha/ja existe
libpub.setItemAtivo.argtypes = [c_int, c_int]
# =====================================
libpub.initDB(None)

# O libpub.so usa variaveis globais (db, stmt, sql) em comanda.c, entao duas
# requisicoes ao mesmo tempo se atropelam. TODA chamada ao libpub passa por
# este lock: so uma por vez, mesmo com o servidor multi-thread.
_lock_libpub = threading.RLock()


def chamar(funcao, *args):
    with _lock_libpub:
        return funcao(*args)


def json_do_c(funcao, *args):
    # Chama uma funcao do C que devolve JSON (ponteiro), copia o texto pro
    # Python e devolve a memoria ao C - tudo dentro do lock.
    with _lock_libpub:
        ponteiro = funcao(*args)
        if not ponteiro:
            return ""
        try:
            return string_at(ponteiro).decode("utf-8")
        finally:
            if hasattr(libpub, "liberarString"):
                libpub.liberarString(ponteiro)


class comandas(Structure):
    _fields_ = [("id_database_comanda", c_int),
                ("mesa", c_char * 15),
                ("cliente", c_char * 15),
                ("valorTotal", c_float),
                ("isLivre", c_int),
                ("quantidadeItens", c_int),
                ("itensComanda", c_void_p),
                ]


class item(Structure):
    _fields_ = [("id_database_item", c_int),
                ("nome", c_char * 15),
                ("categoria", c_char * 15),
                ("quant", c_int),
                ("preco", c_float),
                ("isAtivo", c_int),
                ]


@app.route("/")
def home():
    texto = json_do_c(libpub.getComandasAbertasJSON)
    lista_comandas = json.loads(texto) if texto else []
    return render_template(
        "pages/comandas.html",
        comandas=lista_comandas,
        active_page="mesas",
    )


@app.route("/cardapio")
def cardapio():
    # getItensCardapio() deve devolver o JSON (texto do sqlite3_mprintf), ex.:
    #   [{"ID":1,"nome":"Chopp","categoria":"Bebida","preco":12.5}, ...]
    with _lock_libpub:
        ponteiro = libpub.getItensCardapio()
        if not ponteiro:
            return Response("[]", mimetype="application/json")
        texto = string_at(ponteiro).decode("utf-8", errors="replace")
        try:
            itens = json.loads(texto)
        except ValueError:
            # Versao antiga (devolve struct item*): NAO chama liberarString -
            # sqlite3_free num ponteiro do realloc derrubaria o processo.
            return jsonify({"erro": "getItensCardapio ainda nao devolve JSON"}), 501
        if hasattr(libpub, "liberarString"):
            libpub.liberarString(ponteiro)
    return jsonify(itens)


@app.route("/cardapio/itens", methods=['POST'])
def criar_item_cardapio():
    corpo = request.get_json(silent=True) or {}
    nome = str(corpo.get("nome") or "").strip()
    categoria = str(corpo.get("categoria") or "").strip()
    try:
        preco = float(corpo.get("preco"))
    except (TypeError, ValueError):
        return jsonify({"erro": "preco invalido"}), 400

    if not nome:
        return jsonify({"erro": "informe o nome do item"}), 400
    # nome/categoria sao char[15] na struct item: no maximo 14 bytes
    if len(nome.encode("utf8")) > 14 or len(categoria.encode("utf8")) > 14:
        return jsonify({"erro": "nome/categoria com no maximo 14 caracteres (acento conta 2)"}), 400
    if preco < 0:
        return jsonify({"erro": "preco deve ser >= 0"}), 400

    id_item = chamar(libpub.adicionarItem, nome.encode("utf8"), categoria.encode("utf8"), preco)
    if id_item == -1:
        return jsonify({"erro": "nao foi possivel criar (item ja existe?)"}), 409
    return jsonify({"ID": id_item, "nome": nome, "categoria": categoria, "preco": preco}), 201


@app.route("/comanda/<int:id_comanda>/itens")
def comanda_itens(id_comanda):
    # string vazia (ou nada) vira "[]" pro JS nao quebrar
    texto = json_do_c(libpub.getItensComandaJSON, id_comanda)
    return Response(texto or "[]", mimetype="application/json")


@app.route("/comanda/<int:id_comanda>/item/<int:id_lancamento>/atualizar", methods=['POST'])
def atualizar_item(id_comanda, id_lancamento):
    corpo = request.get_json(silent=True) or {}
    try:
        quantidade = int(corpo.get("quantidade", 0))
        preco_unitario = float(corpo.get("preco_unitario", -1))
    except (TypeError, ValueError):
        return jsonify({"erro": "quantidade/preco_unitario invalidos"}), 400

    if quantidade < 1 or preco_unitario < 0:
        return jsonify({"erro": "quantidade deve ser >= 1 e preco >= 0"}), 400

    ok = chamar(libpub.atualizarItemLancamento, id_lancamento, quantidade, preco_unitario)
    if not ok:
        return jsonify({"erro": "nao foi possivel atualizar"}), 400
    return jsonify({"ok": True})


@app.route("/comanda/<int:id_comanda>/item/<int:id_lancamento>/remover", methods=['POST'])
def remover_item(id_comanda, id_lancamento):
    ok = chamar(libpub.removerItemLancamento, id_lancamento)
    if not ok:
        return jsonify({"erro": "nao foi possivel remover"}), 400
    return jsonify({"ok": True})


@app.route("/comanda/<int:id_comanda>/fechar", methods=['POST'])
def fechar_comanda(id_comanda):
    if not hasattr(libpub, "fecharComandaPorId"):
        return jsonify({"erro": "fecharComandaPorId ainda nao existe no libpub.so"}), 501
    ok = chamar(libpub.fecharComandaPorId, id_comanda)
    if not ok:
        return jsonify({"erro": "nao foi possivel fechar a comanda"}), 400
    return jsonify({"ok": True})


def cortar_bytes(texto, limite=14):
    # Os campos do C sao char[15]: cabem 14 BYTES + '\0'. Letra acentuada ocupa
    # 2 bytes em UTF-8, entao cortar por caractere nao basta. Corta em bytes
    # sem partir um caractere no meio.
    return texto.encode("utf8")[:limite].decode("utf8", errors="ignore")


def ler_mesa_cliente(dados):
    # Basta um dos dois; o vazio vira " ", convencao de renomearComanda().
    # Devolve (mesa, cliente) ou None se os dois estiverem vazios.
    mesa = cortar_bytes(str(dados.get('Mesa') or '').strip()).strip()
    cliente = cortar_bytes(str(dados.get('Cliente') or '').strip()).strip()
    if not mesa and not cliente:
        return None
    return mesa or " ", cliente or " "


@app.route("/comanda/<int:id_comanda>/renomear", methods=['POST'])
def renomear_comanda(id_comanda):
    if not hasattr(libpub, "renomearComandaPorId"):
        return jsonify({"erro": "renomearComandaPorId ainda nao existe no libpub.so"}), 501
    nomes = ler_mesa_cliente(request.get_json(silent=True) or {})
    if nomes is None:
        return jsonify({"erro": "informe a mesa ou o cliente"}), 400
    mesa, cliente = nomes

    ok = chamar(libpub.renomearComandaPorId, id_comanda, mesa.encode("utf8"), cliente.encode("utf8"))
    if not ok:
        return jsonify({"erro": "nao foi possivel renomear a comanda"}), 400
    return jsonify({"ok": True, "mesa": mesa, "cliente": cliente})


@app.route("/criar-comanda", methods=['POST'])
def criar_comanda():
    nomes = ler_mesa_cliente(request.form)
    if nomes is None:
        return jsonify({"erro": "informe a mesa ou o cliente"}), 400
    mesa, cliente = nomes

    id_comanda = chamar(libpub.criarComanda, mesa.encode("utf8"), cliente.encode("utf8"))
    if id_comanda == -1:
        return jsonify({"erro": "nao foi possivel criar a comanda"}), 500

    # Pedido do comandas.js (fetch): devolve so o HTML do card novo, pro JS
    # inserir no grid sem recarregar a pagina.
    if request.headers.get("X-Requested-With") == "fetch":
        return render_template(
            "pages/comanda.html",
            comanda={"ID": id_comanda, "mesa": mesa, "cliente": cliente},
        )
    # Sem JS: POST -> redirect: F5 na lista nao recria a comanda
    return redirect(url_for("home"))


atexit.register(libpub.encerrarSistema)

if __name__ == "__main__":
    # use_reloader=False: o reloader do debug sobe DOIS processos, e os dois
    # abririam o mesmo data.db (initDB rodando duas vezes). Preco: depois de
    # editar o .py voce reinicia na mao. Templates ainda recarregam sozinhos.
    app.run(debug=True, use_reloader=False)
