# Exercício Final — Coeficiente de Variação com OpenMP
#
#   make              compila o programa (./cv_openmp)
#   make run          executa com os dados de 1000 pessoas
#   make verificar    confere o resultado com uma implementação independente em Python
#   make benchmark    mede sequencial x paralelo (1, 2, 4, 8 threads; vários N) -> CSV
#   make protecoes    compara sem proteção x critical x atomic x reduction no histograma
#   make graficos     gera os gráficos a partir do CSV (precisa de matplotlib)
#   make clean        remove o executável

CXX      ?= g++
CXXFLAGS ?= -O2 -std=c++17 -Wall -Wextra
OMPFLAG   = -fopenmp
BIN       = cv_openmp
BIN_PROT  = comparar_protecoes
DADOS     = dados/pessoas_1000.txt
THREADS  ?= $(shell nproc 2>/dev/null || echo 4)

all: $(BIN) $(BIN_PROT)

$(BIN): src/cv_openmp.cpp
	$(CXX) $(CXXFLAGS) $(OMPFLAG) -o $@ $< -lm

$(BIN_PROT): src/comparar_protecoes.cpp
	$(CXX) $(CXXFLAGS) $(OMPFLAG) -o $@ $< -lm

$(DADOS):
	python3 scripts/gerar_dados.py 1000 $(DADOS) 2025

run: $(BIN) $(DADOS)
	./$(BIN) $(DADOS) $(THREADS) | tee resultados/saida_1000.txt

verificar: $(DADOS)
	python3 scripts/verificar.py $(DADOS)

benchmark: $(BIN) $(DADOS)
	./scripts/benchmark.sh

protecoes: $(BIN_PROT) $(DADOS)
	./$(BIN_PROT) $(DADOS) 10000000 $(THREADS) | tee resultados/protecoes.txt

graficos:
	python3 scripts/graficos.py

clean:
	rm -f $(BIN) $(BIN_PROT)

.PHONY: all run verificar benchmark protecoes graficos clean
