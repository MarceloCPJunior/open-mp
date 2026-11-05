#!/usr/bin/env python3
"""
Verificação independente do resultado do programa C++.

Recalcula, em Python puro, a tabela de classes, a média ponderada, o desvio
padrão populacional e o CV de dados agrupados, usando a MESMA regra de classes
do programa C++:
    L0 = floor(menor valor),  classe i = [L0 + i*h, L0 + (i+1)*h),
    k  = floor((maior - L0) / h) + 1,  x_i = L0 + (i + 0.5)*h.

Também mostra o CV calculado com os dados BRUTOS (sem agrupar), só como
referência para ver o efeito do agrupamento em classes.

Uso:  python3 scripts/verificar.py dados/pessoas_1000.txt
"""
import math
import sys


def ler(caminho):
    estatura, peso = [], []
    with open(caminho, encoding="utf-8") as arq:
        for linha in arq:
            partes = linha.replace(";", " ").split()
            if len(partes) < 2:
                continue
            try:
                a, w = float(partes[0]), float(partes[1])
            except ValueError:
                continue  # cabeçalho
            estatura.append(a)
            peso.append(w)
    if max(estatura) < 3.0:  # estatura em metros -> cm
        estatura = [a * 100.0 for a in estatura]
    return estatura, peso


def agrupado(dados, h):
    L0 = math.floor(min(dados))
    k = int(math.floor((max(dados) - L0) / h)) + 1
    freq = [0] * k
    for v in dados:
        freq[min(int((v - L0) / h), k - 1)] += 1
    xm = [L0 + (i + 0.5) * h for i in range(k)]
    n = sum(freq)
    mu = sum(x * f for x, f in zip(xm, freq)) / n
    sigma = math.sqrt(sum(f * (x - mu) ** 2 for x, f in zip(xm, freq)) / n)
    return mu, sigma, sigma / mu * 100.0, k


def bruto(dados):
    n = len(dados)
    mu = sum(dados) / n
    sigma = math.sqrt(sum((v - mu) ** 2 for v in dados) / n)
    return mu, sigma, sigma / mu * 100.0


def main():
    caminho = sys.argv[1] if len(sys.argv) > 1 else "dados/pessoas_1000.txt"
    estatura, peso = ler(caminho)
    print(f"Arquivo: {caminho}   N = {len(estatura)}\n")
    print(f"{'Variavel':<10} {'Metodo':<22} {'k':>3} {'media':>10} {'desvio':>10} {'CV (%)':>9}")
    for nome, dados, h in (("estatura", estatura, 8.0), ("peso", peso, 4.0)):
        mu, s, cv, k = agrupado(dados, h)
        print(f"{nome:<10} {'agrupado (h=%g)' % h:<22} {k:>3} {mu:>10.4f} {s:>10.4f} {cv:>9.4f}")
        mu, s, cv = bruto(dados)
        print(f"{'':<10} {'bruto (referencia)':<22} {'-':>3} {mu:>10.4f} {s:>10.4f} {cv:>9.4f}")


if __name__ == "__main__":
    main()
