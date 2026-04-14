// auxiliar.cpp
// Uso: ./auxiliar n1 m1 n2 m2
// Gera duas matrizes aleatórias M1 (n1 x m1) e M2 (n2 x m2)
// e salva em matrix1.txt e matrix2.txt

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <ctime>
#include <string>

void gerarMatriz(const std::string& nomeArquivo, int linhas, int colunas) {
    std::ofstream arquivo(nomeArquivo);
    if (!arquivo.is_open()) {
        std::cerr << "Erro ao abrir o arquivo: " << nomeArquivo << std::endl;
        exit(1);
    }

    arquivo << linhas << " " << colunas << "\n";

    for (int i = 0; i < linhas; i++) {
        for (int j = 0; j < colunas; j++) {
            arquivo << (rand() % 10 + 1);
            if (j < colunas - 1) arquivo << " ";
        }
        arquivo << "\n";
    }

    arquivo.close();
    std::cout << "Matriz " << linhas << "x" << colunas
              << " salva em " << nomeArquivo << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc != 5) {
        std::cerr << "Uso: " << argv[0] << " n1 m1 n2 m2" << std::endl;
        std::cerr << "  n1 m1: dimensoes da matriz M1" << std::endl;
        std::cerr << "  n2 m2: dimensoes da matriz M2" << std::endl;
        return 1;
    }

    int n1 = atoi(argv[1]);
    int m1 = atoi(argv[2]);
    int n2 = atoi(argv[3]);
    int m2 = atoi(argv[4]);

    if (m1 != n2) {
        std::cerr << "Erro: m1 deve ser igual a n2 para multiplicacao de matrizes!"
                  << std::endl;
        std::cerr << "  m1 = " << m1 << ", n2 = " << n2 << std::endl;
        return 1;
    }

    srand(time(nullptr));

    gerarMatriz("matrix1.txt", n1, m1);
    gerarMatriz("matrix2.txt", n2, m2);

    std::cout << "Matrizes geradas com sucesso!" << std::endl;
    return 0;
}
