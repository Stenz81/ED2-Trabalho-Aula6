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

struct cabecalho_busca{
    int registro_alterado; // Dirá se determinado arquivo foi alterado desde o ultimo carregamento para memoria (0 se não, 1 se sim)
}hdr_bp, hdr_bs_nomes, hdr_bs_chaves;

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


void insereRegistro (FILE *file, FILE *file_bp, FILE *file_bs_nomes, FILE *file_bs_chaves, const Registro &reg)
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

    fseek(file_bs_nomes, 0, SEEK_END);        
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

    //Verifica se o arquivo "listaRegistros.bin" já existe, e em caso negativo, cria e realiza os procedimentos necessarios, como escrita do header
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


    //Verifica se o arquivo "indice_primario.bin" já existe, e em caso negativo, cria e realiza os procedimentos necessarios, como escrita do header
    FILE *file_bp; 
    if (arquivo_existe("indice_primario.bin"))//se o arquivo já existe, abre o arquivo
    {
        file_bp = fopen("indice_primario.bin", "rb+");
        if (file_bp == NULL)
        {
            printf("Nao foi possivel abrir o arquivo.\n");
            return 1;
        }
    }
    else //se nao existe, cria e inicia o cabeçalho
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

    //Verifica se o arquivo "indice_secundario_nomes.bin" já existe, e em caso negativo, cria e realiza os procedimentos necessarios, como escrita do header
    FILE *file_bs_nomes; 
    if (arquivo_existe("indice_secundario_nomes.bin"))//se o arquivo já existe, abre o arquivo
    {
        file_bs_nomes = fopen("indice_secundario_nomes.bin", "rb+");
        if (file_bs_nomes == NULL)
        {
            printf("Nao foi possivel abrir o arquivo.\n");
            return 1;
        }
    }
    else //se nao existe, cria e inicia o cabeçalho
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

    //Verifica se o arquivo "indice_secundario_chaves.bin" já existe, e em caso negativo, cria e realiza os procedimentos necessarios, como escrita do header
    FILE *file_bs_chaves; 
    if (arquivo_existe("indice_secundario_chaves.bin"))//se o arquivo já existe, abre o arquivo
    {
        file_bs_chaves = fopen("indice_secundario_chaves.bin", "rb+");
        if (file_bs_chaves == NULL)
        {
            printf("Nao foi possivel abrir o arquivo.\n");
            return 1;
        }
    }
    else //se nao existe, cria e inicia o cabeçalho
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

    fclose(file);
    fclose(file_bp);
    fclose(file_bs_nomes);
    fclose(file_bs_chaves);

}
