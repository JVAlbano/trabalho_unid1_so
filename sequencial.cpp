// sequencial.cpp
// Uso: ./sequencial matrix1.txt matrix2.txt resultado.txt
// Multiplica M1 x M2 de forma sequencial e salva o resultado com o tempo de execução

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <chrono>
#include <stdexcept>

// Lê uma matriz de um arquivo
std::vector<std::vector<double>> lerMatriz(const std::string& nomeArquivo, int& linhas, int& colunas) {
    std::ifstream arquivo(nomeArquivo);
    if (!arquivo.is_open()) {
        throw std::runtime_error("Erro ao abrir o arquivo: " + nomeArquivo);
    }

    arquivo >> linhas >> colunas;

    std::vector<std::vector<double>> matriz(linhas, std::vector<double>(colunas));
    for (int i = 0; i < linhas; i++)
        for (int j = 0; j < colunas; j++)
            arquivo >> matriz[i][j];

    arquivo.close();
    return matriz;
}

// Salva a matriz resultado em arquivo, incluindo o tempo de execução
void salvarResultado(const std::string& nomeArquivo,
                     const std::vector<std::vector<double>>& matriz,
                     double tempoSegundos) {
    std::ofstream arquivo(nomeArquivo);
    if (!arquivo.is_open()) {
        throw std::runtime_error("Erro ao criar o arquivo: " + nomeArquivo);
    }

    int n = matriz.size();
    int m = matriz[0].size();

    arquivo << n << " " << m << "\n";
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < m; j++) {
            arquivo << matriz[i][j];
            if (j < m - 1) arquivo << " ";
        }
        arquivo << "\n";
    }
    arquivo << tempoSegundos << "\n";

    arquivo.close();
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Uso: " << argv[0] << " matrix1.txt matrix2.txt resultado.txt" << std::endl;
        return 1;
    }

    std::string arq1 = argv[1];
    std::string arq2 = argv[2];
    std::string arqResultado = argv[3];

    int n1, m1, n2, m2;

    // Lê as matrizes dos arquivos
    auto M1 = lerMatriz(arq1, n1, m1);
    auto M2 = lerMatriz(arq2, n2, m2);

    if (m1 != n2) {
        std::cerr << "Erro: dimensoes incompativeis para multiplicacao! "
                  << "m1=" << m1 << " deve ser igual a n2=" << n2 << std::endl;
        return 1;
    }

    // Aloca a matriz resultado
    std::vector<std::vector<double>> C(n1, std::vector<double>(m2, 0.0));

    // Inicia contagem do tempo
    auto inicio = std::chrono::high_resolution_clock::now();

    // Multiplicação sequencial: C[i][j] = sum(M1[i][k] * M2[k][j])
    for (int i = 0; i < n1; i++) {
        for (int j = 0; j < m2; j++) {
            for (int k = 0; k < m1; k++) {
                C[i][j] += M1[i][k] * M2[k][j];
            }
        }
    }

    auto fim = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duracao = fim - inicio;
    double tempoSegundos = duracao.count();

    salvarResultado(arqResultado, C, tempoSegundos);

    std::cout << "Multiplicacao sequencial concluida!" << std::endl;
    std::cout << "Dimensoes: (" << n1 << "x" << m1 << ") x (" << n2 << "x" << m2
              << ") = (" << n1 << "x" << m2 << ")" << std::endl;
    std::cout << "Tempo de execucao: " << tempoSegundos << " segundos" << std::endl;
    std::cout << "Resultado salvo em: " << arqResultado << std::endl;

    return 0;
}
