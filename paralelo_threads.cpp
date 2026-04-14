// paralelo_threads.cpp
// Uso: ./paralelo_threads matrix1.txt matrix2.txt T resultado_base
// Multiplica M1 x M2 usando T threads
// Cada thread processa N1/T linhas da matriz resultado C
// Gera T arquivos: resultado_base_0.txt, resultado_base_1.txt, ...

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <chrono>
#include <stdexcept>
#include <pthread.h>

// Estrutura passada para cada thread
struct DadosThread {
    int id;                                         // ID da thread (0..T-1)
    int linhaInicio;                                // Primeira linha de C a calcular
    int linhaFim;                                   // Última linha (exclusive)
    int n1, m1, m2;                                 // Dimensões das matrizes
    const std::vector<std::vector<double>>* M1;     // Ponteiro para M1 (somente leitura)
    const std::vector<std::vector<double>>* M2;     // Ponteiro para M2 (somente leitura)
    std::vector<std::vector<double>>* C;            // Ponteiro para resultado (escrita em linhas distintas)
    std::string arquivoSaida;                       // Nome do arquivo de saída desta thread
    double tempoExecucao;                           // Preenchido pela thread após conclusão
};


// Lê uma matriz de arquivo
std::vector<std::vector<double>> lerMatriz(const std::string& nomeArquivo, int& linhas, int& colunas) {
    std::ifstream arquivo(nomeArquivo);
    if (!arquivo.is_open())
        throw std::runtime_error("Erro ao abrir o arquivo: " + nomeArquivo);

    arquivo >> linhas >> colunas;

    std::vector<std::vector<double>> matriz(linhas, std::vector<double>(colunas));
    for (int i = 0; i < linhas; i++)
        for (int j = 0; j < colunas; j++)
            arquivo >> matriz[i][j];

    arquivo.close();
    return matriz;
}

// Salva a parte do resultado referente a esta thread
void salvarParcial(const std::string& nomeArquivo,
                   const std::vector<std::vector<double>>& C,
                   int linhaInicio, int linhaFim,
                   double tempoSegundos) {
    std::ofstream arquivo(nomeArquivo);
    if (!arquivo.is_open())
        throw std::runtime_error("Erro ao criar arquivo: " + nomeArquivo);

    int numLinhas = linhaFim - linhaInicio;
    int numColunas = (numLinhas > 0) ? (int)C[linhaInicio].size() : 0;

    arquivo << numLinhas << " " << numColunas << "\n";
    for (int i = linhaInicio; i < linhaFim; i++) {
        for (int j = 0; j < numColunas; j++) {
            arquivo << C[i][j];
            if (j < numColunas - 1) arquivo << " ";
        }
        arquivo << "\n";
    }
    arquivo << tempoSegundos << "\n";
    arquivo.close();
}

// Função executada por cada thread
void* multiplicarLinhas(void* arg) {
    DadosThread* dados = static_cast<DadosThread*>(arg);

    const auto& M1 = *(dados->M1);
    const auto& M2 = *(dados->M2);
    auto& C = *(dados->C);

    auto inicio = std::chrono::high_resolution_clock::now();

    // Cada thread calcula as linhas [linhaInicio, linhaFim) de C
    for (int i = dados->linhaInicio; i < dados->linhaFim; i++) {
        for (int j = 0; j < dados->m2; j++) {
            C[i][j] = 0.0;
            for (int k = 0; k < dados->m1; k++) {
                C[i][j] += M1[i][k] * M2[k][j];
            }
        }
    }

    auto fim = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duracao = fim - inicio;
    dados->tempoExecucao = duracao.count();

    salvarParcial(dados->arquivoSaida, C,
                  dados->linhaInicio, dados->linhaFim,
                  dados->tempoExecucao);

    return nullptr;
}

int main(int argc, char* argv[]) {
    if (argc != 5) {
        std::cerr << "Uso: " << argv[0]
                  << " matrix1.txt matrix2.txt T resultado_base" << std::endl;
        std::cerr << "  T: numero de threads" << std::endl;
        std::cerr << "  resultado_base: prefixo dos arquivos de saida" << std::endl;
        return 1;
    }

    std::string arq1    = argv[1];
    std::string arq2    = argv[2];
    int T               = atoi(argv[3]);
    std::string base    = argv[4];

    if (T <= 0) {
        std::cerr << "Erro: T deve ser um inteiro positivo." << std::endl;
        return 1;
    }

    int n1, m1, n2, m2;
    auto M1 = lerMatriz(arq1, n1, m1);
    auto M2 = lerMatriz(arq2, n2, m2);

    if (m1 != n2) {
        std::cerr << "Erro: dimensoes incompativeis! m1=" << m1
                  << " deve ser igual a n2=" << n2 << std::endl;
        return 1;
    }

    if (T > n1) {
        std::cout << "Aviso: T=" << T << " > n1=" << n1
                  << ". Usando T=" << n1 << " threads." << std::endl;
        T = n1;
    }

    std::vector<std::vector<double>> C(n1, std::vector<double>(m2, 0.0));

    int linhasPorThread = n1 / T;
    int resto           = n1 % T;

    std::vector<pthread_t>    threads(T);
    std::vector<DadosThread>  dados(T);

    int linhaAtual = 0;
    for (int t = 0; t < T; t++) {
        int qtdLinhas = linhasPorThread + (t < resto ? 1 : 0);

        dados[t].id           = t;
        dados[t].linhaInicio  = linhaAtual;
        dados[t].linhaFim     = linhaAtual + qtdLinhas;
        dados[t].n1           = n1;
        dados[t].m1           = m1;
        dados[t].m2           = m2;
        dados[t].M1           = &M1;
        dados[t].M2           = &M2;
        dados[t].C            = &C;
        dados[t].arquivoSaida = base + "_" + std::to_string(t) + ".txt";
        dados[t].tempoExecucao = 0.0;

        linhaAtual += qtdLinhas;
    }

    // Cria as threads
    for (int t = 0; t < T; t++) {
        if (pthread_create(&threads[t], nullptr, multiplicarLinhas, &dados[t]) != 0) {
            std::cerr << "Erro ao criar thread " << t << std::endl;
            return 1;
        }
    }

    for (int t = 0; t < T; t++) {
        pthread_join(threads[t], nullptr);
    }

    double tempoTotal = 0.0;
    for (int t = 0; t < T; t++) {
        if (dados[t].tempoExecucao > tempoTotal)
            tempoTotal = dados[t].tempoExecucao;
    }

    std::cout << "Multiplicacao paralela (threads) concluida!" << std::endl;
    std::cout << "Dimensoes: (" << n1 << "x" << m1 << ") x ("
              << n2 << "x" << m2 << ") = (" << n1 << "x" << m2 << ")" << std::endl;
    std::cout << "Numero de threads: " << T << std::endl;
    std::cout << "Tempo total (maior entre threads): " << tempoTotal << " segundos" << std::endl;
    for (int t = 0; t < T; t++) {
        std::cout << "  Thread " << t << ": linhas ["
                  << dados[t].linhaInicio << ", " << dados[t].linhaFim << ")"
                  << "  tempo=" << dados[t].tempoExecucao << "s"
                  << "  arquivo=" << dados[t].arquivoSaida << std::endl;
    }

    return 0;
}
