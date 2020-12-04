#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <cstdlib>

#include "Enclave.h"
#include "Enclave_t.h"

/* 
 * hello():
 *   Says hello world and prints arg passed
 */
void init_store(int size)
{
    // print("Hello World!");
    // iprint(a);
}

/*
 * ctr: size of the aray (num of instructions)
 * set_instr: pointer to the array of instructions
 * 
 * secure_store():
 *   Loads the data specified by the set instructions
 */
void secure_store(int set_row_count, char **set_instr)
{
    iprint(set_row_count);

    char *instr = new char[BUFFER_SZ];
    char *keyid;
    char *valnum;
    char *key;
    char *val;

    memset(instr, 0, BUFFER_SZ);
    memcpy(instr, set_instr[2046], BUFFER_SZ);
    print(instr);

    if (strncmp(instr, "SET", 3) == 0)
    {
        keyid = strtok_r(instr + 4, " ", &valnum);
        // print(keyid);
        // print(valnum); // should be 1
        key = new char[strlen(keyid)+1];
        memset(key, 0, strlen(keyid)+1);
        memcpy(key, keyid, strlen(keyid));
        print(key);
        
        val = new char[strlen(valnum)];
        memset(val, 0, strlen(valnum));
        memcpy(val, valnum, strlen(valnum)-1);
        print(val);
    } else
    {
        print("Nothing");
    }
}
