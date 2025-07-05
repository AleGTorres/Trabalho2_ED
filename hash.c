#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h> 

#define SEED 0x12345678
#define ARQUIVO_CEPS "Lista_de_CEPs.csv"
#define TOTAL_CEPS_ARQUIVO 734914
#define TAXA_REDIMENSIONAMENTO 0.85f   

typedef enum {
    HASH_SIMPLES, 
    HASH_DUPLO    
} TipoHash;

typedef struct {
    char cep[10];
    char cidade[100];
    char estado[5];
    char chave_cep[6]; 
} tcep;

typedef struct {
    uintptr_t *table;
    int size;
    int max;
    uintptr_t deleted;
    char *(*get_key)(void *);
    TipoHash tipo;
    float taxa_ocupacao_max;
} thash;

// funções de hash

uint32_t hashf(const char *str, uint32_t h) {
    for (; *str; ++str) {
        h ^= *str;
        h *= 0x5bd1e995;
        h ^= h >> 15;
    }
    return h;
}

uint32_t hashf2(const char *str, uint32_t h) {
    for (; *str; ++str) {
        h = (h * 31) + *str;
    }
    return (h % 97) + 1;
}

// funções para manipulação da tabela hash

int hash_insere(thash *h, void *bucket);

void hash_redimensiona(thash *h) {
    printf("-> Redimensionando tabela de %d para %d buckets...\n", h->max, h->max * 2);
    int max_antigo = h->max;
    uintptr_t *table_antiga = h->table;

    int novo_max = h->max * 2;
    h->table = calloc(sizeof(uintptr_t), novo_max);
    if (h->table == NULL) {
        printf("ERRO: Falha ao alocar memória para redimensionamento!\n");
        h->table = table_antiga;
        return;
    }

    h->max = novo_max;
    h->size = 0;

    for (int i = 0; i < max_antigo; i++) {
        if (table_antiga[i] != 0 && table_antiga[i] != h->deleted) {
            hash_insere(h, (void *)table_antiga[i]);
        }
    }
    free(table_antiga);
}

int hash_insere(thash *h, void *bucket) {
    if (h->taxa_ocupacao_max > 0) {
        float taxa_atual = (float)h->size / h->max;
        if (taxa_atual >= h->taxa_ocupacao_max) {
            hash_redimensiona(h);
        }
    }
    
    if (h->size >= h->max -1) {
        printf("ERRO: Tabela HASH CHEIA!\n");
        return EXIT_FAILURE;
    }

    char *key = h->get_key(bucket);
    uint32_t hash = hashf(key, SEED);
    int pos = hash % (h->max);
    int passo = (h->tipo == HASH_DUPLO) ? hashf2(key, SEED) : 1;

    while (h->table[pos] != 0) {
        if (h->table[pos] == h->deleted) break;
        pos = (pos + passo) % h->max;
    }

    h->table[pos] = (uintptr_t)bucket;
    h->size++;
    return EXIT_SUCCESS;
}

void *hash_busca(thash h, const char *key) {
    uint32_t hash = hashf(key, SEED);
    int pos = hash % (h.max);
    int passo = (h.tipo == HASH_DUPLO) ? hashf2(key, SEED) : 1;
    int pos_inicial = pos;

    while (h.table[pos] != 0) {
        if (h.table[pos] != h.deleted && strcmp(h.get_key((void *)h.table[pos]), key) == 0) {
            return (void *)h.table[pos];
        }
        pos = (pos + passo) % h.max;
        if (pos == pos_inicial) break;
    }
    return NULL;
}

void hash_apaga(thash *h) {
    for (int i = 0; i < h->max; i++) {
        if (h->table[i] != 0 && h->table[i] != h->deleted) {
            free((void *)h->table[i]);
        }
    }
    free(h->table);
}

int hash_constroi(thash *h, int nbuckets, char *(*get_key)(void *), TipoHash tipo, float taxa_max) {
    h->table = calloc(sizeof(uintptr_t), nbuckets + 1);
    if (h->table == NULL) return EXIT_FAILURE;

    h->max = nbuckets + 1;
    h->size = 0;
    h->deleted = (uintptr_t)&(h->size);
    h->get_key = get_key;
    h->tipo = tipo;
    h->taxa_ocupacao_max = taxa_max;
    return EXIT_SUCCESS;
}

char *get_key_cep(void *reg) {
    tcep *cep_data = (tcep *)reg;
    strncpy(cep_data->chave_cep, cep_data->cep, 5);
    cep_data->chave_cep[5] = '\0';
    return cep_data->chave_cep;
}

tcep *aloca_cep(const char *cep_str, const char *cidade_str, const char *estado_str) {
    tcep *novo_cep = malloc(sizeof(tcep));
    if (novo_cep) {
        strcpy(novo_cep->cep, cep_str);
        strcpy(novo_cep->cidade, cidade_str);
        strcpy(novo_cep->estado, estado_str);
    }
    return novo_cep;
}

char* trim_quotes(char* str) {
    if (str == NULL || *str != '"') {
        return str;
    }
    char* end = str + strlen(str) - 1;
    *end = '\0';
    return str + 1;
}

int carregar_ceps(const char *nome_arquivo, tcep *ceps_vetor[]) {
    FILE *f = fopen(nome_arquivo, "r");
    if (!f) {
        perror("Nao foi possivel abrir o arquivo de CEPs");
        return 0;
    }

    char linha[512];
    int count = 0;

    fgets(linha, sizeof(linha), f);

    while (fgets(linha, sizeof(linha), f) && count < TOTAL_CEPS_ARQUIVO) {
        linha[strcspn(linha, "\r\n")] = 0;
        
        char* estado_str = strtok(linha, ",");
        char* cidade_str = strtok(NULL, ",");
        strtok(NULL, ",");
        char* cep_str = strtok(NULL, ",");
        
        if (estado_str && cidade_str && cep_str) {
            char *estado_final = trim_quotes(estado_str);
            char *cidade_final = trim_quotes(cidade_str);
            char *cep_final = trim_quotes(cep_str);

            ceps_vetor[count] = aloca_cep(cep_final, cidade_final, estado_final);
            if (ceps_vetor[count]) {
                count++;
            }
        }
    }

    fclose(f);
    return count;
}

tcep *g_ceps_carregados[TOTAL_CEPS_ARQUIVO];
int g_total_ceps_lidos = 0;

void experimento_busca(float taxa, TipoHash tipo) {
    thash h;
    int n_buckets = 6100;
    hash_constroi(&h, n_buckets, get_key_cep, tipo, 0.0f);

    int n_inserir = (int)(n_buckets * taxa);
    if (n_inserir > g_total_ceps_lidos) n_inserir = g_total_ceps_lidos;

    for (int i = 0; i < n_inserir; i++) {
        hash_insere(&h, g_ceps_carregados[i]);
    }

    for (int i = 0; i < 5000000; i++) {
        const char *chave = get_key_cep(g_ceps_carregados[i % n_inserir]);
        hash_busca(h, chave);
    }
    
    free(h.table);
}

void busca10_simples() { experimento_busca(0.10f, HASH_SIMPLES); }
void busca20_simples() { experimento_busca(0.20f, HASH_SIMPLES); }
void busca30_simples() { experimento_busca(0.30f, HASH_SIMPLES); }
void busca40_simples() { experimento_busca(0.40f, HASH_SIMPLES); }
void busca50_simples() { experimento_busca(0.50f, HASH_SIMPLES); }
void busca60_simples() { experimento_busca(0.60f, HASH_SIMPLES); }
void busca70_simples() { experimento_busca(0.70f, HASH_SIMPLES); }
void busca80_simples() { experimento_busca(0.80f, HASH_SIMPLES); }
void busca90_simples() { experimento_busca(0.90f, HASH_SIMPLES); }
void busca99_simples() { experimento_busca(0.99f, HASH_SIMPLES); }

void busca10_duplo() { experimento_busca(0.10f, HASH_DUPLO); }
void busca20_duplo() { experimento_busca(0.20f, HASH_DUPLO); }
void busca30_duplo() { experimento_busca(0.30f, HASH_DUPLO); }
void busca40_duplo() { experimento_busca(0.40f, HASH_DUPLO); }
void busca50_duplo() { experimento_busca(0.50f, HASH_DUPLO); }
void busca60_duplo() { experimento_busca(0.60f, HASH_DUPLO); }
void busca70_duplo() { experimento_busca(0.70f, HASH_DUPLO); }
void busca80_duplo() { experimento_busca(0.80f, HASH_DUPLO); }
void busca90_duplo() { experimento_busca(0.90f, HASH_DUPLO); }
void busca99_duplo() { experimento_busca(0.99f, HASH_DUPLO); }

void insere_todos(int buckets_iniciais, TipoHash tipo) {
    thash h;
    tcep *copia_ceps[g_total_ceps_lidos];
    for (int i=0; i < g_total_ceps_lidos; i++) {
        copia_ceps[i] = aloca_cep(g_ceps_carregados[i]->cep, g_ceps_carregados[i]->cidade, g_ceps_carregados[i]->estado);
    }
    
    hash_constroi(&h, buckets_iniciais, get_key_cep, tipo, TAXA_REDIMENSIONAMENTO);
    for (int i = 0; i < g_total_ceps_lidos; i++) {
        hash_insere(&h, copia_ceps[i]);
    }
    hash_apaga(&h);
}

void insere6100() { insere_todos(6100, HASH_DUPLO); }
void insere1000() { insere_todos(1000, HASH_DUPLO); }

int main() {
    printf("Carregando base de dados de CEPs do arquivo '%s'...\n", ARQUIVO_CEPS);
    g_total_ceps_lidos = carregar_ceps(ARQUIVO_CEPS, g_ceps_carregados);

    if (g_total_ceps_lidos == 0) {
        printf("\nERRO: Nenhum CEP foi carregado. Verifique se o arquivo '%s' esta na pasta correta.\n", ARQUIVO_CEPS);
        return 1;
    }
    printf("%d registros de CEP carregados com sucesso.\n\n", g_total_ceps_lidos);

    printf("--- Demonstracao da Busca (Item 3) ---\n");
    thash h_demo;
    hash_constroi(&h_demo, 100, get_key_cep, HASH_DUPLO, 0.9f);
    for (int i=0; i < 50; i++) {
        tcep *copia = aloca_cep(g_ceps_carregados[i]->cep, g_ceps_carregados[i]->cidade, g_ceps_carregados[i]->estado);
        hash_insere(&h_demo, copia);
    }
    
    char* chave_exemplo = get_key_cep(g_ceps_carregados[10]);
    tcep* resultado = hash_busca(h_demo, chave_exemplo);
    if(resultado){
        printf("Busca por '%s': Encontrado -> Cidade: %s, Estado: %s\n", chave_exemplo, resultado->cidade, resultado->estado);
    }
    hash_apaga(&h_demo);
    printf("--------------------------------------\n\n");
    
    printf("Iniciando experimentos para gprof... Isso pode demorar MUITO.\n");

    printf("Executando Teste 4.1 (Busca x Ocupacao) para HASH SIMPLES...\n");
    busca10_simples(); busca20_simples(); busca30_simples(); busca40_simples();
    busca50_simples(); busca60_simples(); busca70_simples(); busca80_simples();
    busca90_simples(); busca99_simples();
    printf("... concluido.\n");

    printf("Executando Teste 4.1 (Busca x Ocupacao) para HASH DUPLO...\n");
    busca10_duplo(); busca20_duplo(); busca30_duplo(); busca40_duplo();
    busca50_duplo(); busca60_duplo(); busca70_duplo(); busca80_duplo();
    busca90_duplo(); busca99_duplo();
    printf("... concluido.\n");

    printf("Executando Teste 4.2 (Overhead de Insercao)...\n");
    insere_todos(6100, HASH_SIMPLES);
    insere_todos(1000, HASH_SIMPLES);
    printf("... concluido.\n");

    printf("\nTodos os experimentos foram executados.\n");
    printf("Compile com '-pg' e use 'gprof' para analisar o resultado (gmon.out).\n");

    for (int i = 0; i < g_total_ceps_lidos; i++) {
        free(g_ceps_carregados[i]);
    }

    return 0;
}