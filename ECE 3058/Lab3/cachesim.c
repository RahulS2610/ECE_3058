/**
 * @author ECE 3058 TAs
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cachesim.h"

// Statistics you will need to keep track. DO NOT CHANGE THESE.
counter_t accesses = 0;     // Total number of cache accesses
counter_t hits = 0;         // Total number of cache hits
counter_t misses = 0;       // Total number of cache misses
counter_t writebacks = 0;   // Total number of writebacks

/**
 * Function to perform a very basic log2. It is not a full log function, 
 * but it is all that is needed for this assignment. The <math.h> log
 * function causes issues for some people, so we are providing this. 
 * 
 * @param x is the number you want the log of.
 * @returns Techinically, floor(log_2(x)). But for this lab, x should always be a power of 2.
 */
int simple_log_2(int x) {
    int val = 0;
    while (x > 1) {
        x /= 2;
        val++;
    }
    return val; 
}

//  Here are some global variables you may find useful to get you started.
//      Feel free to add/change anyting here.
cache_set_t* cache;     // Data structure for the cache
int block_size;         // Block size
int cache_size;         // Cache size
int ways;               // Ways
int noOfindex;			// Number of index
int num_sets;           // Number of sets
int num_offset_bits;    // Number of offset bits
int num_index_bits;     // Number of index bits. 
int num_tag_bits;		// Number of tag bits
int offsetVal;			// Offset value
int setIndex;			// Value of index
int setTag;				// Value of tag			

/**
 * Function to intialize your cache simulator with the given cache parameters. 
 * Note that we will only input valid parameters and all the inputs will always 
 * be a power of 2.
 * 
 * @param _block_size is the block size in bytes
 * @param _cache_size is the cache size in bytes
 * @param _ways is the associativity
 */
void cachesim_init(int _block_size, int _cache_size, int _ways) {
    // Set cache parameters to global variables
    block_size = _block_size;
    cache_size = _cache_size;
    ways = _ways;

    noOfindex = cache_size / block_size;
    num_sets = noOfindex / ways;
    
    num_offset_bits = simple_log_2(num_sets);
    num_index_bits = simple_log_2(block_size);
    num_tag_bits = 32 - (num_index_bits + num_offset_bits);

    cache = (cache_set_t*)malloc(num_sets * sizeof(cache_set_t));
    for(int r = 0; r < num_sets; r++){
    	cache[i].stack = init_lru_stack(ways);
    	cache[i].blocks = (cache_block_t*)malloc(block_size * ways);
    	for(int s = 0; s < ways; s++){
	    	cache[r].blocks[s].valid = 0;
	    	cache[r].blocks[s].dirty = 0;
	    	cache[r].blocks[s].tag = -1;
    	}
   	}
}

/**
 * Function to perform a SINGLE memory access to your cache. In this function, 
 * you will need to update the required statistics (accesses, hits, misses, writebacks)
 * and update your cache data structure with any changes necessary.
 * 
 * @param physical_addr is the address to use for the memory access. 
 * @param access_type is the type of access - 0 (data read), 1 (data write) or 
 *      2 (instruction read). We have provided macros (MEMREAD, MEMWRITE, IFETCH)
 *      to reflect these values in cachesim.h so you can make your code more readable.
 */
void cachesim_access(addr_t physical_addr, int access_type) {

	setIndex = (physical_addr >> num_offset_bits) & ((1 << num_index_bits) - 1);
	offsetVal = physical_addr & ((1 << num_offset_bits) - 1);
	setTag = (physical_addr >> (num_index_bits + num_offset_bits)) & ((1 << (32 - (num_offset_bits + num_index_bits))) - 1);

	int hitMiss = 0;
	int place = 0;

	// miss
	for(int i = 0; i < ways; i++){
		if(cache[setIndex].blocks[i].tag == setTag && cache[setIndex].blocks[i].valid == 1){
			hits++;
			hitMiss = 1;
			place = i;
		}
	}
	//marking ditry
	if(access_type == MEMWRITE){
		cache[setIndex].blocks[place].dirty = 1;
	}

	int blockValidity = 0;
	int firstInvalid = 0;
	int noInvalid = 0;

	//miss
	if(hitMiss == 0){
		misses++;
		for(int i = 0; i < ways; i++){
			if(cache[setIndex].blocks[i].valid == 1){
				blockValidity = 1;
			} else {
				firstInvalid = i;
				noInvalid = 1;
			}
		}

		if(blockValidity == 0){
			place = 0;
		} else if(noInvalid == 1){
			place = firstInvalid;
		} else {
			place = lru_stack_get_lru(cache[setIndex].stack);
			if(cache[setIndex].blocks[place].dirty == 1)
				writebacks++;
		}
	}

	if(access_type == MEMWRITE){
		cache[setIndex].blocks[place].dirty = 1;
	} else {
		cache[setIndex].blocks[place].dirty = 0;
	}

	cache[setIndex].blocks[place].tag = setTag;
	cache[setIndex].blocks[place].valid = 1;

	lru_stack_set_mru(cache[setIndex].stack, place);
}

/**
 * Function to free up any dynamically allocated memory you allocated
 */
void cachesim_cleanup() {

	for(int i = 0; i < num_sets; i++){
		lru_stack_cleanup(cache[i].stack);
		free(cache[i].blocks);
	}
	free(cache);
}

/**
 * Function to print cache statistics
 * DO NOT update what this prints.
 */
void cachesim_print_stats() {
    printf("%llu, %llu, %llu, %llu\n", accesses, hits, misses, writebacks);  
}

/**
 * Function to open the trace file
 * You do not need to update this function. 
 */
FILE *open_trace(const char *filename) {
    return fopen(filename, "r");
}

/**
 * Read in next line of the trace
 * 
 * @param trace is the file handler for the trace
 * @return 0 when error or EOF and 1 otherwise. 
 */
int next_line(FILE* trace) {
    if (feof(trace) || ferror(trace)) return 0;
    else {
        int t;
        unsigned long long address, instr;
        fscanf(trace, "%d %llx %llx\n", &t, &address, &instr);
        cachesim_access(address, t);
    }
    return 1;
}

/**
 * Main function. See error message for usage. 
 * 
 * @param argc number of arguments
 * @param argv Argument values
 * @returns 0 on success. 
 */
int main(int argc, char **argv) {
    FILE *input;

    if (argc != 5) {
        fprintf(stderr, "Usage:\n  %s <trace> <block size(bytes)>"
                        " <cache size(bytes)> <ways>\n", argv[0]);
        return 1;
    }
    
    input = open_trace(argv[1]);
    cachesim_init(atol(argv[2]), atol(argv[3]), atol(argv[4]));
    while (next_line(input));
    cachesim_print_stats();
    cachesim_cleanup();
    fclose(input);
    return 0;
}

