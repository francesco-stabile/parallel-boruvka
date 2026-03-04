# Nome degli eseguibili
TARGETS = parallel_boruvka gen_graph MST benchmark

# Alias compilazione
CFLAGS = -O3
INCLD = -I/opt/homebrew/Cellar/igraph/1.0.0_1/include -L/opt/homebrew/Cellar/igraph/1.0.0_1/lib


# Regola di compilazione principale
all: $(TARGETS)

# Regole per i vari file da compilare
parallel_boruvka: parallel_boruvka.c MyMPI.c
	mpicc $(INCLD) $(CFLAGS) -o $@ $^ -ligraph
gen_graph: gen_graph.c
	gcc $(INCLD) $< -o $@ -ligraph
MST: MST.c
	gcc $(INCLD) $< -o $@ -ligraph
benchmark: benchmark.c
	gcc $< -o $@

# Regola per cancellare gli eseguibili
clean:
	rm -f $(TARGETS)

# Rebuild completo
rebuild: clean all
