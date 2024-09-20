#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#define MAX_INSERE 8
#define MAX_BUSCA_P 4
#define MAX_BUSCA_S 5

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
}header;

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


void insereRegistro (const Registro &reg)
{
    FILE *file;
    int tamanho_registro = calcularTamanhoRegistro(reg);
///////////////////////////////////////////////////////////////////////////////////////////////////
    file = fopen("listaRegistros.bin", "wb+");
    // Vai para o para o final do arquivo principal e escreve o registro
    fseek(file, 0, SEEK_END);

    // Escreve o tamanho do registro
    fwrite(&tamanho_registro, sizeof(int), 1, file);

    // Escreve o conteúdo do registro
    fwrite(reg.id_aluno, sizeof(reg.id_aluno), 1, file);
    fputc('#', file);
    fwrite(reg.sigla_disciplina, sizeof(reg.sigla_disciplina), 1, file);
    fputc('#', file);

    // Inserir nome_aluno caracter por caracter até encontrar 0x00
    for (int i = 0; i < 50; i++)
    {
        if (reg.nome_aluno[i] == '\0')
        {          // Verifica se encontrou o espaço vazio (0x00)
            break; // Pula para o próximo campo
        }
        fputc(reg.nome_aluno[i], file); // Escreve o caractere no arquivo
    }
    fputc('#', file);
    // Inserir nome_disciplina caracter por caracter até encontrar 0x00
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

    // Atualizar o cabeçalho no arquivo
    header.insere_utilizados++;
    fseek(file, 0, SEEK_SET);
    fwrite(&header, sizeof(struct cabecalho), 1, file);
    fclose(file);

    //////////////////////////////////////////////////////////////////////////////////////////////////////

    //Insere a chave primaria do novo registro no arquivo 
}

int main ()
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

    //Leitura dos registros para busca primaria:

    //Abre o arquivo binário para leiura
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

    //Leitura dos nomes para busca secundaria:

    //Abre o arquivo binário para leitura
    file = fopen("busca_s.bin", "rb");
    if (file == NULL)
    {
        printf("Nao foi possivel abrir o arquivo.\n");
        return 1;
    }

    //Le os nomes do arquivo e armazena no vetor
    fread(vet_bs, sizeof(vet_bs), 1, file);

    // Fecha o arquivo
    fclose(file);

    for(int i = 0; i < MAX_BUSCA_S; i++)
    {
        printf("Nome Aluno: %s\n", vet_bs[i]);
    }

    if (arquivo_existe("listaRegistros.bin"))//se o arquivo já existe, abre o arquivo
    {
        file = fopen("listaRegistros.bin", "rb+");
        if (file == NULL)
        {
            printf("Nao foi possivel abrir o arquivo.\n");
            return 1;
        }
    }
    else //se nao existe, cria e inicia o cabeçalho
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

    //




}
