#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdbool.h>

typedef struct block {
    bool valid_bit;
    bool dirty_bit;
    uint32_t tag;
    uint32_t addr;
} cache_block;
/***************************************************************/
/*                                                             */
/* Procedure : cdump                                           */
/*                                                             */
/* Purpose   : Dump cache configuration                        */
/*                                                             */
/***************************************************************/
void cdump(int capacity, int assoc, int blocksize) {
    printf("Cache Configuration:\n");
    printf("-------------------------------------\n");
    printf("Capacity: %dB\n", capacity);
    printf("Associativity: %dway\n", assoc);
    printf("Block Size: %dB\n", blocksize);
    printf("\n");
}

/***************************************************************/
/*                                                             */
/* Procedure : sdump                                           */
/*                                                             */
/* Purpose   : Dump cache stat		                       */
/*                                                             */
/***************************************************************/
void sdump(int total_reads, int total_writes, int write_backs,
           int reads_hits, int write_hits, int reads_misses, int write_misses) {
    printf("Cache Stat:\n");
    printf("-------------------------------------\n");
    printf("Total reads: %d\n", total_reads);
    printf("Total writes: %d\n", total_writes);
    printf("Write-backs: %d\n", write_backs);
    printf("Read hits: %d\n", reads_hits);
    printf("Write hits: %d\n", write_hits);
    printf("Read misses: %d\n", reads_misses);
    printf("Write misses: %d\n", write_misses);
    printf("\n");
}

/***************************************************************/
/*                                                             */
/* Procedure : xdump                                           */
/*                                                             */
/* Purpose   : Dump current cache state                        */
/* 							       */
/* Cache Design						       */
/*  							       */
/* 	    cache[set][assoc][word per block]		       */
/*      						       */
/*      						       */
/*       ----------------------------------------	       */
/*       I        I  way0  I  way1  I  way2  I                 */
/*       ----------------------------------------              */
/*       I        I  word0 I  word0 I  word0 I                 */
/*       I  set0  I  word1 I  word1 I  work1 I                 */
/*       I        I  word2 I  word2 I  word2 I                 */
/*       I        I  word3 I  word3 I  word3 I                 */
/*       ----------------------------------------              */
/*       I        I  word0 I  word0 I  word0 I                 */
/*       I  set1  I  word1 I  word1 I  work1 I                 */
/*       I        I  word2 I  word2 I  word2 I                 */
/*       I        I  word3 I  word3 I  word3 I                 */
/*       ----------------------------------------              */
/*      						       */
/*                                                             */
/***************************************************************/

void xdump(int set, int way, cache_block **cache) {
    int i, j, k = 0;

    printf("Cache Content:\n");
    printf("-------------------------------------\n");
    for (i = 0; i < way; i++) {
        if (i == 0) {
            printf("    ");
        }
        printf("      WAY[%d]", i);
    }
    printf("\n");

    for (i = 0; i < set; i++) {
        printf("SET[%d]:   ", i);
        for (j = 0; j < way; j++) {
            if (k != 0 && j == 0) {
                printf("          ");
            }
            printf("0x%08x  ", cache[i][j].addr);
        }
        printf("\n");
    }
    printf("\n");
}

void parse_cache_args(char *cache_args, int *capacity, int *assoc, int *block_size) {
    int cache_arg_num = 0;
    char *token;
    token = strtok(cache_args, ":");

    while (token) {
        switch (cache_arg_num) {
            case 0:
                *capacity = strtol(token, NULL, 10);
                break;
            case 1:
                *assoc = strtol(token, NULL, 10);
                break;
            case 2:
                *block_size = strtol(token, NULL, 10);
                break;
        }

        token = strtok(NULL, ":");
        cache_arg_num++;
    }
}

void parse_trace_args(char *trace_entry, bool *is_read_trace, uint32_t *trace_address) {
    int trace_arg_num = 0;
    char *token;
    token = strtok(trace_entry, " ");

    while (token) {
        switch (trace_arg_num) {
            case 0:
                *is_read_trace = (token[0] == 'R');
                break;
            case 1:
                *trace_address = strtol(token, NULL, 16);
                break;
        }

        token = strtok(NULL, " ");
        trace_arg_num++;
    }
}

cache_block **allocate_cache(int num_sets, int associativity) {
    cache_block **cache = (cache_block **) malloc(sizeof(cache_block *) * num_sets);
    for (int i = 0; i < num_sets; i++) {
        cache[i] = (cache_block *) malloc(sizeof(cache_block) * associativity);
    }
    for (int i = 0; i < num_sets; i++) {
        for (int j = 0; j < associativity; j++)
            memset(&cache[i][j], 0, sizeof(cache_block));
    }

    return cache;
}

uint32_t **allocate_page_access_table(int num_sets, int associativity) {
    uint32_t **page_access_table = (uint32_t **) malloc(sizeof(uint32_t *) * num_sets);

    for (int i = 0; i < num_sets; i++) {
        page_access_table[i] = (uint32_t *) malloc(sizeof(uint32_t) * associativity);
    }
    for (int i = 0; i < num_sets; i++) {
        for (int j = 0; j < associativity; j++)
            page_access_table[i][j] = 0;
    }

    return page_access_table;
}

int get_victim_block(const cache_block *target_set, const uint32_t *page_entry, int assoc) {
    int vic_index = 0;

    for (int i = 0; i < assoc; i++) {
        if (!target_set[i].valid_bit) {
            vic_index = i;
            break;
        }

        if (page_entry[i] < page_entry[vic_index]) {
            vic_index = i;
        }
    }

    return vic_index;
}

void process_traces(cache_block **cache, uint32_t **page_access_table, int capacity, int assoc, int block_size,
                    int num_sets, bool print_cache) {
    char *trace_entry;
    cache_block *target_set;
    cache_block set_entry;
    uint32_t *target_page_entry;
    size_t trace_entry_length = 0;

    uint32_t trace_address, aligned_addr, tag, block_offset, block_address, set_index;

    static int total_operations;
    static int read_hits, read_misses;
    static int write_hits, write_misses;
    static int total_write_backs;

    while (getline(&trace_entry, &trace_entry_length, stdin) != -1) {
        bool is_trace_read;
        bool is_cache_hit = false;

        parse_trace_args(trace_entry, &is_trace_read, &trace_address);
        tag = (trace_address / num_sets) / block_size;
        block_address = trace_address / block_size;
        block_offset = trace_address % block_size;
        aligned_addr = trace_address - block_offset;
        set_index = block_address % num_sets;
        target_set = cache[set_index];
        target_page_entry = page_access_table[set_index];

        for (int j = 0; j < assoc; j++) {
            set_entry = target_set[j];

            if (set_entry.valid_bit && (set_entry.tag == tag)) {
                if (is_trace_read)
                    read_hits++;

                else {
                    write_hits++;
                    target_set[j].dirty_bit = true;
                }

                target_page_entry[j] = total_operations;
                is_cache_hit = true;
            }
        }

        if (!is_cache_hit) {

            if (is_trace_read)
                read_misses++;

            else
                write_misses++;

            int victim_block = get_victim_block(target_set, target_page_entry, assoc);

            if (target_set[victim_block].dirty_bit)
                total_write_backs++;

            target_set[victim_block].valid_bit = 1;
            target_set[victim_block].dirty_bit = !is_trace_read;
            target_set[victim_block].tag = tag;
            target_set[victim_block].addr = aligned_addr;
            target_page_entry[victim_block] = total_operations;
        }
        total_operations++;
    }

    cdump(capacity, assoc, block_size);
    sdump((read_hits + read_misses), (write_hits + write_misses), total_write_backs, read_hits, write_hits, read_misses, write_misses);

    if (print_cache)
        xdump(num_sets, assoc, cache);
}


int main(int argc, char *argv[]) {
    bool print_cache = false;
    cache_block **cache;
    uint32_t **page_access_timestamps;

    int capacity, associativity, block_size, num_sets;
    int opt;


    while ((opt = getopt(argc, argv, "c:x:")) != -1) {
        switch (opt) {
            case 'c': {
                parse_cache_args(optarg, &capacity, &associativity, &block_size);
                break;
            }
            case 'x': {
                print_cache = true;
                break;
            }
        }
    }

    freopen(argv[argc - 1], "r", stdin);

    num_sets = (capacity / associativity) / block_size;
    cache = allocate_cache(num_sets, associativity);
    page_access_timestamps = allocate_page_access_table(num_sets, associativity);
    process_traces(cache, page_access_timestamps, capacity, associativity, block_size, num_sets, print_cache);

    return 0;
}
