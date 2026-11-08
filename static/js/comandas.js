// ======================================================================
// Fila unica: TODA requisicao desta aba (carregar itens, salvar, remover,
// fechar) passa por aqui, uma de cada vez. O servidor tambem tem um lock
// em volta do libpub.so, mas manter a fila aqui evita encher o servidor
// de requisicoes paralelas a toa.
// ======================================================================
let fila = Promise.resolve();

function naFila(tarefa) {
    const promessa = fila.then(tarefa);
    fila = promessa.catch(() => {}); // um erro nao pode travar a fila
    return promessa;
}

const INTERVALO_AUTOSAVE_MS = 5000;

const ICONE_LIXEIRA =
    '<svg viewBox="0 0 24 24" width="16" height="16" fill="none" stroke="currentColor" ' +
    'stroke-width="2" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true">' +
    '<path d="M3 6h18"/><path d="M8 6V4a1 1 0 0 1 1-1h6a1 1 0 0 1 1 1v2"/>' +
    '<path d="M19 6l-1 14a2 2 0 0 1-2 2H8a2 2 0 0 1-2-2L5 6"/>' +
    '<path d="M10 11v6"/><path d="M14 11v6"/></svg>';

document.addEventListener('DOMContentLoaded', () => {
    document.querySelectorAll('.mesa-card').forEach((card) => {
        // Card que ja nasce aberto nao dispara "toggle" no carregamento.
        if (card.open) {
            enfileirarCarregamento(card);
        }
        card.addEventListener('toggle', () => {
            if (card.open) {
                enfileirarCarregamento(card);
            }
        });

        const botaoFechar = card.querySelector('[data-fechar]');
        if (botaoFechar) {
            botaoFechar.addEventListener('click', () => fecharMesa(card));
        }
    });

    // Autosave: a cada 5s grava (ou tenta de novo) tudo que estiver pendente.
    setInterval(salvarPendentes, INTERVALO_AUTOSAVE_MS);

    // Ao recarregar/fechar a aba, manda o que sobrou sem esperar resposta.
    window.addEventListener('pagehide', salvarAoSair);
    document.addEventListener('visibilitychange', () => {
        if (document.visibilityState === 'hidden') {
            salvarAoSair();
        }
    });
});

// ---------------------------------------------------------------------
// Carregar itens
// ---------------------------------------------------------------------

function enfileirarCarregamento(card) {
    if (card.dataset.itensCarregados === 'true' || card.dataset.itensCarregando === 'true') {
        return;
    }
    card.dataset.itensCarregando = 'true';
    naFila(() => carregarItens(card));
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

function mostrarVazio(container) {
    container.innerHTML = '';
    const vazio = document.createElement('p');
    vazio.className = 'mesa-vazia';
    vazio.textContent = 'Nenhum item lançado ainda.';
    container.appendChild(vazio);
}

function renderItens(container, itens, idComanda) {
    if (!itens || itens.length === 0) {
        mostrarVazio(container);
        return;
    }

    container.innerHTML = '';
    const lista = document.createElement('ul');
    lista.className = 'mesa-itens';
    itens.forEach((item) => lista.appendChild(criarLinhaItem(item, idComanda)));
    container.appendChild(lista);
}

// ---------------------------------------------------------------------
// Uma linha de item:  [-] [qtd] [+]   nome   [preco]   [lixeira]
// ---------------------------------------------------------------------

// sqlite3_column_text devolve tudo como texto, entao os numeros chegam
// como string mesmo sendo numeros no banco - por isso o Number() aqui.
function criarLinhaItem(item, idComanda) {
    const li = document.createElement('li');
    li.className = 'mesa-item';
    li.dataset.idLancamento = item.id_lancamento;
    li.dataset.idComanda = idComanda;

    const inputQuant = document.createElement('input');
    inputQuant.type = 'number';
    inputQuant.className = 'item-quant-input';
    inputQuant.min = '1';
    inputQuant.step = '1';
    inputQuant.value = Number(item.quantidade);

    const btnMenos = criarBotaoQuantidade('−', 'Diminuir quantidade');
    btnMenos.addEventListener('click', () => ajustarQuantidade(li, inputQuant, -1));

    const btnMais = criarBotaoQuantidade('+', 'Aumentar quantidade');
    btnMais.addEventListener('click', () => ajustarQuantidade(li, inputQuant, 1));

    const quantWrap = document.createElement('span');
    quantWrap.className = 'item-quant-wrap';
    quantWrap.append(btnMenos, inputQuant, btnMais);

    const nome = document.createElement('span');
    nome.className = 'item-nome';
    nome.textContent = item.item;
    nome.title = item.item; // nome completo ao passar o mouse, se for cortado

    const inputPreco = document.createElement('input');
    inputPreco.type = 'number';
    inputPreco.className = 'item-preco-input';
    inputPreco.min = '0';
    inputPreco.step = '0.01';
    inputPreco.value = Number(item.preco_unitario).toFixed(2);

    const btnRemover = document.createElement('button');
    btnRemover.type = 'button';
    btnRemover.className = 'btn-remover-item';
    btnRemover.title = 'Remover item';
    btnRemover.setAttribute('aria-label', 'Remover item');
    btnRemover.innerHTML = ICONE_LIXEIRA; // constante nossa, sem dado do usuario
    btnRemover.addEventListener('click', () => removerItem(li));

    [inputQuant, inputPreco].forEach((campo) => {
        // digitou algo -> pendente (o autosave grava em ate 5s)
        campo.addEventListener('input', () => marcarSujo(li));
        // saiu do campo -> grava agora
        campo.addEventListener('change', () => salvarItem(li));
        // Enter -> grava agora
        campo.addEventListener('keydown', (ev) => {
            if (ev.key === 'Enter') {
                ev.preventDefault();
                salvarItem(li);
            }
        });
    });

    li.append(quantWrap, nome, inputPreco, btnRemover);
    return li;
}

function criarBotaoQuantidade(texto, rotulo) {
    const btn = document.createElement('button');
    btn.type = 'button';
    btn.className = 'btn-quant';
    btn.textContent = texto;
    btn.setAttribute('aria-label', rotulo);
    return btn;
}

function ajustarQuantidade(li, inputQuant, delta) {
    const atual = Number(inputQuant.value) || 1;
    // minimo 1: para tirar o item de vez existe a lixeira
    inputQuant.value = Math.max(1, atual + delta);
    marcarSujo(li);
}

function marcarSujo(li) {
    li.classList.add('item-sujo');
    li.classList.remove('item-erro', 'item-salvo');
}

// Le e valida os campos. Devolve null se algum valor for invalido.
function lerLinha(li) {
    const quantidade = Number(li.querySelector('.item-quant-input').value);
    const textoPreco = li.querySelector('.item-preco-input').value;
    const preco = Number(textoPreco);

    if (!Number.isInteger(quantidade) || quantidade < 1) {
        return null;
    }
    if (textoPreco === '' || !Number.isFinite(preco) || preco < 0) {
        return null;
    }
    return { quantidade, preco };
}

// ---------------------------------------------------------------------
// Salvar (autosave, Enter, sair do campo, fechar mesa)
// ---------------------------------------------------------------------

// Resolve com true se a linha esta gravada, false se falhou.
function salvarItem(li) {
    return naFila(() => enviarItem(li));
}

async function enviarItem(li) {
    // Roda na vez dela na fila: se outra chamada ja gravou, nao ha mais nada a fazer.
    if (!li.isConnected || !li.classList.contains('item-sujo')) {
        return true;
    }

    const enviado = lerLinha(li);
    if (!enviado) {
        li.classList.add('item-erro');
        return false;
    }

    const url = `/comanda/${li.dataset.idComanda}/item/${li.dataset.idLancamento}/atualizar`;
    try {
        const resposta = await fetch(url, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ quantidade: enviado.quantidade, preco_unitario: enviado.preco }),
        });
        if (!resposta.ok) {
            throw new Error(`HTTP ${resposta.status}`);
        }

        // Se o usuario mexeu durante o envio, continua pendente.
        const agora = lerLinha(li);
        if (agora && agora.quantidade === enviado.quantidade && agora.preco === enviado.preco) {
            li.classList.remove('item-sujo');
        }
        li.classList.remove('item-erro');
        li.classList.add('item-salvo');
        setTimeout(() => li.classList.remove('item-salvo'), 800);
        return true;
    } catch (erro) {
        li.classList.add('item-erro'); // continua "sujo": o autosave tenta de novo
        console.error('Falha ao salvar item', li.dataset.idLancamento, erro);
        return false;
    }
}

function salvarPendentes() {
    document.querySelectorAll('.mesa-item.item-sujo').forEach((li) => {
        if (li.dataset.naFila === 'true') {
            return; // ja tem um salvamento esperando na fila
        }
        li.dataset.naFila = 'true';
        salvarItem(li).finally(() => {
            li.dataset.naFila = 'false';
        });
    });
}

// Ao sair da pagina nao da pra esperar resposta: sendBeacon e feito pra isso.
function salvarAoSair() {
    document.querySelectorAll('.mesa-item.item-sujo').forEach((li) => {
        const dados = lerLinha(li);
        if (!dados) {
            return;
        }
        const url = `/comanda/${li.dataset.idComanda}/item/${li.dataset.idLancamento}/atualizar`;
        const corpo = new Blob(
            [JSON.stringify({ quantidade: dados.quantidade, preco_unitario: dados.preco })],
            { type: 'application/json' }
        );
        navigator.sendBeacon(url, corpo);
    });
}

// ---------------------------------------------------------------------
// Remover item (lixeira)
// ---------------------------------------------------------------------

function removerItem(li) {
    const nome = li.querySelector('.item-nome').textContent;
    if (!confirm(`Remover "${nome}" da comanda?`)) {
        return;
    }

    const url = `/comanda/${li.dataset.idComanda}/item/${li.dataset.idLancamento}/remover`;
    naFila(() => fetch(url, { method: 'POST' }))
        .then((resposta) => {
            if (!resposta.ok) {
                throw new Error(`HTTP ${resposta.status}`);
            }
            const container = li.closest('.mesa-itens-container');
            li.remove();
            if (container && !container.querySelector('.mesa-item')) {
                mostrarVazio(container);
            }
        })
        .catch((erro) => {
            li.classList.add('item-erro');
            console.error('Falha ao remover item', li.dataset.idLancamento, erro);
            alert('Não foi possível remover o item.');
        });
}

// ---------------------------------------------------------------------
// Fechar mesa: grava tudo, confirma com o total, e so entao fecha.
// ---------------------------------------------------------------------

function somarTotal(card) {
    let total = 0;
    card.querySelectorAll('.mesa-item').forEach((li) => {
        const dados = lerLinha(li);
        if (dados) {
            total += dados.quantidade * dados.preco;
        }
    });
    return total;
}

async function fecharMesa(card) {
    const id = card.dataset.comandaId;
    const nome = card.querySelector('.mesa-nome').textContent.trim();
    const botao = card.querySelector('[data-fechar]');

    if (card.dataset.itensCarregados !== 'true') {
        alert('Os itens ainda estão carregando. Tente de novo em instantes.');
        return;
    }

    botao.disabled = true;
    try {
        // 1) grava tudo que estiver pendente nesta mesa
        const sujas = Array.from(card.querySelectorAll('.mesa-item.item-sujo'));
        const resultados = await Promise.all(sujas.map((li) => salvarItem(li)));
        if (resultados.includes(false)) {
            alert('Há itens que não foram salvos (campos em vermelho). Corrija e tente fechar de novo.');
            return;
        }

        // 2) confirma, mostrando o total
        const total = somarTotal(card);
        if (!confirm(`Fechar ${nome}?\nTotal: R$ ${total.toFixed(2)}`)) {
            return;
        }

        // 3) fecha
        const resposta = await naFila(() => fetch(`/comanda/${id}/fechar`, { method: 'POST' }));
        if (!resposta.ok) {
            const corpo = await resposta.json().catch(() => ({}));
            throw new Error(corpo.erro || `HTTP ${resposta.status}`);
        }

        card.remove();
        const grid = document.querySelector('.comandas-grid');
        if (grid && !grid.querySelector('.mesa-card')) {
            const vazio = document.createElement('p');
            vazio.className = 'comandas-vazio';
            vazio.textContent = 'Nenhuma comanda aberta.';
            grid.appendChild(vazio);
        }
    } catch (erro) {
        console.error('Falha ao fechar mesa', id, erro);
        alert(`Não foi possível fechar a mesa: ${erro.message}`);
    } finally {
        botao.disabled = false;
    }
}
