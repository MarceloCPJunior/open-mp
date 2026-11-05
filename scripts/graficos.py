#!/usr/bin/env python3
"""
Gera os gráficos do relatório a partir de resultados/benchmark.csv e dos dados.

    python3 scripts/graficos.py          (precisa de matplotlib)

Saída em resultados/graficos/:
    distribuicoes.png   histogramas das classes de estatura (8 cm) e peso (4 kg)
    speedup.png         speedup e eficiência x nº de threads, para cada N
    etapas_1000.png     tempo de cada etapa, sequencial x paralelo, N = 1000
"""
import csv
import math
import os
import sys

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt  # noqa: E402

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from verificar import ler  # noqa: E402

RAIZ = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
CSV = os.path.join(RAIZ, "resultados", "benchmark.csv")
DADOS = os.path.join(RAIZ, "dados", "pessoas_1000.txt")
SAIDA = os.path.join(RAIZ, "resultados", "graficos")

# Paleta: série única/sequencial = azul; 2ª série = laranja;
# tamanhos de N (ordenados) = rampa de um só tom, do claro ao escuro.
SUPERFICIE = "#fcfcfb"
TEXTO = "#0b0b0b"
TEXTO_2 = "#52514e"
GRADE = "#e4e3df"
AZUL = "#2a78d6"
LARANJA = "#eb6834"
RAMPA_N = ["#86b6ef", "#5598e7", "#2a78d6", "#1c5cab", "#104281"]

plt.rcParams.update({
    "figure.facecolor": SUPERFICIE,
    "axes.facecolor": SUPERFICIE,
    "axes.edgecolor": GRADE,
    "axes.labelcolor": TEXTO_2,
    "axes.titlecolor": TEXTO,
    "axes.titlesize": 11,
    "axes.titleweight": "bold",
    "axes.labelsize": 9,
    "axes.grid": True,
    "axes.axisbelow": True,
    "grid.color": GRADE,
    "grid.linewidth": 0.6,
    "axes.spines.top": False,
    "axes.spines.right": False,
    "xtick.color": TEXTO_2,
    "ytick.color": TEXTO_2,
    "xtick.labelsize": 8,
    "ytick.labelsize": 8,
    "legend.fontsize": 8,
    "legend.frameon": False,
    "font.family": "DejaVu Sans",
})


def ler_csv():
    with open(CSV, encoding="utf-8") as arq:
        linhas = list(csv.DictReader(arq))
    for l in linhas:
        for chave, valor in l.items():
            l[chave] = float(valor)
    return linhas


def rotulo_n(n):
    n = int(n)
    return f"N = {n:,}".replace(",", ".")


def distribuicoes():
    estatura, peso = ler(DADOS)
    fig, eixos = plt.subplots(1, 2, figsize=(9, 3.2))
    for eixo, dados, h, nome, unidade in (
        (eixos[0], estatura, 8.0, "Estatura", "cm"),
        (eixos[1], peso, 4.0, "Peso", "kg"),
    ):
        L0 = math.floor(min(dados))
        k = int(math.floor((max(dados) - L0) / h)) + 1
        freq = [0] * k
        for v in dados:
            freq[min(int((v - L0) / h), k - 1)] += 1
        xm = [L0 + (i + 0.5) * h for i in range(k)]
        mu = sum(x * f for x, f in zip(xm, freq)) / sum(freq)
        eixo.bar(xm, freq, width=h - 0.6, color=AZUL, edgecolor=SUPERFICIE, linewidth=0)
        topo = max(freq) * 1.15
        eixo.set_ylim(0, topo)
        eixo.axvline(mu, color=TEXTO, linewidth=1)
        eixo.annotate(f"μ = {mu:.1f} {unidade}", xy=(mu, topo), xytext=(5, -3),
                      textcoords="offset points", fontsize=8, color=TEXTO, va="top")
        eixo.set_title(f"{nome} — classes de {h:g} {unidade} (k = {k})", loc="left")
        eixo.set_xlabel(f"{nome.lower()} ({unidade})")
        eixo.set_ylabel("frequência (pessoas)")
        eixo.grid(axis="x", visible=False)
    fig.tight_layout()
    fig.savefig(os.path.join(SAIDA, "distribuicoes.png"), dpi=200)
    plt.close(fig)


def speedup(linhas):
    tamanhos = sorted({l["n"] for l in linhas})
    threads = sorted({l["threads"] for l in linhas})
    fig, (e1, e2) = plt.subplots(1, 2, figsize=(9, 3.6))
    e1.plot(threads, threads, color=TEXTO_2, linewidth=1)
    e1.annotate("ideal", xy=(threads[-1], threads[-1]), xytext=(-4, 0),
                textcoords="offset points", ha="right", va="bottom", fontsize=8, color=TEXTO_2)
    e1.axhline(1.0, color=TEXTO_2, linewidth=0.8)
    for cor, n in zip(RAMPA_N, tamanhos):
        sel = [l for l in linhas if l["n"] == n]
        t = [l["threads"] for l in sel]
        e1.plot(t, [l["speedup"] for l in sel], color=cor, linewidth=2, marker="o",
                markersize=5, label=rotulo_n(n))
        e2.plot(t, [100 * l["eficiencia"] for l in sel], color=cor, linewidth=2, marker="o",
                markersize=5, label=rotulo_n(n))
    for eixo in (e1, e2):
        eixo.set_xscale("log", base=2)
        eixo.set_xticks(threads)
        eixo.set_xticklabels([str(int(t)) for t in threads])
        eixo.set_xlabel("threads")
    e1.set_ylabel("speedup (T_seq / T_par)")
    e1.set_ylim(0, max(threads) + 0.5)
    e1.set_title("Speedup do cálculo", loc="left")
    e2.set_ylabel("eficiência (%)")
    e2.set_ylim(0, 105)
    e2.set_title("Eficiência = speedup / threads", loc="left")
    e1.legend(loc="upper left")
    fig.tight_layout()
    fig.savefig(os.path.join(SAIDA, "speedup.png"), dpi=200)
    plt.close(fig)


def etapas(linhas, threads=4):
    sel = [l for l in linhas if l["n"] == 1000 and l["threads"] == threads]
    if not sel:
        return
    l = sel[0]
    nomes = ["min/max\n(N)", "frequências\n(N)", "média\n(k)", "desvio + CV\n(k)", "total"]
    chaves = ["minmax", "freq", "media", "desvio", "total"]
    seq = [l[f"seq_{c}_us"] for c in chaves]
    par = [l[f"par_{c}_us"] for c in chaves]
    x = range(len(nomes))
    larg = 0.36
    fig, eixo = plt.subplots(figsize=(7.5, 3.4))
    b1 = eixo.bar([i - larg / 2 - 0.01 for i in x], seq, width=larg, color=AZUL, label="sequencial")
    b2 = eixo.bar([i + larg / 2 + 0.01 for i in x], par, width=larg, color=LARANJA,
                  label=f"paralelo ({threads} threads)")
    for barras in (b1, b2):
        for b in barras:
            eixo.annotate(f"{b.get_height():.2f}", xy=(b.get_x() + b.get_width() / 2, b.get_height()),
                          xytext=(0, 2), textcoords="offset points", ha="center", fontsize=7,
                          color=TEXTO_2)
    eixo.set_xticks(list(x))
    eixo.set_xticklabels(nomes)
    eixo.set_ylabel("tempo (µs, mediana)")
    eixo.set_title("N = 1000: tempo por etapa (estatura + peso)", loc="left")
    eixo.grid(axis="x", visible=False)
    eixo.legend(loc="upper left")
    fig.tight_layout()
    fig.savefig(os.path.join(SAIDA, "etapas_1000.png"), dpi=200)
    plt.close(fig)


def main():
    os.makedirs(SAIDA, exist_ok=True)
    distribuicoes()
    if os.path.exists(CSV):
        linhas = ler_csv()
        speedup(linhas)
        etapas(linhas)
    else:
        print(f"{CSV} não encontrado: rode 'make benchmark' para gerar os gráficos de desempenho")
    print(f"Gráficos gravados em {os.path.normpath(SAIDA)}")


if __name__ == "__main__":
    main()
