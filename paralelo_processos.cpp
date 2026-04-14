// paralelo_processos.cpp
// Uso: ./paralelo_processos matrix1.txt matrix2.txt P resultado_base
// Multiplica M1 x M2 usando P processos filhos (fork)
// Cada processo calcula N1/P linhas da matriz resultado C
// Gera P arquivos: resultado_base_0.txt, resultado_base_1.txt, ...

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <chrono>
#include <stdexcept>
#include <sys/wait.h>
#include <unistd.h>

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

// Salva a parte do resultado referente a este processo
void salvarParcial(const std::string& nomeArquivo,
                   const std::vector<std::vector<double>>& C,
                   int linhaInicio, int linhaFim,
                   double tempoSegundos) {
    std::ofstream arquivo(nomeArquivo);
    if (!arquivo.is_open())
        throw std::runtime_error("Erro ao criar arquivo: " + nomeArquivo);

    int numLinhas  = linhaFim - linhaInicio;
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

int main(int argc, char* argv[]) {
    if (argc != 5) {
        std::cerr << "Uso: " << argv[0]
                  << " matrix1.txt matrix2.txt P resultado_base" << std::endl;
        std::cerr << "  P: numero de processos" << std::endl;
        std::cerr << "  resultado_base: prefixo dos arquivos de saida" << std::endl;
        return 1;
    }

    std::string arq1 = argv[1];
    std::string arq2 = argv[2];
    int P            = atoi(argv[3]);
    std::string base = argv[4];

    if (P <= 0) {
        std::cerr << "Erro: P deve ser um inteiro positivo." << std::endl;
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

    if (P > n1) {
        std::cout << "Aviso: P=" << P << " > n1=" << n1
                  << ". Usando P=" << n1 << " processos." << std::endl;
        P = n1;
    }

    int linhasPorProcesso = n1 / P;
    int resto             = n1 % P;

    // Calcula os intervalos de cada processo antes do fork
    std::vector<int> inicios(P), fins(P);
    int linhaAtual = 0;
    for (int p = 0; p < P; p++) {
        int qtdLinhas = linhasPorProcesso + (p < resto ? 1 : 0);
        inicios[p] = linhaAtual;
        fins[p]    = linhaAtual + qtdLinhas;
        linhaAtual += qtdLinhas;
    }

    // Cria os processos filhos via fork()
    std::vector<pid_t> pids(P);

    for (int p = 0; p < P; p++) {
        pid_t pid = fork();

        if (pid < 0) {
            std::cerr << "Erro ao criar processo " << p << std::endl;
            return 1;
        }

        if (pid == 0) {
            int linhaInicio = inicios[p];
            int linhaFim    = fins[p];

            std::vector<std::vector<double>> C(n1, std::vector<double>(m2, 0.0));

            auto inicio = std::chrono::high_resolution_clock::now();

            for (int i = linhaInicio; i < linhaFim; i++) {
                for (int j = 0; j < m2; j++) {
                    C[i][j] = 0.0;
                    for (int k = 0; k < m1; k++) {
                        C[i][j] += M1[i][k] * M2[k][j];
                    }
                }
            }

            auto fim = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> duracao = fim - inicio;
            double tempoSegundos = duracao.count();

            std::string arquivoSaida = base + "_" + std::to_string(p) + ".txt";
            salvarParcial(arquivoSaida, C, linhaInicio, linhaFim, tempoSegundos);

            exit(0);
        }

        pids[p] = pid;
    }

    for (int p = 0; p < P; p++) {
        int status;
        waitpid(pids[p], &status, 0);
        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
            std::cerr << "Processo " << p << " terminou com erro." << std::endl;
        }
    }

    double tempoTotal = 0.0;
    std::cout << "Multiplicacao paralela (processos) concluida!" << std::endl;
    std::cout << "Dimensoes: (" << n1 << "x" << m1 << ") x ("
              << n2 << "x" << m2 << ") = (" << n1 << "x" << m2 << ")" << std::endl;
    std::cout << "Numero de processos: " << P << std::endl;

    for (int p = 0; p < P; p++) {
        std::string arquivoSaida = base + "_" + std::to_string(p) + ".txt";
        std::ifstream arq(arquivoSaida);
        if (arq.is_open()) {
            int nl, nc;
            arq >> nl >> nc;
            double val;
            for (int i = 0; i < nl * nc; i++) arq >> val;
            double tempo;
            arq >> tempo;
            arq.close();
            if (tempo > tempoTotal) tempoTotal = tempo;
            std::cout << "  Processo " << p << ": linhas ["
                      << inicios[p] << ", " << fins[p] << ")"
                      << "  tempo=" << tempo << "s"
                      << "  arquivo=" << arquivoSaida << std::endl;
        }
    }

    std::cout << "Tempo total (maior entre processos): " << tempoTotal << " segundos" << std::endl;

    return 0;
}
