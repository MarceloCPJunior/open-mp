#!/usr/bin/env python3
"""
Gera um arquivo de dados sintético com estatura (cm) e peso (kg) de N pessoas.

Formato de saída (o mesmo exigido pelo enunciado): uma pessoa por linha,
estatura e peso separados por espaço, com uma casa decimal.

    172.4 68.9
    165.0 59.3
    ...

Modelo usado (para os dados ficarem realistas):
  - estatura ~ Normal(média = 170 cm, desvio = 9 cm), limitada a [140, 205] cm;
  - IMC      ~ Normal(média = 25 kg/m², desvio = 4 kg/m²), limitado a [16, 45];
  - peso     = IMC * (estatura em metros)²   -> peso correlacionado com a altura.

A semente fixa garante que o mesmo comando gere sempre o mesmo arquivo.

Uso:
    python3 scripts/gerar_dados.py N ARQUIVO [SEMENTE]
Exemplo:
    python3 scripts/gerar_dados.py 1000 dados/pessoas_1000.txt 2025
"""
import random
import sys


def limitar(valor, minimo, maximo):
    return max(minimo, min(maximo, valor))


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        sys.exit(1)

    n = int(sys.argv[1])
    caminho = sys.argv[2]
    semente = int(sys.argv[3]) if len(sys.argv) > 3 else 2025

    rng = random.Random(semente)
    linhas = []
    for _ in range(n):
        estatura = limitar(rng.gauss(170.0, 9.0), 140.0, 205.0)
        imc = limitar(rng.gauss(25.0, 4.0), 16.0, 45.0)
        peso = imc * (estatura / 100.0) ** 2
        linhas.append(f"{estatura:.1f} {peso:.1f}\n")

    with open(caminho, "w", encoding="ascii", newline="\n") as arq:
        arq.writelines(linhas)

    print(f"{n} pessoas gravadas em {caminho} (semente {semente})")


if __name__ == "__main__":
    main()
