#include<iostream>
#include<cstdlib>
#include<cstring>
#include <sys/time.h>

#include "main.h"

void print(const char *str)
{
    printf("%s\n", str);
}

void iprint(int i, int v)
{
    printf("SecureStore[%d]: %d\n", i, v);
}

void cprint(char str)
{
    printf("%c\n", str);
}

void pprint(int *str, int c)
{
    printf("\n[Untrusted Side]\n");
    printf("Encl Addr: %p\t", str);
    printf("Dereferenced: %d\t", *str);
    printf("Value recvd from Enclave: %d\n", c);

    printf("EAddr + 2: %p\t", str+2);
    printf("Dereferenced: %d\n", *(str+2));

    printf("EAddr + 4: %p\t", str+4);
    printf("Dereferenced: %d\n\n", *(str+4));
}

static char **set_insts = NULL;
static uint set_row_ctr;

///////////////////////////////////////////////////////////////
/*************************************************************/
///////////////////////////////////////////////////////////////

static store *gst = NULL;

/* 
 * hello():
 *   Says hello world and prints arg passed
 */
void init_store(int size)
{
    if (size < 1)
        print("init_store::Cannot initialise store of negative size.");
    else {
        gst = new store;
        if (gst == NULL)
            print("init_store::Could not allocate memory for global store.");

        gst->size = size;

        gst->entries = new item[size];
        if (gst->entries == NULL)
            print("init_store::Could not allocate memory for entries");

        // Initialise the items with -1
        for (int i = 0; i < size; i++)
        {
            gst->entries[i].key = -1;
            gst->entries[i].val = -1;
        }

        print("\ninit_store::Allocated memory for global store.");
    }
}

void destroy_store(int size)
{
    delete[] gst->entries;
    delete[] gst;
    print("\ndestroy_store::Deallocated memory for global store.");
}

/*
 * hash():
 *   Generate a hash from key to index into global store.
 *   It's just the key right now.
 */
int hash(int k)
{
    return k % gst->size;
}

/*
 * get():
 *   Read data from the store.
 * TODO: convert int key,val to handle arbitrary char *
 */
item * get(int data_key)
{
    int found = 0;
    int id = hash(data_key);

    if (gst->entries[id].key == data_key)
    {
        return &(gst->entries[id]);
    } else
    {
        return NULL;
    }
}

/*
 * set():
 *   Load data into the store.
 * TODO: convert int key,val to handle arbitrary char *
 */
void set(int data_key, int data_val)
{
    int id = hash(data_key);

    // If we have an item for this key
    if (gst->entries[id].key == data_key)
    {
        // Replace the value
        gst->entries[id].val = data_val;
    } else
    {
        // Create a new item entry
        // entries[id].key must be -1
        if (gst->entries[id].key != -1)
        {
            // Another key is here
            // We can't print but handle somehow
        } else
        {
            gst->entries[id].key = data_key;
            gst->entries[id].val = data_val;
        }
    } // End of add new key
}

/*
 * ctr: size of the aray (num of instructions)
 * set_instr: pointer to the array of instructions
 * 
 * secure_store_read():
 *   Loads the data specified by the set instructions
 */
void secure_store_read(int set_row_count, char **set_instr)
{
    // iprint(set_row_count);

    char *instr = new char[BUFFER_SZ];
    char *keyid;
    char *valnum;
    char *key;
    char *val;

    for (int i = 0; i < 2048; i++)
    {
        memset(instr, 0, BUFFER_SZ);
        memcpy(instr, set_instr[i], BUFFER_SZ);
        // print(instr);

        if (strncmp(instr, "SET", 3) == 0)
        {
            keyid = strtok_r(instr + 4, " ", &valnum);
            // print(keyid);
            // print(valnum); // should be 1
            key = new char[strlen(keyid)+1];
            memset(key, 0, strlen(keyid)+1);
            memcpy(key, keyid, strlen(keyid));
            // print(key);
            
            val = new char[strlen(valnum)];
            memset(val, 0, strlen(valnum));
            memcpy(val, valnum, strlen(valnum)-1);
            // print(val);

            set(atoi(key), atoi(val));

            delete[] key;
            delete[] val;
        } else
        {
            print("Nothing");
        }
    }
    print("secure_set::Set instructions executed.");
    // iprint(gst->entries[hash(2047+1)].key);
}

void get_from_store(char *command)
{
    char *keyid;
    char *extra;
    char *key;
    item *read_entry;

    if (strncmp(command, "GET", 3) == 0)
    {
        keyid = strtok_r(command + 4, " ", &extra);

        key = new char[strlen(keyid)];
        memset(key, 0, strlen(keyid));
        memcpy(key, keyid, strlen(keyid)-1);
        // printf("key is %s\n", key);
        read_entry = get(atoi(key));

        if (read_entry == NULL)
            print("secure_get:Key not found");
        else if (read_entry->val == -1)
            print("secure_get:Data empty.");
        else
            print("\nsecure_get::Data Read:");
            iprint(read_entry->key, read_entry->val);
            pprint(&(read_entry->val), read_entry->val);
    }
}

///////////////////////////////////////////////////////////////
/*************************************************************/
///////////////////////////////////////////////////////////////

// Read set instructions into global buffer
void read_set_insts(uint8_t sz, const char *file)
{
    FILE *w_file;
    char *inst = new char[BUFFER_SZ];
    uint set_sz = WKLD_MULT * sz;
    
    printf("Allocating %d MB for the incoming instructions.\n", \
    (set_sz * BUFFER_SZ)/(1024*1024));
    
    set_insts = new char *[set_sz];
    for (uint i = 0; i < set_sz; i++)
        set_insts[i] = new char[BUFFER_SZ];
    set_row_ctr = 0; // prepare for reading instrs

    // Read file
    w_file = fopen(file, "r");

    if (w_file == NULL)
    {
        printf("File: %s", file);
        perror("Error opening file");
    } else
    {
        for (uint i = 0; i < set_sz; i++)
        {
            if (fgets(inst, BUFFER_SZ, w_file) != NULL)
            {
                // Did we read the line correctly?
                // puts(inst);

                // We use set_row_ctr as i will increment even when fgets fails
                // And we need it to pass to the enclave
                memcpy(set_insts[set_row_ctr++], inst, BUFFER_SZ);
            }
        }
        printf("Closing file.\n");
        fclose(w_file);
    }
}

void clear_instructions(uint8_t sz)
{
    uint set_sz = WKLD_MULT * sz;

    for (int i = 0; i < set_sz; i++)
        delete[] set_insts[i];
    delete[] set_insts;
    printf("Deallocate memory with instructions.\n");
}

void secure_store_read(uint set_row_count)
{
    // for loop until set_row_count
    printf("%s", set_insts[4]);
}

int main()
{
    // uint a = 16480;
    // printf("%u", a * (uint)sizeof(char));
    struct timeval filestart, fileend, enc_start, enc_end;
	double file_diff, set_diff = 0;

    uint8_t workload_size = 1;

    gettimeofday(&filestart, NULL);
    read_set_insts(workload_size, "/home/am/kvs/workloads/set1.dat");
    gettimeofday(&fileend, NULL);

    printf("==========");
    init_store(set_row_ctr);

    gettimeofday(&enc_start, NULL);
    secure_store_read(set_row_ctr * (int)sizeof(char), set_insts);
    gettimeofday(&enc_end, NULL);

    char cmd1[] = "GET 1017\n";
    char cmd2[] = "GET 1018\n";
    char cmd3[] = "GET 1019\n";
    get_from_store(cmd1);
    get_from_store(cmd2);
    get_from_store(cmd3);
    
    destroy_store(set_row_ctr);
    printf("==========\n");

    clear_instructions(workload_size);

    file_diff = (double)(fileend.tv_usec - filestart.tv_usec) / 1000 + (double)(fileend.tv_sec - filestart.tv_sec);
    set_diff = (double)(enc_end.tv_usec - enc_start.tv_usec) / 1000 + (double)(enc_end.tv_sec - enc_start.tv_sec);
    
    FILE *resfile = fopen("readings.txt", "a");
    fprintf(resfile, "2048 Instructions\tFile Read Time: %f ms\tEnclave Set Time:%f ms\n", file_diff, set_diff);
    return 0;
}