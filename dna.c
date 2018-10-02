#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>

// MAX char table (ASCII)
#define MAX 256

#define MAX_STRING_LENGTH 10000
#define MAX_QUERY_LENGTH 100

#define DISTRIBUTION_TAG 0
#define QUERY_SEND_TAG 1
#define PART_SEND_TAG 2
#define RESULT_TAG 3

#define QUERY_INDEX 0
#define QUERY_SIZE 1
#define POSITION 1
#define STRING_INDEX 2
#define PART_SIZE 3

#define MASTER 0


typedef struct s_string {
	char description[80];
	char* string;
	int index;
	struct s_string *next; // ponteiro para o próximo de uma lista encadeada
} String;

typedef struct s_query {
	char description[80];
	char* substring;
	int index;
	struct s_query *next;
} Query;

typedef struct s_result {
	String *database;
	Query *query;
	int pos; // position (já com offset)
	struct s_result *next;
} Result;


Result* insert_result(Result *first, Result *new_result) {
	if (first == NULL) return new_result;
	Result *atual = first;
	while (atual->next != NULL) atual = atual->next;
	atual->next = new_result;
	return first;
}

String* insert_database(String *first, String *new_database) {
	if (first == NULL) return new_database;
	String *atual = first;
	while (atual->next != NULL) atual = atual->next;
	atual->next = new_database;
	return first;
}

Query* insert_query(Query *first, Query *new_query) {
	if (first == NULL) return new_query;
	Query *atual = first;
	while (atual->next != NULL) atual = atual->next;
	atual->next = new_query;
	return first;
}

void free_results(Result* first) {
	/*if (first == NULL) return;
	free_results(first->next);
	free(first->database->string);
	free(first->query->substring);
	free(first->database);
	free(first->query);
	free(first); */

	Result *ant, *atual;
	atual = first;

	while (atual != NULL) {
		ant = atual;
		atual = atual->next;
		free(ant->database->string);
		free(ant->query->substring);
		free(ant->database);
		free(ant->query);
		free(ant);
	}
}

// Boyers-Moore-Hospool-Sunday algorithm for string matching
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
			return k + 1; // se encontrou, retorna posição
		i = i + d[(int) string[i + 1]];
	}

	return -1; // se não encontrou
}

FILE *fdatabase, *fquery, *fout;

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

/* função que recebe uma string e preenche o buffer com a substring de start até end */
void slice_str(const char *str, char *buffer, size_t start, size_t end) {
	size_t j = 0;
	for (size_t i = start; i <= end; ++i) {
		buffer[j++] = str[i];
	}

	buffer[j] = 0;
}

char *database_str;
char *query_str;


int main(int argc, char** argv) {
	MPI_Init(&argc, &argv);

	int my_rank, np, tag = 0;
	MPI_Status status;

	MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
	MPI_Comm_size(MPI_COMM_WORLD, &np); // número de processos

	if (my_rank == MASTER) { // processo coordenador
		Result* results = NULL; // lista encadeada de resultados, recebidos dos outros processos

		/* buffers para leitura de databases e queries */
		database_str = (char*) malloc(sizeof(char) * MAX_STRING_LENGTH);
		if (database_str == NULL) {
			perror("database malloc");
			exit(EXIT_FAILURE);
		}

		query_str = (char*) malloc(sizeof(char) * MAX_QUERY_LENGTH);
		if (query_str == NULL) {
			perror("query malloc");
			exit(EXIT_FAILURE);
		}

		openfiles();

		String* database = (String*) malloc(sizeof(String));
		if (database == NULL) {
			perror("malloc database");
			exit(EXIT_FAILURE);
		}

		Query* query = (Query*) malloc(sizeof(Query));
		if (query == NULL) {
			perror("malloc query");
			exit(EXIT_FAILURE);
		}

		database->string = (char*) malloc(sizeof(char) * MAX_STRING_LENGTH);
		query->substring = (char*) malloc(sizeof(char) * MAX_QUERY_LENGTH);

		char line[100];

		/* leitura do arquivo de queries */
		fseek(fquery, 0, SEEK_SET);

		while (!feof(fquery)) {
			fgets(line, 100, fquery);
			remove_eol(line);

			if (line[0] == '>') // salvar descrição da query
				strcpy(query->description, line);

			fgets(line, 100, fquery);
			remove_eol(line);

			if (strlen(line) == 0) break;

			strcpy(query->substring, line); // salvar conteúdo da query


			/* leitura do arquivo de strings */
			fseek(fdatabase, 0, SEEK_SET);
			int cur = 0;
			int found = 0;

			while (!feof(fdatabase)) {
				fgets(line, 100, fdatabase);
				remove_eol(line);

				if (line[0] == '>')
					strcpy(database->description, line); // salvar descrição da string

				strcpy(database_str, "");
				char* first = (char*) malloc(sizeof(char) * 2);

				while (!feof(fdatabase)) { // loop para ler as linhas da string
					cur = ftell(fdatabase);
					fgets(first, 2, fdatabase);
					fseek(fdatabase, cur, SEEK_SET);

					if (first[0] != '>') {
						fgets(line, 100, fdatabase);
						remove_eol(line);
						strcat(database_str, line);
					}

					else
						break;
				}

				if (strlen(database_str) == 0) break;
				strcpy(database->string, database_str); // salvar conteúdo da string

				int n = strlen(database->string);
				int m = strlen(query->substring);
				int margem = m-1;
				size_t ini, fim;
				char part[n+1];

				/* distribuição de partes para os outros processos */
				int i;
				size_t pos = 0;

				size_t ini_offsets[np];
				ini_offsets[MASTER] = 0; // para usar na hora do retorno

				for (i = 1; i < np; i++) { // loop para os outros processos
					if (i == 1) { // primeiro processo
						ini = pos;
						pos += (n / np) + margem;
						fim = pos;
					} else if (i < np - 1) { // processos entre primeiro e o último
						ini = pos - margem;
						fim = pos + n / np;
						pos += n / np;
					} else { // último processo
						ini = pos - margem;
						fim = (size_t) n - 1;
						pos += n / np;
					}

					ini_offsets[i] = ini; // guarda offset

					slice_str(database->string, part, ini, fim);
					int info[4] = {query->index, strlen(query->substring), database->index, strlen(database->string)};
					int part_size = (int) strlen(part);
					MPI_Send(&info, 4, MPI_INT, i, DISTRIBUTION_TAG, MPI_COMM_WORLD);
					MPI_Send(query->substring, strlen(query->substring), MPI_CHAR, i, QUERY_SEND_TAG, MPI_COMM_WORLD); // envia query
					MPI_Send(&part_size, 1, MPI_INT, i, PART_SEND_TAG, MPI_COMM_WORLD); // envia pedaço
				}

			} // fim da leitura das databases

			if (!found) {
				// adicionar um Result dummy com valor -1 pra poder printar o NOT FOUND depois
				Result* result = (Result*) malloc(sizeof(Result));
				if (result == NULL) { perror("malloc result\n"); exit(EXIT_FAILURE); }
				result->database = NULL;
				result->query = query;
				result->pos = -1;

				results = insert_result(results, result);
			}
		} // fim da leitura das queries

		int result_info[3];
		int i;

		for (i = 1; i < np; i++) {
			MPI_Recv(&result_info, 3, MPI_INT, MPI_ANY_SOURCE, RESULT_TAG, MPI_COMM_WORLD, &status);
			printf("recebeu ALGUMA mensagem\n");

			if (result_info[POSITION] != -1) {
				// found = 1;
				Result* result = (Result*) malloc(sizeof(Result));
				if (result == NULL) { perror("malloc result\n"); exit(EXIT_FAILURE); }

				result->database = database;
				result->query = query;
				result->pos = result_info[POSITION] /*+ ini_offsets[i]*/;

				results = insert_result(results, result);
			}
		}


		/* percorrer lista de results e printar */
		Result* atual = results;
		while (atual != NULL) {
			printf("%s\t%s\t%d\n", atual->query->description, atual->database->description, atual->pos);
			atual = atual->next;
		}


		closefiles();
		free_results(results);

	} // fim do if do processo master


	else { // outros processos
		/* recebe query e pedaço da string do processo master */
		int info[4]; // {query_index, query_size, string_index, part_size}
		int pos;

		MPI_Recv(&info, 4, MPI_INT, MASTER, DISTRIBUTION_TAG, MPI_COMM_WORLD, &status); // recebe info de distribuição
		int n = info[PART_SIZE];
		int m = info[QUERY_SIZE];

		char query[m];
		MPI_Recv(&query, m, MPI_CHAR, MASTER, QUERY_SEND_TAG, MPI_COMM_WORLD, &status); // recebe query

		char part[n];
		MPI_Recv(&part, n, MPI_CHAR, MASTER, PART_SEND_TAG, MPI_COMM_WORLD, &status); // recebe pedaço


		/* faz busca */
		pos = bmhs(part, n, query, m);
		int result[3] = {info[QUERY_INDEX], pos, info[STRING_INDEX]};
		MPI_Send(&result, 3, MPI_INT, MASTER, RESULT_TAG, MPI_COMM_WORLD); // envia resultado pra master


	} // fim do if dos outros processos


	MPI_Finalize();

	return EXIT_SUCCESS;
}
