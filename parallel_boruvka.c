/*include <igraph.h>
#include <mpi.h>
#include <stdio.h>


int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    printf("Hello from process %d of %d\n", rank, size);
    
    MPI_Finalize();
    return 0;
}*/
#include <mpi.h>
#include "MyMPI.h"
#include <igraph/igraph.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef struct UF
{
    igraph_int_t rank;
    igraph_int_t parent;
}UnionFind;

void init_union_find(UnionFind* uf, igraph_int_t n_vertex, igraph_int_t* num_components){
    for (int i = 0; i < n_vertex; i++)
    {
        uf[i].parent = i;
        uf[i].rank = 0;
    }
    *num_components = n_vertex;
}

igraph_int_t find(UnionFind* uf, igraph_int_t vertex){
    if(uf[vertex].parent == vertex){
        return vertex;
    }
    uf[vertex].parent = find(uf, uf[vertex].parent);
    return uf[vertex].parent;
}

int merge_components(UnionFind* uf, igraph_int_t v1, igraph_int_t v2, igraph_int_t* num_components){
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

typedef struct
{
    igraph_int_t from;
    igraph_int_t to;
    igraph_real_t weight;
}Edge;

typedef struct MP
{
    igraph_int_t vertex1;
    igraph_int_t vertex2;
}MergePairs;

MPI_Datatype MPI_EDGE;
MPI_Datatype MPI_MERGE_PAIRS;
MPI_Op MPI_MIN_EDGE;

void create_MPI_Datatype_struct_edge(){
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

void create_MPI_Datatype_struct_merge_pairs(){
    MPI_Datatype typelist[2];
    int block_lengths[2];
    MPI_Aint displacements[2];
    MPI_Aint start_address;
    MPI_Aint address;
    MergePairs mp;
    block_lengths[0] = block_lengths[1] = 1; 
    typelist[0] = MPI_INT64_T;
    typelist[1] = MPI_INT64_T;
    MPI_Get_address(&(mp.vertex1), &start_address);
    displacements[0] = MPI_Aint_diff(start_address, start_address);
    MPI_Get_address(&(mp.vertex2), &address);
    displacements[1] = MPI_Aint_diff(address, start_address);
    MPI_Type_create_struct(2, block_lengths, displacements, typelist, &MPI_MERGE_PAIRS); 
    MPI_Type_commit(&MPI_MERGE_PAIRS);
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

void find_min_edge(void* edge_in, void* edge_inout, int* len, MPI_Datatype* dptr){
    for (int i=0; i < *len; i++){
        if(((Edge*)edge_in)[i].weight < ((Edge*)edge_inout)[i].weight){
            ((Edge*)edge_inout)[i].weight = ((Edge*)edge_in)[i].weight;
            ((Edge*)edge_inout)[i].from = ((Edge*)edge_in)[i].from;
            ((Edge*)edge_inout)[i].to = ((Edge*)edge_in)[i].to;
        }
        /*
        edge_inout[i] rimane uguale
        */
    }
}

int main(int argc, char* argv[]) {
    int id, p;
    double seconds;
    double max_seconds;
    MPI_Init (&argc, &argv);
    MPI_Comm_rank (MPI_COMM_WORLD, &id);
    MPI_Comm_size (MPI_COMM_WORLD, &p);
    
    igraph_set_attribute_table(&igraph_cattribute_table);
    igraph_setup();
    igraph_t graph;
    igraph_int_t num_vertex, num_edges;
    Edge* global_edges = NULL, *MST = NULL;
    if(argc != 2){
        printf("Errore, utilizzo: ./parallel_boruvka <nome_file>\n");
        fflush(stdout);
        MPI_Abort(MPI_COMM_WORLD, -1);
    }
    if(strchr(argv[1], '.') == NULL){
        printf("Nome file non valido, inserire l'estensione\n");
        fflush(stdout);
        MPI_Abort(MPI_COMM_WORLD, -2);
    }
    FILE* fp;
    fp = fopen(argv[1], "r");
    if(fp == NULL){
        printf("Errore apertura file\n");
        fflush(stdout);
        MPI_Abort(MPI_COMM_WORLD, -3);
    }
    if(!id){
        igraph_error_t error = igraph_read_graph_ncol(&graph, fp, NULL, false, IGRAPH_ADD_WEIGHTS_YES, IGRAPH_UNDIRECTED);
        if(error == IGRAPH_PARSEERROR){
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
        igraph_vector_int_init(&vertex_pairs, 2*num_edges);
        igraph_edges(&graph, igraph_ess_all(IGRAPH_EDGEORDER_ID), &vertex_pairs, 0);
        igraph_vector_init(&global_weights, num_edges);
        igraph_cattribute_EANV(&graph, "weight", igraph_ess_all(IGRAPH_EDGEORDER_ID), &global_weights);
        //igraph_es_as_vector(&graph, igraph_ess_all(IGRAPH_EDGEORDER_ID), &vertex_pairs); problema, mi dà solo gli eid
        global_edges = calloc(num_edges, sizeof(Edge));
        if (global_edges == NULL){
            printf("Errore, memoria non sufficiente\n");
            MPI_Abort(MPI_COMM_WORLD, -1);
        }
        for (int i = 0; i < num_edges; i ++)
        {
            global_edges[i].from = VECTOR(vertex_pairs)[2*i];
            global_edges[i].to = VECTOR(vertex_pairs)[2*i+1];
            global_edges[i].weight = VECTOR(global_weights)[i];
        }
        igraph_vector_int_destroy(&vertex_pairs);
        igraph_vector_destroy(&global_weights);
        igraph_destroy(&graph);
    }
    igraph_int_t stats[2];
    if(!id){
        stats[0] = num_vertex;
        stats[1] = num_edges;
    }
    MPI_Bcast(stats, 2, MPI_INT64_T, 0, MPI_COMM_WORLD);
    num_vertex = stats[0];
    num_edges = stats[1];
    int* count = NULL, *disp = NULL;
    create_mixed_xfer_arrays(id, p, num_edges, &count, &disp);
    //igraph_vector_int_init(&local_edges, BLOCK_SIZE(id, p, 2*num_edges));
    Edge* local_edges = calloc(BLOCK_SIZE(id, p, num_edges), sizeof(Edge));
    if (local_edges == NULL)    {
        printf("Errore allocazione local_edges\n");
        MPI_Abort(MPI_COMM_WORLD, -1);
    }
    create_MPI_Datatype_struct_edge();
    create_MPI_Datatype_struct_merge_pairs();
    MPI_Op_create(find_min_edge, 1, &MPI_MIN_EDGE);
    MPI_Scatterv(global_edges, count, disp, MPI_EDGE, local_edges, BLOCK_SIZE(id, p, num_edges), MPI_EDGE, 0, MPI_COMM_WORLD);
    free(count); free(disp);
    if(!id){
        free(global_edges);
    }
    UnionFind* global_union_find = calloc(num_vertex, sizeof(UnionFind));
    if(global_union_find == NULL){
        printf("Memoria non sufficiente per la Union Find\n");
        fflush(stdout);
        free(local_edges);
        MPI_Abort(MPI_COMM_WORLD, -1);
    }
    igraph_int_t global_num_components;
    init_union_find(global_union_find, num_vertex, &global_num_components);
    // for (int i = 0; i < BLOCK_SIZE(id, p, num_edges); i ++)
    // {
    //     printf("%d) Ho l'arco: %lld %lld %.0f\n", id, local_edges[i].from, local_edges[i].to, local_edges[i].weight);
    // }
    Edge* local_min = calloc(global_num_components, sizeof(Edge));
    if (local_min == NULL){
        printf("Memoria non sufficiente per local_min\n");
        fflush(stdout);
        free(local_edges);
        free(global_union_find);
        MPI_Abort(MPI_COMM_WORLD, -1);
    }
    Edge* global_min = calloc(global_num_components, sizeof(Edge));
    if (global_min == NULL){
        printf("Memoria non sufficiente per global_min\n");
        fflush(stdout);
        free(local_edges);
        free(local_min);
        free(global_union_find);
        MPI_Abort(MPI_COMM_WORLD, -1);
    }
    igraph_int_t* mapping = calloc(num_vertex, sizeof(igraph_int_t));
    if (mapping == NULL){
        printf("Memoria non sufficiente per mapping\n");
        fflush(stdout);
        free(local_edges);
        free(local_min);
        free(global_union_find);
        free(global_min);
        MPI_Abort(MPI_COMM_WORLD, -1);
    }
    igraph_int_t local_num_edges = BLOCK_SIZE(id, p, num_edges);
    MergePairs* mp = calloc(global_num_components, sizeof(MergePairs));
    if(mp == NULL){
        printf("Memoria non sufficiente per la struttura Merge Pairs\n");
        fflush(stdout);
        free(local_edges);
        free(local_min);
        free(global_union_find);
        free(mapping);
        MPI_Abort(MPI_COMM_WORLD, -1);
    }
    if(!id){
        MST = calloc(num_vertex-1, sizeof(Edge));
        if(MST == NULL){
            printf("Memoria non sufficiente per l'MST\n");
            fflush(stdout);
            free(local_edges);
            free(local_min);
            free(global_union_find);
            MPI_Abort(MPI_COMM_WORLD, -1);
        }
    }
    int MST_count = 0;

    seconds = - MPI_Wtime();
    MPI_Barrier(MPI_COMM_WORLD);
    while (global_num_components > 1){
        igraph_int_t active_count = 0;
        for (igraph_int_t i = 0; i < num_vertex; i++){
            if(global_union_find[i].parent == i){
                mapping[i] = active_count;
                active_count++;
            }
        }
        for (igraph_int_t i = 0; i < global_num_components; i++){
            local_min[i].weight = IGRAPH_INTEGER_MAX;
        }
        igraph_int_t active_edges = 0;
        for (int i = 0; i < local_num_edges; i++){
            igraph_int_t comp_from, comp_to;
            comp_from = find(global_union_find, local_edges[i].from);
            comp_to = find(global_union_find, local_edges[i].to);
            comp_from = mapping[comp_from];
            comp_to = mapping[comp_to];
            if (comp_from != comp_to){
                if (local_edges[i].weight < local_min[comp_from].weight){
                    local_min[comp_from].from = local_edges[i].from;
                    local_min[comp_from].to = local_edges[i].to;
                    local_min[comp_from].weight = local_edges[i].weight;
                }
                if (local_edges[i].weight < local_min[comp_to].weight){
                    local_min[comp_to].from = local_edges[i].from;
                    local_min[comp_to].to = local_edges[i].to;
                    local_min[comp_to].weight = local_edges[i].weight;
                }
                local_edges[active_edges].from = local_edges[i].from;
                local_edges[active_edges].to = local_edges[i].to;
                local_edges[active_edges].weight = local_edges[i].weight;
                active_edges++;
            }
        }
        local_num_edges = active_edges;
        
        MPI_Reduce(local_min, global_min, global_num_components, MPI_EDGE, MPI_MIN_EDGE, 0, MPI_COMM_WORLD);
        igraph_int_t num_merge = 0;
        if(!id){
            for (int i = 0; i < global_num_components; i++){
                if(global_min[i].weight < IGRAPH_INTEGER_MAX){
                    int merged = merge_components(global_union_find, global_min[i].from, global_min[i].to, &global_num_components);
                    if(merged){
                        MST[MST_count].from = global_min[i].from;
                        MST[MST_count].to = global_min[i].to;
                        MST[MST_count].weight = global_min[i].weight;
                        MST_count++;
                        mp[num_merge].vertex1 = global_min[i].from;
                        mp[num_merge].vertex2 = global_min[i].to;
                        num_merge++;
                    }
                }
            }
        }
        MPI_Bcast(&num_merge, 1, MPI_INT64_T, 0, MPI_COMM_WORLD);
        MPI_Bcast(mp, num_merge, MPI_MERGE_PAIRS, 0, MPI_COMM_WORLD);
        if(id){
            for (igraph_int_t i = 0; i < num_merge; i++){
                merge_components(global_union_find, mp[i].vertex1, mp[i].vertex2, &global_num_components);
            }
        }
    }

    seconds += MPI_Wtime();
    MPI_Reduce(&seconds, &max_seconds, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    if (!id){
        igraph_real_t total_weight = 0;
        for (int i = 0; i < num_vertex -1; i++){
            total_weight += MST[i].weight;
            if(num_vertex < 20){
                printf("Arco: %lld, %lld\n", MST[i].from, MST[i].to);
            }
        }
        printf("Peso totale MST: %.1f\n", total_weight);
        free(MST);
        printf ("Parallel_Boruvka) V = %lld, E = %lld, Processes = %d, Time =%12.6f sec\n", num_vertex, num_edges, p, max_seconds);
    }
    // igraph_int_t num_edges = igraph_ecount(&graph);
    // igraph_int_t p_num_edges = num_edges / p;
    // igraph_int_t start_edge_id = id*p_num_edges;
    // igraph_int_t last_edge_id = (id+1)*p_num_edges -1;
    // while (global_num_components > 1){
    //     igraph_vector_int_t local_min;
    //     igraph_vector_int_init(&local_min, global_num_components);
    //     for (igraph_int_t i = 0; i < global_num_components; i++){
    //         VECTOR(local_min)[i] = IGRAPH_INTEGER_MAX;
    //     }
    //     for (igraph_int_t i = 0; i < p_num_edges; i++){
    //         igraph_int_t from, to;
    //         igraph_int_t eid = start_edge_id + i;
    //         igraph_edge(&graph, eid, &from, &to);
    //         igraph_int_t comp_from, comp_to;
    //         comp_from = find(global_union_find, from);
    //         comp_to = find(global_union_find, to);
    //         if(comp_from != comp_to){
    //             /*
    //             if(peso(eid) < peso(VECTOR(local_min)[comp_from]))
    //                 VECTOR(local_min)[comp_from] = eid;
    //             if(peso(eid) < peso(VECTOR(local_min)[comp_to]))
    //                 VECTOR(local_min)[comp_to] = eid;
    //             */
    //         }
    //     }
    //     //MPI_Allreduce(&local_min, &global_min, global_num_components, MPI_MIN, MPI_COMM_WORLD);
    //     igraph_vector_int_destroy(&local_min);
    //     /*
    //     for each arco in global min
    //         prendi dall'arco from e to
    //         merge_components(from, to, &global_num_components)
    //     */
    // }
    free(mp);
    free(mapping);
    free(global_min);
    free(local_min);
    free(global_union_find);
    free(local_edges);
    MPI_Type_free(&MPI_EDGE);
    MPI_Op_free(&MPI_MIN_EDGE);
    MPI_Finalize();
    return 0;
}

