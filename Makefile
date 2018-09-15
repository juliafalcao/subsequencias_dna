FLAGS=-O2

CC=gcc

RM=rm -f

EXEC=dna

all: $(EXEC)

$(EXEC):
	$(CC) $(FLAGS)  -o dna dna.c 
		
run:
	./$(EXEC)

clean:
	$(RM) dna.o $(EXEC)
