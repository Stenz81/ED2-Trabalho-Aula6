#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#define MAX_INSERE 80
#define MAX_BUSCA_P 40
#define MAX_BUSCA_S 50

struct Registro
{
    char id_aluno[4];
    char sigla_disciplina[4];
    char nome_aluno[50];
    char nome_disciplina[50];
    float media;
    float frequencia;
} insere[MAX_INSERE];

struct busca_p
{
    char id_aluno[4];
    char sigla_disc[4];
} vet_bp[MAX_BUSCA_P];

char vet_bs[MAX_BUSCA_S][50];

struct cabecalho
{
    int insere_utilizados = 0;
    int buscap_utilizados = 0;
    int buscas_utilizados = 0;
    int quantidade_nomes = 0;
} header;

struct cabecalho_busca
{
    int registro_alterado; // Dirá se determinado arquivo foi alterado desde o ultimo carregamento para memoria (0 se não, 1 se sim)
} hdr_bp, hdr_bs_nomes, hdr_bs_chaves;

struct busca_p2
{
    char id_aluno[4];
    char sigla_disc[4];
    int offset;
} indices_p[MAX_INSERE];

struct busca_s_nomes
{
    char nome_aluno[50];
    int offset;
} indices_s_nomes[MAX_INSERE];

char indice_busca_s_chaves[12 * MAX_INSERE + 4];

// Função para verificar se um arquivo já existe
int arquivo_existe(const char *nome_arquivo)
{
    FILE *arquivo = fopen(nome_arquivo, "rb");
    if (arquivo)
    {
        fclose(arquivo);
        return 1; // Arquivo existe
    }
    return 0; // Arquivo não existe
}

// Função para calcular o tamanho do registro
int calcularTamanhoRegistro(const Registro &reg)
{
    int tamanho_nome_aluno = 0, tamanho_nome_disciplina = 0;
    for (int i = 0; i < 50; i++)
    {

        if (reg.nome_aluno[i] == '\0')
        { // Verifica se encontrou o espaço vazio (0x00)
            break;
        }
        tamanho_nome_aluno++;
    }

    for (int i = 0; i < 50; i++)
    {
        if (reg.nome_disciplina[i] == '\0')
        { // Verifica se encontrou o espaço vazio (0x00)
            break;
        }
        tamanho_nome_disciplina++;
    }

    int tamanho = sizeof(reg.id_aluno) + 1 +
                  sizeof(reg.sigla_disciplina) + 1 +
                  tamanho_nome_aluno * sizeof(char) + 1 + // +1 para o separador '#'
                  tamanho_nome_disciplina * sizeof(char) + 1 +
                  sizeof(reg.media) + 1 +
                  sizeof(reg.frequencia);
    return tamanho;
}

// Função para comparar chaves (id_aluno + sigla_disc)
int compararChaves(const struct busca_p2 *registro, const struct busca_p *busca)
{
    int cmp = strncmp(registro->id_aluno, busca->id_aluno, 4);
    if (cmp == 0)
    {
        cmp = strncmp(registro->sigla_disc, busca->sigla_disc, 4);
    }
    return cmp;
}

// Função para ler e imprimir um registro a partir de um offset
void lerRegistro(FILE *file, int offset)
{

    // Posiciona o ponteiro do arquivo no offset fornecido
    fseek(file, offset, SEEK_SET);

    // Lê o tamanho do registro (número inteiro no início)
    int tamanhoRegistro = 0;
    fread(&tamanhoRegistro, sizeof(int), 1, file);

    // Lê o id_aluno e o delimitador '#'
    char id_aluno[4] = {0};
    fread(id_aluno, sizeof(char), 4, file);
    fgetc(file);

    id_aluno[3] = '\0';

    // Lê a sigla_disciplina e o delimitador '#'
    char sigla_disciplina[4] = {0};
    fread(sigla_disciplina, sizeof(char), 4, file);
    fgetc(file);

    sigla_disciplina[3] = '\0';

    // Lê o nome_aluno até encontrar o delimitador '#'
    char nome_aluno[50] = {0};
    int i = 0;
    char c;
    while (fread(&c, sizeof(char), 1, file) == 1 && c != '#')
    {
        if (i < 50 - 1)
        {
            nome_aluno[i++] = c;
        }
    }
    nome_aluno[i] = '\0';

    // Lê o nome_disciplina até encontrar o delimitador '#'
    char nome_disciplina[50] = {0};
    i = 0;
    while (fread(&c, sizeof(char), 1, file) == 1 && c != '#')
    {
        if (i < 50 - 1)
        {
            nome_disciplina[i++] = c;
        }
    }
    nome_disciplina[i] = '\0';

    // Lê a média e o delimitador '#'
    float media = 0.0;
    fread(&media, sizeof(float), 1, file);
    fgetc(file);

    // Lê a frequência
    float frequencia = 0.0;
    fread(&frequencia, sizeof(float), 1, file);

    // Exibe os dados lidos
    printf("ID Aluno: %s\n", id_aluno);
    printf("Sigla Disciplina: %s\n", sigla_disciplina);
    printf("Nome Aluno: %s\n", nome_aluno);
    printf("Nome Disciplina: %s\n", nome_disciplina);
    printf("Media: %.2f\n", media);
    printf("Frequencia: %.2f\n\n", frequencia);
    printf("===============================================================\n\n");

    return;
}

void inserirOrdenado(struct busca_p2 indices_p[], struct busca_p2 novoRegistro, int numRegistros)
{
    // Encontrar a posição onde o novo registro deve ser inserido
    int i = numRegistros - 1;
    while (i >= 0 && (strcmp(indices_p[i].id_aluno, novoRegistro.id_aluno) > 0 ||
                      (strcmp(indices_p[i].id_aluno, novoRegistro.id_aluno) == 0 && strcmp(indices_p[i].sigla_disc, novoRegistro.sigla_disc) > 0)))
    {
        // Deslocar o registro para a direita
        indices_p[i + 1] = indices_p[i];
        --i;
    }

    // Inserir o novo registro na posição correta
    indices_p[i + 1] = novoRegistro;

    return;
}

void inserirOrdenadoPorNome(struct busca_s_nomes indices_s_nomes[], struct busca_s_nomes novo_nome, int numRegistros)
{

    // Encontrar a posição onde o novo registro deve ser inserido
    int i = numRegistros - 1;
    while (i >= 0 && strcmp(indices_s_nomes[i].nome_aluno, novo_nome.nome_aluno) > 0)
    {
        // Deslocar o registro para a direita
        strcpy(indices_s_nomes[i + 1].nome_aluno, indices_s_nomes[i].nome_aluno);
        indices_s_nomes[i + 1].offset = indices_s_nomes[i].offset;
        --i;
    }

    // Inserir o novo registro na posição correta
    strcpy(indices_s_nomes[i + 1].nome_aluno, novo_nome.nome_aluno);
    indices_s_nomes[i + 1].offset = novo_nome.offset;

    return;
}

void insereRegistro(FILE *file, struct busca_p2 *indices_p, FILE *file_bp, struct busca_s_nomes *indices_s_nomes, FILE *file_bs_nomes, char indice_busca_s_chaves[], FILE *file_bs_chaves, const Registro &reg)
{
    int tamanho_registro = calcularTamanhoRegistro(reg);
    int inicio_registro;
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Vai para o para o final do arquivo principal e escreve o registro
    fseek(file, 0, SEEK_END);
    inicio_registro = ftell(file);

    // Escreve o tamanho do registro
    fwrite(&tamanho_registro, sizeof(int), 1, file);

    // Escreve o conteúdo do registro
    fwrite(reg.id_aluno, sizeof(reg.id_aluno), 1, file);
    fputc('#', file);
    fwrite(reg.sigla_disciplina, sizeof(reg.sigla_disciplina), 1, file);
    fputc('#', file);

    // Insere nome_aluno caracter por caracter até encontrar 0x00
    for (int i = 0; i < 50; i++)
    {
        if (reg.nome_aluno[i] == '\0')
        {          // Verifica se encontrou o espaço vazio (0x00)
            break; // Pula para o próximo campo
        }
        fputc(reg.nome_aluno[i], file); // Escreve o caractere no arquivo
    }
    fputc('#', file);
    // Insere nome_disciplina caracter por caracter até encontrar 0x00
    for (int i = 0; i < 50; i++)
    {
        if (reg.nome_disciplina[i] == '\0')
        {          // Verifica se encontrou o espaço vazio (0x00)
            break; // Pula para o próximo campo
        }
        fputc(reg.nome_disciplina[i], file); // Escreve o caractere no arquivo
    }
    fputc('#', file);
    fwrite(&reg.media, sizeof(float), 1, file);
    fputc('#', file);
    fwrite(&reg.frequencia, sizeof(float), 1, file);

    // Lê o cabeçalho no arquivo
    struct cabecalho header;
    fseek(file, 0, SEEK_SET);
    fread(&header, sizeof(struct cabecalho), 1, file);

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    struct busca_p2 nova_chave;

    strcpy(nova_chave.id_aluno, reg.id_aluno);           // Copia o id_aluno
    strcpy(nova_chave.sigla_disc, reg.sigla_disciplina); // Copia a sigla_disc
    nova_chave.offset = inicio_registro;

    inserirOrdenado(indices_p, nova_chave, header.insere_utilizados);

    /*strcpy(indices_p[header.insere_utilizados].id_aluno, reg.id_aluno);   // Copia o id_aluno
    strcpy(indices_p[header.insere_utilizados].sigla_disc, reg.sigla_disciplina);  // Copia a sigla_disc
    indices_p[header.insere_utilizados].offset = inicio_registro;*/

    // Atualiza o cabeçalho do arquivo de chaves primarias
    struct cabecalho_busca hdr_bp;
    hdr_bp.registro_alterado = 1;                                // indica que o arquivo está desatualizado
    fseek(file_bp, 0, SEEK_SET);                                 // Vai para o inicio do arquivo
    fwrite(&hdr_bp, sizeof(struct cabecalho_busca), 1, file_bp); // Atualiza o header

    /*//Insere a chave primaria do novo registro no final do arquivo
    fseek(file_bp, 0, SEEK_END);
    fwrite(&reg.id_aluno, sizeof(reg.id_aluno), 1, file_bp);
    fwrite(&reg.sigla_disciplina, sizeof(reg.sigla_disciplina), 1, file_bp);
    fwrite(&inicio_registro, sizeof(inicio_registro), 1, file_bp);*/

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    struct busca_s_nomes nome_atual;
    int novo_offset_inicio_lista;
    bool nome_encontrado = false;

    for (int i = 0; i < header.quantidade_nomes; i++) // Passa por todos os nomes já presentes no array
    {
        if (strcmp(indices_s_nomes[i].nome_aluno, reg.nome_aluno) == 0) // Verifica se o nome do registro corresponde ao do arquivo
        {
            // Caso os nomes sejam iguais:
            printf("Nome encontrado. Atualizando...\n");

            // Após o cabeçalho, cada struct 'busca_p2' ocupa 12 bytes (4 bytes + 4 bytes + 4 bytes)
            novo_offset_inicio_lista = sizeof(cabecalho_busca) + (header.insere_utilizados * sizeof(busca_p2));

            // Primeiro, copia o 'id_aluno' (4 bytes)
            memcpy(&indice_busca_s_chaves[novo_offset_inicio_lista], reg.id_aluno, sizeof(reg.id_aluno));

            // Depois, copia a 'sigla_disc' (4 bytes), logo após o 'id_aluno'
            memcpy(&indice_busca_s_chaves[novo_offset_inicio_lista + 4], reg.sigla_disciplina, sizeof(reg.sigla_disciplina));

            // Copia o 'offset' (4 bytes), logo após a 'sigla_disc'
            memcpy(&indice_busca_s_chaves[novo_offset_inicio_lista + 8], &indices_s_nomes[i].offset, sizeof(indices_s_nomes[i].offset));

            // Atualiza o offset para o inicio da lista no indice de nomes
            indices_s_nomes[i].offset = novo_offset_inicio_lista;

            nome_encontrado = true;
            break;
        }
    }
    if (nome_encontrado == false)
    {
        struct busca_s_nomes novo_nome;
        strcpy(novo_nome.nome_aluno, reg.nome_aluno);

        // Após o cabeçalho, cada struct 'busca_p2' ocupa 12 bytes (4 bytes + 4 bytes + 4 bytes)
        novo_offset_inicio_lista = sizeof(cabecalho_busca) + (header.insere_utilizados * sizeof(busca_p2));

        // Primeiro, copia o 'id_aluno' (4 bytes)
        memcpy(&indice_busca_s_chaves[novo_offset_inicio_lista], reg.id_aluno, sizeof(reg.id_aluno));

        // Depois, copia a 'sigla_disc' (4 bytes), logo após o 'id_aluno'
        memcpy(&indice_busca_s_chaves[novo_offset_inicio_lista + 4], reg.sigla_disciplina, sizeof(reg.sigla_disciplina));

        // Como esse é o primeiro item da lista, o offset deve ser -1
        int offset_inicio_lista = -1;
        memcpy(&indice_busca_s_chaves[novo_offset_inicio_lista + 8], &offset_inicio_lista, sizeof(offset_inicio_lista));

        // Define o offset para o nome do aluno como sendo o inicio da lista criada
        novo_nome.offset = novo_offset_inicio_lista;

        inserirOrdenadoPorNome(indices_s_nomes, novo_nome, header.quantidade_nomes);

        printf("Novo nome adicionado.\n");

        // Atualiza a quantidade de nomes diferentes
        header.quantidade_nomes++;
    }

    // Atualiza o cabeçalho do arquivo de nomes
    struct cabecalho_busca hdr_bs_nomes;
    hdr_bs_nomes.registro_alterado = 1;                                      // indica que o arquivo foi alterado
    fseek(file_bs_nomes, 0, SEEK_SET);                                       // Vai para o inicio do arquivo
    fwrite(&hdr_bs_nomes, sizeof(struct cabecalho_busca), 1, file_bs_nomes); // Atualiza o header

    // Atualiza o cabeçalho do arquivo com as listas de chaves
    struct cabecalho_busca hdr_bs_chaves;
    hdr_bs_chaves.registro_alterado = 1;                                       // indica que o arquivo foi alterado
    fseek(file_bs_chaves, 0, SEEK_SET);                                        // Vai para o inicio do arquivo
    fwrite(&hdr_bs_chaves, sizeof(struct cabecalho_busca), 1, file_bs_chaves); // Atualiza o header

    // Atualiza o cabeçalho do arquivo principal
    header.insere_utilizados++;
    fseek(file, 0, SEEK_SET);
    fwrite(&header, sizeof(struct cabecalho), 1, file);
    /*
        for (int i = 0; i < header.quantidade_nomes; i++)
        {
            printf("Nome %d: %s\n", i, &indices_s_nomes[i]);
        }
        printf("===========================================================================\n");
        for(int i = 0; i < header.insere_utilizados; i++){
            printf("Chave %d: ID %s, Sigla %s\n", i, &indices_p[i].id_aluno, &indices_p[i].sigla_disc);
        }
    */
}

// Função de busca primária
void buscarPrimaria(struct busca_p chave, struct busca_p2 indices_p[], FILE *fileDados)
{
    struct cabecalho header;
    fread(&header, sizeof(struct cabecalho), 1, fileDados);
    bool registro_encontrado = false;

    for (int i = 0; i < header.insere_utilizados; i++)
    {
        // Compara as chaves
        if (compararChaves(&indices_p[i], &chave) == 0)
        {

            // Se encontrou, vai para o offset no arquivo de dados
            printf("Registro encontrado para ID Aluno: %s, Disciplina: %s\n", chave.id_aluno, chave.sigla_disc);
            lerRegistro(fileDados, indices_p[i].offset);

            registro_encontrado = true;
            break;
        }
    }

    if (registro_encontrado == false)
    {
        printf("Registro nao encontrado para ID Aluno: %s, Disciplina: %s\n", chave.id_aluno, chave.sigla_disc);
    }
    fseek(fileDados, 0, SEEK_SET);
}

void buscarSecundaria(char nome_aluno[], struct busca_s_nomes indices_s_nomes[], char indice_busca_s_chaves[], FILE *fileDados)
{
    // faz a leitura do header para verificar a quantidade de nomes
    struct cabecalho header;
    fread(&header, sizeof(struct cabecalho), 1, fileDados);
    struct busca_p prox_buscap;
    int offset;
    bool nome_encontrado;
    // int posicao_inicial;
    printf("Nome a ser buscado: %s\n\n", nome_aluno);

    for (int i = 0; i < header.quantidade_nomes; i++)
    {

        if (strcmp(indices_s_nomes[i].nome_aluno, nome_aluno) == 0)
        {
            int posicao_inicial = indices_s_nomes[i].offset;
            while (posicao_inicial != -1)
            {
                memcpy(prox_buscap.id_aluno, &indice_busca_s_chaves[posicao_inicial], 4);
                memcpy(prox_buscap.sigla_disc, &indice_busca_s_chaves[posicao_inicial + 4], 4);
                memcpy(&offset, &indice_busca_s_chaves[posicao_inicial + 8], 4);

                // printf("Chave %d: ID %s, Sigla %s\n", i, &prox_buscap.id_aluno, &prox_buscap.sigla_disc);

                buscarPrimaria(prox_buscap, indices_p, fileDados);
                posicao_inicial = offset;
            }
            nome_encontrado = true;
        }
    }
    if (nome_encontrado == false)
    {
        printf("O nome %s não foi encontrado.\n", nome_aluno);
    }
}

// Escreve o vetor indices_p no arquivo de chaves primárias
void escreveIndicesP(struct busca_p2 indices_p[], struct cabecalho header, FILE *file_bp)
{
    // Atualiza o cabecalho para indicar que ele está atualizado
    struct cabecalho_busca hdr_bp;
    hdr_bp.registro_alterado = 0;
    fseek(file_bp, 0, SEEK_SET);
    fwrite(&hdr_bp, sizeof(struct cabecalho_busca), 1, file_bp);

    for (int i = 0; i < header.insere_utilizados; i++)
    {
        fwrite(&indices_p[i].id_aluno, sizeof(indices_p[i].id_aluno), 1, file_bp);
        fwrite(&indices_p[i].sigla_disc, sizeof(indices_p[i].sigla_disc), 1, file_bp);
        fwrite(&indices_p[i].offset, sizeof(indices_p[i].offset), 1, file_bp);
    }
}

// Le o vetor indices_p a partir do arquivo de chaves primárias
void leIndicesP(struct busca_p2 indices_p[], FILE *file_bp)
{
    int i = 0;
    struct busca_p2 chave_atual;
    fseek(file_bp, sizeof(struct cabecalho_busca), SEEK_SET);
    while (fread(&chave_atual, sizeof(struct busca_p2), 1, file_bp))
    {
        strcpy(indices_p[i].id_aluno, chave_atual.id_aluno);
        strcpy(indices_p[i].sigla_disc, chave_atual.sigla_disc);
        indices_p[i].offset = chave_atual.offset;
        i++;
    }
}

// Escreve o vetor indices_s_nomes no arquivo de nomes
void escreveNomesIndicesS(struct busca_s_nomes indices_s_nomes[], struct cabecalho header, FILE *file_bs_nomes)
{
    // Atualiza o cabecalho para indicar que ele está atualizado
    struct cabecalho_busca hdr_bs_nomes;
    hdr_bs_nomes.registro_alterado = 0;
    fseek(file_bs_nomes, 0, SEEK_SET);
    fwrite(&hdr_bs_nomes, sizeof(struct cabecalho_busca), 1, file_bs_nomes);

    for (int i = 0; i < header.quantidade_nomes; i++)
    {
        fwrite(&indices_s_nomes[i].nome_aluno, sizeof(indices_s_nomes[i].nome_aluno), 1, file_bs_nomes);
        fwrite(&indices_s_nomes[i].offset, sizeof(indices_s_nomes[i].offset), 1, file_bs_nomes);
    }
}

// Le o vetor indices_s_nomes a partir do arquivo de nomes
void leNomesIndicesS(struct busca_s_nomes indices_s_nomes[], FILE *file_bs_nomes)
{
    int i = 0;
    struct busca_s_nomes nome_atual;
    fseek(file_bs_nomes, sizeof(struct cabecalho_busca), SEEK_SET);
    while (fread(&nome_atual, sizeof(struct busca_s_nomes), 1, file_bs_nomes))
    {
        strcpy(indices_s_nomes[i].nome_aluno, nome_atual.nome_aluno);
        indices_s_nomes[i].offset = nome_atual.offset;
        i++;
    }
}

// Escreve o vetor indices_s_chaves no arquivo de lista invertida de chaves
void escreveChavesIndicesS(char indice_busca_s_chaves[], struct cabecalho header, FILE *file_bs_chaves)
{
    struct cabecalho_busca hdr_bs_chaves;
    fseek(file_bs_chaves, 0, SEEK_SET);
    fwrite(indice_busca_s_chaves, sizeof(struct cabecalho_busca) + (header.insere_utilizados * sizeof(struct busca_p2)), 1, file_bs_chaves);

    // Atualiza o cabecalho para indicar que ele está atualizado
    hdr_bs_chaves.registro_alterado = 0;
    fseek(file_bs_chaves, 0, SEEK_SET);
    fwrite(&hdr_bs_chaves, sizeof(struct cabecalho_busca), 1, file_bs_chaves);
}

// Le o vetor indices_s_chaves a partir do arquivo de lista invertida de chaves
void leChavesIndicesS(char indice_busca_s_chaves[], FILE *file_bs_chaves)
{
    int tamanho;
    fseek(file_bs_chaves, 0, SEEK_END);
    tamanho = ftell(file_bs_chaves);

    fread(&indice_busca_s_chaves, tamanho, 1, file_bs_chaves);
}

// Reconstroi os arquivos e recria os vetores a partir do arquivo principal
void reconstruirArquivos(FILE *file, FILE *file_bp, struct busca_p2 indices_p[], FILE *file_bs_nomes, struct busca_s_nomes indices_s_nomes[], FILE *file_bs_chaves, char indice_busca_s_chaves[])
{
    struct cabecalho header;

    fseek(file, 0, SEEK_SET);
    fread(&header, sizeof(struct cabecalho), 1, file);

    int inicio_registro = ftell(file);
    int tamanho_registro;
    int qtd_nomes;

    for (int i = 0; i < header.insere_utilizados; i++)
    {
        struct Registro registro_atual;

        fread(&tamanho_registro, sizeof(int), 1, file);
        fread(&registro_atual.id_aluno, sizeof(registro_atual.id_aluno), 1, file);
        fseek(file, ftell(file) + sizeof(char), SEEK_SET);
        fread(&registro_atual.sigla_disciplina, sizeof(registro_atual.sigla_disciplina), 1, file);
        fseek(file, ftell(file) + sizeof(char), SEEK_SET);
        // Lê o nome_aluno até encontrar o delimitador '#'
        char nome_aluno[50] = {0};
        int counter_nome = 0;
        char c;
        while (fread(&c, sizeof(char), 1, file) == 1 && c != '#')
        {
            if (i < 50 - 1)
            {
                registro_atual.nome_aluno[i++] = c;
            }
        }
        registro_atual.nome_aluno[i] = '\0';

        // Lê o nome_disciplina até encontrar o delimitador '#'
        i = 0;
        while (fread(&c, sizeof(char), 1, file) == 1 && c != '#')
        {
            if (i < 50 - 1)
            {
                registro_atual.nome_disciplina[i++] = c;
            }
        }
        registro_atual.nome_disciplina[i] = '\0';

        // Lê a média e o delimitador '#'
        fread(&registro_atual.media, sizeof(registro_atual.media), 1, file);
        fgetc(file);

        // Lê a frequência
        fread(&registro_atual.frequencia, sizeof(registro_atual.frequencia), 1, file);

        // Uma vez lido o registro, podemos prosseguir conforme necessário

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        // Atualiza o array de chaves primárias
        struct busca_p2 nova_chave;

        strcpy(nova_chave.id_aluno, registro_atual.id_aluno);           // Copia o id_aluno
        strcpy(nova_chave.sigla_disc, registro_atual.sigla_disciplina); // Copia a sigla_disc
        nova_chave.offset = inicio_registro;

        inserirOrdenado(indices_p, nova_chave, i);

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        struct busca_s_nomes nome_atual;
        int novo_offset_inicio_lista;
        bool nome_encontrado = false;

        for (int i = 0; i < qtd_nomes; i++) // Passa por todos os nomes já presentes no array
        {
            if (strcmp(indices_s_nomes[i].nome_aluno, registro_atual.nome_aluno) == 0) // Verifica se o nome do registro corresponde ao do arquivo
            {
                // Após o cabeçalho, cada struct 'busca_p2' ocupa 12 bytes (4 bytes + 4 bytes + 4 bytes)
                novo_offset_inicio_lista = sizeof(cabecalho_busca) + (header.insere_utilizados * sizeof(busca_p2));

                // Primeiro, copia o 'id_aluno' (4 bytes)
                memcpy(&indice_busca_s_chaves[novo_offset_inicio_lista], registro_atual.id_aluno, sizeof(registro_atual.id_aluno));

                // Depois, copia a 'sigla_disc' (4 bytes), logo após o 'id_aluno'
                memcpy(&indice_busca_s_chaves[novo_offset_inicio_lista + 4], registro_atual.sigla_disciplina, sizeof(registro_atual.sigla_disciplina));

                // Copia o 'offset' (4 bytes), logo após a 'sigla_disc'
                memcpy(&indice_busca_s_chaves[novo_offset_inicio_lista + 8], &indices_s_nomes[i].offset, sizeof(indices_s_nomes[i].offset));

                // Atualiza o offset para o inicio da lista no indice de nomes
                indices_s_nomes[i].offset = novo_offset_inicio_lista;

                nome_encontrado = true;
                break;
            }
        }
        if (nome_encontrado == false)
        {
            struct busca_s_nomes novo_nome;
            strcpy(novo_nome.nome_aluno, registro_atual.nome_aluno);

            // Após o cabeçalho, cada struct 'busca_p2' ocupa 12 bytes (4 bytes + 4 bytes + 4 bytes)
            novo_offset_inicio_lista = sizeof(cabecalho_busca) + (header.insere_utilizados * sizeof(busca_p2));

            // Primeiro, copia o 'id_aluno' (4 bytes)
            memcpy(&indice_busca_s_chaves[novo_offset_inicio_lista], registro_atual.id_aluno, sizeof(registro_atual.id_aluno));

            // Depois, copia a 'sigla_disc' (4 bytes), logo após o 'id_aluno'
            memcpy(&indice_busca_s_chaves[novo_offset_inicio_lista + 4], registro_atual.sigla_disciplina, sizeof(registro_atual.sigla_disciplina));

            // Como esse é o primeiro item da lista, o offset deve ser -1
            int offset_inicio_lista = -1;
            memcpy(&indice_busca_s_chaves[novo_offset_inicio_lista + 8], &offset_inicio_lista, sizeof(offset_inicio_lista));

            // Define o offset para o nome do aluno como sendo o inicio da lista criada
            novo_nome.offset = novo_offset_inicio_lista;

            inserirOrdenadoPorNome(indices_s_nomes, novo_nome, header.quantidade_nomes);

            // Atualiza a quantidade de nomes diferentes
            qtd_nomes++;
        }
    }

    escreveIndicesP(indices_p, header, file_bp);
    hdr_bp.registro_alterado = 0;
    fwrite(&hdr_bp, sizeof(struct cabecalho_busca), 0, file_bp);
    escreveNomesIndicesS(indices_s_nomes, header, file_bs_nomes);
    hdr_bs_nomes.registro_alterado = 0;
    fwrite(&hdr_bs_nomes, sizeof(struct cabecalho_busca), 0, file_bs_nomes);
    escreveChavesIndicesS(indice_busca_s_chaves, header, file_bs_chaves);
    hdr_bs_chaves.registro_alterado = 0;
    fwrite(&hdr_bs_chaves, sizeof(struct cabecalho_busca), 0, file_bs_chaves);
}

/*void insereRegistro (FILE *file, FILE *file_bp, FILE *file_bs_nomes, FILE *file_bs_chaves, const Registro &reg)
{
    int tamanho_registro = calcularTamanhoRegistro(reg);
    int inicio_registro;
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Vai para o para o final do arquivo principal e escreve o registro
    fseek(file, 0, SEEK_END);
    inicio_registro = ftell(file);

    // Escreve o tamanho do registro
    fwrite(&tamanho_registro, sizeof(int), 1, file);

    // Escreve o conteúdo do registro
    fwrite(reg.id_aluno, sizeof(reg.id_aluno), 1, file);
    fputc('#', file);
    fwrite(reg.sigla_disciplina, sizeof(reg.sigla_disciplina), 1, file);
    fputc('#', file);

    // Insere nome_aluno caracter por caracter até encontrar 0x00
    for (int i = 0; i < 50; i++)
    {
        if (reg.nome_aluno[i] == '\0')
        {          // Verifica se encontrou o espaço vazio (0x00)
            break; // Pula para o próximo campo
        }
        fputc(reg.nome_aluno[i], file); // Escreve o caractere no arquivo
    }
    fputc('#', file);
    // Insere nome_disciplina caracter por caracter até encontrar 0x00
    for (int i = 0; i < 50; i++)
    {
        if (reg.nome_disciplina[i] == '\0')
        {          // Verifica se encontrou o espaço vazio (0x00)
            break; // Pula para o próximo campo
        }
        fputc(reg.nome_disciplina[i], file); // Escreve o caractere no arquivo
    }
    fputc('#', file);
    fwrite(&reg.media, sizeof(float), 1, file);
    fputc('#', file);
    fwrite(&reg.frequencia, sizeof(float), 1, file);

    // Atualiza o cabeçalho no arquivo
    struct cabecalho header;
    fseek(file, 0, SEEK_SET);
    fread(&header, sizeof(struct cabecalho), 1, file);
    header.insere_utilizados++;
    fseek(file, 0, SEEK_SET);
    fwrite(&header, sizeof(struct cabecalho), 1, file);

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    //Insere a chave primaria do novo registro no final do arquivo
    fseek(file_bp, 0, SEEK_END);
    fwrite(&reg.id_aluno, sizeof(reg.id_aluno), 1, file_bp);
    fwrite(&reg.sigla_disciplina, sizeof(reg.sigla_disciplina), 1, file_bp);
    fwrite(&inicio_registro, sizeof(inicio_registro), 1, file_bp);

    //Atualiza o cabeçalho do arquivo de chaves primarias
    struct cabecalho_busca hdr_bp;
    hdr_bp.registro_alterado = 1; // indica que o arquivo foi alterado
    fseek(file_bp, 0, SEEK_SET); // Vai para o inicio do arquivo
    fwrite(&hdr_bp, sizeof(struct cabecalho_busca), 1, file_bp); //Atualiza o header

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    char nome_atual[50];
    int offset_inicio_lista = -1;
    int novo_offset_inicio_lista;
    bool nome_encontrado = false;

    fseek(file_bs_nomes, sizeof(cabecalho_busca), SEEK_SET);
    while(fread(&nome_atual, sizeof(nome_atual), 1, file_bs_nomes)) //Le o nome do aluno no arquivo
    {
        if(strcmp(nome_atual, reg.nome_aluno) == 0) //Verifica se o nome do registro corresponde ao do arquivo
        {
            //Se os nomes forem iguais:
            fread(&offset_inicio_lista, sizeof(int), 1, file_bs_nomes); // Lê a posição de inicio da lista para o nome atual
            fseek(file_bs_chaves, 0, SEEK_END); // Vai até o final do arquivo que contém as chaves
            novo_offset_inicio_lista = ftell(file_bs_chaves); //Grava o offset para o inicio do novo membro da lista encadeada
            fwrite(&reg.id_aluno, sizeof(reg.id_aluno), 1, file_bs_chaves); //Escreve a 1a parte da chave
            fwrite(&reg.sigla_disciplina, sizeof(reg.sigla_disciplina), 1, file_bs_chaves); //Escreve a 2a parte da chave
            fwrite(&offset_inicio_lista, sizeof(offset_inicio_lista), 1, file_bs_chaves); //Escreve o offset para o antigo ultimo membro da lista

            fseek(file_bs_nomes, ftell(file_bs_nomes) - sizeof(int), SEEK_SET); //Volta até o final do nome do aluno
            fwrite(&novo_offset_inicio_lista, sizeof(int), 1, file_bs_nomes); //Escreve o novo offset de inicio da lista endadeada

            nome_encontrado = true;
            break;
        }else
        {
            //Caso os nomes sejam diferentes, pula o offset da lista e lê o próximo nome
            fseek(file_bs_nomes, ftell(file_bs_nomes) + sizeof(int), SEEK_SET);
        }
    }
    if (nome_encontrado == false){
        //Caso nenhuma correspondencia tenha sido encontrada:

        fseek(file_bs_nomes, 0, SEEK_END); // Vai até o final do arquivo
        fwrite(&reg.nome_aluno, sizeof(reg.nome_aluno), 1, file_bs_nomes); //Escreve o nome do aluno

        fseek(file_bs_chaves, 0, SEEK_END); // Vai até o final do arquivo que contém as chaves
        novo_offset_inicio_lista = ftell(file_bs_chaves); //Grava o offset para o inicio da nova lista encadeada

        fwrite(&reg.id_aluno, sizeof(reg.id_aluno), 1, file_bs_chaves); //Escreve a 1a parte da chave
        fwrite(&reg.sigla_disciplina, sizeof(reg.sigla_disciplina), 1, file_bs_chaves); //Escreve a 2a parte da chave
        fwrite(&offset_inicio_lista, sizeof(offset_inicio_lista), 1, file_bs_chaves); //Por ser o 1o membro da lista, o offset escrito é -1

        fwrite(&novo_offset_inicio_lista, sizeof(int), 1, file_bs_nomes); //Escreve o offset de inicio da nova lista endadeada
    }

    //Atualiza o cabeçalho do arquivo de nomes
    struct cabecalho_busca hdr_bs_nomes;
    hdr_bs_nomes.registro_alterado = 1; // indica que o arquivo foi alterado
    fseek(file_bs_nomes, 0, SEEK_SET); // Vai para o inicio do arquivo
    fwrite(&hdr_bs_nomes, sizeof(struct cabecalho_busca), 1, file_bs_nomes); //Atualiza o header

    //Atualiza o cabeçalho do arquivo com as listas de chaves
    struct cabecalho_busca hdr_bs_chaves;
    hdr_bs_chaves.registro_alterado = 1; // indica que o arquivo foi alterado
    fseek(file_bs_chaves, 0, SEEK_SET); // Vai para o inicio do arquivo
    fwrite(&hdr_bs_chaves, sizeof(struct cabecalho_busca), 1, file_bs_chaves); //Atualiza o header

}*/

int main()
{
    FILE *file;
    // Leitura dos registros a serem inseridos:

    // Abre o arquivo binário para leitura
    file = fopen("insere.bin", "rb");
    if (file == NULL)
    {
        printf("Nao foi possivel abrir o arquivo.\n");
        return 1;
    }

    // Le os registros do arquivo e armazena no vetor
    fread(insere, sizeof(insere), 1, file);

    // Fecha o arquivo
    fclose(file);

    // Exibe os registros lidos para verificar se tudo foi lido corretamente
    for (int i = 0; i < MAX_INSERE; i++)
    {
        printf("ID Aluno: %s\n", insere[i].id_aluno);
        printf("Sigla Disciplina: %s\n", insere[i].sigla_disciplina);
        printf("Nome Aluno: %s\n", insere[i].nome_aluno);
        printf("Nome Disciplina: %s\n", insere[i].nome_disciplina);
        printf("Media: %.2f\n", insere[i].media);
        printf("Frequencia: %.2f\n\n", insere[i].frequencia);
    }

    // Leitura dos registros para busca primaria:

    // Abre o arquivo binário para leiura
    file = fopen("busca_p.bin", "rb");
    if (file == NULL)
    {
        printf("Nao foi possivel abrir o arquivo.\n");
        return 1;
    }

    // Le os registros do arquivo e armazena no vetor
    fread(vet_bp, sizeof(vet_bp), 1, file);

    // Fecha o arquivo
    fclose(file);

    // Exibe os registros lidos para verificar se tudo foi lido corretamente
    for (int i = 0; i < MAX_BUSCA_P; i++)
    {
        printf("ID Aluno: %s\n", vet_bp[i].id_aluno);
        printf("Sigla Disciplina: %s\n", vet_bp[i].sigla_disc);
        printf("\n");
    }

    // Leitura dos nomes para busca secundaria:

    // Abre o arquivo binário para leitura
    file = fopen("busca_s.bin", "rb");
    if (file == NULL)
    {
        printf("Nao foi possivel abrir o arquivo.\n");
        return 1;
    }

    // Le os nomes do arquivo e armazena no vetor
    fread(vet_bs, sizeof(vet_bs), 1, file);

    // Fecha o arquivo
    fclose(file);

    for (int i = 0; i < MAX_BUSCA_S; i++)
    {
        printf("Nome Aluno: %s\n", vet_bs[i]);
    }

    // Verifica se o arquivo "listaRegistros.bin" já existe, e em caso negativo, cria e realiza os procedimentos necessarios, como escrita do header
    if (arquivo_existe("listaRegistros.bin")) // se o arquivo já existe, abre o arquivo
    {
        file = fopen("listaRegistros.bin", "rb+");
        if (file == NULL)
        {
            printf("Nao foi possivel abrir o arquivo.\n");
            return 1;
        }
    }
    else // se nao existe, cria e inicia o cabeçalho
    {
        file = fopen("listaRegistros.bin", "wb+");
        if (file == NULL)
        {
            printf("Nao foi possivel abrir o arquivo.\n");
            return 1;
        }
        header.insere_utilizados = 0;
        header.buscap_utilizados = 0;
        header.buscas_utilizados = 0;

        fseek(file, 0, SEEK_SET);
        fwrite(&header, sizeof(struct cabecalho), 1, file);
    }

    // Verifica se o arquivo "indice_primario.bin" já existe, e em caso negativo, cria e realiza os procedimentos necessarios, como escrita do header
    FILE *file_bp;
    if (arquivo_existe("indice_primario.bin")) // se o arquivo já existe, abre o arquivo
    {
        file_bp = fopen("indice_primario.bin", "rb+");
        if (file_bp == NULL)
        {
            printf("Nao foi possivel abrir o arquivo.\n");
            return 1;
        }
    }
    else // se nao existe, cria e inicia o cabeçalho
    {
        file_bp = fopen("indice_primario.bin", "wb+");
        if (file_bp == NULL)
        {
            printf("Nao foi possivel abrir o arquivo.\n");
            return 1;
        }
        hdr_bp.registro_alterado = 0;

        fseek(file_bp, 0, SEEK_SET);
        fwrite(&hdr_bp, sizeof(struct cabecalho_busca), 1, file_bp);
    }

    // Verifica se o arquivo "indice_secundario_nomes.bin" já existe, e em caso negativo, cria e realiza os procedimentos necessarios, como escrita do header
    FILE *file_bs_nomes;
    if (arquivo_existe("indice_secundario_nomes.bin")) // se o arquivo já existe, abre o arquivo
    {
        file_bs_nomes = fopen("indice_secundario_nomes.bin", "rb+");
        if (file_bs_nomes == NULL)
        {
            printf("Nao foi possivel abrir o arquivo.\n");
            return 1;
        }
    }
    else // se nao existe, cria e inicia o cabeçalho
    {
        file_bs_nomes = fopen("indice_secundario_nomes.bin", "wb+");
        if (file_bs_nomes == NULL)
        {
            printf("Nao foi possivel abrir o arquivo.\n");
            return 1;
        }
        hdr_bp.registro_alterado = 0;

        fseek(file_bs_nomes, 0, SEEK_SET);
        fwrite(&hdr_bs_nomes, sizeof(struct cabecalho_busca), 1, file_bs_nomes);
    }

    // Verifica se o arquivo "indice_secundario_chaves.bin" já existe, e em caso negativo, cria e realiza os procedimentos necessarios, como escrita do header
    FILE *file_bs_chaves;
    if (arquivo_existe("indice_secundario_chaves.bin")) // se o arquivo já existe, abre o arquivo
    {
        file_bs_chaves = fopen("indice_secundario_chaves.bin", "rb+");
        if (file_bs_chaves == NULL)
        {
            printf("Nao foi possivel abrir o arquivo.\n");
            return 1;
        }
    }
    else // se nao existe, cria e inicia o cabeçalho
    {
        file_bs_chaves = fopen("indice_secundario_chaves.bin", "wb+");
        if (file_bs_chaves == NULL)
        {
            printf("Nao foi possivel abrir o arquivo.\n");
            return 1;
        }
        hdr_bp.registro_alterado = 0;

        fseek(file_bs_chaves, 0, SEEK_SET);
        fwrite(&hdr_bs_chaves, sizeof(struct cabecalho_busca), 1, file_bs_chaves);
    }

    fseek(file_bp, 0, SEEK_SET);
    fseek(file_bs_nomes, 0, SEEK_SET);
    fseek(file_bs_chaves, 0, SEEK_SET);

    fread(&hdr_bp, sizeof(struct cabecalho_busca), 1, file_bp);
    fread(&hdr_bs_nomes, sizeof(struct cabecalho_busca), 1, file_bs_nomes);
    fread(&hdr_bs_chaves, sizeof(struct cabecalho_busca), 1, file_bs_chaves);

    if(hdr_bp.registro_alterado == 1 || hdr_bs_nomes.registro_alterado == 1 || hdr_bs_chaves.registro_alterado == 1)
    {
        reconstruirArquivos(file, file_bp, indices_p, file_bs_chaves, indices_s_nomes, file_bs_chaves, indice_busca_s_chaves);
    }else{
        leIndicesP(indices_p, file_bp);
        leNomesIndicesS(indices_s_nomes, file_bs_nomes);
        leChavesIndicesS(indice_busca_s_chaves, file_bs_chaves);
    }

    int choice;

    do
    {
        printf("\nSelecione uma das seguintes opcoes:\n0-Sair;\n1-Inserir novo registro;\n2-Buscar registro por chave primaria;\n3-Buscar registro por chave secundaria;\nOpcao:");
        scanf("%i", &choice);

        switch (choice)
        {
        case 0:
            escreveIndicesP(indices_p, header, file_bp);
            hdr_bp.registro_alterado = 0;
            fwrite(&hdr_bp, sizeof(struct cabecalho_busca), 0, file_bp);
            escreveNomesIndicesS(indices_s_nomes, header, file_bs_nomes);
            hdr_bs_nomes.registro_alterado = 0;
            fwrite(&hdr_bs_nomes, sizeof(struct cabecalho_busca), 0, file_bs_nomes);
            escreveChavesIndicesS(indice_busca_s_chaves, header, file_bs_chaves);
            hdr_bs_chaves.registro_alterado = 0;
            fwrite(&hdr_bs_chaves, sizeof(struct cabecalho_busca), 0, file_bs_chaves);
            fclose(file);
            fclose(file_bp);
            fclose(file_bs_nomes);
            fclose(file_bs_chaves);

            break;

        case 1:
            fseek(file, 0, SEEK_SET);
            fread(&header, sizeof(struct cabecalho), 1, file);
            // Chame a função insereRegistro
            insereRegistro(file, indices_p, file_bp, indices_s_nomes, file_bs_nomes, indice_busca_s_chaves, file_bs_chaves, insere[header.insere_utilizados]); // Ajuste os parâmetros conforme necessário
            break;

        case 2: // Busca por chave primária
            fseek(file, 0, SEEK_SET);
            fread(&header, sizeof(struct cabecalho), 1, file);
            struct busca_p chavePrimaria;
            buscarPrimaria(vet_bp[header.buscap_utilizados], indices_p, file);
            header.buscap_utilizados++;
            fwrite(&header, sizeof(struct cabecalho), 1, file);
            break;

        case 3: // Busca por chave secundária
            fseek(file, 0, SEEK_SET);
            fread(&header, sizeof(struct cabecalho), 1, file);
            struct busca_p chaveSecundaria;
            buscarSecundaria(vet_bs[header.buscas_utilizados], indices_s_nomes, indice_busca_s_chaves, file);
            header.buscas_utilizados++;
            fwrite(&header, sizeof(struct cabecalho), 1, file);
            break;

        default:
            printf("Insira uma opcao valida.\n");
            break;
        }
    } while (choice != 0);

    return 0;
}
/*insereRegistro(file, indices_p, file_bp, indices_s_nomes, file_bs_nomes, indice_busca_s_chaves, file_bs_chaves, insere[0]);
insereRegistro(file, indices_p, file_bp, indices_s_nomes, file_bs_nomes, indice_busca_s_chaves, file_bs_chaves, insere[1]);
insereRegistro(file, indices_p, file_bp, indices_s_nomes, file_bs_nomes, indice_busca_s_chaves, file_bs_chaves, insere[2]);
insereRegistro(file, indices_p, file_bp, indices_s_nomes, file_bs_nomes, indice_busca_s_chaves, file_bs_chaves, insere[3]);
insereRegistro(file, indices_p, file_bp, indices_s_nomes, file_bs_nomes, indice_busca_s_chaves, file_bs_chaves, insere[4]);
insereRegistro(file, indices_p, file_bp, indices_s_nomes, file_bs_nomes, indice_busca_s_chaves, file_bs_chaves, insere[5]);
insereRegistro(file, indices_p, file_bp, indices_s_nomes, file_bs_nomes, indice_busca_s_chaves, file_bs_chaves, insere[6]);
insereRegistro(file, indices_p, file_bp, indices_s_nomes, file_bs_nomes, indice_busca_s_chaves, file_bs_chaves, insere[7]);

buscarPrimaria(vet_bp[0], indices_p, file);
buscarPrimaria(vet_bp[1], indices_p, file);

buscarSecundaria(vet_bs[0], indices_s_nomes, indice_busca_s_chaves, file);
buscarSecundaria(vet_bs[4], indices_s_nomes, indice_busca_s_chaves, file);

fseek(file, 0, SEEK_SET);
fread(&header, sizeof(struct cabecalho), 1, file);

for (int i = 0; i < header.quantidade_nomes; i++)
{
    printf("Nome %d: %s\n", i, &indices_s_nomes[i]);
}
printf("===========================================================================\n");
for (int i = 0; i < header.insere_utilizados; i++)
{
    printf("Chave %d: ID %s, Sigla %s\n", i, &indices_p[i].id_aluno, &indices_p[i].sigla_disc);
}

escreveIndicesP(indices_p, header, file_bp);
escreveNomesIndicesS(indices_s_nomes, header, file_bs_nomes);
escreveChavesIndicesS(indice_busca_s_chaves, header, file_bs_chaves);

fclose(file);
fclose(file_bp);
fclose(file_bs_nomes);
fclose(file_bs_chaves);*/
