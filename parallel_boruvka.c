#include <mpi.h>
#include "MyMPI.h"
#include <igraph/igraph.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
Definisco la struttura Union Find e i relativi metodi
*/

typedef struct UF {
    igraph_int_t rank;
    igraph_int_t parent;
} UnionFind;


/// @brief Inizializza la struttura Union-Find e imposta tutti i vertici con radice pari al vertice e rank pari a 0
/// @param uf la struttura Union-Find da inizializzare
/// @param n_vertex numero di vertici della struttura
/// @param num_components numero di componenti
void init_union_find(UnionFind* uf, igraph_int_t n_vertex, igraph_int_t* num_components) {
    for (int i = 0; i < n_vertex; i++) {
        uf[i].parent = i;
        uf[i].rank = 0;
    }
    *num_components = n_vertex;
}

/// @brief Trova la componente di appartenenza del vertice risalendo la struttura fino a trovare la radice
/// @param uf la struttura Union-Find
/// @param vertex il vertice di cui si vuole trovare la radice
/// @return la radice della componente di appartenenza
igraph_int_t find(UnionFind* uf, igraph_int_t vertex) {
    if(uf[vertex].parent == vertex){
        return vertex;
    }
    uf[vertex].parent = find(uf, uf[vertex].parent);
    return uf[vertex].parent;
}

/// @brief Fonde le componenti mantenendo l'altezza della Union-Find minore possibile
/// @param uf la struttura Union-Find
/// @param v1 primo vertice
/// @param v2 secondo vertice
/// @param num_components nuemro di componenti attuale
/// @return 1 se il merge va a buon fine, 0 altrimenti. Inoltre imposta num_components al nuovo valore.
int merge_components(UnionFind* uf, igraph_int_t v1, igraph_int_t v2, igraph_int_t* num_components) {
    int r1 = find(uf, v1);
    int r2 = find(uf, v2);
    if(r1 != r2){
        if(uf[r1].rank < uf[r2].rank){
            uf[r1].parent = r2;
        }
        else if(uf[r1].rank > uf[r2].rank){
            uf[r2].parent = r1;
        }
        else{
            uf[r2].parent = r1;
            uf[r1].rank += 1;
        }
        *num_components -= 1;
        return 1;
    }
    return 0;
}

/*
Definisco com'è fatto un arco:
Vertice sorgente - Vertice Destinazione e peso
*/

typedef struct {
    igraph_int_t from;
    igraph_int_t to;
    igraph_real_t weight;
} Edge;

MPI_Datatype MPI_EDGE;
MPI_Op MPI_MIN_EDGE;

/*
Creo il dato personalizzato Edge per MPI, servirà per la AllReduce
*/

void create_MPI_Datatype_struct_edge() {
    MPI_Datatype typelist[3];
    int block_lengths[3];
    MPI_Aint displacements[3];
    MPI_Aint start_address;
    MPI_Aint address;
    Edge e;
    block_lengths[0] = block_lengths[1] = block_lengths[2] = 1;
    typelist[0] = MPI_INT64_T;
    typelist[1] = MPI_INT64_T;
    typelist[2] = MPI_DOUBLE;
    MPI_Get_address(&(e.from), &start_address);
    displacements[0] = MPI_Aint_diff(start_address, start_address);
    MPI_Get_address(&(e.to), &address);
    displacements[1] = MPI_Aint_diff(address, start_address);
    MPI_Get_address(&(e.weight), &address);
    displacements[2] = MPI_Aint_diff(address, start_address);
    MPI_Type_create_struct(3, block_lengths, displacements, typelist, &MPI_EDGE);
    MPI_Type_commit(&MPI_EDGE);
}

/*Chapter 4 slide 58*/
/*void find_min_edge(Edge* edge_in, Edge* edge_inout, int* len, MPI_Datatype* dptr){
    for (int i=0; i < *len; i++){
        if(edge_in[i].weight < edge_inout[i].weight){
            edge_inout[i].weight = edge_in[i].weight;
            edge_inout[i].from = edge_in[i].from;
            edge_inout[i].to = edge_in[i].to;
        }
        //edge_inout[i] rimane uguale
    }
}*/

/*
Definisco l'operazione da usare nella AllReduce
Controllo per ciascuna cella dell'array l'arco con il peso minimo.
L'algoritmo di Boruvka vuole una regola di spareggio:
Se il peso è uguale, allora prendo l'arco con il vertice sorgente minimo
Se anche il vertice sorgente è uguale, allora prendo l'arco con il vertice di destinazione minimo
*/

void find_min_edge(void* edge_in, void* edge_inout, int* len, MPI_Datatype* dptr) {
    Edge* e_in = (Edge*) edge_in;
    Edge* e_out = (Edge*) edge_inout;
    for (int i = 0; i < *len; i++) {
        if (e_in[i].weight < e_out[i].weight) {
            e_out[i] = e_in[i];
        }
        else if (e_in[i].weight == e_out[i].weight && e_in[i].weight < IGRAPH_INFINITY) {
            if (e_in[i].from < e_out[i].from) {
                e_out[i] = e_in[i];
            } else if (e_in[i].from == e_out[i].from && e_in[i].to < e_out[i].to) {
                e_out[i] = e_in[i];
            }
        }
    }
}

int main(int argc, char* argv[]) {
    /*
    Inizializzo le variabili per MPI e igraph
    Controllo eventuali errori nei parametri passati a linea di comando
    */
    int id, p;
    double seconds;
    double max_seconds;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &id);
    MPI_Comm_size(MPI_COMM_WORLD, &p);
    
    igraph_set_attribute_table(&igraph_cattribute_table);
    igraph_setup();
    igraph_t graph;
    igraph_int_t num_vertex, num_edges;
    Edge* global_edges = NULL, *MST = NULL;
    
    if (argc != 2) {
        if (!id) printf("Errore, utilizzo: ./parallel_boruvka <nome_file>\n");
        fflush(stdout);
        MPI_Abort(MPI_COMM_WORLD, -1);
    }
    if (strchr(argv[1], '.') == NULL) {
        if (!id) printf("Nome file non valido, inserire l'estensione\n");
        fflush(stdout);
        MPI_Abort(MPI_COMM_WORLD, -2);
    }
    
    FILE* fp;
    fp = fopen(argv[1], "r");
    if (fp == NULL) {
        if (!id) printf("Errore apertura file\n");
        fflush(stdout);
        MPI_Abort(MPI_COMM_WORLD, -3);
    }
    
    /*
    Fase di I/O, leggo con igraph_read_graph_ncol il grafo
    Creo l'array contenente tutti gli archi del grafo
    Invio il numero di archi e il numero di vertici totali
    Tutti i processi allocano la memoria necessaria
    Effettuo la Scatterv.
    */
    if (!id) {
        igraph_error_t error = igraph_read_graph_ncol(&graph, fp, NULL, false, IGRAPH_ADD_WEIGHTS_YES, IGRAPH_UNDIRECTED);
        if (error == IGRAPH_PARSEERROR) {
            printf("Errore lettura file\n");
            fflush(stdout);
            fclose(fp);
            MPI_Abort(MPI_COMM_WORLD, -4);
        }
        fclose(fp);
        igraph_vector_int_t vertex_pairs;
        igraph_vector_t global_weights;
        num_vertex = igraph_vcount(&graph);
        num_edges = igraph_ecount(&graph);
        igraph_vector_int_init(&vertex_pairs, 2 * num_edges);
        igraph_edges(&graph, igraph_ess_all(IGRAPH_EDGEORDER_ID), &vertex_pairs, 0);
        igraph_vector_init(&global_weights, num_edges);
        igraph_cattribute_EANV(&graph, "weight", igraph_ess_all(IGRAPH_EDGEORDER_ID), &global_weights);
        
        global_edges = calloc(num_edges, sizeof(Edge));
        if (global_edges == NULL) {
            printf("Errore, memoria non sufficiente\n");
            MPI_Abort(MPI_COMM_WORLD, -1);
        }
        for (int i = 0; i < num_edges; i++) {
            global_edges[i].from = VECTOR(vertex_pairs)[2 * i];
            global_edges[i].to = VECTOR(vertex_pairs)[2 * i + 1];
            global_edges[i].weight = VECTOR(global_weights)[i];
        }
        // /*
        // Mescolamento Archi con algoritmo Fisher Yates
        // Simulo un grafo in input con ordinamento casuale degli archi
        // */
        // for (igraph_int_t i = num_edges - 1; i > 0; i--) {
        //     igraph_int_t j = rand() % (i + 1);
        //     Edge temp = global_edges[i];
        //     global_edges[i] = global_edges[j];
        //     global_edges[j] = temp;
        // }
        igraph_vector_int_destroy(&vertex_pairs);
        igraph_vector_destroy(&global_weights);
        igraph_destroy(&graph);
    }
    
    igraph_int_t stats[2];
    if (!id) {
        stats[0] = num_vertex;
        stats[1] = num_edges;
    }
    MPI_Bcast(stats, 2, MPI_INT64_T, 0, MPI_COMM_WORLD);
    num_vertex = stats[0];
    num_edges = stats[1];
    
    int* count = NULL, *disp = NULL;
    create_mixed_xfer_arrays(id, p, num_edges, &count, &disp);
    igraph_int_t local_num_edges = BLOCK_SIZE(id, p, num_edges);
    Edge* local_edges = calloc(local_num_edges, sizeof(Edge));
    if (local_edges == NULL) {
        printf("Errore allocazione local_edges\n");
        MPI_Abort(MPI_COMM_WORLD, -1);
    }
    create_MPI_Datatype_struct_edge();
    MPI_Op_create(find_min_edge, 1, &MPI_MIN_EDGE);
    MPI_Scatterv(global_edges, count, disp, MPI_EDGE, local_edges, BLOCK_SIZE(id, p, num_edges), MPI_EDGE, 0, MPI_COMM_WORLD);
    free(count);
    free(disp);
    if (!id) {
        free(global_edges);
    }
    
    /*
    Inizializzo tutte le strutture ausiliarie
    Union-Find per trovare le componenti di appartenenza dei vertici
    Local_min per gli archi minimi locali trovati
    Mapping per avere indici delle componenti da 0 a numero componenti -1 e ridurre peso di AllReduce
    MST per salvare gli archi del Minimum Spanning Tree (solo su processo 0)
    */

    UnionFind* global_union_find = calloc(num_vertex, sizeof(UnionFind));
    if (global_union_find == NULL) {
        printf("Memoria non sufficiente per la Union Find\n");
        fflush(stdout);
        free(local_edges);
        MPI_Abort(MPI_COMM_WORLD, -1);
    }
    
    igraph_int_t global_num_components;
    init_union_find(global_union_find, num_vertex, &global_num_components);
    
    Edge* local_min = calloc(num_vertex, sizeof(Edge));
    if (local_min == NULL) {
        printf("Memoria non sufficiente per local_min\n");
        fflush(stdout);
        free(local_edges);
        free(global_union_find);
        MPI_Abort(MPI_COMM_WORLD, -1);
    }
    
    igraph_int_t* mapping = malloc(num_vertex * sizeof(igraph_int_t));
    if (mapping == NULL) {
        printf("Memoria non sufficiente per l'array mapping\n");
        fflush(stdout);
        free(local_edges);
        free(local_min);
        free(global_union_find);
        MPI_Abort(MPI_COMM_WORLD, -1);
    }
    
    if (!id) {
        MST = calloc(num_vertex - 1, sizeof(Edge));
        if (MST == NULL) {
            printf("Memoria non sufficiente per l'MST\n");
            fflush(stdout);
            free(local_edges);
            free(local_min);
            free(global_union_find);
            MPI_Abort(MPI_COMM_WORLD, -1);
        }
    }
    int MST_count = 0;

    int iteration = 0;
    igraph_real_t total_weight = 0;
    MPI_Barrier(MPI_COMM_WORLD);
    seconds = -MPI_Wtime();
    while (global_num_components > 1) {
        iteration++;

        /*
        Se global_union_find[i].parent == i allora i è una componente
        Effettuo la mappatura, mapping[i] = nuovo indice compreso tra [0-numero componenti -1]
        Effettuo il reset di local_min
        */
        igraph_int_t active_count = 0;
        for (igraph_int_t i = 0; i < num_vertex; i++) {
            if (global_union_find[i].parent == i) {
                mapping[i] = active_count;
                local_min[active_count].weight = IGRAPH_INFINITY;
                active_count++;
            }
        }
        
        /*
        Ricerca negli archi assegnati dei minimi locali
        All'inizio ogni vertice è una componente
        Successivamente sovrascrivo i vertici con la loro componente
        Se le componenti sono diverse, allora l'arco potrebbe essere valido
        Si controlla se è il minimo attuale per entrambe le componenti (from e to)
        */
        for (igraph_int_t i = 0; i < local_num_edges; i++) {
            igraph_int_t comp_from = local_edges[i].from;
            igraph_int_t comp_to = local_edges[i].to;
            if (comp_from != comp_to) {
                comp_from = mapping[comp_from];
                comp_to = mapping[comp_to];
                if (local_edges[i].weight < local_min[comp_from].weight) {
                    local_min[comp_from] = local_edges[i];
                }
                if (local_edges[i].weight < local_min[comp_to].weight) {
                    local_min[comp_to] = local_edges[i];
                }
            }
        }
        
        /*
        Allreduce con MPI_IN_PLACE per salvare spazio
        Allreduce eseguita active_count volte grazie al mapping
        Tutti avranno i minimi globali e procedono con la fase di merge
        Solo il processo 0 aggiorna l'MST se c'è stata una fusione
        */
        MPI_Allreduce(MPI_IN_PLACE, local_min, active_count, MPI_EDGE, MPI_MIN_EDGE, MPI_COMM_WORLD);
        for (igraph_int_t i = 0; i < active_count; i++) {
            if (local_min[i].weight < IGRAPH_INFINITY) {
                int merged = merge_components(global_union_find, local_min[i].from, local_min[i].to, &global_num_components);
                if (merged && !id) {
                    MST[MST_count] = local_min[i];
                    MST_count++;
                    total_weight += local_min[i].weight;
                }
            }
        }
        
        /*
        Si eliminano gli archi locali interni alle componenti
        Dopo le fusioni delle componenti, si sovrascrive, per ciascun arco, le radici delle componenti sui vertici
        Se le due componenti sono diverse, allora l'arco può essere ancora utile nella prossima iterazione
        Sovrascrivo il numero di archi che il processo possiede con il numero di archi attivi trovati
        */
        igraph_int_t active_edge = 0;
        for (igraph_int_t i = 0; i < local_num_edges; i++) {
            local_edges[i].from = find(global_union_find, local_edges[i].from);
            local_edges[i].to = find(global_union_find, local_edges[i].to);
            
            if (local_edges[i].from != local_edges[i].to) {
                local_edges[active_edge] = local_edges[i];
                active_edge++;
            }
        }

        local_num_edges = active_edge;
    }
    MPI_Barrier(MPI_COMM_WORLD);
    seconds += MPI_Wtime();
    MPI_Reduce(&seconds, &max_seconds, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    
    /*
    Il processo 0 detiene il MST finale e lo stampa
    */
    if (!id) {
        if (num_edges < 20) {
            for (int i = 0; i < num_vertex - 1; i++) {
                printf("Arco: %lld, %lld\n", MST[i].from, MST[i].to);
            }
        }
        printf("Peso totale MST: %.1f\n", total_weight);
        printf("Parallel_Boruvka) V = %lld, E = %lld, Processes = %d, Iterations = %d, Time = %12.6f sec\n",
               num_vertex, num_edges, p, iteration, max_seconds);
       free(MST);
    }


    //Liberazione delle risorse allocate.
    free(local_min);
    free(global_union_find);
    free(local_edges);
    free(mapping);
    MPI_Type_free(&MPI_EDGE);
    MPI_Op_free(&MPI_MIN_EDGE);
    MPI_Finalize();
    return 0;
}