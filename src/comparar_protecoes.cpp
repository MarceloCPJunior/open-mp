/*-----------------------------------------------------------------------------
 * Arquivo    : comparar_protecoes.cpp
 * Curso      : Ciência da Computação — IESB
 * Disciplina : Programação Paralela
 * Trabalho   : Exercício Final — Análise de Dados com OpenMP (experimento extra)
 *
 * Objetivo
 *   Mostrar, com números, por que o programa principal usa `reduction` para
 *   contar as pessoas de cada classe (o histograma de frequências).
 *
 *   O mesmo laço de contagem é executado de cinco formas:
 *     1. sequencial    -> referência (resultado correto)
 *     2. sem proteção  -> parallel for sem nada: CONDIÇÃO DE CORRIDA (errado de propósito)
 *     3. critical      -> uma thread por vez entra no f[c]++
 *     4. atomic        -> f[c]++ feito como operação atômica do processador
 *     5. reduction     -> cada thread conta num histograma privado; soma no final
 *
 *   Os dados são os PESOS do arquivo de 1000 pessoas, repetidos até N valores
 *   (padrão: 10 milhões), agrupados em classes de 4 kg — a mesma regra de
 *   classes do programa principal.
 *
 * Compilar e executar:
 *   g++ -O2 -std=c++17 -fopenmp src/comparar_protecoes.cpp -o comparar_protecoes
 *   ./comparar_protecoes [arquivo] [N] [threads]
 *   (ou: make protecoes)
 *---------------------------------------------------------------------------*/

#include <omp.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

static const double AMPLITUDE_PESO = 4.0;  // kg
static const int    REPETICOES     = 3;    // mediana de 3 (+1 aquecimento)

static inline int classe_de(double v, double L0, double h, int k) {
    int c = static_cast<int>((v - L0) / h);
    return (c < k) ? c : k - 1;
}

// Lê só a 2ª coluna (peso) das linhas que têm dois números.
static std::vector<double> ler_pesos(const std::string& caminho) {
    std::vector<double> pesos;
    std::ifstream arq(caminho);
    std::string linha;
    while (std::getline(arq, linha)) {
        std::istringstream ss(linha);
        double estatura, peso;
        if (ss >> estatura >> peso) pesos.push_back(peso);
    }
    return pesos;
}

// Executa `contar` várias vezes e devolve a mediana do tempo (segundos).
template <typename F>
static double medir(F contar, std::vector<long long>& f) {
    std::vector<double> tempos;
    for (int r = 0; r <= REPETICOES; ++r) {  // r = 0: aquecimento, descartado
        std::fill(f.begin(), f.end(), 0);
        const double t0 = omp_get_wtime();
        contar();
        const double t = omp_get_wtime() - t0;
        if (r > 0) tempos.push_back(t);
    }
    std::sort(tempos.begin(), tempos.end());
    return tempos[tempos.size() / 2];
}

int main(int argc, char** argv) {
    const std::string arquivo = (argc > 1) ? argv[1] : "dados/pessoas_1000.txt";
    const long long   N       = (argc > 2) ? std::atoll(argv[2]) : 10000000LL;
    const int         threads = (argc > 3) ? std::atoi(argv[3]) : omp_get_max_threads();
    if (N < 1 || threads < 1) {
        std::fprintf(stderr, "Uso: %s [arquivo] [N] [threads]\n", argv[0]);
        return 1;
    }
    omp_set_num_threads(threads);

    const std::vector<double> base = ler_pesos(arquivo);
    if (base.empty()) {
        std::fprintf(stderr, "Erro: nenhum dado valido em '%s'\n", arquivo.c_str());
        return 1;
    }

    // Repete os pesos do arquivo até ter N valores.
    std::vector<double> x(N);
    for (long long i = 0; i < N; ++i) x[i] = base[i % base.size()];

    // Mesma regra de classes do programa principal.
    const double mn = *std::min_element(x.begin(), x.end());
    const double mx = *std::max_element(x.begin(), x.end());
    const double h  = AMPLITUDE_PESO;
    const double L0 = std::floor(mn);
    const int    k  = static_cast<int>(std::floor((mx - L0) / h)) + 1;

    std::vector<long long> f(k), referencia;
    long long* F = f.data();
    const double* X = x.data();

    struct Resultado { const char* nome; double tempo; long long total; bool correto; };
    std::vector<Resultado> res;

    auto registrar = [&](const char* nome, double t) {
        long long total = 0;
        for (long long v : f) total += v;
        if (referencia.empty()) referencia = f;  // a 1ª (sequencial) é o gabarito
        res.push_back({nome, t, total, f == referencia});
    };

    // 1. Sequencial — referência.
    registrar("sequencial", medir([&] {
        for (long long i = 0; i < N; ++i) F[classe_de(X[i], L0, h, k)]++;
    }, f));

    // 2. Sem proteção — ERRADO DE PROPÓSITO: várias threads fazem F[c]++ na
    //    mesma posição ao mesmo tempo; cada uma lê o valor antigo, soma 1 e
    //    grava, e uma atualização apaga a outra (condição de corrida).
    registrar("sem protecao", medir([&] {
        #pragma omp parallel for schedule(static)
        for (long long i = 0; i < N; ++i) F[classe_de(X[i], L0, h, k)]++;
    }, f));

    // 3. critical — correto, mas só uma thread por vez executa o incremento.
    registrar("critical", medir([&] {
        #pragma omp parallel for schedule(static)
        for (long long i = 0; i < N; ++i) {
            const int c = classe_de(X[i], L0, h, k);
            #pragma omp critical
            F[c]++;
        }
    }, f));

    // 4. atomic — correto, mas todas as threads disputam as mesmas k posições
    //    de memória (a linha de cache "pula" de núcleo em núcleo).
    registrar("atomic", medir([&] {
        #pragma omp parallel for schedule(static)
        for (long long i = 0; i < N; ++i) {
            const int c = classe_de(X[i], L0, h, k);
            #pragma omp atomic
            F[c]++;
        }
    }, f));

    // 5. reduction — a forma usada no programa principal.
    registrar("reduction", medir([&] {
        #pragma omp parallel for schedule(static) reduction(+ : F[:k])
        for (long long i = 0; i < N; ++i) F[classe_de(X[i], L0, h, k)]++;
    }, f));

    // ---- Relatório --------------------------------------------------------
    const double t_seq = res[0].tempo;
    std::printf("Contagem por classe de peso (h = 4 kg, k = %d classes)\n", k);
    std::printf("N = %lld valores, %d threads, mediana de %d repeticoes\n\n", N, threads, REPETICOES);
    std::printf("  %-13s %11s %18s %12s   %s\n", "Versao", "Tempo (ms)", "vs sequencial",
                "Contagens", "Resultado");
    for (const Resultado& r : res) {
        char comparacao[32];
        if (r.tempo <= t_seq) std::snprintf(comparacao, sizeof comparacao, "%.1fx mais rapido", t_seq / r.tempo);
        else                  std::snprintf(comparacao, sizeof comparacao, "%.1fx mais lento", r.tempo / t_seq);
        if (&r == &res[0]) std::snprintf(comparacao, sizeof comparacao, "-");

        char resultado[64];
        if (r.correto) std::snprintf(resultado, sizeof resultado, "correto");
        else           std::snprintf(resultado, sizeof resultado, "ERRADO (perdeu %.1f%% das contagens)",
                                     100.0 * (N - r.total) / N);
        std::printf("  %-13s %11.3f %18s %12lld   %s\n", r.nome, r.tempo * 1e3, comparacao,
                    r.total, resultado);
    }
    return 0;
}
