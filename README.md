# Trabalho 2 de Estrutura de Dados

O objetivo principal do trabalho foi comparar na prática o desempenho de duas técnicas para resolver colisões em tabelas hash: hash simples e hash duplo.

## Como Executar

1. Compilar o Código 

    `gcc -Wall -pg hash.c -o hash`

2. Executar o Programa

    `./hash.exe`

3. Gerar o Relatório

    Após executar o programa, um arquivo `gmon.out` será gerado na pasta. É nele que estão os dados da execução, e para transformar em um relatório txt deve-se executar:

    `gprof hash.exe gmon.out > relatorio.txt`

    Com isso, será gerado o relatório final com as tabelas de tempo de execução para cada função do código `hash.c`