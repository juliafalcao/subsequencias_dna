#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>

// MAX char table (ASCII)
#define MAX 256

#define MAX_STRING_LENGTH 10000
#define MAX_QUERY_LENGTH 100

// TODO: criar estruturas para organizar strings, queries e substrings
// TODO: impedir o retorno quando um processo encontra a mesma substring em outro lugar da mesma substring

typedef struct s_string {
	char description[80];
	char* string;
} String;

typedef struct s_query {
	char description[80];
	char* substring;
} Query;

typedef struct s_result {
	String *database;
	Query *query;
	int pos; // position (já com offset)
} Result;


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

Result* divide(String *database, int n, Query *query, int m) {
	Result* result = (Result*) malloc(sizeof(Result));
	if (result == NULL) {
		perror("malloc result (divide)");
		exit(EXIT_FAILURE);
	}

	int meu_rank, np, tag = 0;
	MPI_Status status;

	MPI_Comm_rank(MPI_COMM_WORLD, &meu_rank);
	MPI_Comm_size(MPI_COMM_WORLD, &np); // número de processos

	int margem = m-1; // margem de erro
	int i, pos = 0;

	if (meu_rank == 0) {
		int msg_fim; // índice final
		int msg_ini; // índice inicial

		for (i = 0; i < np; i++) {
			if (i == 0) { // primeiro processo
				pos += (n/np) + margem;
			}

			else if (i <= np-2) { // processos entre primeiro e o último
				msg_ini = pos - margem;
				msg_fim = pos + n/np;
				MPI_Send(&msg_ini, 1, MPI_INT, i, tag, MPI_COMM_WORLD);
				MPI_Send(&msg_fim, 1, MPI_INT, i, tag, MPI_COMM_WORLD);
				pos += n/np;
			}

			else { // último processo
				msg_ini = pos-margem;
				msg_fim = n-1;
				MPI_Send(&msg_ini, 1, MPI_INT, i, tag, MPI_COMM_WORLD);
				MPI_Send(&msg_fim, 1, MPI_INT, i, tag, MPI_COMM_WORLD);
				pos += n/np;
			}
		}

		/* processo 0 pega seu pedaço da string e faz a busca */
		char buffer[n+1];
		msg_ini = 0; // TODO: renomear (não é uma msg)
		msg_fim = (n/np) + margem;

		slice_str(database->string, buffer, msg_ini, msg_fim);

		result->database = database;
		result->query = query;

		result->pos = bmhs(buffer, strlen(buffer), query->substring, m);

		if (result->pos != -1) {
			// printf("%s\n", result->query->description);
			// printf("%s\n", result->database->description);
			// printf("%d\n", result->pos);
			// printf("\n");
		}

		// TODO: receber resultados de todos os processos e escrever no dna.out
	}

	else { // outros processos
		/* receber do processo 0 os índices da substring */
		int msg_ini;
		int msg_fim;
		MPI_Recv(&msg_ini, 1, MPI_INT, MPI_ANY_SOURCE, tag, MPI_COMM_WORLD, &status);
		MPI_Recv(&msg_fim, 1, MPI_INT, MPI_ANY_SOURCE, tag, MPI_COMM_WORLD, &status);
		char buffer[n+1];
		slice_str(database->string, buffer, msg_ini, msg_fim);


		result->pos = bmhs(buffer, strlen(buffer), query->substring, m);

		if (result->pos != -1) {
			// printf(">Query string %s\n", buffer);
			// printf(">String %c\n", database->string[0]);
			// printf("%d\n", result->pos + msg_ini);
			// printf("\n");

			result->pos += msg_ini; // ?
		}
	}

	return result;
}

char *database_str;
char *query_str;

int main(int argc, char** argv) {

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

	// char desc_dna[100], desc_query[100];
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
	int found = 0;

	MPI_Init(&argc, &argv);


	/* leitura do arquivo de queries */
	fseek(fquery, 0, SEEK_SET);

	while (!feof(fquery)) {
		fgets(line, 100, fquery);
		remove_eol(line);

		if (line[0] == '>') // salvar descrição
			strcpy(query->description, line);

		fgets(line, 100, fquery);
		remove_eol(line);

		if (strlen(line) == 0) break;

		strcpy(query->substring, line); // salvar substring
		// printf("%s\n", query->description);
		fprintf(fout, "%s\n", query->description);

		/* leitura do arquivo de strings */
		fseek(fdatabase, 0, SEEK_SET);
		int cur = 0;
		found = 0;

		while (!feof(fdatabase)) {
			fgets(line, 100, fdatabase);
			remove_eol(line);

			if (line[0] == '>') // salvar descrição
				strcpy(database->description, line);

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
			strcpy(database->string, database_str);

			/* início da busca */
			Result* result = (Result*) malloc(sizeof(Result*));
			if (result == NULL) {
				perror("malloc result");
				exit(EXIT_FAILURE);
			}

			result->database = database;
			result->query = query;

			result = divide(database, strlen(database->string), query, strlen(query->substring));

			if (result->pos != -1) {
				found = 1;
				// printf("%s\n", result->database->description);
				// printf("%d\n", result->pos);
				fprintf(fout, "%s\n", result->database->description);
				fprintf(fout, "%d\n", result->pos);
			}

			free(result);
		}

		if (found == 0) {
			// printf("NOT FOUND\n");
			fprintf(fout, "NOT FOUND\n");
		}
	}


	MPI_Finalize();
	closefiles();

	free(database->string);
	free(query->substring);
	free(database);
	free(query);

	return EXIT_SUCCESS;
}
