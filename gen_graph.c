#include <igraph/igraph.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>

int main(int argc, char const *argv[])
{
    if(argc != 4){
        printf("Errore, utilizzo: ./gen_graph <numero_vertici> <nome_file> <peso_max>\n");
        fflush(stdout);
        return -1;
    }
    igraph_int_t n = atoi(argv[1]);
    if(!n){
        printf("Errore conversione numero vertici oppure numero vertici = 0\n");
        fflush(stdout);
        return -2;
    }
    if(strchr(argv[2], '.') != NULL){
        printf("Nome file non valido, inserire solo il nome, non l'estensione\n");
        fflush(stdout);
        return -3;
    }
    int max_weight = atoi(argv[3]);
    if(!max_weight){
        printf("Errore conversione numero vertici oppure peso massimo = 0\n");
        fflush(stdout);
        return -4;
    }
    FILE* fp;
    char* filename = calloc(strlen(argv[2]) + 6, sizeof(char));
    if(filename == NULL){
        printf("Errore, memoria non sufficiente per il nome del file\n");
        fflush(stdout);
        return -5;
    }
    snprintf(filename, strlen(argv[2]) + 6, "%s.ncol", argv[2]);
    fp = fopen(filename, "w");
    if(fp == NULL){
        printf("Errore apertura file\n");
        fflush(stdout);
        return -6;
    }
    igraph_set_attribute_table(&igraph_cattribute_table);
    igraph_setup();
    igraph_real_t p = 2 * log(n) / n;
    igraph_t graph;
    igraph_error_t error = igraph_erdos_renyi_game_gnp(&graph, n, p, IGRAPH_UNDIRECTED, IGRAPH_SIMPLE_SW, IGRAPH_EDGE_UNLABELED);
    if(error == IGRAPH_ENOMEM){
        printf("Errore, memoria non sufficiente per il grafo\n");
        fclose(fp);
        free(filename);
        return -7;
    }
    igraph_bool_t connected;
    igraph_is_connected(&graph, &connected, IGRAPH_WEAK);
    if(!connected){
        printf("Errore, il grafo generato non è connesso, riprova\n");
        fflush(stdout);
        igraph_destroy(&graph);
        fclose(fp);
        free(filename);
        return -8;
    }
    igraph_vector_t weights;
    igraph_int_t num_edges = igraph_ecount(&graph);
    igraph_vector_init(&weights, num_edges);
    srand(time(NULL));
    for (igraph_int_t i = 0; i < num_edges; i++)
    {
        VECTOR(weights)[i] = rand() % (max_weight) +1;
    }
    igraph_cattribute_EAN_setv(&graph, "weight", &weights);
    error = igraph_write_graph_ncol(&graph, fp, NULL, "weight");
    if(error == IGRAPH_EFILE){
        printf("Errore nella scrittura del grafo nel file, riprova\n");
        fflush(stdout);
        igraph_destroy(&graph);
        igraph_vector_destroy(&weights);
        fclose(fp);
        free(filename);
        return -9;
    }
    printf("Grafo generato correttamente in %s\n", filename);
    fflush(stdout);
    free(filename);
    fclose(fp);
    igraph_vector_destroy(&weights);
    igraph_destroy(&graph);
    return 0;
}
