#!/usr/bin/env bash
# Mede a versão sequencial x paralela para vários tamanhos de entrada (N) e
# números de threads, e grava tudo em resultados/benchmark.csv.
#
#   Uso: ./scripts/benchmark.sh ["THREADS"] ["TAMANHOS"]
#   Ex.: ./scripts/benchmark.sh "1 2 4 8" "1000 100000 1000000 10000000"
#
# - N = 1000 usa o arquivo oficial do exercício (dados/pessoas_1000.txt).
# - Os demais tamanhos são gerados (mesmo modelo, semente fixa) em dados/gerados/,
#   só para estudar a escalabilidade; essa pasta não vai para o Git.
# - Cada execução do programa já faz 1 rodada de aquecimento (descartada) e
#   reporta a MEDIANA de várias repetições; aqui só coletamos a linha "CSV,".
set -euo pipefail
cd "$(dirname "$0")/.."

THREADS=${1:-"1 2 4 8"}
TAMANHOS=${2:-"1000 10000 100000 1000000 10000000"}
BIN=./cv_openmp
SAIDA=resultados/benchmark.csv

[ -x "$BIN" ] || make
mkdir -p resultados dados/gerados

primeira=1
for N in $TAMANHOS; do
    if [ "$N" -eq 1000 ]; then
        ARQ=dados/pessoas_1000.txt
    else
        ARQ=dados/gerados/pessoas_${N}.txt
        [ -f "$ARQ" ] || python3 scripts/gerar_dados.py "$N" "$ARQ" 2025
    fi
    for T in $THREADS; do
        SAIDA_PROG=$("$BIN" "$ARQ" "$T")
        if [ $primeira -eq 1 ]; then
            echo "$SAIDA_PROG" | grep '^CSV_CABECALHO,' | cut -d, -f2- > "$SAIDA"
            primeira=0
        fi
        echo "$SAIDA_PROG" | grep '^CSV,' | cut -d, -f2- >> "$SAIDA"
        LINHA=$(echo "$SAIDA_PROG" | grep '^CSV,')
        echo "  N=$N threads=$T  speedup=$(echo "$LINHA" | cut -d, -f16)x"
    done
done

echo
echo "Dados em $SAIDA  (processadores: $(nproc))"
