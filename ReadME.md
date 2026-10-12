# Parallel Boruvka MST

📌 **Language / Lingua:**  
🇬🇧 [English Version](#-english-version) | 🇮🇹 [Versione Italiana](#-versione-italiana)

---

## 🇬🇧 English Version

Implementation of Borůvka's parallel algorithm for computing a **Minimum Spanning Tree** using **igraph** and **MPI**.

> For comprehensive implementation details, performance analysis, and benchmarks, please refer to `doc_en.pdf`.

---

## 🛠️ Technologies and Tools Used

* **C** (Main language)
* **MPI** (Library for parallel programming and process communication)
* **igraph** (Library for graph manipulation and analysis)

---

## 📁 Repository Structure

```text
├── documentazione/               # Documentation (ita / eng) folder
│   ├── doc_en.pdf                # [EN] Detailed documentation of the problem, strategy, analysis, and results
│   └── documentazione.pdf        # [IT] Detailed documentation of the problem, strategy, analysis, and results
├── Makefile              # Automation file for project compilation
├── MyMPI.c               # Implementation of support functions for MPI programming
├── MyMPI.h               # Header file for MyMPI.c
├── benchmark.c           # Code for running automatic benchmarks given iterations, processes, and target graph
├── gen_graph.c           # Erdős–Rényi connected graph generator, given the number of vertices and output filename
└── parallel_boruvka.c    # Parallel implementation of Borůvka's algorithm
```

---

## 🚀 Execution Pipeline

### 1. Prerequisites and Installation
The algorithm requires **MPI** and **igraph** libraries. To install igraph via Homebrew:
```bash
brew install igraph
```
* **Note:** Ensure that the Makefile includes the correct paths for the igraph library.

### 2. Compilation
To build the project, execute the following command inside the directory:
```bash
make
```

### 3. Usage and Execution
To run the application, use the command:
```bash
mpirun -np <p> ./parallel_boruvka <nome_file>
```
Where:
1. **`p`:** number of MPI processes to allocate.
2. **`nome_file`:** path to the file containing the graph (`.ncol` format).

---

## 🇮🇹 Versione Italiana

Implementazione dell'algoritmo parallelo di Borůvka per il calcolo di un **Minimum Spanning Tree** utilizzando **igraph** e **MPI**.

> Per tutti i dettagli dell'implementazione, l'analisi delle prestazioni e i risultati ottenuti, consultare `documentazione.pdf`.

---

## 🛠️ Tecnologie e Strumenti Utilizzati

* **C** (Linguaggio principale)
* **MPI** (Libreria per la programmazione parallela e la comunicazione tra processi)
* **igraph** (Libreria per la manipolazione e l'analisi dei grafi)

---

## 📁 Struttura della Directory

```text
├── documentazione/               # Cartella della documentazione (ita / eng)
│   ├── doc_en.pdf                # [EN] Documentazione del problema, della strategia adottata, con analisi e risultati
│   └── documentazione.pdf        # [IT] Documentazione del problema, della strategia adottata, con analisi e risultati
├── Makefile              # File per la compilazione automatica
├── MyMPI.c               # Implementazione di funzioni di supporto per la programmazione con MPI
├── MyMPI.h               # Header di MyMPI.c
├── benchmark.c           # Codice per l'esecuzione di benchmark automatici dato il numero di iterazioni, di processi e grafo
├── gen_graph.c           # Generatore di grafi connessi Erdős–Rényi, dato il numero di vertici e il nome del file di output
└── parallel_boruvka.c    # Implementazione dell'algoritmo di Borůvka parallelo
```

---

## 🚀 Pipeline di Esecuzione

### 1. Prerequisiti e Installazione
L'algoritmo richiede le librerie **MPI** e **igraph**. Per installare igraph tramite Homebrew:
```bash
brew install igraph
```
* **Nota:** Assicurarsi che il Makefile includa i percorsi corretti per la libreria igraph.

### 2. Compilazione
Per procedere alla compilazione del progetto, eseguire all'interno della directory il comando:
```bash
make
```

### 3. Utilizzo e Configurazione
Per lanciare l'applicazione, utilizzare il seguente comando:
```bash
mpirun -np <p> ./parallel_boruvka <nome_file>
```
I parametri da configurare sono:
1. **`p`:** numero di processi MPI da utilizzare.
2. **`nome_file`:** percorso del file contenente il grafo (formato `.ncol`).
