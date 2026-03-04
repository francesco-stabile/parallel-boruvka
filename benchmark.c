#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

double mst_values[7] = {1457.00, 12266.0, 23786.0, 57506.0, 112589.0, 221664.0, 436398.0};
/// @brief Restituisce l'indice dell'array che contiene il valore dei Minimum spanning tree pre-calcolati dei grafi
/// @param num_vertex il numero di vertici presenti nel grafo
/// @return l'indice corrispondente dell'array mst_values per risalire al valore del MST del grafo con num_vertex, -1 se il valore per quel grafo non è stato calcolato
int find_mst_index(int num_vertex){
    switch (num_vertex)
    {
        case 1000:
            return 0;
            break;
        case 10000:
            return 1;
            break;
        case 20000:
            return 2;
            break;
        case 50000:
            return 3;
            break;
        case 100000:
            return 4;
            break;
        case 200000:
            return 5;
            break;
        case 400000:
            return 6;
            break;
        default:
            return -1;
            break;
    }
}

typedef struct {
    double MST;
    double iteration;
    double time;
} Stats;

/// @brief Cerca nella stringa il prefisso dato, salta il prefisso, toglie gli spazi e il simbolo uguale e i due punti e restituisce il valore
/// @param line la stringa in cui ricercare il prefisso
/// @param prefix il prefisso da cercare per poi saltarlo ed estrarre il valore
/// @return Valore del prefisso, -1 altrimenti
double extract_value(const char *line, const char *prefix) {
    const char *pos = strstr(line, prefix);
    if (pos == NULL) return -1.0;

    pos += strlen(prefix);
    while (*pos == ' ' || *pos == '=' || *pos == ':') pos++;

    char *end;
    double value = strtod(pos, &end);
    
    return value;
}

/*
Esegue l'algoritmo parallelo dato il numero di iterazioni, il numero di processori e il file su cui eseguirlo
Ottiene le statistiche per ciascuna iterazione e fa la media stampandole
*/

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Uso: %s <iterazioni> <num_processori> <nome_file>\n", argv[0]);
        return -1;
    }
    
    int iterations = atoi(argv[1]);
    char *num_procs = argv[2];
    char *filename = argv[3];
    int mst_index = -1;
    double mst_value = 0;
    
    Stats total_p = {0.0, 0.0, 0.0};
    Stats total_1 = {0.0, 0.0, 0.0};
    int valid_runs_1 = 0;
    int valid_runs_p = 0;
    int edges = 0, vertexes = 0;
    char program[24];
    printf("Esecuzione benchmark per %d volte...\n", iterations);
    printf("Comando: mpirun -np %s ./parallel_boruvka %s\n", num_procs, filename);
    
    // Su p processori:
    for (int run = 0; run < iterations; run++) {
        char command[512];
        snprintf(command, sizeof(command),"mpirun -np %s ./parallel_boruvka %s\n", num_procs, filename);
        
        // Apro una pipe per leggere l'output del comando dal terminale
        FILE *fp = popen(command, "r");
        if (fp == NULL) {
            printf("Errore nell'esecuzione del comando %s\n", command);
            continue;
        }
        
        Stats current_run = {-1.0, -1.0, -1.0};
        char line[1024];
        
        /*
        Legge l'output riga per riga e cerca le metriche
        Prendo il valore e lo salvo
        */ 
        while (fgets(line, sizeof(line), fp) != NULL) {
            if(run == 0){
                if(strstr(line, "V =") != NULL){
                    vertexes = extract_value(line, "V =");
                    mst_index = find_mst_index(vertexes);
                    if(mst_index == -1){
                        printf("Nessun valore MST pre-calcolato per questo grafo, correttezza non assicurata\n");
                    }else{
                        mst_value = mst_values[mst_index];
                    }
                }
                if(strstr(line, "E =") != NULL){
                    edges = extract_value(line, "E =");
                }
            }
            if(strstr(line, "Iterations =") != NULL){
                current_run.iteration = extract_value(line, "Iterations =");
            }
            if(strstr(line, "MST:") != NULL){
                current_run.MST = extract_value(line, "MST:");
            }
            if (strstr(line, "Time =") != NULL) {
                current_run.time = extract_value(line, "Time =");
            }
        }
        
        pclose(fp);
        
        // Verifica che tutti i valori siano stati estratti correttamente
        if (current_run.MST >=0 && current_run.iteration >= 0 && current_run.time >= 0) {
            
            total_p.MST += current_run.MST;
            total_p.iteration += current_run.iteration;
            total_p.time += current_run.time;
            valid_runs_p++;
            
        } else {
            printf("Attenzione: Alcune metriche non trovate nella run %d\n", run + 1);
        }
        
        usleep(100000); // 100ms
    }


    //Su 1 processore
    for (int run = 0; run < iterations; run++) {
        char command[512];
        snprintf(command, sizeof(command), "mpirun -np 1 ./parallel_boruvka %s\n", filename);
        
        // Apro una pipe per leggere l'output del comando dal terminale
        FILE *fp = popen(command, "r");
        if (fp == NULL) {
            printf("Errore nell'esecuzione del comando %s\n", command);
            continue;
        }
        
        Stats current_run = {-1.0, -1.0, -1.0};
        char line[1024];
        
        /*
        Legge l'output riga per riga e cerca le metriche
        Prendo il valore e lo salvo
        */ 
        while (fgets(line, sizeof(line), fp) != NULL) {
            if(strstr(line, "Iterations =") != NULL){
                current_run.iteration = extract_value(line, "Iterations =");
            }
            if(strstr(line, "MST:") != NULL){
                current_run.MST = extract_value(line, "MST:");
            }
            if (strstr(line, "Time =") != NULL) {
                current_run.time = extract_value(line, "Time =");
            }
        }
        
        pclose(fp);
        
        // Verifica che tutti i valori siano stati estratti correttamente
        if (current_run.MST >=0 && current_run.iteration >= 0&& current_run.time >= 0) {
            
            total_1.MST += current_run.MST;
            total_1.iteration += current_run.iteration;
            total_1.time += current_run.time;
            valid_runs_1++;
        } else {
            printf("Attenzione: Alcune metriche non trovate nell'esecuzione %d\n", run + 1);
        }
        
        usleep(100000); // 100ms
    }
    
    // Calcola e stampa le medie
    if (valid_runs_1 > 0 && valid_runs_p > 0) {
        double MST_p = total_p.MST / valid_runs_p;
        double MST_1 = total_1.MST / valid_runs_1;
        printf("RISULTATI DEL BENCHMARK 1-%s (esecuzioni valide %d - %d):\n", num_procs, valid_runs_1, valid_runs_p);
        printf("V = %d   E = %d\n\n", vertexes, edges);
        printf("Media Iterazioni:  %2.2f - %2.2f iterazioni\n", total_1.iteration / valid_runs_1, total_p.iteration / valid_runs_p);
        printf("Media Time:      %10.6f - %10.6f secondi\n", total_1.time / valid_runs_1, total_p.time / valid_runs_p);
        if (MST_p != MST_1){
            printf("Algoritmo non corretto!\n");
        }
        else{
            if(mst_index != -1){
                if(mst_value == MST_1) printf("Algoritmo corretto!\n");
                else printf("Algoritmo errato!\n");
            }
        }
        printf("MST:   %10.1f - %10.1f\n", MST_1, MST_p);
        printf("MST vero: %7.1f\n", mst_value);
        double avg_time_p = total_p.time / valid_runs_p;
        double avg_time_1 = total_1.time / valid_runs_1;
        printf("Avg. Speedup 1-%s : %10.6f\n", num_procs, avg_time_1 / avg_time_p);
        printf("Efficienza relativa 1-%s : %3.2f%%\n", num_procs, (avg_time_1 / (avg_time_p * atoi(num_procs))) * 100);
    } else {
        printf("Errore: Nessuna esecuzione valida completata\n");
        return -1;
    }
    
    return 0;
}