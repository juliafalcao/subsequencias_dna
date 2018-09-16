#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>

// MAX char table (ASCII)
#define MAX 256

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
			return k + 1;
		i = i + d[(int) string[i + 1]];
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

inline void remove_eol(char *line) {
	int i = strlen(line) - 1;
	while (line[i] == '\n' || line[i] == '\r') {
		line[i] = 0;
		i--;
	}
}

char *bases;
char *query;


char ** queryProcessadas

int indexQuery = 0

int main(int argc, char **argv) {
	int my_rank, np; // rank e número de processos
	int tag = 200;
	MPI_Status status;

	MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &np);

	//SOMENTE O PROCESSO 0 EFETUA A LEITURA LOGO SO ELE DA O MALLOC DO MAL

	bases = (char*) malloc(sizeof(char) * 1000001);
	if (bases == NULL) {
		perror("malloc bases");
		exit(EXIT_FAILURE);
	}
	query = (char*) malloc(sizeof(char) * 1000001);
	if (query == NULL) {
		perror("malloc query");
		exit(EXIT_FAILURE);
	}
	
		
	//TODOS OS PROCESSOS PRECISAM ARMAZENAR ESSAS DISGRAÇAS //ESSE MALLOC TA ERRADO CTZ
	
	queryProcessadas = (char**) malloc(sizeof(char) ** 1000001);
	if (query == NULL) {
		perror("malloc queryProcessadas");
		exit(EXIT_FAILURE);
	}

	
	
	
	
	//SOMENTE O PROCESSO 0 FAZ A LEITURA DOS ARQUIVOS
	if(my_rank==0)
	{
		openfiles();
	}
	//MUDEI PRA 80 PQ FAZ MTO MAIS SENTIDO
	char desc_dna[80], desc_query[80];
	char line[80];
	int i, found, result;
	
	//LEITURA DO ARQUIVO DE QUERY E DISTRIBUIÇÃO DAS QUERYS ENTRE OS PROCESSO
	if(my_rank==0)
	{
		int index =0;
		while (!feof(fquery)) 
		{	
			//Pega a descrição da query
			fgets(desc_query, 80, fquery);
			remove_eol(desc_query);
			
			//LEITURA DOS PRIMEIROS 80 CHARS DA CADEIRA DA QUERY
			fgets(line, 80, fquery);
			remove_eol(line);
			//VETOR PRA ARMAZENAMENTO 
			query[0] = 0;
			i = 0;
			do 
			{
				strcat(query + i, line);
				if (fgets(line, 80, fquery) == NULL)
					break;
				remove_eol(line);
				i += 80;
			} while (line[0] != '>');
			
			//ARMAZENA A INFORMAÇÂO
			if(index=0)
			{
				//BEM COCO
				strcpy(queryProcessadas[indexQuery],query)
				indexQuery++;
			}
			//ENVIA A INFORMAÇÂO
			else
			{
				//ENVIAR A DESCRIÇÂO TAMBEM... 
				MPI_Send(&query, 1, MPI_CHAR, index, tag, MPI_COMM_WORLD);
			}
			if(index == np -1)
			{
				index = 0
			}
			else
			{
				index++;
			}
			
		}
		for(int i =1; i < np;i++){
			//FAZER ISSO DE UMA MANEIRA ELEGANTE, TALVEZ TAG RESOLVA
			MPI_Send("ACABO", 1, MPI_CHAR, i, tag, MPI_COMM_WORLD);
		}
	}
	else
	{
		//Recebe a informação
		//armazena a informação
		MPI_Recv(&val, 1, MPI_CHAR, 0, tag, MPI_COMM_WORLD, &status)
		//COMO DISSE ANTES... COCO
		strcpy(queryProcessadas[indexQuery],query)
		indexQuery++;
		
	}
	
	//APOS A DISTRIBUIÇÃO DAS QUERYS COMEÇAR A BRINCADEIRA
	
	// PROCESSO 0 ENVIA AS INFORMAÇÂO
	if(my_rank == 0)
	{
		while (!feof(fdatabase)) 
		{
			//EFETUA A LEITURA
			//ENVIA A INFORMAÇÂO
			//PROCESSA A INFORMAÇÂO
			fgets(desc_dna, 80, fdatabase);
			remove_eol(desc_dna);
			fgets(line, 80, fdatabase);
			remove_eol(line);
			bases[0] = 0;
			i = 0;
			do 
			{
				strcat(bases + i, line);
				if (fgets(line, 100, fdatabase) == NULL)
					break;
				remove_eol(line);
				i += 80;
			} while (line[0] != '>');
			//ENVIA PARA TODOS OS PROCESSOS A INFORMAÇÂO
			for(int i =1; i < np;i++){
				
				MPI_Send(&line, 1, MPI_CHAR, i, tag, MPI_COMM_WORLD);
			}
			for(int i =0;i<indexQuery;i++){
				//TA ERRADO SO SEGUE A IDEIA
				query = queryProcessadas[i];
				result = bmhs(line, strlen(line), query, strlen(query));
				if (result > 0) {
					//falta a desc
					fprintf(fout, "%s\n%d\n", desc_dna, result);
					//???
					found++;
				}				
			}
			
			
		}
	}
	else
	{
		//RECEBE A INFORMAÇÂO
		//PROCESSA A INFORMAÇÂO
		MPI_Recv(&line, 1, MPI_FLOAT, source, tag, MPI_COMM_WORLD, &status);
		for(int i =0;i<indexQuery;i++){
			//TA ERRADO SO SEGUE A IDEIA
			query = queryProcessadas[i];
			result = bmhs(line, strlen(line), query, strlen(query));
			if (result > 0) {
				//falta a desc
				fprintf(fout, "%s\n%d\n", desc_dna, result);
				//???
				found++;
			}				
		}
	}


	// fgets(desc_query, 100, fquery);
	// remove_eol(desc_query);
	
	
	
	
	// while (!feof(fquery)) {
		
		
		// fprintf(fout, "%s\n", desc_query);
		// // read query string
		// fgets(line, 100, fquery);
		// remove_eol(line);
		// query[0] = 0;
		// i = 0;
		// do {
			// strcat(query + i, line);
			// if (fgets(line, 100, fquery) == null)
				// break;
			// remove_eol(line);
			// i += 80;
		// } while (line[0] != '>');
		// strcpy(desc_query, line);

		// // read database and search
		// found = 0;
		// fseek(fdatabase, 0, seek_set);
		// fgets(line, 100, fdatabase);
		// remove_eol(line);
		
		
		
		
		// while (!feof(fdatabase)) {
			// strcpy(desc_dna, line);
			// bases[0] = 0;
			// i = 0;
			// fgets(line, 100, fdatabase);
			// remove_eol(line);
			// do {
				// strcat(bases + i, line);
				// if (fgets(line, 100, fdatabase) == null)
					// break;
				// remove_eol(line);
				// i += 80;
			// } while (line[0] != '>');

			// result = bmhs(bases, strlen(bases), query, strlen(query));
			// if (result > 0) {
				// fprintf(fout, "%s\n%d\n", desc_dna, result);
				// found++;
			// }
		// }

		// if (!found)
			// fprintf(fout, "not found\n");
	// }

	if(my_rank ==0){
	closefiles();
	}
	

	free(query);
	free(bases);
	free(queryProcessadas);
	
	MPI_Finalize();
	
	return EXIT_SUCCESS;
}
