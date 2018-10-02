#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <mpi.h>

// MAX char table (ASCII)
#define MAX 256

#define MAX_DATABASE_LENGTH 1000001

#define MASTER 0 // processo coordenador

// estrutura que armazena um registro de DNA obtido de um arquivo FASTA
typedef struct DNA_s {
    char descricao[80];
    int index; // índice sequencial dentro do arquivo
    char *conteudo;
}
DNA;

// elemento de uma lista encadeada de estruturas DNA
typedef struct ListaDNA_s {
    DNA *dna;
    struct ListaDNA_s *next;
}
ListaDNA;

// lista encadeada com a estrutura do arquivo de saída
typedef struct resp_s {
    char *query;
    char *descricao;
    struct resp_s *next;
}
Resp;

/*função que insere na lista de acordo com os critérios
- insere se não existir query
- se já houver query, verifica se não é NOT FOUND antes de inserir */
void pushResp(Resp *head, char *query, char *descricao) {
    printf("PUSH RESP -- query: %s, descricao: %s\n", query, descricao);
    Resp* current = head, *last = NULL;

    while (current != NULL) {
        // verifica se é uma query nova ou não
        if (strcmp(current->query, query) == 0) {
            // compara se ainda é NOT FOUND ou não
            if (strstr(current->descricao, "NOT FOUND") != NULL) {
                current->descricao[0] = 0;
            }

            int size = strlen(current->descricao) + strlen(descricao) + 1;

            // realoca vetor para concatenar mais informações
            current->descricao = (char*) realloc(current->descricao, sizeof(char) *(strlen(current->descricao) + strlen(descricao) + 2));
            strcat(current->descricao, "\n");
            strcat(current->descricao, descricao);
            return;
        }

        last = current;
        current = current->next;
    }

    printf("PUSH RESP -- inserindo fora do while\n");

    // caso não existir, insere um novo no final da lista
    last->next = (Resp*) malloc(sizeof(Resp));
    last->next->query = (char*) malloc(sizeof(char) *(strlen(query) + 1));
    stpcpy(last->next->query, query);
    last->next->descricao = (char*) malloc(sizeof(char) *(strlen(descricao) + 1));
    stpcpy(last->next->descricao, descricao);
    last->next->next = NULL;
}

/* função para liberar lista de respostas */
void liberaListaResp(Resp *head) {
    while (head != NULL) {
        free(head->descricao);
        free(head->query);
        Resp *anterior = head;
        head = head->next;
        free(anterior);
    }
}

/* função para inserir DNA na lista */
void push(ListaDNA *head, DNA *val) {
    if (head->dna == NULL) {
        head->dna = val;
        head->next = NULL;
    } else {
        ListaDNA *current = head;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = (ListaDNA*) malloc(sizeof(ListaDNA));
        current->next->dna = val;
        current->next->next = NULL;
    }

}

/* obter elemento da lista */
DNA *getElement(ListaDNA *head, int index) {
    ListaDNA *current = head;
    int temp = 0;
    while (current != NULL) {
        if (temp = index) {
            return current->dna;
        }

        current = current->next;
        temp++;
    }
}

/* função para liberar lista */
void liberaLista(ListaDNA *head) {
    while (head != NULL) {
        free(head->dna);
        ListaDNA *anterior = head;
        head = head->next;
        free(anterior);
    }
}

/* função que recebe uma string, uma posição e um tamanho e retorna uma substring */
char *substring(char *string, int position, int length) {
    char *pointer;
    int c;

    pointer = malloc(length + 1);

    if (pointer == NULL) {
        printf("Unable to allocate memory.\n");
        exit(1);
    }

    for (c = 0; c < length; c++) {
        *(pointer + c) = *(string + position - 1);
        string++;
    }

    *(pointer + c) = '\0';

    return pointer;
}

/*função que recebe uma string e preenche o buffer com a substring de start até end */
void slice_str(const char *str, char *buffer, size_t start, size_t end) {
    size_t j = 0;
    for (size_t i = start; i <= end; ++i) {
        buffer[j++] = str[i];
    }

    buffer[j] = 0;
}

/* Boyers-Moore-Hospool-Sunday algorithm for string matching */
int bmhs(char *string, int n, char *substr, int m) {

    int d[MAX];
    int i, j, k;

    // pre-processing
    for (j = 0; j < MAX; j++)
        d[j] = m + 1;
    for (j = 0; j < m; j++)
        d[(int) substr[j]] = m - j;

    // searching
    i = m - 1;
    while (i < n) {
        k = i;
        j = m - 1;
        while ((j >= 0) && (string[k] == substr[j])) {
            j--;
            k--;
        }

        if (j < 0)
            return k + 1;
        i = i + d[(int) string[i + 1]];
    }

    return -1;
}

FILE *fdatabase, *fquery, *fout;


/* função que abre os arquivos */
void openfiles() {
    fdatabase = fopen("dna.in", "r+");
    if (fdatabase == NULL) {
        perror("dna.in");
        exit(EXIT_FAILURE);
    }

    fquery = fopen("query.in", "r");

    if (fquery == NULL) {
        perror("query.in");
        exit(EXIT_FAILURE);
    }

    fout = fopen("dna.out", "w");

    if (fout == NULL) {
        perror("fout");
        exit(EXIT_FAILURE);
    }
}

/* função que fecha os arquivos */
void closefiles() {
    fflush(fdatabase);
    fclose(fdatabase);

    fflush(fquery);
    fclose(fquery);

    fflush(fout);
    fclose(fout);
}


void remove_eol(char *line) {
    int i = strlen(line) - 1;
    while (line[i] == '\n' || line[i] == '\r') {
        line[i] = 0;
        i--;
    }
}

/* função que recebe a lista de respostas e escreve no arquivo */
void ImprimeSaida(Resp *head) {
    while (head != NULL) {
        fprintf(fout, "%s\n", head->query);
        fprintf(fout, "%s\n", head->descricao);
        head = head->next;
    }
}

/* função que imprime lista no terminal */
void PrintaLista(Resp *head) {
    printf("-----------------\n");
    while (head != NULL) {
        printf("%s\n", head->query);
        printf("%s\n", head->descricao);
        head = head->next;
    }
    printf("-----------------\n");
}


int main(int argc, char **argv) {
    int my_rank, np; // rank e número de processos
    int tag = 200;
    int dnasQuery = 0; // índice sequencial de queries
    int dnasBase = 0; // índice sequencial de bases
    int resto = 0;

    /* inicialização do MPI */
    MPI_Status status;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &np);

    /* criação de listas encadeadas com as informações dos arquivos de entrada */
    ListaDNA *ListaQuery = (ListaDNA*) malloc(sizeof(ListaDNA));
    ListaQuery->next = NULL;
    ListaQuery->dna = NULL;
    ListaDNA *listaBase = (ListaDNA*) malloc(sizeof(ListaDNA));
    listaBase->next = NULL;
    listaBase->dna = NULL;
    Resp *resposta = NULL;

    /* buffer de leitura com o tamanho máximo possível do arquivo */
    char *bases = (char*) malloc(sizeof(char) * MAX_DATABASE_LENGTH);
    if (bases == NULL) {
        perror("malloc bases");
        exit(EXIT_FAILURE);
    }

    char desc_dna[80], desc_query[80];
    char line[80];
    int i, found, result;

    /* processo coordenador abre e lê arquivo de queries e cria lista encadeada */
    if (my_rank == MASTER) {
        openfiles();

        while (!feof(fquery)) {
            if (line[0] == '>') {
                strcpy(desc_query, line);
            } else {
                fgets(desc_query, 80, fquery);
            }
            //INICIALIZA STRUCT
            DNA *query = (DNA*) malloc(sizeof(DNA));
            query->index = dnasQuery;
            remove_eol(desc_query);
            strcpy(query->descricao, desc_query);
            //COMEÇA A LEITURA DO CONTEUDO DO ARQUIVO
            fgets(line, 80, fquery);
            remove_eol(line);
            bases[0] = 0;
            i = 0;
            while (line[0] != '>') {
                strcat(bases, line);
                if (fgets(line, 80, fquery) == NULL)
                    break;
                remove_eol(line);
                i += 80;
            }

            query->conteudo = (char*) malloc(sizeof(char) *strlen(bases));
            strcpy(query->conteudo, bases);

            //Atualiza o resto
            int temp = strlen(query->conteudo);
            if (temp > resto) {
                resto = temp;
            }
            //ATUALIZA LISTA
            push(ListaQuery, query);
            //ENVIA A INFORMACAO PARA OS OUTROS PROCESSOS
            for (int source = 1; source < np; source++) {

                int desc_length = strlen(desc_query);
                char *desc = (char*) malloc(sizeof(char) *(strlen(desc_query) + 1));
                stpcpy(desc, desc_query);

                //SEND DA DESCRICAO
                MPI_Send(&desc_length, 2, MPI_INT, source, tag, MPI_COMM_WORLD);
                MPI_Send(desc, desc_length + 1, MPI_CHAR, source, tag, MPI_COMM_WORLD);

                //SEND DO INDEX
                MPI_Send(&dnasQuery, 2, MPI_INT, source, tag, MPI_COMM_WORLD);

                int bases_length = strlen(bases);
                char *basetxt = (char*) malloc(sizeof(char) *(strlen(bases) + 1));
                stpcpy(basetxt, bases);

                MPI_Send(&bases_length, 2, MPI_INT, source, tag, MPI_COMM_WORLD);
                MPI_Send(basetxt, bases_length + 1, MPI_CHAR, source, tag, MPI_COMM_WORLD);

            }

            /* preenche lista de respostas com todas as queries como NOT FOUND para facilitar o processamento */
            if (resposta == NULL) {
                resposta = (Resp*) malloc(sizeof(Resp*));
                resposta->query = (char*) malloc(sizeof(char) *(strlen(desc_query) + 2));
                stpcpy(resposta->query, desc_query);
                resposta->next = NULL;
                resposta->descricao = (char*) malloc(sizeof(char) * 12);
                strcpy(resposta->descricao, "NOT FOUND\n");
            } else {
                pushResp(resposta, desc_query, "NOT FOUND\n");
            }
            dnasQuery++;
        }

        int index = -1;
        char *aux1 = (char*) malloc(sizeof(char));
        int aux1Size = strlen(aux1);
        char *aux2 = (char*) malloc(sizeof(char));
        int aux2Size = strlen(aux2);
        //ENVIA UMA MENSAGEM AVISANDO QUE O PROCESSO DE ENVIO ACABOU
        for (int source = 1; source < np; source++) {
            MPI_Send(&aux1Size, 2, MPI_INT, source, tag, MPI_COMM_WORLD);
            MPI_Send(aux1, aux1Size + 1, MPI_CHAR, source, tag, MPI_COMM_WORLD);

            MPI_Send(&index, 2, MPI_INT, source, tag, MPI_COMM_WORLD);

            MPI_Send(&aux2Size, 2, MPI_INT, source, tag, MPI_COMM_WORLD);
            MPI_Send(aux2, aux2Size + 1, MPI_CHAR, source, tag, MPI_COMM_WORLD);
        }
        free(aux1);
        free(aux2);
        // PrintaLista(resposta);
    } else {
        while (1) {
            //FICA EM LOOP ESPERANDO RECEBER INFORMACOES DA MASTER
            DNA *query = (DNA*) malloc(sizeof(DNA));
            int index;
            int desc_length;
            int bases_length;

            //RECEIVE DA DESCRICAO
            MPI_Recv(&desc_length, 2, MPI_INT, 0, tag, MPI_COMM_WORLD, & status);
            char *descricao = (char*) malloc(desc_length + 1);
            MPI_Recv(descricao, desc_length + 1, MPI_CHAR, 0, tag, MPI_COMM_WORLD, & status);
            strcpy(query->descricao, descricao);

            //RECEIVE DO INDEX
            MPI_Recv(&index, 2, MPI_INT, 0, tag, MPI_COMM_WORLD, & status);

            query->index = index;

            //RECEIVE DO CONTEUDO
            MPI_Recv(&bases_length, 2, MPI_INT, 0, tag, MPI_COMM_WORLD, & status);

            char *conteudo = (char*) malloc(bases_length + 1);
            MPI_Recv(conteudo, bases_length + 1, MPI_CHAR, 0, tag, MPI_COMM_WORLD, & status);

            query->conteudo = (char*) malloc(bases_length + 1);
            stpcpy(query->conteudo, conteudo);

            //CRITERIO DE PARADA 
            if (query->index != -1) {
                push(ListaQuery, query);
            } else {
                free(query);
                break;
            }
        }
    }

    // PROCESSO 0 CRIA A LISTA DE BASES
    if (my_rank == 0) {
        while (!feof(fdatabase)) {

            if (line[0] == '>') {
                strcpy(desc_dna, line);
            } else {
                fgets(desc_dna, 80, fdatabase);
            }

            DNA *base = (DNA*) malloc(sizeof(DNA));
            base->index = dnasBase++;
            //EFETUA A LEITURA
            remove_eol(desc_dna);
            strcpy(base->descricao, desc_dna);

            fgets(line, 80, fdatabase);

            remove_eol(line);
            //VARIAVEL DO DNA CORRENTE
            bases[0] = 0;
            i = 0;
            while (line[0] != '>') {

                strcat(bases, line);
                if (fgets(line, 80, fdatabase) == NULL)
                    break;
                remove_eol(line);
                i += 80;
            }

            base->conteudo = (char*) malloc(sizeof(char) *strlen(bases));
            strcpy(base->conteudo, bases);
            push(listaBase, base);
        }
    }

    if (my_rank == 0) {
        ListaDNA *current = listaBase;
        //PERCORRE A LISTA DE BASES PARA DISTRIBUIR ENTRE OS PROCESSOS
        while (current != NULL) {

            DNA *dnaBase = current->dna;
            int size = strlen(dnaBase->conteudo);
            char *linha = (char*) malloc(size + 1);
            strcat(linha, dnaBase->conteudo);

            int tam = size / (np - 1);
            //DIVIDE A STRING DA BASE E ENVIA AO PROCESSOS
            for (int i = 1; i < np; i++) {

                int temp = (i - 1) *tam;
                int tamanho = tam + resto;
                if (temp + tamanho > size) {
                    tamanho = size - temp;
                }
                char *stringprocess = (char*) malloc(tamanho + 1);
                slice_str(linha, stringprocess, temp, temp + tamanho);
                //ENVIO DO OFFSET PARA CALCULO DO RESULT
                MPI_Send(&temp, 1, MPI_INT, i, tag, MPI_COMM_WORLD);
                //ENVIO DO TAMANHO DA STRING
                MPI_Send(&tamanho, 1, MPI_INT, i, tag, MPI_COMM_WORLD);
                //ENVIO DA STRING
                MPI_Send(stringprocess, tamanho, MPI_CHAR, i, tag, MPI_COMM_WORLD);

            }
            //EFETUA O RECEIVE DAS INFORMACAO E PROCESSAMENTO DA MESMA
            for (int i = 1; i < np;) {

                int result;
                int detalhesize;

                //ESPERA A INFORMACAO DE QUALQUER PROCESSO
                MPI_Recv(&result, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, & status);

                //AO CHEGAR UMA INFORMACAO O PROCESSO SEGUE A PARTIR DE UM SOURCE 
                MPI_Recv(&detalhesize, 1, MPI_INT, status.MPI_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, & status);

                char *detalheRecv = (char*) malloc(detalhesize + 1);
                MPI_Recv(detalheRecv, detalhesize + 1, MPI_CHAR, status.MPI_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, & status);
                // printf("REQUESTER: %d, TAG: %d, RESULT: %d ,detalheRecv: %s\n", status.MPI_SOURCE, status.MPI_TAG, result, detalheRecv);

                //SE TUDO OK ATUALIZA A LISTA
                if (status.MPI_TAG == 200) {
                    if (resposta == NULL) {

                        resposta = (Resp*) malloc(sizeof(Resp*));

                        resposta->query = (char*) malloc(sizeof(char) *(strlen(detalheRecv) + 2));
                        stpcpy(resposta->query, detalheRecv);

                        char strResult[12];
                        sprintf(strResult, "%d", result);
                        resposta->descricao = (char*) malloc(sizeof(char) *(strlen(dnaBase->descricao) + strlen(strResult) + 2));
                        strcat(resposta->descricao, dnaBase->descricao);
                        strcat(resposta->descricao, "\n");
                        strcat(resposta->descricao, strResult);
                        resposta->next = NULL;

                    } else {

                        char strResult[12];
                        sprintf(strResult, "%d", result);

                        char *descricao = (char*) malloc(sizeof(char) *(strlen(dnaBase->descricao) + strlen(strResult) + 2));

                        strcat(descricao, dnaBase->descricao);
                        strcat(descricao, "\n");
                        strcat(descricao, strResult);

                        pushResp(resposta, detalheRecv, descricao);

                    }
                    //TAG QUE AVISA A MASTER QUE UM PROCESSO ACABOU O PROCESSAMENTO DA SUBSTRING BASE
                } else if (status.MPI_TAG == 100) {
                    i++;
                }

            }
            //PROX ELEMENTO
            //PrintaLista(resposta);
            current = current->next;
        }
        //ENVIA A INFORMACAO PARA OS OUTROS PROCESSOS PARA AVISAR DO FIM DO PROCESSO
        for (int i = 1; i < np; i++) {
            int tamanho = 0;
            char *stringprocess = (char*) malloc(sizeof(char));
            MPI_Send(&tamanho, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
            MPI_Send(&tamanho, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
            MPI_Send(stringprocess, tamanho, MPI_CHAR, i, 0, MPI_COMM_WORLD);
        }
        ImprimeSaida(resposta);
    } else {
        while (1) {

            int tam;
            int offset;
            //ESPERA INFORMACAO DO MASTER MAS DE QUALQUER TAG
            //RECEBE O OFFSET
            MPI_Recv(&offset, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, & status);
            //RECEBE O TAMANHO
            MPI_Recv(&tam, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, & status);

            //RECEBE O CONTEUDO DA SUBSTRING BASE
            char *base = (char*) malloc(tam + 1);
            MPI_Recv(base, tam + 1, MPI_CHAR, 0, MPI_ANY_TAG, MPI_COMM_WORLD, & status);

            //CRITERIO DE PARADA
            if (status.MPI_TAG != 200) {
                break;
            }

            ListaDNA *current = ListaQuery;
            int result = -1;
            int desc_size;
            char *detalheSend;
            //PERCORRE AS QUERYS E VERIFICA SE ALGUMA ATENDE A CONSULTA
            while (current != NULL) {

                DNA *dnaQuery = current->dna;
                desc_size = strlen(dnaQuery->descricao);
                detalheSend = (char*) malloc(desc_size + 1);
                stpcpy(detalheSend, dnaQuery->descricao);
                result = bmhs(base, strlen(base), dnaQuery->conteudo, strlen(dnaQuery->conteudo));
                //QUANDO ACHA UMA INFORMACAO ENVIA PARA A MASTER
                if (result > 0) {
                    result += offset;
                    MPI_Send(&result, 1, MPI_INT, 0, tag, MPI_COMM_WORLD);
                    MPI_Send(&desc_size, 1, MPI_INT, 0, tag, MPI_COMM_WORLD);
                    MPI_Send(detalheSend, desc_size + 1, MPI_CHAR, 0, tag, MPI_COMM_WORLD);
                }
                free(detalheSend);
                current = current->next;
            }
            //AVISA QUE ACABOU O PROCESSAMENTO DAS QUERYS
            MPI_Send(&result, 1, MPI_INT, 0, 100, MPI_COMM_WORLD);
            MPI_Send(&desc_size, 1, MPI_INT, 0, 100, MPI_COMM_WORLD);
            MPI_Send(detalheSend, desc_size + 1, MPI_CHAR, 0, 100, MPI_COMM_WORLD);
        }
    }

    if (my_rank == 0) {
        closefiles();
        free(bases);
        liberaLista(ListaQuery);
        liberaLista(listaBase);
        liberaListaResp(resposta);
    }

    MPI_Finalize();

    return EXIT_SUCCESS;
}
