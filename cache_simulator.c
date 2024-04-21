#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define L1_INSTRUCTION_CACHE_SIZE 32768  // 32KB
#define L1_DATA_CACHE_SIZE 32768         // 32KB
#define L2_CACHE_SIZE 262144             // 256KB
#define BLOCK_SIZE 64                    // cache block size of 64 bytes
#define L1_INSTRUCTION_NUM_BLOCKS (L1_INSTRUCTION_CACHE_SIZE / BLOCK_SIZE)
#define L1_DATA_NUM_BLOCKS (L1_DATA_CACHE_SIZE / BLOCK_SIZE)
#define L2_NUM_BLOCKS (L2_CACHE_SIZE / BLOCK_SIZE)
#define L2_ASSOCIATIVITY 4
#define NUM_SETS (L2_NUM_BLOCKS / L2_ASSOCIATIVITY)

// debug mode
#define DEBUG 1

typedef struct {
    int valid;
    int dirty;
    int tag;
    int data[BLOCK_SIZE / sizeof(int)]; // int is 4 bytes
} CacheBlock;

// cache data
CacheBlock l1_instruction_cache [L1_INSTRUCTION_NUM_BLOCKS];
CacheBlock l1_data_cache        [L1_DATA_NUM_BLOCKS];
CacheBlock l2_cache             [NUM_SETS][L2_ASSOCIATIVITY];

// stats
unsigned long int l1_icache_misses = 0;
unsigned long int l1_icache_hits = 0;
unsigned long int l1_energy = 0;

unsigned long int l1_dcache_misses = 0;
unsigned long int l1_dcache_hits = 0;

unsigned long int l2_misses = 0;
unsigned long int l2_hits = 0;
unsigned long int l2_energy = 0;

unsigned long int dram_energy = 0;

unsigned long int total_mem_acces_time = 0;

// function decls
void print_title();
void init_caches();
void process_trace_file(const char* filename);
int* read_l1_icache(unsigned long int address);
int* read_l1_dcache(unsigned long int address);
int* read_l2_cache(unsigned long int address);
void access_dram(unsigned long int address);


/**************************************
 * Main entry point
***************************************/
int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <trace_file.din>\n", argv[0]);
        return 1;
    }

    print_title();

    // Initialize caches
    init_caches();

    // print args
    printf("File: %s\n\n", argv[1]);

    // Process trace file
    FILE* file = fopen(argv[1], "r");
    if (file == NULL) {
        fprintf(stderr, "Error: Unable to open trace file. Is it in Dinero 3 .din format?\n");
        exit(1);
    }

    // TODO fix Dinero 3 input file processing
    char operation;
    unsigned long int address, value;

    printf("Running simulation ...\n(Not doing anything right now)\n");
    while (fscanf(file, "%c %lx %lx\n", &operation, &address, &value) == 3) {
        // TODO execute corresponding operation
        //printf("Operation: %c, Address: 0x%lx, Value: 0x%lx\n", operation, address, value);
    }
    fclose(file);

    printf("Simulation Complete.\n\n");

    // stats
    printf("\n\nStatistics: \n");

    printf("Misses:\n");
    printf("L1 icache: %lu, L1 dcache: %lu, L2: %lu\n\n", l1_icache_misses, l1_dcache_misses, 
        l2_misses);

    printf("Hits:\n");
    printf("L1 icache: %lu, L1 dcache: %lu, L2: %lu\n\n", l1_icache_hits, l1_dcache_hits, 
        l2_hits);

    printf("Energy Consumption:\n");
    printf("L1: %lu, L2: %lu, DRAM: %lu\n\n", l1_energy, l2_energy, dram_energy);

    printf("Total energy access time: %lu\n", total_mem_acces_time);

    // Process memory accesses (assuming address is read from command line arguments)
    // unsigned long int address = strtol(argv[1], NULL, 16);
    // access_l1_instruction_cache(address);
    // access_l1_data_cache(address);

    // Print cache miss statistics
    // printf("L1 Instruction Cache Misses: %lu\n", l1_instruction_cache_misses);
    // printf("L1 Data Cache Misses: %lu\n", l1_data_cache_misses);
    // printf("L2 Cache Misses: %lu\n", l2_cache_misses);

    

    printf("==========================\n");

    return 0;
}

void print_title() {
    printf("_________               .__               _________.__              .__          __                \n");
    printf("\\_   ___ \\_____    ____ |  |__   ____    /   _____/|__| _____  __ __|  | _____ _/  |_  ___________ \n");
    printf("/    \\  \\/\\__  \\ _/ ___\\|  |  \\_/ __ \\   \\_____  \\ |  |/     \\|  |  \\  | \\__  \\   __\\/  _ \\_  __ \n");
    printf("\\     \\____/ __ \\\\  \\___|   Y  \\  ___/   /        \\|  |  Y Y  \\  |  /  |__/ __ \\|  | (  <_> )  | \\/\n");
    printf(" \\______  (____  /\\___  >___|  /\\___  > /_______  /|__|__|_|  /____/|____(____  /__|  \\____/|__|  \n");
    printf("        \\/     \\/     \\/     \\/     \\/          \\/          \\/                \\/                   \n");
    printf("_________   _________ _________________  ______ \n");
    printf("\\_   ___ \\ /   _____/ \\_____  \\______  \\/  __  \\ \n");
    printf("/    \\  \\/ \\_____  \\    _(__  <   /    />      < \n");
    printf("\\     \\____/        \\  /       \\ /    //   --   \\\n");
    printf(" \\______  /_______  / /______  //____/ \\______  / \n");
    printf("        \\/        \\/         \\/               \\/  \n\n");
}

/**
 * Initialize all caches
*/
void init_caches() {
    for (size_t i = 0; i < L1_INSTRUCTION_NUM_BLOCKS; i++) {
        l1_instruction_cache[i].valid = 0;
        l1_instruction_cache[i].dirty = 0;
        l1_instruction_cache[i].tag = -1;
        for (size_t j = 0; j < BLOCK_SIZE / sizeof(int); j++) {
            l1_instruction_cache[i].data[j] = 0;
        }
    }

    for (size_t i = 0; i < L1_DATA_NUM_BLOCKS; i++) {
        l1_data_cache[i].valid = 0;
        l1_data_cache[i].dirty = 0;
        l1_data_cache[i].tag = -1;
        for (size_t j = 0; j < BLOCK_SIZE / sizeof(int); j++) {
            l1_data_cache[i].data[j] = 0;
        }
    }

    for (size_t i = 0; i < NUM_SETS; i++) {
        for (size_t j = 0; j < L2_ASSOCIATIVITY; j++) {
            l2_cache[i][j].valid = 0;
            l2_cache[i][j].dirty = 0;
            l2_cache[i][j].tag = -1;
            for (size_t k = 0; k < BLOCK_SIZE / sizeof(int); k++) {
                l2_cache[i][j].data[k] = 0;
            }
        }
    }
}

/**
 * Read L1 Instruction Cache
*/
int* read_l1_icache(unsigned long int address) {
    if (DEBUG) {
        printf("Addy: %lx\n", address);
    }
    
    // Calculate cache index and tag from the address
    size_t index = (address / BLOCK_SIZE) % L1_INSTRUCTION_NUM_BLOCKS;
    int tag = address / (BLOCK_SIZE * L1_INSTRUCTION_NUM_BLOCKS);

    // Cache hit
    if (l1_instruction_cache[index].valid && l1_instruction_cache[index].tag == tag) {
        l1_icache_hits++;

        return l1_instruction_cache[index].data;
    }

    // cache miss
    l1_icache_misses++;

    // Cache miss, access L2 cache to fetch data
    int* data = read_l2_cache(address);

    // Update L1 instruction cache with fetched data
    l1_instruction_cache[index].valid = 1;
    l1_instruction_cache[index].tag = tag;
    memcpy(l1_instruction_cache[index].data, data, BLOCK_SIZE);

    // Return the fetched data
    return data;
}

/**
 * Write L1 Instruction Cache
*/
void write_l1_icache(unsigned long int address) {
    if (DEBUG) {
        printf("Addy: %lx\n", address);
    }
}

/**
 * Read L1 Data Cache
*/
int* read_l1_dcache(unsigned long int address) {
    if (DEBUG) {
        printf("Addy: %lx\n", address);
    }
    return 0;
}

/**
 * Read L2 Cache
*/
int* read_l2_cache(unsigned long int address) {
    if (DEBUG) {
        printf("Addy: %lx\n", address);
    }
    return 0;
}

/**
 * Access DRAM
 * Simulate only time and energy
*/
void access_dram(unsigned long int address) {
    if (DEBUG) {
        printf("Addy: %lx\n", address);
    }
}

/**
 * Proccess input trace file in Din 3 format
*/
void process_trace_file(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "Error: Unable to open trace file\n");
        exit(1);
    }

    char operation;
    unsigned long int address;

    while (fscanf(file, "%c %lx\n", &operation, &address) == 2) {
        // ex
        printf("Operation: %c, Address: 0x%lx\n", operation, address);
    }

    fclose(file);
}