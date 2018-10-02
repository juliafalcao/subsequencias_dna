#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <mpi.h>

// MAX char table (ASCII)
#define MAX 256

// estrutura que armazena um registro de DNA obtido de um arquivo FASTA
typedef struct DNA_s {
	char descricao[80];
	int index; // índice sequencial dentro do arquivo
	char *conteudo;
} DNA;

// elemento de uma lista encadeada de estruturas DNA
typedef struct ListaDNA_s {
	DNA *dna;
	struct ListaDNA_s *next;
} ListaDNA;

void push(ListaDNA *head, DNA *val) {
		ListaDNA *current = head;
		while (current->next != NULL) {
			current = current->next;
		}

		current->next = (ListaDNA *)malloc(sizeof(ListaDNA));
		current->next->dna = val;
		current->next->next = NULL;

}

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

void liberaLista(ListaDNA *head) {
	while (head != NULL) {
		free(head->dna);
		ListaDNA *anterior = head;
		head = head->next;
		free(anterior);
	}
}

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

// Boyers-Moore-Hospool-Sunday algorithm for string matching
int bmhs(char *string, int n, char *substr, int m) {

	int d[MAX];
	int i, j, k;

	// pre-processing
	for (j = 0; j < MAX; j++)
		d[j] = m + 1;
	for (j = 0; j < m; j++)
		d[(int)substr[j]] = m - j;

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
		i = i + d[(int)string[i + 1]];
	}

	return -1;
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

int main(int argc, char **argv) {
	int my_rank, np; // rank e número de processos
	int tag = 200;
	int dnasQuery = 0;
	int dnasBase = 0;
	int resto = 0;

	//INICIALIZA MPI
	MPI_Status status;
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
	MPI_Comm_size(MPI_COMM_WORLD, &np);

	//LISTA ENCADEADA COM A INFORMAÇAO DOS ARQUIVOS
	ListaDNA *ListaQuery = (ListaDNA*) malloc(sizeof(ListaDNA));
	ListaQuery->next = NULL;
	ListaDNA *listaBase = (ListaDNA*) malloc(sizeof(ListaDNA));
	listaBase->next = NULL;
	int controleProcesso[np];

	//BUFFER DE LEITURA COM O TAMANHO MAXIMO POSSIVEL DO ARQUIVO
	char *bases = (char*) malloc(sizeof(char) * 1000001);
	if (bases == NULL) {
		perror("malloc bases");
		exit(EXIT_FAILURE);
	}

	//https://stackoverflow.com/questions/9864510/struct-serialization-in-c-and-transfer-over-mpi
	const int nitems = 3;
	int blocklengths[3] = {1, 1, 1};
	MPI_Datatype types[3] = {MPI_CHAR, MPI_INT, MPI_CHAR};
	MPI_Datatype mpi_dna_type;
	MPI_Aint offsets[3];

	offsets[0] = offsetof(DNA, descricao);
	offsets[1] = offsetof(DNA, index);
	offsets[2] = offsetof(DNA, conteudo);

	MPI_Type_create_struct(nitems, blocklengths, offsets, types, &mpi_dna_type);
	MPI_Type_commit(&mpi_dna_type);

	//SOMENTE O PROCESSO 0 FAZ A LEITURA DOS ARQUIVOS
	if (my_rank == 0){
		openfiles();
	}
	char desc_dna[80], desc_query[80];
	char line[80];
	int i, found, result;

	//LEITURA DO ARQUIVO DE QUERY E CRIAÇAO DA LISTA DE QUERY
	if (my_rank == 0) {
		while (!feof(fquery)) {
			 if(line[0] == '>'){
                strcpy(desc_query, line);
            } else{
                fgets(desc_query, 80, fquery);
            }
			//INICIALIZA STRUCT
			DNA * query = (DNA*) malloc(sizeof(DNA));
			query->index = dnasQuery++;
            remove_eol(desc_query);
            strcpy(query->descricao, desc_query);
			//COMEÇA A LEITURA DO CONTEUDO DO ARQUIVO
			fgets(line, 80, fquery);
            remove_eol(line);
			bases[0] = 0;
            i = 0;
            while (line[0] != '>')
			{
				strcat(bases, line);
				if (fgets(line, 80, fquery) == NULL)
					break;
				remove_eol(line);
				i += 80;
			}

            query->conteudo = (char *)malloc(sizeof(char)*strlen(bases));
			strcpy(query->conteudo,bases);

			//Atualiza o resto
			int temp = strlen(query->conteudo);
			if(temp > resto){
				resto = temp;
			}
			//ATUALIZA LISTA
			push(ListaQuery,query);

			for (int source = 1; source < np; source++)
				MPI_Send(&query, 1, mpi_dna_type, source, tag, MPI_COMM_WORLD);
		}

		DNA *query = (DNA*) malloc(sizeof(DNA));
		query->index = -1;

		for (int source = 1; source < np; source++)
			MPI_Send(&query, 1, mpi_dna_type, source, tag, MPI_COMM_WORLD);
	}

	else {
		while (1) {
			DNA *query;
			MPI_Recv(&query, 1, MPI_CHAR, 0, tag, MPI_COMM_WORLD, &status);
			if (query->index != -1)
				push(ListaQuery, query);
			
			else
				break;
		}
	}
/*
	// PROCESSO 0 CRIA A LISTA DE BASES
	if (my_rank == 0) {
		while (!feof(fdatabase)) {

			 if(line[0] == '>'){
			strcpy(desc_dna, line);
			} else{
				fgets(desc_dna, 80, fdatabase);
			}

			DNA * base = (DNA*) malloc(sizeof(DNA));
			base->index = dnasBase++;
			//EFETUA A LEITURA
			remove_eol(desc_dna);
			strcpy(base->descricao, desc_dna);

			fgets(line, 80, fdatabase);
			remove_eol(line);
			//VARIAVEL DO DNA CORRENTE
			bases[0] = 0;
			i = 0;
			while (line[0] != '>')
			{
				strcat(bases, line);
				if (fgets(line, 80, fdatabase) == NULL)
					break;
				remove_eol(line);
				i += 80;
			}
			base->conteudo = (char *)malloc(sizeof(char)*strlen(bases));
			strcpy(base->conteudo, bases);
			push(listaBase,base);
		}
	}
	
	if (my_rank == 0)
	{
		int sends = 0;
		int receives = 0;
		int indice = 0;
		ListaDNA *current = listaBase;

		while (current != NULL) {
			if (sends == np - 1) {
				sends = 0;

				while (receives != np - 1) {
					int index;
					char *result;
					MPI_Recv(&index, 1, MPI_FLOAT, MPI_ANY_SOURCE, tag, MPI_COMM_WORLD, &status);
					MPI_Recv(&result, 1, MPI_CHAR, MPI_ANY_SOURCE, tag, MPI_COMM_WORLD, &status);

					if (tag == 200) {
						int controle = controleProcesso[status.MPI_SOURCE];
						DNA *no = getElement(listaBase, controle);
						fprintf(fout, "%s\n%s\n%d\n", result, no->descricao, index);
					}

					else
						fprintf(fout, "%s\nNOT FOUND\n", result);
					
					receives++;
				}

				receives = 0;
			}

			DNA *dnaBase = current->dna;
			char *linha = dnaBase->conteudo;
			int size = strlen(linha);
			//gambiarra do stack
			int tam = size/np;

			for (int i = 0; i < np; i++) {
				int temp = i*tam;
				int tamanho = tam + resto;
				if(temp+tamanho>size){
					tamanho  = size - temp ;
				}

				char *stringprocess = substring(linha, temp, tamanho);
				controleProcesso[sends] = indice;
				MPI_Send(&stringprocess, 1, MPI_CHAR, sends++, tag, MPI_COMM_WORLD);
			}

			current = current->next;
			indice++;
		}
	}

	else {
		while (1) {
			char *base;
			MPI_Recv(&base, 1, MPI_CHAR, 0, tag, MPI_COMM_WORLD, &status);

			if (tag != 200)
				break;

			ListaDNA *current = ListaQuery;
			while (current != NULL) {
				DNA *dnaBase = current->dna;
				char *linha = dnaBase->conteudo;
				char *detalhe = dnaBase->descricao;
				result = bmhs(base, strlen(base), linha, strlen(linha));

				if (result > 0) {
					MPI_Send(&result, 1, MPI_FLOAT, 0, tag, MPI_COMM_WORLD);
					MPI_Send(&detalhe, 1, MPI_CHAR, 0, tag, MPI_COMM_WORLD);
				}

				else {
					int i = 0;
					MPI_Send(&i, 1, MPI_FLOAT, 0, 0, MPI_COMM_WORLD);
					MPI_Send(&detalhe, 1, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
				}

				current = current->next;
			}
		}
	}
*/
	if (my_rank == 0)
		closefiles();

	free(bases);
	liberaLista(ListaQuery);
	liberaLista(listaBase);

	MPI_Type_free(&mpi_dna_type);
	MPI_Finalize();

	return EXIT_SUCCESS;
}
