/*-----------------------------------------------------------------------------
 * Arquivo    : cv_openmp.cpp
 * Curso      : Ciência da Computação — IESB
 * Disciplina : Programação Paralela
 * Professor  : Rodrigo Gonçalves Pinto
 * Trabalho   : Exercício Final — Análise de Dados com OpenMP
 *
 * Objetivo
 *   Calcular o coeficiente de variação (CV) da ESTATURA (cm) e do PESO (kg)
 *   de um grupo de pessoas, com os dados AGRUPADOS EM CLASSES:
 *       estatura -> classes de 8 cm
 *       peso     -> classes de 4 kg
 *   Os laços de cálculo são paralelizados com OpenMP e comparados com a
 *   versão sequencial do MESMO algoritmo.
 *
 * Fórmulas (dados agrupados; x_i = ponto médio da classe i, f_i = frequência)
 *
 *                 k                        k
 *   (1)  μ  =  (  Σ  x_i · f_i  )  /  N ,   N = Σ f_i
 *                i=1                      i=1
 *                       k
 *   (2)  σ  =  sqrt( (  Σ  f_i · (x_i − μ)²  ) / N )
 *                      i=1
 *
 *   (3)  CV =  (σ / μ) × 100 %
 *
 * Compilar (Linux / contêiner da disciplina / MinGW no Windows):
 *   g++ -O2 -std=c++17 -fopenmp src/cv_openmp.cpp -o cv_openmp
 *   (ou simplesmente: make)
 *
 * Executar:
 *   ./cv_openmp [arquivo] [threads] [repeticoes]
 *     arquivo    : texto com "estatura peso" por linha (padrão: dados/pessoas_1000.txt)
 *     threads    : nº de threads da versão paralela (padrão: todos os núcleos)
 *     repeticoes : nº de repetições cronometradas   (padrão: automático)
 *
 *   Exemplo: ./cv_openmp dados/pessoas_1000.txt 4
 *---------------------------------------------------------------------------*/

#include <omp.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

// Amplitudes de classe pedidas no enunciado.
static const double AMPLITUDE_ESTATURA = 8.0;  // cm
static const double AMPLITUDE_PESO     = 4.0;  // kg

// ============================================================================
// 1. Estruturas de dados
// ============================================================================

// Distribuição de frequências (tabela de classes) de UMA variável.
struct Distribuicao {
    double amplitude       = 0.0;  // h   : largura de cada classe (8 cm ou 4 kg)
    double minimo          = 0.0;  // menor valor observado
    double maximo          = 0.0;  // maior valor observado
    double limite_inferior = 0.0;  // L0  : início da 1ª classe = floor(minimo)
    int    k               = 0;    // número de classes
    std::vector<long long> freq;         // f_i : quantas pessoas caem na classe i
    std::vector<double>    ponto_medio;  // x_i : centro da classe i
    long long N            = 0;    // Σ f_i (deve ser igual ao nº de pessoas)
    double soma_xf         = 0.0;  // Σ x_i·f_i
    double soma_desvios2   = 0.0;  // Σ f_i·(x_i − μ)²
    double media           = 0.0;  // μ
    double desvio          = 0.0;  // σ
    double cv              = 0.0;  // CV em %
};

// Tempo (em segundos) gasto em cada etapa do cálculo.
struct Tempos {
    double minmax      = 0.0;  // etapa 1: menor e maior valor
    double frequencias = 0.0;  // etapa 2: contagem por classe (histograma)
    double media       = 0.0;  // etapa 3: Σ x_i·f_i e Σ f_i
    double desvio      = 0.0;  // etapa 4: Σ f_i·(x_i − μ)²
    double total       = 0.0;  // análise completa das duas variáveis
};

// ============================================================================
// 2. Montagem das classes
// ============================================================================
//
// Regra usada (a mesma nas versões sequencial e paralela):
//   - a 1ª classe começa em L0 = floor(menor valor);
//   - cada classe tem largura h e é FECHADA à esquerda e ABERTA à direita:
//         classe i = [L0 + i·h , L0 + (i+1)·h)        (notação "L0 |- L0+h")
//   - k = floor((maior − L0) / h) + 1   garante que o maior valor caia
//     dentro da última classe;
//   - ponto médio x_i = L0 + (i + 0,5)·h.
static void montar_classes(Distribuicao& d, double h) {
    d.amplitude       = h;
    d.limite_inferior = std::floor(d.minimo);
    d.k = static_cast<int>(std::floor((d.maximo - d.limite_inferior) / h)) + 1;

    d.freq.assign(d.k, 0);
    d.ponto_medio.resize(d.k);
    for (int i = 0; i < d.k; ++i) {
        d.ponto_medio[i] = d.limite_inferior + (i + 0.5) * h;
    }
}

// Índice da classe em que o valor v cai.
static inline int classe_de(double v, double L0, double h, int k) {
    int c = static_cast<int>((v - L0) / h);
    return (c < k) ? c : k - 1;  // proteção contra arredondamento no último limite
}

// ============================================================================
// 3. Etapas do cálculo — versão SEQUENCIAL
// ============================================================================

// Etapa 1 (seq): menor e maior valor. Percorre os N dados.
static void min_max_seq(const double* x, long long n, double& menor, double& maior) {
    double mn = x[0], mx = x[0];
    for (long long i = 0; i < n; ++i) {
        if (x[i] < mn) mn = x[i];
        if (x[i] > mx) mx = x[i];
    }
    menor = mn;
    maior = mx;
}

// Etapa 2 (seq): frequência de cada classe. Percorre os N dados.
static void frequencias_seq(const double* x, long long n,
                            double L0, double h, int k, long long* f) {
    for (long long i = 0; i < n; ++i) {
        f[classe_de(x[i], L0, h, k)]++;
    }
}

// Etapa 3 (seq): somas da média — Σ x_i·f_i e Σ f_i. Percorre as k classes.
static void somas_media_seq(const double* xm, const long long* f, int k,
                            double& soma_xf, long long& soma_f) {
    double sxf = 0.0;
    long long sf = 0;
    for (int i = 0; i < k; ++i) {
        sxf += xm[i] * static_cast<double>(f[i]);
        sf  += f[i];
    }
    soma_xf = sxf;
    soma_f  = sf;
}

// Etapa 4 (seq): soma do desvio padrão — Σ f_i·(x_i − μ)². Percorre as k classes.
static double soma_desvios_seq(const double* xm, const long long* f, int k, double mu) {
    double s = 0.0;
    for (int i = 0; i < k; ++i) {
        const double d = xm[i] - mu;
        s += static_cast<double>(f[i]) * d * d;
    }
    return s;
}

// ============================================================================
// 4. Etapas do cálculo — versão PARALELA (OpenMP)
// ============================================================================
//
// Todos os laços abaixo acumulam um resultado em variáveis COMPARTILHADAS
// (soma, mínimo, máximo, contadores). Se cada thread escrevesse direto nelas
// haveria CONDIÇÃO DE CORRIDA (leituras e escritas simultâneas perdem
// atualizações). A solução usada em todos os laços é a cláusula `reduction`:
//   - cada thread recebe uma CÓPIA PRIVADA da variável, iniciada com o
//     elemento neutro da operação (0 para +, +inf para min, −inf para max);
//   - cada thread trabalha só na sua cópia, sem sincronização;
//   - no fim do laço, o OpenMP combina as cópias UMA vez por thread.
// Alternativas descartadas:
//   - `critical` / `atomic` em cada iteração: serializam ou disputam a mesma
//     posição de memória milhões de vezes; ficam mais lentas que o sequencial.
//
// `schedule(static)`: toda iteração custa o mesmo, então dividir o laço em
// blocos contíguos e iguais equilibra a carga sem custo de escalonamento.

// Etapa 1 (par): menor e maior valor com reduction(min) e reduction(max).
static void min_max_par(const double* x, long long n, double& menor, double& maior) {
    double mn = x[0], mx = x[0];
    #pragma omp parallel for schedule(static) reduction(min:mn) reduction(max:mx)
    for (long long i = 0; i < n; ++i) {
        if (x[i] < mn) mn = x[i];
        if (x[i] > mx) mx = x[i];
    }
    menor = mn;
    maior = mx;
}

// Etapa 2 (par): histograma com redução sobre VETOR (OpenMP 4.5+).
// Duas threads podem achar pessoas da mesma classe ao mesmo tempo e fazer
// f[c]++ simultaneamente -> corrida. Com `reduction(+ : f[:k])` cada thread
// conta num histograma privado de k posições, e os k contadores de todas as
// threads são somados no final. Como os histogramas são privados, também não
// há "false sharing" entre as threads durante o laço.
static void frequencias_par(const double* x, long long n,
                            double L0, double h, int k, long long* f) {
    #pragma omp parallel for schedule(static) reduction(+ : f[:k])
    for (long long i = 0; i < n; ++i) {
        f[classe_de(x[i], L0, h, k)]++;
    }
}

// Etapa 3 (par): somas da média — duas variáveis na mesma reduction(+).
static void somas_media_par(const double* xm, const long long* f, int k,
                            double& soma_xf, long long& soma_f) {
    double sxf = 0.0;
    long long sf = 0;
    #pragma omp parallel for schedule(static) reduction(+ : sxf, sf)
    for (int i = 0; i < k; ++i) {
        sxf += xm[i] * static_cast<double>(f[i]);
        sf  += f[i];
    }
    soma_xf = sxf;
    soma_f  = sf;
}

// Etapa 4 (par): soma do desvio padrão. μ já é conhecido e só é LIDO
// pelas threads (compartilhado sem risco); a soma usa reduction(+).
static double soma_desvios_par(const double* xm, const long long* f, int k, double mu) {
    double s = 0.0;
    #pragma omp parallel for schedule(static) reduction(+ : s)
    for (int i = 0; i < k; ++i) {
        const double d = xm[i] - mu;
        s += static_cast<double>(f[i]) * d * d;
    }
    return s;
}

// ============================================================================
// 5. Análise completa de uma variável (sequencial ou paralela)
// ============================================================================
static void analisar(const std::vector<double>& dados, double h, bool paralelo,
                     Distribuicao& d, Tempos& t) {
    const double*   x = dados.data();
    const long long n = static_cast<long long>(dados.size());

    // Etapa 1 — menor e maior valor (definem onde as classes começam e terminam)
    double t0 = omp_get_wtime();
    if (paralelo) min_max_par(x, n, d.minimo, d.maximo);
    else          min_max_seq(x, n, d.minimo, d.maximo);
    double t1 = omp_get_wtime();

    // Montagem das classes: O(k), k pequeno (7 e 22 aqui) — trabalho desprezível, fica sequencial.
    montar_classes(d, h);

    // Etapa 2 — frequência f_i de cada classe
    double t2 = omp_get_wtime();
    if (paralelo) frequencias_par(x, n, d.limite_inferior, h, d.k, d.freq.data());
    else          frequencias_seq(x, n, d.limite_inferior, h, d.k, d.freq.data());
    double t3 = omp_get_wtime();

    // Etapa 3 — média ponderada: μ = Σ x_i·f_i / Σ f_i       (fórmula 1)
    if (paralelo) somas_media_par(d.ponto_medio.data(), d.freq.data(), d.k, d.soma_xf, d.N);
    else          somas_media_seq(d.ponto_medio.data(), d.freq.data(), d.k, d.soma_xf, d.N);
    d.media = d.soma_xf / static_cast<double>(d.N);
    double t4 = omp_get_wtime();

    // Etapa 4 — desvio padrão populacional: σ = sqrt(Σ f_i·(x_i−μ)² / N)   (fórmula 2)
    if (paralelo) d.soma_desvios2 = soma_desvios_par(d.ponto_medio.data(), d.freq.data(), d.k, d.media);
    else          d.soma_desvios2 = soma_desvios_seq(d.ponto_medio.data(), d.freq.data(), d.k, d.media);
    d.desvio = std::sqrt(d.soma_desvios2 / static_cast<double>(d.N));

    // Coeficiente de variação                                           (fórmula 3)
    d.cv = d.desvio / d.media * 100.0;
    double t5 = omp_get_wtime();

    t.minmax      += t1 - t0;
    t.frequencias += t3 - t2;
    t.media       += t4 - t3;
    t.desvio      += t5 - t4;
}

// Análise das DUAS variáveis (estatura e peso), cronometrada.
static Tempos analisar_tudo(const std::vector<double>& estatura,
                            const std::vector<double>& peso, bool paralelo,
                            Distribuicao& de, Distribuicao& dp) {
    Tempos t;
    double ini = omp_get_wtime();
    analisar(estatura, AMPLITUDE_ESTATURA, paralelo, de, t);
    analisar(peso,     AMPLITUDE_PESO,     paralelo, dp, t);
    t.total = omp_get_wtime() - ini;
    return t;
}

// ============================================================================
// 6. Leitura do arquivo de entrada
// ============================================================================
//
// Formato esperado: uma pessoa por linha, "estatura peso" separados por espaço.
// Para tolerar pequenas variações de formato:
//   - linhas em branco ou de cabeçalho (sem dois números) são ignoradas;
//   - TAB e ';' também são aceitos como separadores;
//   - vírgula decimal ("172,5") é aceita quando o arquivo não tem nenhum ponto;
//   - se TODAS as estaturas forem < 3, assume-se que estão em metros e
//     elas são convertidas para cm (o enunciado pede cm).
// A leitura é feita uma vez, de forma sequencial (é E/S + conversão de texto),
// e NÃO entra na comparação de tempo entre as versões (é igual para as duas).
static bool ler_arquivo(const std::string& caminho,
                        std::vector<double>& estatura, std::vector<double>& peso,
                        long long& ignoradas, bool& converteu_metros) {
    std::ifstream arq(caminho, std::ios::binary);
    if (!arq) {
        std::fprintf(stderr, "Erro: nao foi possivel abrir '%s'\n", caminho.c_str());
        return false;
    }
    std::stringstream ss;
    ss << arq.rdbuf();
    std::string buf = ss.str();  // termina em '\0' (garantido por std::string)

    const bool tem_ponto = buf.find('.') != std::string::npos;
    for (char& c : buf) {
        if (c == '\t' || c == ';' || c == '\r') c = ' ';
        else if (c == ',') c = tem_ponto ? ' ' : '.';
    }

    ignoradas = 0;
    const char* p   = buf.c_str();
    const char* fim = p + buf.size();
    while (p < fim) {
        const char* eol = static_cast<const char*>(std::memchr(p, '\n', fim - p));
        if (!eol) eol = fim;

        char* e1 = nullptr;
        char* e2 = nullptr;
        double a = std::strtod(p, &e1);
        bool ok = (e1 != p) && (e1 <= eol);
        double w = 0.0;
        if (ok) {
            w  = std::strtod(e1, &e2);
            ok = (e2 != e1) && (e2 <= eol) && std::isfinite(a) && std::isfinite(w) && a > 0 && w > 0;
        }
        if (ok) {
            estatura.push_back(a);
            peso.push_back(w);
        } else if (std::strspn(p, " ") < static_cast<size_t>(eol - p)) {
            ++ignoradas;  // linha não vazia que não tem dois números (ex.: cabeçalho)
        }
        p = eol + 1;
    }

    if (estatura.empty()) {
        std::fprintf(stderr, "Erro: nenhuma linha valida em '%s'\n", caminho.c_str());
        return false;
    }

    converteu_metros = *std::max_element(estatura.begin(), estatura.end()) < 3.0;
    if (converteu_metros) {
        for (double& v : estatura) v *= 100.0;
    }
    return true;
}

// ============================================================================
// 7. Utilitários: mediana, validação e impressão
// ============================================================================
static double mediana(std::vector<double> v) {
    std::sort(v.begin(), v.end());
    const size_t m = v.size() / 2;
    return (v.size() % 2) ? v[m] : 0.5 * (v[m - 1] + v[m]);
}

static bool quase_igual(double a, double b) {
    return std::fabs(a - b) <= 1e-9 * std::max(1.0, std::fabs(a));
}

// A versão paralela tem que produzir EXATAMENTE a mesma tabela de frequências
// e as mesmas estatísticas da sequencial (a menos de arredondamento na ordem
// das somas de ponto flutuante).
static bool mesma_distribuicao(const Distribuicao& a, const Distribuicao& b) {
    return a.k == b.k && a.freq == b.freq && a.N == b.N &&
           quase_igual(a.limite_inferior, b.limite_inferior) &&
           quase_igual(a.media, b.media) && quase_igual(a.desvio, b.desvio) &&
           quase_igual(a.cv, b.cv);
}

static void imprimir_tabela(const char* titulo, const char* unidade, const Distribuicao& d) {
    std::printf("\n--- %s (%s) | amplitude de classe h = %.0f %s ", titulo, unidade, d.amplitude, unidade);
    std::printf("---------------\n");
    std::printf("  menor valor = %.1f %s   maior valor = %.1f %s   classes (k) = %d\n\n",
                d.minimo, unidade, d.maximo, unidade, d.k);
    std::printf("  %-17s %8s %7s %7s %12s %16s\n",
                "Classe", "x_i", "f_i", "fr(%)", "x_i*f_i", "f_i*(x_i-mu)^2");
    for (int i = 0; i < d.k; ++i) {
        const double li  = d.limite_inferior + i * d.amplitude;
        const double dev = d.ponto_medio[i] - d.media;
        char classe[32];
        std::snprintf(classe, sizeof classe, "%6.1f |- %6.1f", li, li + d.amplitude);
        std::printf("  %-17s %8.1f %7lld %7.2f %12.1f %16.2f\n", classe, d.ponto_medio[i],
                    d.freq[i], 100.0 * d.freq[i] / d.N, d.ponto_medio[i] * d.freq[i],
                    d.freq[i] * dev * dev);
    }
    std::printf("  %-17s %8s %7lld %7.2f %12.1f %16.2f\n", "Total", "", d.N, 100.0,
                d.soma_xf, d.soma_desvios2);
    std::printf("\n  Media ponderada        mu    = %10.4f %s\n", d.media, unidade);
    std::printf("  Desvio padrao pop.     sigma = %10.4f %s\n", d.desvio, unidade);
    std::printf("  Coef. de variacao      CV    = %10.4f %%\n", d.cv);
}

// ============================================================================
// 8. Programa principal
// ============================================================================
int main(int argc, char** argv) {
    const std::string arquivo = (argc > 1) ? argv[1] : "dados/pessoas_1000.txt";
    const int threads = (argc > 2) ? std::atoi(argv[2]) : omp_get_max_threads();
    int repeticoes    = (argc > 3) ? std::atoi(argv[3]) : 0;

    if (threads < 1) {
        std::fprintf(stderr, "Erro: numero de threads invalido\n");
        return 1;
    }
    omp_set_num_threads(threads);

    // ---- Leitura (sequencial) ---------------------------------------------
    std::vector<double> estatura, peso;
    long long ignoradas = 0;
    bool converteu_metros = false;
    const double tl = omp_get_wtime();
    if (!ler_arquivo(arquivo, estatura, peso, ignoradas, converteu_metros)) return 1;
    const double tempo_leitura = omp_get_wtime() - tl;
    const long long n = static_cast<long long>(estatura.size());

    // Repetições automáticas: muitas para N pequeno (cada execução dura
    // microssegundos e um único tempo seria só ruído), poucas para N grande.
    if (repeticoes <= 0) {
        repeticoes = static_cast<int>(std::min(2000LL, std::max(5LL, 200000000LL / n)));
    }

    std::printf("==================================================================\n");
    std::printf(" Coeficiente de variacao com dados agrupados em classes (OpenMP)\n");
    std::printf("==================================================================\n");
    std::printf(" Arquivo               : %s\n", arquivo.c_str());
    std::printf(" Pessoas (N)           : %lld\n", n);
    if (ignoradas) std::printf(" Linhas ignoradas      : %lld (sem dois numeros validos)\n", ignoradas);
    if (converteu_metros) std::printf(" Aviso                 : estaturas em metros convertidas para cm\n");
    std::printf(" Threads (paralelo)    : %d   (processadores disponiveis: %d)\n", threads, omp_get_num_procs());
    std::printf(" Repeticoes medidas    : %d   (+1 de aquecimento, descartada)\n", repeticoes);

    // ---- Medição: sequencial x paralelo -----------------------------------
    // A rodada r = 0 é aquecimento (cria o pool de threads do OpenMP, carrega
    // os dados na cache) e é descartada. As duas versões são executadas de
    // forma intercalada, para que sofram as mesmas condições da máquina.
    Distribuicao est_seq, pes_seq, est_par, pes_par;
    std::vector<double> s_mm, s_fr, s_me, s_dp, s_tt;
    std::vector<double> p_mm, p_fr, p_me, p_dp, p_tt;
    bool resultados_iguais = true;

    for (int r = 0; r <= repeticoes; ++r) {
        Tempos ts = analisar_tudo(estatura, peso, false, est_seq, pes_seq);
        Tempos tp = analisar_tudo(estatura, peso, true,  est_par, pes_par);
        resultados_iguais = resultados_iguais &&
                            mesma_distribuicao(est_seq, est_par) &&
                            mesma_distribuicao(pes_seq, pes_par);
        if (r == 0) continue;
        s_mm.push_back(ts.minmax); s_fr.push_back(ts.frequencias);
        s_me.push_back(ts.media);  s_dp.push_back(ts.desvio); s_tt.push_back(ts.total);
        p_mm.push_back(tp.minmax); p_fr.push_back(tp.frequencias);
        p_me.push_back(tp.media);  p_dp.push_back(tp.desvio); p_tt.push_back(tp.total);
    }

    // ---- Tabelas de frequência e resultados -------------------------------
    imprimir_tabela("ESTATURA", "cm", est_par);
    imprimir_tabela("PESO", "kg", pes_par);

    std::printf("\n==================================================================\n");
    std::printf(" RESULTADO\n");
    std::printf("==================================================================\n");
    std::printf("  CV da estatura = %.4f %%   (mu = %.4f cm, sigma = %.4f cm)\n",
                est_par.cv, est_par.media, est_par.desvio);
    std::printf("  CV do peso     = %.4f %%   (mu = %.4f kg, sigma = %.4f kg)\n",
                pes_par.cv, pes_par.media, pes_par.desvio);
    std::printf("  Validacao sequencial x paralelo: %s\n",
                resultados_iguais ? "OK (mesmas frequencias, mesma media, desvio e CV)"
                                  : "DIFERENTE -- verificar!");

    // ---- Desempenho -------------------------------------------------------
    const double us = 1e6;  // segundos -> microssegundos
    struct Linha { const char* nome; double seq, par; };
    const Linha linhas[] = {
        {"1. min/max            (N)", mediana(s_mm) * us, mediana(p_mm) * us},
        {"2. frequencias        (N)", mediana(s_fr) * us, mediana(p_fr) * us},
        {"3. media              (k)", mediana(s_me) * us, mediana(p_me) * us},
        {"4. desvio padrao + CV (k)", mediana(s_dp) * us, mediana(p_dp) * us},
        {"TOTAL (2 variaveis)",       mediana(s_tt) * us, mediana(p_tt) * us},
    };
    std::printf("\n==================================================================\n");
    std::printf(" DESEMPENHO  (mediana de %d repeticoes, em microssegundos)\n", repeticoes);
    std::printf("==================================================================\n");
    std::printf("  %-27s %13s %13s %9s\n", "Etapa", "Sequencial", "Paralelo", "Speedup");
    for (const Linha& l : linhas) {
        std::printf("  %-27s %13.2f %13.2f %8.2fx\n", l.nome, l.seq, l.par, l.seq / l.par);
    }
    const double t_seq = linhas[4].seq, t_par = linhas[4].par;
    const double speedup = t_seq / t_par;
    std::printf("\n  Speedup = %.2fx   Eficiencia = %.1f %%   (%d threads)\n",
                speedup, 100.0 * speedup / threads, threads);
    std::printf("  Leitura do arquivo (sequencial, fora da comparacao): %.2f ms\n",
                tempo_leitura * 1e3);

    // Linha em CSV para o script de benchmark (scripts/benchmark.sh).
    std::printf("\nCSV_CABECALHO,n,threads,repeticoes,leitura_us,"
                "seq_minmax_us,par_minmax_us,seq_freq_us,par_freq_us,"
                "seq_media_us,par_media_us,seq_desvio_us,par_desvio_us,"
                "seq_total_us,par_total_us,speedup,eficiencia,cv_estatura,cv_peso,valido\n");
    std::printf("CSV,%lld,%d,%d,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.4f,%.4f,%.6f,%.6f,%d\n",
                n, threads, repeticoes, tempo_leitura * us,
                linhas[0].seq, linhas[0].par, linhas[1].seq, linhas[1].par,
                linhas[2].seq, linhas[2].par, linhas[3].seq, linhas[3].par,
                t_seq, t_par, speedup, speedup / threads, est_par.cv, pes_par.cv,
                resultados_iguais ? 1 : 0);

    return resultados_iguais ? 0 : 2;
}
