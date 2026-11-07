from ctypes import *
import atexit
import json
from flask import Flask, render_template, request, Response, jsonify

libpub = CDLL('./libpub.so')
app = Flask(__name__)

# CTYPES ARGTYPES DATABASE
libpub.initDB.argtypes = [c_void_p]

# CTYPES ARGTYPES COMANDA
libpub.criarComanda.argtypes = [c_char_p, c_char_p]
libpub.calculaValorTotal.argtypes = [c_int]
libpub.calculaValorTotal.restype = c_float
libpub.adicionarItemComanda.argtypes = [c_int, c_int, c_int, c_float]
libpub.resetarComanda.argtypes = [c_void_p]
libpub.resetarComanda.restype = None
libpub.fecharComanda.argtypes = [c_int]
libpub.renomearComanda.argtypes = [c_void_p, c_char_p, c_char_p]
libpub.getComandaIndice.argtypes = [c_int]

libpub.getComandasAbertasJSON.restype = c_char_p
libpub.getItensComandaJSON.argtypes = [c_int]
libpub.getItensComandaJSON.restype = c_char_p

libpub.atualizarItemLancamento.argtypes = [c_int, c_int, c_float]
libpub.atualizarItemLancamento.restype = c_int
libpub.removerItemLancamento.argtypes = [c_int]
libpub.removerItemLancamento.restype = c_int

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
    dados = libpub.getComandasAbertasJSON()
    lista_comandas = json.loads(dados.decode()) if dados else []
    return render_template(
        "pages/comandas.html",
        comandas=lista_comandas,
        active_page="mesas",
    )


@app.route("/comanda/<int:id_comanda>/itens")
def comanda_itens(id_comanda):
    # getItensComandaJSON deve devolver "[]" (ou string vazia) quando a
    # comanda nao tem itens - os dois casos viram "[]" aqui.
    dados = libpub.getItensComandaJSON(id_comanda)
    corpo = dados if dados else b"[]"
    return Response(corpo, mimetype="application/json")


@app.route("/comanda/<int:id_comanda>/item/<int:id_lancamento>/atualizar", methods=['POST'])
def atualizar_item(id_comanda, id_lancamento):
    corpo = request.get_json(silent=True) or {}
    try:
        quantidade = int(corpo.get("quantidade", 0))
        preco_unitario = float(corpo.get("preco_unitario", 0))
    except (TypeError, ValueError):
        return jsonify({"erro": "quantidade/preco_unitario invalidos"}), 400

    ok = libpub.atualizarItemLancamento(id_lancamento, quantidade, preco_unitario)
    if not ok:
        return jsonify({"erro": "nao foi possivel atualizar"}), 400
    return jsonify({"ok": True})


@app.route("/comanda/<int:id_comanda>/item/<int:id_lancamento>/remover", methods=['POST'])
def remover_item(id_comanda, id_lancamento):
    ok = libpub.removerItemLancamento(id_lancamento)
    if not ok:
        return jsonify({"erro": "nao foi possivel remover"}), 400
    return jsonify({"ok": True})


@app.route("/criar-comanda", methods=['POST'])
def criar_comanda():
    libpub.criarComanda(
        request.form['Mesa'].encode("utf8"),
        request.form['Cliente'].encode("utf8"),
    )
    return render_template("pages/criarComanda.html")


if __name__ == "__main__":
    app.run(debug=True)

atexit.register(libpub.encerrarSistema)