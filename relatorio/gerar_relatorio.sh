#!/usr/bin/env bash
# Gera relatorio.pdf (e relatorio.docx, editável) a partir de relatorio.md.
# Requer: pandoc; para o PDF, Node.js com o pacote "playwright" (Chromium).
set -euo pipefail
cd "$(dirname "$0")"
pandoc relatorio.md -s --mathml --no-highlight --embed-resources --css estilo.css \
       --resource-path=.:.. -o relatorio.html
node html_para_pdf.js relatorio.html relatorio.pdf
pandoc relatorio.md --resource-path=.:.. -o relatorio.docx
rm -f relatorio.html
echo "Gerados: relatorio/relatorio.pdf e relatorio/relatorio.docx"
