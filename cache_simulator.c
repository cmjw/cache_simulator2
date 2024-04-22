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
#define DEBUG 0

// opcodes
#define MEMORY_READ  0
#define MEMORY_WRITE 1
#define INSTR_FETCH  2
#define IGNORE       3
#define FLUSH_CACHE  4

typedef struct {
    int valid;
    int dirty;
    int tag;
    int unsigned long data[BLOCK_SIZE / sizeof(int)]; // int is 4 bytes
} CacheBlock;

// cache data
CacheBlock l1_instruction_cache [L1_INSTRUCTION_NUM_BLOCKS];
CacheBlock l1_data_cache        [L1_DATA_NUM_BLOCKS];
CacheBlock l2_cache             [NUM_SETS][L2_ASSOCIATIVITY];

// stats
unsigned long int l1_icache_misses = 0;
unsigned long int l1_dcache_misses = 0;
unsigned long int l2_misses = 0;

unsigned long int l1_icache_hits = 0;
unsigned long int l1_dcache_hits = 0;
unsigned long int l2_hits = 0;

double l1_energy = 0;
double l2_energy = 0;
double dram_energy = 0;

unsigned long int total_mem_acces_time = 0;

// function decls
void print_title();
void print_stats();
void init_caches();
void process_trace_file(const char* filename);
void process_dinero_trace(const char* filename);

unsigned long int* read_l1_icache(unsigned long int address);
unsigned long int* read_l1_dcache(unsigned long int address);
unsigned long int* read_l2_cache(unsigned long int address);
void write_l1_icache(unsigned long int address, unsigned long int* data);
unsigned long int* access_dram(unsigned long int address);

void do_memory_read(unsigned long int address);
void do_memory_write(unsigned long int address, unsigned long int* data);
void do_instruction_fetch(unsigned long int address, unsigned long int value);
void do_ignore();
void do_cache_flush();

void l1_idle();
void l2_idle();
void dram_idle();


/**************************************
 * Main entry point
***************************************/
int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <trace_file.din>\n", argv[0]);
        return 1;
    }

    if (argc == 2) {
        print_title();
    }

    // Initialize caches
    init_caches();

    // print args
    printf("File: %s\n\n", argv[1]);

    // simulation
    process_dinero_trace(argv[1]);

    printf("Simulation Complete.\n\n");

    // stats
    print_stats();

    printf("==========================\n");

    return 0;
}

/**
 * Print a lil title
*/
void print_title() {
    printf("_________               .__               _________.__              .__          __                \n");
    printf("\\_   ___ \\_____    ____ |  |__   ____    /   _____/|__| _____  __ __|  | _____ _/  |_  ___________ \n");
    printf("/    \\  \\/\\__  \\ _/ ___\\|  |  \\_/ __ \\   \\_____  \\ |  |/     \\|  |  \\  | \\__  \\   __\\/  _  \\_  __ \n");
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
 * Print Stats
*/
void print_stats() {
    printf("\n\nStatistics: \n");

    printf("Misses:\n");
    printf("L1 icache: %lu, L1 dcache: %lu, L2: %lu\n\n", l1_icache_misses, l1_dcache_misses, 
        l2_misses);

    printf("Hits:\n");
    printf("L1 icache: %lu, L1 dcache: %lu, L2: %lu\n\n", l1_icache_hits, l1_dcache_hits, 
        l2_hits);

    printf("Energy Consumption:\n");
    printf("L1: %f, L2: %f, DRAM: %f\n\n", l1_energy, l2_energy, dram_energy);

    printf("Total energy access time: %lu\n", total_mem_acces_time);
}

/**
 * Process File
*/
void process_dinero_trace(const char* filename) {
    // Process trace file
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "Error: Unable to open trace file. Is it in Dinero 3 .din format?\n");
        exit(1);
    }

    // TODO fix Dinero 3 input file processing
    char operation;
    int opcode;
    unsigned long int address, value;

    printf("Running simulation ...\n(Not complete right now)\n\n");
    while (fscanf(file, "%c %lx %lx\n", &operation, &address, &value) == 3) {
        // TODO execute corresponding operation
        //printf("Operation: %c, Address: 0x%lx, Value: 0x%lx\n", operation, address, value);
        opcode = operation - '0';

        // memory read
        if (opcode == MEMORY_READ && value == 0) {
            do_memory_read(address);
        }
        // memory write
        else if (opcode == MEMORY_WRITE && value == 0) {
            // what do we write to??
            // just acces DRAM for time?
            do_memory_write(address, &value);
        }
        // instruction fetch
        else if (opcode == INSTR_FETCH) {
            void do_instruction_fetch(unsigned long int address, unsigned long int value);
        }
        // ignore
        else if (opcode == IGNORE && address == 0) {
            void do_ignore();
        }
        // flush cache
        else if (opcode == FLUSH_CACHE && address == 0) {
            void do_cache_flush();
        }

        else {
            printf("Operation: %c, Address: 0x%lx, Value: 0x%lx\n", operation, address, value);
            fprintf(stderr, "Error: Invalid operation code or arguments.\n");
            exit(1);
        }
    }
    fclose(file);
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
 * Do a memory read.
*/
void do_memory_read(unsigned long int address) {
    read_l1_dcache(address);
}

/**
 * Do a memory write.
*/
void do_memory_write(unsigned long int address, unsigned long int* data) {
    // idk yet
    write_l1_icache(address, data);
}

/**
 * Do an instruction fetch.
*/
void do_instruction_fetch(unsigned long int address, unsigned long int value) {
    read_l1_icache(address);
    if (0) { printf("%lu", value); }


}

/**
 * Do an ignore.
*/
void do_ignore() {
    // idle energy consumption
    l1_idle();
    l2_idle();
    dram_idle();
}

/**
 * Do a cache flush.
*/
void do_cache_flush() {
    init_caches();
    do_ignore();
}

/**
 * Read L1 Instruction Cache
*/
unsigned long int* read_l1_icache(unsigned long int address) {
    if (DEBUG) {
        printf("Addy: %lx\n", address);
    }
    
    l1_energy += 1;

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
    unsigned long int* data = read_l2_cache(address);

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
void write_l1_icache(unsigned long int address, unsigned long int* data) {
    if (DEBUG) {
        printf("Addy: %lx\n", address);
    }

    l1_energy += 1;

    // Calculate cache index and tag from the address
    size_t index = (address / BLOCK_SIZE) % L1_DATA_NUM_BLOCKS;
    int tag = address / (BLOCK_SIZE * L1_DATA_NUM_BLOCKS);

    l1_data_cache[index].valid = 1;
    l1_data_cache[index].tag = tag;
    l1_data_cache[index].dirty = 1;

    memcpy(l1_data_cache[index].data, data, BLOCK_SIZE);
}

/**
 * Read L1 Data Cache
*/
unsigned long int* read_l1_dcache(unsigned long int address) {
    if (DEBUG) {
        printf("Addy: %lx\n", address);
    }

    l1_energy += 1;

    return 0;
}

/**
 * Read L2 Cache
*/
unsigned long int* read_l2_cache(unsigned long int address) {
    if (DEBUG) {
        printf("Addy: %lx\n", address);
    }

    l2_energy += 2;
    
    // Calculate set index and tag from the address
    size_t setIndex = (address / BLOCK_SIZE) % NUM_SETS;
    int tag = address / (BLOCK_SIZE * NUM_SETS);

    // find block in the set
    for (int i = 0; i < L2_ASSOCIATIVITY; i++) {
        if (l2_cache[setIndex][i].valid && l2_cache[setIndex][i].tag == tag) {
            // Cache hit
            return l2_cache[setIndex][i].data; 
        }
    }

    // Simulate data fetching from memory
    unsigned long int* data = access_dram(address);
    // data is null because DRAM doesn't exist ...

    // Random replacement policy
    int replacementIndex = rand() % L2_ASSOCIATIVITY;

    // Update block with fetched data
    l2_cache[setIndex][replacementIndex].valid = 1;
    l2_cache[setIndex][replacementIndex].tag = tag;
    memcpy(l2_cache[setIndex][replacementIndex].data, data, BLOCK_SIZE);

    // will be null!!
    return l2_cache[setIndex][replacementIndex].data;
}

/**
 * Access DRAM
 * Simulate only time and energy
*/
unsigned long int* access_dram(unsigned long int address) {
    if (DEBUG) {
        printf("Addy: %lx\n", address);
    }

    dram_energy += 4;

    return NULL;
}

/**
 * Simulate idle L1 cache
*/
void l1_idle() {
    l1_energy += 0.5;
}

/**
 * Simulate idle L2 cache
*/
void l2_idle() {
    l2_energy += 0.8;
}

/**
 * Simulate idle DRAM
*/
void dram_idle() {
    dram_energy += 0.8;
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