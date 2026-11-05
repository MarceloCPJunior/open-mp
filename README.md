# Exercício Final — Análise de Dados com OpenMP

Coeficiente de variação (CV) da **estatura** e do **peso** de 1000 pessoas, com dados
agrupados em classes (8 cm e 4 kg), em C++ com OpenMP.
Disciplina de Programação Paralela, Ciência da Computação (IESB), Prof. Rodrigo Gonçalves Pinto.

**Aluno:** Marcelo de Castro Pereira Junior · **Matrícula:** 2222130019

## Resultado

| Variável | Média (μ) | Desvio padrão (σ) | CV |
|---|--:|--:|--:|
| Estatura | 170,30 cm | 9,24 cm | **5,43 %** |
| Peso | 73,06 kg | 13,99 kg | **19,16 %** |

Com N = 1000 o cálculo leva ~6 µs e a versão paralela é **mais lenta** (0,43× com
4 threads), por causa do custo fixo das regiões paralelas. O ganho aparece a partir de
~10⁴ pessoas e chega a **≈ 3× com 4 threads** para N ≥ 10⁵. Os detalhes estão no relatório.

Os tempos variam de uma execução para outra, principalmente com N = 1000, em que cada medida
dura poucos microssegundos. Em testes repetidos, a versão paralela levou de ~13 µs a ~25 µs, e
foi mais lenta que a sequencial (~6 µs) em todas as execuções. Por isso `resultados/saida_1000.txt`
pode mostrar um speedup diferente do relatório, que usa `resultados/benchmark.csv`.

## Entregáveis

| Item pedido | Onde está |
|---|---|
| 1) Código-fonte C++ com OpenMP | [`src/cv_openmp.cpp`](src/cv_openmp.cpp) |
| 2) Relatório em PDF (o que foi paralelizado e por quê) | `Relatorio_Exercicio_Final_OpenMP.pdf`, entregue junto com esta pasta |
| 3) Resultados: CV da estatura e do peso | tabela acima e [`resultados/saida_1000.txt`](resultados/saida_1000.txt) |
| 4) Análise de desempenho (sequencial x paralelo) | relatório (Seção 4), [`resultados/benchmark.csv`](resultados/benchmark.csv) e [`resultados/graficos/`](resultados/graficos/) |

## Como rodar do zero

Precisa de `g++` com OpenMP (Linux, contêiner Docker da disciplina ou MinGW no Windows) e `python3`.

```bash
make                                    # compila ./cv_openmp
./cv_openmp dados/pessoas_1000.txt 4    # arquivo, nº de threads [, repetições]
make run                                # o mesmo, salvando em resultados/saida_1000.txt
make verificar                          # recalcula tudo em Python (conferência independente)
make benchmark                          # 1/2/4/8 threads x N = 10³..10⁷ -> resultados/benchmark.csv
make protecoes                          # sem proteção x critical x atomic x reduction (veja abaixo)
make graficos                           # gráficos (precisa de matplotlib)
```

Windows (PowerShell, MinGW):

```powershell
g++ -O2 -std=c++17 -fopenmp src\cv_openmp.cpp -o cv_openmp.exe
.\cv_openmp.exe dados\pessoas_1000.txt 4
g++ -O2 -std=c++17 -fopenmp src\comparar_protecoes.cpp -o comparar_protecoes.exe
.\comparar_protecoes.exe dados\pessoas_1000.txt 10000000 4
```

A saída mostra a tabela de classes de cada variável (x_i, f_i, x_i·f_i, f_i·(x_i−μ)²), μ, σ
e CV, a validação sequencial x paralelo e uma tabela de tempo por etapa com o speedup.

## Usando outro arquivo de dados

O enunciado não trouxe o arquivo, então `dados/pessoas_1000.txt` é **sintético**, gerado com
semente fixa por `python3 scripts/gerar_dados.py 1000 dados/pessoas_1000.txt 2025`. Se o
professor fornecer um arquivo, é só passá-lo como argumento (`./cv_openmp arquivo.txt`).
Linhas de cabeçalho são ignoradas, TAB/`;` e vírgula decimal são aceitos, e estatura em metros
é convertida para cm. Com dados novos, `make run`, `make benchmark` e `make graficos` refazem
as medições e os gráficos.

## Estrutura

```
src/cv_openmp.cpp          programa: leitura, versões sequencial e paralela, validação e medição
src/comparar_protecoes.cpp experimento: condição de corrida e critical x atomic x reduction
dados/pessoas_1000.txt     entrada: "estatura peso" por linha (1000 pessoas)
scripts/gerar_dados.py     gera dados sintéticos (usado também para N grande no benchmark)
scripts/verificar.py       conferência independente do resultado (Python puro)
scripts/benchmark.sh       mede sequencial x paralelo e grava CSV
scripts/graficos.py        gráficos a partir do CSV
resultados/                saída do programa, CSV de medições e gráficos
```

## O que foi paralelizado (resumo)

| Etapa | Laço | Diretiva |
|---|---|---|
| menor/maior valor | N pessoas | `#pragma omp parallel for reduction(min:mn) reduction(max:mx)` |
| frequências (histograma) | N pessoas | `#pragma omp parallel for reduction(+ : f[:k])` |
| média ponderada Σx_i·f_i, Σf_i | k classes | `#pragma omp parallel for reduction(+ : sxf, sf)` |
| desvio padrão Σf_i·(x_i−μ)² | k classes | `#pragma omp parallel for reduction(+ : s)` |

Todas as acumulações usam `reduction`, que evita a condição de corrida sem o custo de
`critical` ou `atomic` em cada iteração. `schedule(static)` porque todas as iterações custam
o mesmo. A leitura do arquivo fica sequencial (E/S e conversão de texto) e corresponde a ~97 %
do tempo total do programa, o que limita o speedup global a ≈ 1,03× (Lei de Amdahl).

## Prova: por que `reduction`

`src/comparar_protecoes.cpp` (`make protecoes`) faz a contagem por classe (o histograma do
passo 2) de cinco formas, com os pesos do arquivo repetidos até 10 milhões de valores e
4 threads. Resultado de uma execução ([`resultados/protecoes.txt`](resultados/protecoes.txt)):

| Forma | Tempo | Comparado ao sequencial | Contagem |
|---|--:|--:|---|
| sequencial | 17,2 ms | — | correta |
| sem proteção | 71,3 ms | 4,1× mais lento | **errada: perdeu 60 % das contagens** |
| `critical` | 2 140 ms | 124× mais lento | correta |
| `atomic` | 249 ms | 14× mais lento | correta |
| `reduction` | 5,0 ms | **3,4× mais rápido** | correta |

Os números mudam de uma execução para outra, mas a ordem foi sempre a mesma. Em outras
execuções, a versão sem proteção perdeu de 60 % a 73 % das contagens, `critical` ficou de 63× a
143× mais lento, `atomic` de 11× a 16× mais lento, e `reduction` de 2,5× a 3,9× mais rápido.
Mesmo com só 1000 valores (`./comparar_protecoes dados/pessoas_1000.txt 1000`), a versão sem
proteção perde cerca de metade das contagens.
