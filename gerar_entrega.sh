#!/usr/bin/env bash
# Monta o .zip de entrega com duas partes separadas:
#   - Relatorio_Exercicio_Final_OpenMP.pdf   (o relatório)
#   - codigo/                                (só o código e os arquivos que ele usa/gera)
# Arquivos de trabalho (fonte do relatório, .docx, notas de estudo) ficam de fora.
#
# Uso: ./gerar_entrega.sh   ->   entrega/<NOME>.zip
set -euo pipefail
cd "$(dirname "$0")"

NOME=Exercicio_Final_OpenMP_Marcelo_de_Castro_Pereira_Junior_2222130019
PDF=Relatorio_Exercicio_Final_OpenMP.pdf
ARQUIVOS=(
    README.md Makefile
    src/cv_openmp.cpp src/comparar_protecoes.cpp
    dados/pessoas_1000.txt
    scripts/gerar_dados.py scripts/verificar.py scripts/benchmark.sh scripts/graficos.py
    resultados/saida_1000.txt resultados/benchmark.csv resultados/protecoes.txt
    resultados/graficos/*.png
)

TMP=$(mktemp -d)
trap 'rm -rf "${TMP:?}"' EXIT

mkdir -p "$TMP/$NOME/codigo"
cp relatorio/relatorio.pdf "$TMP/$NOME/$PDF"
for f in "${ARQUIVOS[@]}"; do
    mkdir -p "$TMP/$NOME/codigo/$(dirname "$f")"
    cp -p "$f" "$TMP/$NOME/codigo/$f"
done

SAIDA="$PWD/entrega/$NOME.zip"
mkdir -p entrega
rm -f "$SAIDA"
(cd "$TMP" && zip -qr -X "$SAIDA" "$NOME")
echo "Gerado: entrega/$NOME.zip"
unzip -l "$SAIDA"
