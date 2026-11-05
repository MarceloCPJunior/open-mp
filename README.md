# Exercício Final — Análise de Dados com OpenMP

Coeficiente de variação (CV) da **estatura** e do **peso** de 1000 pessoas, com dados
agrupados em classes (8 cm e 4 kg), em C++ com OpenMP.
Disciplina de Programação Paralela, Ciência da Computação (IESB), Prof. Rodrigo Gonçalves Pinto.

## Resultado

| Variável | Média (μ) | Desvio padrão (σ) | CV |
|---|--:|--:|--:|
| Estatura | 170,30 cm | 9,24 cm | **5,43 %** |
| Peso | 73,06 kg | 13,99 kg | **19,16 %** |

Com N = 1000 o cálculo leva ~6 µs e a versão paralela é **mais lenta** (0,43× com
4 threads), por causa do custo fixo das regiões paralelas. O ganho aparece a partir de
~10⁴ pessoas e chega a **≈ 3× com 4 threads** para N ≥ 10⁵. Os detalhes estão no relatório.

## Entregáveis

| Item pedido | Onde está |
|---|---|
| 1) Código-fonte C++ com OpenMP | [`src/cv_openmp.cpp`](src/cv_openmp.cpp) |
| 2) Relatório em PDF (o que foi paralelizado e por quê) | [`relatorio/relatorio.pdf`](relatorio/relatorio.pdf) (versão editável: `relatorio/relatorio.docx`) |
| 3) Resultados: CV da estatura e do peso | tabela acima e [`resultados/saida_1000.txt`](resultados/saida_1000.txt) |
| 4) Análise de desempenho (sequencial x paralelo) | relatório, Seção 6, [`resultados/benchmark.csv`](resultados/benchmark.csv) e [`resultados/graficos/`](resultados/graficos/) |

## Como rodar do zero

Precisa de `g++` com OpenMP (Linux, contêiner Docker da disciplina ou MinGW no Windows) e `python3`.

```bash
make                                    # compila ./cv_openmp
./cv_openmp dados/pessoas_1000.txt 4    # arquivo, nº de threads [, repetições]
make run                                # o mesmo, salvando em resultados/saida_1000.txt
make verificar                          # recalcula tudo em Python (conferência independente)
make benchmark                          # 1/2/4/8 threads x N = 10³..10⁷ -> resultados/benchmark.csv
make graficos                           # gráficos (precisa de matplotlib)
```

Windows (PowerShell, MinGW):

```powershell
g++ -O2 -std=c++17 -fopenmp src\cv_openmp.cpp -o cv_openmp.exe
.\cv_openmp.exe dados\pessoas_1000.txt 4
```

A saída mostra a tabela de classes de cada variável (x_i, f_i, x_i·f_i, f_i·(x_i−μ)²), μ, σ
e CV, a validação sequencial x paralelo e uma tabela de tempo por etapa com o speedup.

## Usando outro arquivo de dados

O enunciado não trouxe o arquivo, então `dados/pessoas_1000.txt` é **sintético**, gerado com
semente fixa por `python3 scripts/gerar_dados.py 1000 dados/pessoas_1000.txt 2025`. Se o
professor fornecer um arquivo, é só passá-lo como argumento (`./cv_openmp arquivo.txt`).
Linhas de cabeçalho são ignoradas, TAB/`;` e vírgula decimal são aceitos, e estatura em metros
é convertida para cm. Com dados novos, rode de novo `make run`, `make benchmark` e
`make graficos`, e atualize os números do relatório (`relatorio/relatorio.md`; o PDF é
gerado por `relatorio/gerar_relatorio.sh`).

## Estrutura

```
src/cv_openmp.cpp          programa: leitura, versões sequencial e paralela, validação e medição
dados/pessoas_1000.txt     entrada: "estatura peso" por linha (1000 pessoas)
scripts/gerar_dados.py     gera dados sintéticos (usado também para N grande no benchmark)
scripts/verificar.py       conferência independente do resultado (Python puro)
scripts/benchmark.sh       mede sequencial x paralelo e grava CSV
scripts/graficos.py        gráficos a partir do CSV
resultados/                saída do programa, CSV de medições e gráficos
relatorio/                 relatorio.md (fonte), relatorio.pdf, relatorio.docx
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

## Para a apresentação: perguntas prováveis

- **Por que `reduction` e não `critical`/`atomic`?** `critical` serializa cada iteração e
  `atomic` faz todas as threads disputarem a mesma posição de memória. Com `reduction`, cada
  thread acumula numa cópia privada e as cópias são combinadas uma vez só, no fim.
- **Onde estaria a condição de corrida?** No `soma +=` e no `f[c]++` do histograma: duas
  threads leem o valor antigo, as duas somam 1, e uma das atualizações se perde.
- **Por que o paralelo perde com 1000 pessoas?** Cada região paralela custa ~1–2 µs (acordar
  as threads, barreira, combinação da redução), e o cálculo inteiro leva ~6 µs. Nos laços
  sobre as classes (k = 7 e 22 iterações) o custo fixo é dezenas de vezes maior que o trabalho.
- **Por que não chega a 4× com 4 threads?** Custo de sincronização e redução, banda de memória
  (com N = 10⁷ os vetores têm 80 MB, mais que a cache L3) e vCPUs virtualizadas.
- **Por que 8 threads piora?** A máquina tem 4 núcleos (sobreassinatura): as threads revezam
  a CPU e cada barreira espera por uma thread que está fora do processador.
- **Por que as somas paralelas podem diferir na última casa?** A soma em ponto flutuante não
  é associativa, e a ordem muda com as threads. Por isso a validação usa tolerância de 10⁻⁹
  nas somas, mas exige frequências idênticas.
