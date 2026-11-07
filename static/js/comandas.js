// Fila global: garante que so existe UM fetch de itens em voo por vez,
// mesmo se varios cards abrirem quase ao mesmo tempo no carregamento da
// pagina. Isso evita bater varias chamadas concorrentes no libpub.so
// (que reusa handles globais de sqlite3_stmt em comanda.c).
let filaCarregamento = Promise.resolve();

document.addEventListener('DOMContentLoaded', () => {
    const cards = Array.from(document.querySelectorAll('.mesa-card'));

    cards.forEach((card) => {
        if (card.open) {
            enfileirarCarregamento(card);
        }

        card.addEventListener('toggle', () => {
            if (!card.open) {
                return;
            }
            enfileirarCarregamento(card);
        });
    });
});

function enfileirarCarregamento(card) {
    if (card.dataset.itensCarregados === 'true' || card.dataset.itensCarregando === 'true') {
        return;
    }
    card.dataset.itensCarregando = 'true';
    filaCarregamento = filaCarregamento.then(() => carregarItens(card));
}

function carregarItens(card) {
    const id = card.dataset.comandaId;
    const container = card.querySelector('.mesa-itens-container');

    return fetch(`/comanda/${id}/itens`)
        .then((resposta) => {
            if (!resposta.ok) {
                throw new Error(`HTTP ${resposta.status}`);
            }
            return resposta.json();
        })
        .then((itens) => {
            renderItens(container, itens, id);
            card.dataset.itensCarregados = 'true';
        })
        .catch((erro) => {
            container.innerHTML = '';
            const msg = document.createElement('p');
            msg.className = 'mesa-erro';
            msg.textContent = 'Não foi possível carregar os itens.';
            container.appendChild(msg);
            console.error('Falha ao buscar itens da comanda', id, erro);
        })
        .finally(() => {
            card.dataset.itensCarregando = 'false';
        });
}

function renderItens(container, itens, idComanda) {
    container.innerHTML = '';

    if (!itens || itens.length === 0) {
        const vazio = document.createElement('p');
        vazio.className = 'mesa-vazia';
        vazio.textContent = 'Nenhum item lançado ainda.';
        container.appendChild(vazio);
        return;
    }

    const lista = document.createElement('ul');
    lista.className = 'mesa-itens';

    itens.forEach((item) => {
        lista.appendChild(criarLinhaItem(item, idComanda));
    });

    container.appendChild(lista);
}

// sqlite3_column_text devolve tudo como texto, entao os numeros chegam
// como string mesmo sendo numeros no banco - por isso o Number() aqui.
function criarLinhaItem(item, idComanda) {
    const idLancamento = item.id_lancamento; // precisa existir no JSON - ver nota acima
    const quantidadeInicial = Number(item.quantidade);
    const precoInicial = Number(item.preco_unitario);

    const li = document.createElement('li');
    li.className = 'mesa-item';
    li.dataset.idLancamento = idLancamento;

    const inputQuant = document.createElement('input');
    inputQuant.type = 'number';
    inputQuant.className = 'item-quant-input';
    inputQuant.min = '0';
    inputQuant.step = '1';
    inputQuant.value = quantidadeInicial;

    const btnMenos = document.createElement('button');
    btnMenos.type = 'button';
    btnMenos.className = 'btn-quant';
    btnMenos.textContent = '−';
    btnMenos.setAttribute('aria-label', 'Diminuir quantidade');
    btnMenos.addEventListener('click', () => ajustarQuantidade(inputQuant, -1));

    const btnMais = document.createElement('button');
    btnMais.type = 'button';
    btnMais.className = 'btn-quant';
    btnMais.textContent = '+';
    btnMais.setAttribute('aria-label', 'Aumentar quantidade');
    btnMais.addEventListener('click', () => ajustarQuantidade(inputQuant, 1));

    const quantWrap = document.createElement('span');
    quantWrap.className = 'item-quant-wrap';
    quantWrap.append(btnMenos, inputQuant, btnMais);

    const nome = document.createElement('span');
    nome.className = 'item-nome';
    nome.textContent = item.item; // trocar item.nome se o alias no SQL for outro

    const inputPreco = document.createElement('input');
    inputPreco.type = 'number';
    inputPreco.className = 'item-preco-input';
    inputPreco.min = '0';
    inputPreco.step = '0.01';
    inputPreco.value = precoInicial.toFixed(2);

    const btnConfirmar = document.createElement('button');
    btnConfirmar.type = 'button';
    btnConfirmar.className = 'btn-confirmar-item';
    btnConfirmar.textContent = '✓';
    btnConfirmar.title = 'Salvar alterações';
    btnConfirmar.addEventListener('click', () => salvarItem(li, idComanda));

    const confirmarComEnter = (ev) => {
        if (ev.key === 'Enter') {
            ev.preventDefault();
            salvarItem(li, idComanda);
        }
    };
    inputQuant.addEventListener('keydown', confirmarComEnter);
    inputPreco.addEventListener('keydown', confirmarComEnter);

    li.append(quantWrap, nome, inputPreco, btnConfirmar);
    return li;
}

function ajustarQuantidade(inputQuant, delta) {
    const atual = Number(inputQuant.value) || 0;
    inputQuant.value = Math.max(0, atual + delta);
}

function salvarItem(li, idComanda) {
    const idLancamento = li.dataset.idLancamento;
    const quantidade = Number(li.querySelector('.item-quant-input').value);
    const precoUnitario = Number(li.querySelector('.item-preco-input').value);

    limparEstadoItem(li);

    if (quantidade <= 0) {
        removerItem(li, idComanda, idLancamento);
        return;
    }

    fetch(`/comanda/${idComanda}/item/${idLancamento}/atualizar`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ quantidade, preco_unitario: precoUnitario }),
    })
        .then((resposta) => {
            if (!resposta.ok) {
                throw new Error(`HTTP ${resposta.status}`);
            }
            li.classList.add('item-salvo');
            setTimeout(() => li.classList.remove('item-salvo'), 800);
        })
        .catch((erro) => {
            li.classList.add('item-erro');
            console.error('Falha ao atualizar item', idLancamento, erro);
        });
}

function removerItem(li, idComanda, idLancamento) {
    fetch(`/comanda/${idComanda}/item/${idLancamento}/remover`, { method: 'POST' })
        .then((resposta) => {
            if (!resposta.ok) {
                throw new Error(`HTTP ${resposta.status}`);
            }
            li.remove();
        })
        .catch((erro) => {
            li.classList.add('item-erro');
            console.error('Falha ao remover item', idLancamento, erro);
        });
}

function limparEstadoItem(li) {
    li.classList.remove('item-salvo', 'item-erro');
}