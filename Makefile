# Exercício Final — Coeficiente de Variação com OpenMP
#
#   make              compila o programa (./cv_openmp)
#   make run          executa com os dados de 1000 pessoas
#   make verificar    confere o resultado com uma implementação independente em Python
#   make benchmark    mede sequencial x paralelo (1, 2, 4, 8 threads; vários N) -> CSV
#   make graficos     gera os gráficos a partir do CSV (precisa de matplotlib)
#   make clean        remove o executável

CXX      ?= g++
CXXFLAGS ?= -O2 -std=c++17 -Wall -Wextra
OMPFLAG   = -fopenmp
BIN       = cv_openmp
DADOS     = dados/pessoas_1000.txt
THREADS  ?= $(shell nproc 2>/dev/null || echo 4)

all: $(BIN)

$(BIN): src/cv_openmp.cpp
	$(CXX) $(CXXFLAGS) $(OMPFLAG) -o $@ $< -lm

$(DADOS):
	python3 scripts/gerar_dados.py 1000 $(DADOS) 2025

run: $(BIN) $(DADOS)
	./$(BIN) $(DADOS) $(THREADS) | tee resultados/saida_1000.txt

verificar: $(DADOS)
	python3 scripts/verificar.py $(DADOS)

benchmark: $(BIN) $(DADOS)
	./scripts/benchmark.sh

graficos:
	python3 scripts/graficos.py

clean:
	rm -f $(BIN)

.PHONY: all run verificar benchmark graficos clean
