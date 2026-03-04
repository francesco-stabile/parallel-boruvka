# Parallel Boruvka MST

Implementazione dell'algoritmo parallelo di Borůvka per il calcolo di un **Minimum Spanning Tree** utilizzando **igraph** e **MPI**.

> Per tutti i dettagli dell'implementazione, l'analisi delle prestazioni e i risultati ottenuti, consultare `documentazione.pdf`.

## Utilizzo

```bash
mpirun -np <p> ./parallel_boruvka <nome_file>
```

dove:
- **`p`** — numero di processi MPI da utilizzare.
- **`nome_file`** — percorso del file contenente il grafo (formato `.ncol`).

## Prerequisiti e installazione

L'algoritmo richiede le librerie **MPI** e **igraph**. Per installare igraph:

```bash
brew install igraph
```

Assicurarsi che il Makefile includa i percorsi corretti per la libreria igraph.

## Compilazione

Eseguire all'interno della directory il comando:

```bash
make
```

## Struttura della directory

| File | Descrizione |
|------|-------------|
| `benchmark.c`| Codice per l'esecuzione di benchmark automatici dato il numero di iterazioni, di processi e grafo su cui eseguire.|
| `documentazione.pdf` | Documentazione dettagliata del problema, della strategia adottata, con analisi e risultati. |
| `Makefile` | File per la compilazione automatica. |
| `parallel_boruvka.c` | Implementazione dell'algoritmo di Borůvka parallelo. |
| `gen_graph.c` | Generatore di grafi connessi Erdős–Rényi, dato il numero di vertici e il nome del file di output. |
| `MyMPI.c` | Implementazione di funzioni di supporto per la programmazione con MPI. |
| `MyMPI.h` | Header di `MyMPI.c`. |

## English version

I'm working to translate all the files...