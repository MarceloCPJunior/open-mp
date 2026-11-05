# Estudo para a apresentação (não faz parte da entrega)

Respostas curtas para perguntas prováveis sobre o projeto. Os números vêm de
`resultados/benchmark.csv` e `resultados/protecoes.txt`.

- **Por que `reduction` e não `critical`/`atomic`?** `critical` serializa cada iteração e
  `atomic` faz todas as threads disputarem a mesma posição de memória. Com `reduction`, cada
  thread acumula numa cópia privada e as cópias são combinadas uma vez só, no fim. No teste de
  `make protecoes`, `critical` ficou ~124× e `atomic` ~14× mais lentos que o sequencial.
- **Onde estaria a condição de corrida?** No `soma +=` e no `f[c]++` do histograma: duas
  threads leem o valor antigo, as duas somam 1, e uma das atualizações se perde. No teste de
  `make protecoes`, a contagem sem proteção perdeu cerca de 60 % dos valores.
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
