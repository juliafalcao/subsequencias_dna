#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#define MAX 256


typedef struct DNA_s {
	char descricao [80] ;
	int index;
	char * conteudo;
} DNA;

typedef struct ListaDNA_s{
	DNA * dna;
	struct ListaDNA_s * next;
} ListaDNA;


void push(ListaDNA * head, DNA * val) {
        ListaDNA * current = head;
		while (current->next != NULL) {
			current = current->next;
		}
		current->next = (ListaDNA * ) malloc(sizeof(ListaDNA));
		current->next->dna = val;
		current->next->next = NULL;

}

DNA * getElement(ListaDNA * head,int index) {
    ListaDNA * current = head;
	int temp =0;
    while (current != NULL) {
		if(temp=index){
			return current->dna;
		}
        current = current->next;
		temp++;
    }

}
void print_list(ListaDNA * head) {
    ListaDNA * current = head;

    while (current != NULL) {
        printf("%s\n", current->dna->descricao);
        printf("%d\n", current->dna->index);
        printf("%s\n", current->dna->conteudo);
        current = current->next;
    }
}

void liberaLista(ListaDNA * head){
	while(head != NULL){
		free(head->dna);
		ListaDNA * anterior = head;
		head = head->next;
		free(anterior);
	}
}

char *substring(char *string, int position, int length)
{
   char *pointer;
   int c;

   pointer = malloc(length+1);

   if (pointer == NULL)
   {
      printf("Unable to allocate memory.\n");
      exit(1);
   }

   for (c = 0 ; c < length ; c++)
   {
      *(pointer+c) = *(string+position-1);
      string++;
   }

   *(pointer+c) = '\0';

   return pointer;
}


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

void remove_eol(char *line) {
	int i = strlen(line) - 1;
	while (line[i] == '\n' || line[i] == '\r') {
		line[i] = 0;
		i--;
	}
}


int main()
{
    int dnasQuery = 0;
	int dnasBase =0;
	int resto = 0;
    ListaDNA * ListaQuery = (ListaDNA * ) malloc(sizeof(ListaDNA));
	ListaQuery->next = NULL;
	ListaDNA * listaBase = (ListaDNA * ) malloc(sizeof(ListaDNA));
	listaBase->next = NULL;


    char * bases = (char*) malloc(sizeof(char) * 1000001);
	if (bases == NULL) {
		perror("malloc bases");
		exit(EXIT_FAILURE);
    }
    openfiles();

    char desc_dna[80], desc_query[80];
	char line[80];
	int i, found, result;
    while (!feof(fquery))
		{
			//INICIALIZA STRUCT
			DNA * query = (DNA*) malloc(sizeof(DNA));
			query->index = dnasQuery++;
			//RECUPERA DESCRICAO
			fgets(desc_dna, 80, fquery);
			stpcpy(query->descricao, desc_dna);
			 printf("%s\n", query->descricao);
			//COMEÇA A LEITURA DO CONTEUDO DO ARQUIVO
			fgets(line, 80, fquery);
			remove_eol(line);
			bases[0] = 0;
			do
			{
				strcat(bases + i, line);
				if (fgets(line, 100, fquery) == NULL)
					break;
				remove_eol(line);
				i += 80;
			} while (line[0] != '>');
			char * linha = (char *)malloc(sizeof(line));
			strcpy(linha,bases);
			query->conteudo = linha;
			printf("%s\n", linha);

			//Atualiza o resto
			int temp = strlen(linha);
			if(temp > resto){
				resto = temp;
			}

			//ATUALIZA LISTA
			push(ListaQuery,query);

		}
        while (!feof(fdatabase))
		{

			DNA * base = (DNA*) malloc(sizeof(DNA));
			base->index = dnasQuery++;

			//EFETUA A LEITURA

			fgets(desc_dna, 80, fdatabase);
			stpcpy(base->descricao, desc_dna);
			remove_eol(desc_dna);
			fgets(line, 80, fdatabase);
			remove_eol(line);
			//VARIAVEL DO DNA CORRENTE
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

			char * linha = (char *)malloc(sizeof(char)*strlen(bases));
			strcpy(linha,bases);
			strcpy(base->conteudo, linha);
			push(listaBase,base);
		}


		int sends =0;
		int receives =0;
		int indice =0;
        ListaDNA * current = listaBase;
		while(current !=NULL){

			DNA * dnaBase = current->dna;
			char * linha = (char*)malloc(sizeof(char)*strlen(dnaBase->conteudo)+1);
			strcpy(linha,dnaBase->conteudo);
			int size = strlen(linha);
			//gambiarra do stack
			int parts = 1 + ((size - 1) / 80);

			for(int i=0; i<parts;i++){
				int temp = i+80+resto;
				int tamanho = 80 + resto;
				if(temp>size){
					tamanho  = tamanho - (temp-resto);
				}
				char * stringprocess = substring(linha,i*80,tamanho);


			}
			current = current->next;

		}
    print_list (ListaQuery);
    print_list (listaBase);
    free(bases);
	liberaLista(ListaQuery);
	liberaLista(listaBase);

    return 0;
}
