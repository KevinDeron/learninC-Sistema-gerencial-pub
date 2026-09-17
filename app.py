from ctypes import *
import atexit
libpub = CDLL('./libpub.so')
from flask import Flask, render_template, request
app = Flask(__name__)

#CTYPES ARGTYPE DATABASE
libpub.initDB.argtypes = [c_void_p]
#CTYPES ARGTYPES COMANDA
libpub.criarComanda.argtypes = [c_char_p, c_char_p]
libpub.calculaValorTotal.argtypes = [c_int]
libpub.calculaValorTotal.restype = c_float
libpub.adicionarItemComanda.argtypes = [c_int, c_int, c_int, c_float]
libpub.resetarComanda.argtypes = [c_void_p]
libpub.resetarComanda.restype = None
libpub.fecharComanda.argtypes = [c_int]
libpub.renomearComanda.argtypes = [c_void_p, c_char_p, c_char_p]
libpub.getComandaIndice.argtypes = [c_int]
#CTYPES ARGTYPES COMANDA > encerrarSistema() | limpar memoria
libpub.encerrarSistema.restype = None
#CTYPES ARGTYPES CARDAPIO
libpub.getItensCardapio.restype = c_void_p
libpub.getItemCardapioIndice.argtypes = [c_int]
#CTYPES ARGTYPES ITEM
libpub.adicionarItem.argtypes = [c_char_p, c_char_p, c_float]
libpub.setItemAtivo.argtypes = [c_int, c_int]
#=====================================
libpub.initDB(None)
# libpub.getItensCardapio()
# comandas = c_int.in_dll(libpub,"totalItensCardapio")
# print(comandas)
class comandas(Structure):
    _fields_ =  [("id_database_comanda",c_int),
                 ("mesa",c_char_p),
                 ("cliente",c_char_p),
                 ("valorTotal",c_float),
                 ("isLivre",c_int),
                 ("quantidadeItens",c_int),
                 ("itensComanda",c_void_p),
                ]

class item(Structure):
    _fields_ =  [("id_database_item",c_int),
                 ("nome",c_char_p),
                 ("categoria",c_char_p),
                 ("quant",c_int),
                 ("preco",c_float),
                 ("isAtivo",c_int),
                 ]

@app.route("/")
def home():
    return render_template("pages/comandas.html")

@app.route("/criar-comanda", methods=['POST'])
def criar_comanda():
    if request.method == 'POST':
        libpub.criarComanda(request.form['Mesa'].encode("utf8"),request.form['Cliente'].encode("utf8"))
        return render_template("pages/comandas.html")


# if __name__ == "__main__":
#     app.run(debug=True)
    
atexit.register(libpub.encerrarSistema)
    



# libpub.criarComanda('mesa 1'.encode('utf8'),'test123'.encode('utf8'))
