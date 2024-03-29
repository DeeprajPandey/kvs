#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <cstdlib>

#include "Enclave.h"
#include "Enclave_t.h"

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

// [in]: contents of buffer copied into the enclave
// [out]: not copied into encl, but final value is copied outside
// void hello_world(char *text, int n)
// {
//     for (int i = 0; i < n; i++)
//     {
//         pprint(&text[i], text[i]);
//         text[i] = 'f';
//     }
// }
