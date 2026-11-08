from ctypes import *
import atexit
import json
import threading
from flask import Flask, render_template, request, Response, jsonify

libpub = CDLL('./libpub.so')
app = Flask(__name__)

# CTYPES ARGTYPES DATABASE
libpub.initDB.argtypes = [c_void_p]

# CTYPES ARGTYPES COMANDA
libpub.criarComanda.argtypes = [c_char_p, c_char_p]
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

# CTYPES ARGTYPES COMANDA > encerrarSistema() | limpar memoria
libpub.encerrarSistema.restype = None

# CTYPES ARGTYPES CARDAPIO
libpub.getItensCardapio.restype = c_void_p
libpub.getItemCardapioIndice.argtypes = [c_int]

# CTYPES ARGTYPES ITEM
libpub.adicionarItem.argtypes = [c_char_p, c_char_p, c_float]
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


@app.route("/criar-comanda", methods=['POST'])
def criar_comanda():
    chamar(
        libpub.criarComanda,
        request.form['Mesa'].encode("utf8"),
        request.form['Cliente'].encode("utf8"),
    )
    return render_template("pages/criarComanda.html")


atexit.register(libpub.encerrarSistema)

if __name__ == "__main__":
    # use_reloader=False: o reloader do debug sobe DOIS processos, e os dois
    # abririam o mesmo data.db (initDB rodando duas vezes). Preco: depois de
    # editar o .py voce reinicia na mao. Templates ainda recarregam sozinhos.
    app.run(debug=True, use_reloader=False)
