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
    document.querySelectorAll('.mesa-card').forEach(iniciarCard);

    iniciarMenuFlutuante();
    iniciarBoxCardapio();
    iniciarBoxNovoItem();

    const formNova = document.querySelector('[data-nova-comanda]');
    if (formNova) {
        formNova.addEventListener('submit', (ev) => criarComanda(ev, formNova));
    }

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

// Liga os eventos de um card de mesa (os da pagina e os criados via JS).
function iniciarCard(card) {
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

    const botaoAdicionar = card.querySelector('[data-adicionar]');
    if (botaoAdicionar) {
        botaoAdicionar.addEventListener('click', () => abrirBoxCardapio(card));
    }

    iniciarArrastarCard(card);

    const botaoEditar = card.querySelector('[data-editar]');
    const formRenomear = card.querySelector('[data-renomear]');
    if (botaoEditar && formRenomear) {
        botaoEditar.addEventListener('click', (ev) => {
            ev.preventDefault(); // dentro do <summary>: nao abre/fecha o card
            abrirRenomear(card);
        });
        formRenomear.addEventListener('submit', (ev) => {
            ev.preventDefault();
            renomearComanda(card);
        });
        formRenomear.querySelector('[data-cancelar]')
            .addEventListener('click', () => fecharRenomear(card));
        formRenomear.addEventListener('keydown', (ev) => {
            if (ev.key === 'Escape') {
                fecharRenomear(card);
            }
        });
    }
}

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
    inputQuant.autocomplete = 'off';
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
    inputPreco.autocomplete = 'off';
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
            grid.prepend(vazio); // o card "Nova comanda" continua sendo o ultimo
        }
    } catch (erro) {
        console.error('Falha ao fechar mesa', id, erro);
        alert(`Não foi possível fechar a mesa: ${erro.message}`);
    } finally {
        botao.disabled = false;
    }
}

// ---------------------------------------------------------------------
// Nova comanda: basta Mesa OU Cliente (o vazio vira " " no servidor)
// ---------------------------------------------------------------------

// Valida; devolve false (e mostra o aviso) se os dois campos estao vazios.
function validarNovaComanda(form) {
    const mesa = form.elements.Mesa;
    const cliente = form.elements.Cliente;
    if (mesa.value.trim() === '' && cliente.value.trim() === '') {
        mesa.setCustomValidity('Preencha a mesa ou o cliente.');
        mesa.reportValidity();
        // limpa o aviso assim que o usuario digitar em qualquer um dos dois
        const limpar = () => mesa.setCustomValidity('');
        mesa.addEventListener('input', limpar, { once: true });
        cliente.addEventListener('input', limpar, { once: true });
        return false;
    }
    return true;
}

// Envia via fetch e insere o card devolvido pelo servidor, sem recarregar.
async function criarComanda(ev, form) {
    ev.preventDefault();
    if (!validarNovaComanda(form)) {
        return;
    }

    const botao = form.querySelector('button[type="submit"]');
    botao.disabled = true;
    try {
        const resposta = await naFila(() => fetch(form.action, {
            method: 'POST',
            headers: { 'X-Requested-With': 'fetch' },
            body: new FormData(form),
        }));
        if (!resposta.ok) {
            const corpo = await resposta.json().catch(() => ({}));
            throw new Error(corpo.erro || `HTTP ${resposta.status}`);
        }

        // HTML vem do template comanda.html (Jinja ja escapou mesa/cliente)
        const molde = document.createElement('template');
        molde.innerHTML = (await resposta.text()).trim();
        const card = molde.content.querySelector('.mesa-card');
        if (!card) {
            throw new Error('resposta sem card');
        }

        const grid = document.querySelector('.comandas-grid');
        grid.querySelector('.comandas-vazio')?.remove();
        form.closest('.nova-comanda-card').before(card);
        iniciarCard(card);

        form.reset();
        form.elements.Mesa.focus();
    } catch (erro) {
        console.error('Falha ao criar comanda', erro);
        alert(`Não foi possível criar a comanda: ${erro.message}`);
    } finally {
        botao.disabled = false;
    }
}

// ---------------------------------------------------------------------
// Renomear comanda (lapis no cabecalho do card)
// ---------------------------------------------------------------------

function nomesAtuais(card) {
    const cliente = card.querySelector('.mesa-cliente');
    return {
        mesa: card.querySelector('.mesa-nome').textContent.trim(),
        cliente: cliente ? cliente.textContent.trim() : '',
    };
}

function abrirRenomear(card) {
    const form = card.querySelector('[data-renomear]');
    const atuais = nomesAtuais(card);
    form.elements.Mesa.value = atuais.mesa;
    form.elements.Cliente.value = atuais.cliente;
    form.elements.Mesa.setCustomValidity('');

    // Card recolhido esconde tudo menos o <summary>, e o summary some durante a
    // edicao: abre o card agora e lembra de recolher de novo ao terminar.
    card.dataset.recolherAoTerminar = card.open ? 'false' : 'true';
    card.open = true;

    card.querySelector('.mesa-card-header').hidden = true;
    form.hidden = false;
    form.elements.Mesa.focus();
    form.elements.Mesa.select();
}

function fecharRenomear(card) {
    card.querySelector('[data-renomear]').hidden = true;
    card.querySelector('.mesa-card-header').hidden = false;
    if (card.dataset.recolherAoTerminar === 'true') {
        card.open = false;
    }
    card.dataset.recolherAoTerminar = 'false';
}

// Atualiza os textos do cabecalho com o que o servidor gravou.
function aplicarNomes(card, mesa, cliente) {
    card.querySelector('.mesa-nome').textContent = mesa;

    let spanCliente = card.querySelector('.mesa-cliente');
    if (cliente.trim() === '') {
        spanCliente?.remove();
        return;
    }
    if (!spanCliente) {
        spanCliente = document.createElement('span');
        spanCliente.className = 'mesa-cliente';
        card.querySelector('.mesa-card-header').appendChild(spanCliente);
    }
    spanCliente.textContent = cliente;
}

async function renomearComanda(card) {
    const form = card.querySelector('[data-renomear]');
    if (!validarNovaComanda(form)) { // mesma regra: mesa OU cliente
        return;
    }

    const botao = form.querySelector('button[type="submit"]');
    botao.disabled = true;
    try {
        const resposta = await naFila(() => fetch(`/comanda/${card.dataset.comandaId}/renomear`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({
                Mesa: form.elements.Mesa.value,
                Cliente: form.elements.Cliente.value,
            }),
        }));
        const corpo = await resposta.json().catch(() => ({}));
        if (!resposta.ok) {
            throw new Error(corpo.erro || `HTTP ${resposta.status}`);
        }
        aplicarNomes(card, corpo.mesa, corpo.cliente);
        fecharRenomear(card);
    } catch (erro) {
        console.error('Falha ao renomear comanda', card.dataset.comandaId, erro);
        alert(`Não foi possível renomear: ${erro.message}`);
    } finally {
        botao.disabled = false;
    }
}

// ---------------------------------------------------------------------
// Menu flutuante: engrenagem que sobe/recolhe o balao de acoes
// ---------------------------------------------------------------------

function iniciarMenuFlutuante() {
    const menu = document.querySelector('[data-menu-flutuante]');
    if (!menu) {
        return;
    }
    const botao = menu.querySelector('.btn-engrenagem');

    const definirAberto = (aberto) => {
        menu.classList.toggle('is-aberto', aberto);
        botao.setAttribute('aria-expanded', String(aberto));
    };

    botao.addEventListener('click', () => {
        definirAberto(!menu.classList.contains('is-aberto'));
    });

    // clique fora ou Esc recolhe
    document.addEventListener('click', (ev) => {
        if (!menu.contains(ev.target)) {
            definirAberto(false);
        }
    });
    document.addEventListener('keydown', (ev) => {
        if (ev.key === 'Escape' && menu.classList.contains('is-aberto')) {
            definirAberto(false);
            botao.focus();
        }
    });

    menu.querySelectorAll('[data-acao]').forEach((item) => {
        item.addEventListener('click', () => {
            definirAberto(false);
            if (item.dataset.acao === 'nova-comanda') {
                abrirCardNovaComanda();
            } else if (item.dataset.acao === 'novo-item') {
                abrirBoxNovoItem();
            }
        });
    });
}

// Leva ate o card "+ Nova comanda", abre e foca no campo Mesa.
function abrirCardNovaComanda() {
    const card = document.querySelector('.nova-comanda-card');
    if (!card) {
        return;
    }
    card.open = true;
    card.scrollIntoView({ behavior: 'smooth', block: 'center' });
    card.querySelector('input[name="Mesa"]').focus({ preventScroll: true });
}

// ---------------------------------------------------------------------
// Box do cardapio (botao "Adicionar" do card)
//   digitar filtra | setas escolhem | Enter ou clique lanca na comanda
//   nome que nao existe -> "Criar item" (cadastra e ja lanca)
// ---------------------------------------------------------------------

const boxCardapio = {
    dialog: null,
    card: null,       // card da comanda que abriu o box
    itens: [],        // cardapio vindo de GET /cardapio
    carregado: false, // so oferece "criar" depois de saber o que ja existe
    ativo: -1,        // posicao escolhida pelas setas; == filtrados.length -> "Criar item"
    lancando: false,  // evita lancar duas vezes com Enter repetido
};

// minusculo e sem acento: "Pão" acha "pao"
function normalizar(texto) {
    return String(texto ?? '').normalize('NFD').replace(/[\u0300-\u036f]/g, '').toLowerCase().trim();
}

function formatarPreco(valor) {
    return `R$ ${Number(valor).toFixed(2).replace('.', ',')}`;
}

function elBox(seletor) {
    return boxCardapio.dialog.querySelector(seletor);
}

// GET /cardapio -> lista de itens ativos {ID, nome, categoria, preco}
async function buscarCardapio() {
    const resposta = await naFila(() => fetch('/cardapio'));
    const corpo = await resposta.json().catch(() => null);
    if (!resposta.ok || !Array.isArray(corpo)) {
        throw new Error((corpo && corpo.erro) || `HTTP ${resposta.status}`);
    }
    return corpo;
}

// POST /cardapio/itens -> item criado {ID, nome, categoria, preco}
async function cadastrarItem(nome, categoria, preco) {
    const resposta = await naFila(() => fetch('/cardapio/itens', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ nome, categoria, preco }),
    }));
    const corpo = await resposta.json().catch(() => ({}));
    if (!resposta.ok) {
        throw new Error(corpo.erro || `HTTP ${resposta.status}`);
    }
    return corpo;
}

function iniciarBoxCardapio() {
    const dialog = document.querySelector('[data-box-cardapio]');
    if (!dialog) {
        return;
    }
    boxCardapio.dialog = dialog;

    const busca = elBox('[data-busca]');
    busca.addEventListener('input', () => {
        fecharCriarItem();
        renderCardapio();
    });
    busca.addEventListener('keydown', (ev) => {
        if (ev.key === 'Enter') {
            ev.preventDefault();
            enterNaBusca();
        } else if (ev.key === 'ArrowDown' || ev.key === 'ArrowUp') {
            ev.preventDefault();
            moverAtivo(ev.key === 'ArrowDown' ? 1 : -1);
        }
    });

    elBox('[data-fechar-box]').addEventListener('click', () => dialog.close());
    // clique no fundo escuro (fora do conteudo) fecha. Esc o <dialog> ja trata.
    dialog.addEventListener('click', (ev) => {
        if (ev.target === dialog) {
            dialog.close();
        }
    });

    elBox('[data-criar-abrir]').addEventListener('click', abrirCriarItem);
    elBox('[data-criar-cancelar]').addEventListener('click', () => {
        fecharCriarItem();
        renderCardapio();
        elBox('[data-busca]').focus();
    });
    elBox('[data-criar-form]').addEventListener('submit', (ev) => {
        ev.preventDefault();
        criarItemCardapio();
    });
}

function nomeDoCard(card) {
    const nomes = nomesAtuais(card);
    return nomes.mesa || nomes.cliente || `Comanda ${card.dataset.comandaId}`;
}

async function abrirBoxCardapio(card) {
    const { dialog } = boxCardapio;
    if (!dialog) {
        return;
    }
    boxCardapio.card = card;
    boxCardapio.itens = [];
    boxCardapio.carregado = false;

    elBox('[data-box-titulo]').textContent = `Adicionar · ${nomeDoCard(card)}`;
    const busca = elBox('[data-busca]');
    busca.value = '';
    fecharCriarItem();
    renderCardapio();
    mostrarStatusCardapio('Carregando cardápio…');

    dialog.showModal();
    busca.focus();

    try {
        const itens = await buscarCardapio();
        if (boxCardapio.card !== card) {
            return; // o box ja foi reaberto para outro card
        }
        boxCardapio.itens = itens;
        boxCardapio.carregado = true;
        renderCardapio();
    } catch (erro) {
        console.error('Falha ao carregar cardapio', erro);
        mostrarStatusCardapio(`Não foi possível carregar o cardápio: ${erro.message}`, 'erro');
    }
}

// tipo: '' (neutro), 'erro' ou 'ok'
function mostrarStatusCardapio(texto, tipo = '') {
    const status = elBox('[data-status]');
    status.textContent = texto;
    status.classList.toggle('is-erro', tipo === 'erro');
    status.classList.toggle('is-ok', tipo === 'ok');
    status.hidden = !texto;
}

function itensFiltrados() {
    const termo = normalizar(elBox('[data-busca]').value);
    if (!termo) {
        return boxCardapio.itens;
    }
    return boxCardapio.itens.filter((item) =>
        normalizar(item.nome).includes(termo) || normalizar(item.categoria).includes(termo));
}

const SEM_CATEGORIA = 'Sem categoria';

function categoriaDe(item) {
    return (item.categoria || '').trim() || SEM_CATEGORIA;
}

// Filtrados na ordem em que aparecem na tela: categorias em ordem alfabetica
// ("Sem categoria" por ultimo) e, dentro delas, itens por nome. As setas e o
// Enter usam esta mesma ordem, entao o indice "ativo" bate com a tela.
function itensEmOrdem() {
    const comparar = (a, b) => a.localeCompare(b, 'pt-BR', { sensitivity: 'base' });
    return [...itensFiltrados()].sort((a, b) => {
        const ca = categoriaDe(a);
        const cb = categoriaDe(b);
        if (ca !== cb) {
            if (ca === SEM_CATEGORIA) return 1;
            if (cb === SEM_CATEGORIA) return -1;
            return comparar(ca, cb);
        }
        return comparar(a.nome, b.nome);
    });
}

function existeItemComNome(nome) {
    const alvo = normalizar(nome);
    return boxCardapio.itens.some((item) => normalizar(item.nome) === alvo);
}

function podeCriarItem() {
    const texto = elBox('[data-busca]').value.trim();
    return boxCardapio.carregado && texto !== '' && !existeItemComNome(texto);
}

// manterStatus: nao apaga a mensagem "lancado" logo depois de lancar
function renderCardapio({ manterStatus = false } = {}) {
    const lista = elBox('[data-lista]');
    const texto = elBox('[data-busca]').value.trim();
    const filtrados = itensEmOrdem();

    lista.innerHTML = '';
    let categoriaAtual = null;
    filtrados.forEach((item) => {
        const categoria = categoriaDe(item);
        if (categoria !== categoriaAtual) {
            categoriaAtual = categoria;
            const titulo = document.createElement('li');
            titulo.className = 'cardapio-categoria';
            titulo.textContent = categoria;
            lista.appendChild(titulo);
        }
        lista.appendChild(criarLinhaCardapio(item));
    });

    if (boxCardapio.carregado && !manterStatus) {
        if (boxCardapio.itens.length === 0 && !texto) {
            mostrarStatusCardapio('Cardápio vazio. Digite um nome para criar o primeiro item.');
        } else if (filtrados.length === 0 && texto) {
            mostrarStatusCardapio('Nenhum item encontrado. Enter cria um novo.');
        } else {
            mostrarStatusCardapio('');
        }
    }

    const btnCriar = elBox('[data-criar-abrir]');
    const formAberto = !elBox('[data-criar-form]').hidden;
    btnCriar.hidden = formAberto || !podeCriarItem();
    elBox('[data-criar-nome]').textContent = texto;

    // Com texto digitado, o primeiro resultado ja vem escolhido (Enter lanca).
    // Sem texto, nada escolhido: Enter sozinho nao lanca nada por engano.
    boxCardapio.ativo = texto && filtrados.length > 0 ? 0 : -1;
    marcarAtivo();
}

// Opcoes navegaveis com as setas: os itens visiveis + "Criar item" (se aparecer)
function opcoesNavegaveis() {
    const opcoes = Array.from(elBox('[data-lista]').querySelectorAll('.cardapio-item'));
    const btnCriar = elBox('[data-criar-abrir]');
    if (!btnCriar.hidden) {
        opcoes.push(btnCriar);
    }
    return opcoes;
}

function moverAtivo(passo) {
    const total = opcoesNavegaveis().length;
    if (total === 0) {
        return;
    }
    const atual = boxCardapio.ativo;
    boxCardapio.ativo = atual < 0
        ? (passo > 0 ? 0 : total - 1)
        : (atual + passo + total) % total;
    marcarAtivo();
}

function marcarAtivo() {
    opcoesNavegaveis().forEach((el, i) => {
        const ativo = i === boxCardapio.ativo;
        el.classList.toggle('is-ativo', ativo);
        if (ativo) {
            el.scrollIntoView({ block: 'nearest' });
        }
    });
}

// [nome ..................  R$ 0,00  (+)]
// Clicar na linha so marca (como as setas); quem lanca e o "+" ou o Enter.
function criarLinhaCardapio(item) {
    const li = document.createElement('li');
    li.className = 'cardapio-item';
    li.dataset.idItem = item.ID;

    const nome = document.createElement('span');
    nome.className = 'cardapio-item-nome';
    nome.textContent = item.nome;
    nome.title = item.nome;

    const preco = document.createElement('span');
    preco.className = 'cardapio-item-preco';
    preco.textContent = formatarPreco(item.preco);

    const botao = document.createElement('button');
    botao.type = 'button';
    botao.className = 'btn-lancar-item';
    botao.textContent = '+';
    botao.title = `Lançar ${item.nome}`;
    botao.setAttribute('aria-label', `Lançar ${item.nome}`);
    botao.addEventListener('click', () => lancarItem(item));

    li.addEventListener('click', (ev) => {
        if (ev.target === botao) {
            return;
        }
        boxCardapio.ativo = opcoesNavegaveis().indexOf(li);
        marcarAtivo();
        elBox('[data-busca]').focus();
    });

    li.append(nome, preco, botao);
    return li;
}

// Enter na busca: lanca o escolhido; se o escolhido for "Criar item", ou
// se nao houver resultado, abre o cadastro.
function enterNaBusca() {
    const filtrados = itensEmOrdem();
    const ativo = boxCardapio.ativo;
    if (ativo >= 0 && ativo < filtrados.length) {
        lancarItem(filtrados[ativo]);
    } else if (podeCriarItem() && (ativo === filtrados.length || filtrados.length === 0)) {
        abrirCriarItem();
    }
}

// Lanca 1 unidade do item na comanda (preco do cardapio; da para mudar
// quantidade e preco depois, no proprio card). O box continua aberto para
// lancar o proximo: Esc fecha.
async function lancarItem(item) {
    const card = boxCardapio.card;
    if (!card || boxCardapio.lancando) {
        return;
    }
    boxCardapio.lancando = true;
    try {
        const resposta = await naFila(() => fetch(`/comanda/${card.dataset.comandaId}/adicionar`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ id_item: item.ID, quantidade: 1, preco_unitario: Number(item.preco) }),
        }));
        const corpo = await resposta.json().catch(() => ({}));
        if (!resposta.ok) {
            throw new Error(corpo.erro || `HTTP ${resposta.status}`);
        }

        const busca = elBox('[data-busca]');
        busca.value = '';
        fecharCriarItem();
        renderCardapio({ manterStatus: true });
        mostrarStatusCardapio(`✓ ${item.nome} lançado em ${nomeDoCard(card)}`, 'ok');
        busca.focus();

        recarregarItensCard(card);
    } catch (erro) {
        console.error('Falha ao lancar item', item, erro);
        mostrarStatusCardapio(`Não foi possível lançar ${item.nome}: ${erro.message}`, 'erro');
    } finally {
        boxCardapio.lancando = false;
    }
}

// Recarrega a lista do card. Antes grava o que estiver pendente nele, senao
// a recarga apagaria a edicao. Se algo nao gravar (campo em vermelho), nao
// recarrega: o item novo aparece quando o card for recarregado depois.
async function recarregarItensCard(card) {
    const sujas = Array.from(card.querySelectorAll('.mesa-item.item-sujo'));
    const resultados = await Promise.all(sujas.map((li) => salvarItem(li)));
    if (resultados.includes(false)) {
        return;
    }
    card.dataset.itensCarregados = 'false';
    if (card.open) {
        enfileirarCarregamento(card);
    }
}

function abrirCriarItem() {
    if (!podeCriarItem()) {
        return;
    }
    const form = elBox('[data-criar-form]');
    form.reset();
    elBox('[data-criar-nome-form]').textContent = elBox('[data-busca]').value.trim();
    elBox('[data-criar-erro]').hidden = true;
    form.hidden = false;
    elBox('[data-criar-abrir]').hidden = true;
    form.elements.categoria.focus();
}

function fecharCriarItem() {
    elBox('[data-criar-form]').hidden = true;
}

// Cadastra o item digitado na busca e ja lanca na comanda.
async function criarItemCardapio() {
    const form = elBox('[data-criar-form]');
    const erro = elBox('[data-criar-erro]');
    if (!form.reportValidity()) {
        return;
    }
    const botao = form.querySelector('button[type="submit"]');
    botao.disabled = true;
    erro.hidden = true;
    try {
        const item = await cadastrarItem(
            elBox('[data-busca]').value.trim(),
            form.elements.categoria.value.trim(),
            Number(form.elements.preco.value),
        );
        boxCardapio.itens.push(item);
        await lancarItem(item);
    } catch (e) {
        console.error('Falha ao criar item', e);
        erro.textContent = `Não foi possível criar: ${e.message}`;
        erro.hidden = false;
    } finally {
        botao.disabled = false;
    }
}

// ---------------------------------------------------------------------
// Box "Novo item" (menu da engrenagem): cadastra no cardapio, sem comanda.
// Enter em qualquer campo confirma; o box fica aberto para o proximo.
// ---------------------------------------------------------------------

const boxNovoItem = { dialog: null, nomes: [] };

function iniciarBoxNovoItem() {
    const dialog = document.querySelector('[data-box-novo-item]');
    if (!dialog) {
        return;
    }
    boxNovoItem.dialog = dialog;
    const form = dialog.querySelector('[data-novo-item-form]');

    form.addEventListener('submit', (ev) => {
        ev.preventDefault();
        enviarNovoItem(form);
    });
    dialog.querySelectorAll('[data-fechar-novo-item]').forEach((b) =>
        b.addEventListener('click', () => dialog.close()));
    dialog.addEventListener('click', (ev) => {
        if (ev.target === dialog) {
            dialog.close();
        }
    });
    form.elements.nome.addEventListener('input', () => form.elements.nome.setCustomValidity(''));
}

function statusNovoItem(texto, tipo = '') {
    const status = boxNovoItem.dialog.querySelector('[data-novo-item-status]');
    status.textContent = texto;
    status.classList.toggle('is-erro', tipo === 'erro');
    status.classList.toggle('is-ok', tipo === 'ok');
    status.hidden = !texto;
}

async function abrirBoxNovoItem() {
    const { dialog } = boxNovoItem;
    if (!dialog) {
        return;
    }
    const form = dialog.querySelector('[data-novo-item-form]');
    form.reset();
    statusNovoItem('');
    dialog.showModal();
    form.elements.nome.focus();

    // Categorias ja usadas viram sugestao; nomes servem para avisar repetido.
    try {
        const itens = await buscarCardapio();
        boxNovoItem.nomes = itens.map((i) => normalizar(i.nome));
        const lista = dialog.querySelector('#categorias-cardapio');
        lista.innerHTML = '';
        [...new Set(itens.map((i) => i.categoria).filter(Boolean))].sort().forEach((cat) => {
            const opcao = document.createElement('option');
            opcao.value = cat;
            lista.appendChild(opcao);
        });
    } catch (erro) {
        console.error('Falha ao carregar cardapio', erro);
    }
}

async function enviarNovoItem(form) {
    const nome = form.elements.nome.value.trim();
    if (boxNovoItem.nomes.includes(normalizar(nome))) {
        form.elements.nome.setCustomValidity('Já existe um item com esse nome.');
    }
    if (!form.reportValidity()) {
        return;
    }
    const botao = form.querySelector('button[type="submit"]');
    botao.disabled = true;
    try {
        const item = await cadastrarItem(
            nome,
            form.elements.categoria.value.trim(),
            Number(form.elements.preco.value),
        );
        boxNovoItem.nomes.push(normalizar(item.nome));
        statusNovoItem(`✓ ${item.nome} cadastrado (${formatarPreco(item.preco)})`, 'ok');
        const categoria = form.elements.categoria.value;
        form.reset();
        form.elements.categoria.value = categoria; // costuma cadastrar varios da mesma categoria
        form.elements.nome.focus();
    } catch (erro) {
        console.error('Falha ao cadastrar item', erro);
        statusNovoItem(`Não foi possível cadastrar: ${erro.message}`, 'erro');
    } finally {
        botao.disabled = false;
    }
}

// ---------------------------------------------------------------------
// Arrastar cards para mudar a ordem (mouse e toque, via pointer events;
// o drag and drop nativo do HTML nao funciona com toque).
// Teclado: foco na alca + setas. A ordem vai para POST /comandas/ordem.
// ---------------------------------------------------------------------

const MARGEM_AUTOSCROLL = 70; // px da borda da janela onde a pagina rola sozinha

let arrasto = null; // estado do arrasto em andamento

function iniciarArrastarCard(card) {
    const alca = card.querySelector('[data-arrastar]');
    if (!alca) {
        return;
    }
    // dentro do <summary>: clicar na alca nao abre/fecha o card
    alca.addEventListener('click', (ev) => ev.preventDefault());
    alca.addEventListener('pointerdown', (ev) => comecarArrasto(ev, card, alca));
    alca.addEventListener('keydown', (ev) => {
        const passos = { ArrowLeft: -1, ArrowUp: -1, ArrowRight: 1, ArrowDown: 1 };
        if (ev.key in passos) {
            ev.preventDefault();
            moverCardTeclado(card, passos[ev.key]);
        }
    });
}

function gridComandas() {
    return document.querySelector('.comandas-grid');
}

function ordemAtual() {
    return Array.from(gridComandas().querySelectorAll('.mesa-card'))
        .map((c) => c.dataset.comandaId).join(',');
}

function comecarArrasto(ev, card, alca) {
    if (ev.button !== 0 || arrasto) {
        return;
    }
    ev.preventDefault();

    const ret = card.getBoundingClientRect();
    const espaco = document.createElement('div');
    espaco.className = 'mesa-card-espaco';
    espaco.style.height = `${ret.height}px`;
    card.before(espaco);

    arrasto = {
        card,
        alca,
        espaco,
        dx: ev.clientX - ret.left,
        dy: ev.clientY - ret.top,
        ordemAntes: ordemAtual(),
        x: ev.clientX,
        y: ev.clientY,
        rolagem: null,
    };

    // o card sai do grid e segue o ponteiro; o "espaco" guarda o lugar dele
    card.classList.add('is-arrastando');
    card.style.width = `${ret.width}px`;
    posicionarCardArrastado();

    alca.setPointerCapture(ev.pointerId);
    alca.addEventListener('pointermove', moverArrasto);
    alca.addEventListener('pointerup', terminarArrasto);
    alca.addEventListener('pointercancel', terminarArrasto);
    arrasto.rolagem = setInterval(rolarPertoDaBorda, 16);
}

function posicionarCardArrastado() {
    const { card, dx, dy, x, y } = arrasto;
    card.style.left = `${x - dx}px`;
    card.style.top = `${y - dy}px`;
}

function moverArrasto(ev) {
    if (!arrasto) {
        return;
    }
    arrasto.x = ev.clientX;
    arrasto.y = ev.clientY;
    posicionarCardArrastado();
    reposicionarEspaco();
}

// Coloca o "espaco" antes ou depois do card que esta sob o ponteiro.
function reposicionarEspaco() {
    const { card, espaco, x, y } = arrasto;
    const sob = document.elementFromPoint(x, y);
    if (!sob) {
        return;
    }
    const grid = gridComandas();

    // em cima do "+ Nova comanda": vai para o fim (ele e sempre o ultimo)
    const nova = sob.closest('.nova-comanda-card');
    if (nova && grid.contains(nova)) {
        if (espaco.nextElementSibling !== nova) {
            nova.before(espaco);
        }
        return;
    }

    const alvo = sob.closest('.mesa-card');
    if (!alvo || alvo === card || !grid.contains(alvo)) {
        return;
    }
    const r = alvo.getBoundingClientRect();
    // uma coluna (tela estreita): decide pela altura; varias: pela largura
    const umaColuna = getComputedStyle(grid).gridTemplateColumns.split(' ').length === 1;
    const antes = umaColuna ? y < r.top + r.height / 2 : x < r.left + r.width / 2;
    if (antes && espaco.nextElementSibling !== alvo) {
        alvo.before(espaco);
    } else if (!antes && alvo.nextElementSibling !== espaco) {
        alvo.after(espaco);
    }
}

// Com o ponteiro parado perto do topo/rodape da janela, a pagina rola.
function rolarPertoDaBorda() {
    if (!arrasto) {
        return;
    }
    const { y } = arrasto;
    let passo = 0;
    if (y < MARGEM_AUTOSCROLL) {
        passo = -Math.ceil((MARGEM_AUTOSCROLL - y) / 6);
    } else if (y > window.innerHeight - MARGEM_AUTOSCROLL) {
        passo = Math.ceil((y - (window.innerHeight - MARGEM_AUTOSCROLL)) / 6);
    }
    if (passo) {
        window.scrollBy(0, passo);
        reposicionarEspaco();
    }
}

function terminarArrasto() {
    if (!arrasto) {
        return;
    }
    const { card, alca, espaco, ordemAntes, rolagem } = arrasto;
    clearInterval(rolagem);
    alca.removeEventListener('pointermove', moverArrasto);
    alca.removeEventListener('pointerup', terminarArrasto);
    alca.removeEventListener('pointercancel', terminarArrasto);

    espaco.replaceWith(card);
    card.classList.remove('is-arrastando');
    card.style.removeProperty('width');
    card.style.removeProperty('left');
    card.style.removeProperty('top');
    arrasto = null;

    if (ordemAtual() !== ordemAntes) {
        salvarOrdemCards();
    }
}

function moverCardTeclado(card, passo) {
    const cards = Array.from(gridComandas().querySelectorAll('.mesa-card'));
    const i = cards.indexOf(card);
    const destino = cards[i + passo];
    if (!destino) {
        return;
    }
    if (passo < 0) {
        destino.before(card);
    } else {
        destino.after(card);
    }
    card.querySelector('[data-arrastar]').focus();
    salvarOrdemCards();
}

// Varios movimentos seguidos (setas) viram uma gravacao so.
let timerOrdem = null;

function salvarOrdemCards() {
    clearTimeout(timerOrdem);
    timerOrdem = setTimeout(enviarOrdemCards, 400);
}

async function enviarOrdemCards() {
    const ids = Array.from(gridComandas().querySelectorAll('.mesa-card'))
        .map((c) => Number(c.dataset.comandaId));
    if (ids.length === 0) {
        return;
    }
    try {
        const resposta = await naFila(() => fetch('/comandas/ordem', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ ids }),
        }));
        const corpo = await resposta.json().catch(() => ({}));
        if (!resposta.ok) {
            throw new Error(corpo.erro || `HTTP ${resposta.status}`);
        }
    } catch (erro) {
        console.error('Falha ao salvar a ordem das comandas', erro);
        avisar(`A nova ordem não foi salva: ${erro.message}`);
    }
}

// Aviso curto no canto da tela (some sozinho).
function avisar(texto) {
    let aviso = document.querySelector('.aviso-flutuante');
    if (!aviso) {
        aviso = document.createElement('div');
        aviso.className = 'aviso-flutuante';
        aviso.setAttribute('role', 'status');
        document.body.appendChild(aviso);
    }
    aviso.textContent = texto;
    aviso.classList.add('is-visivel');
    clearTimeout(avisar.timer);
    avisar.timer = setTimeout(() => aviso.classList.remove('is-visivel'), 4000);
}
