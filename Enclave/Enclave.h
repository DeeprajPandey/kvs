#ifndef _ENCLAVE_H_
#define _ENCLAVE_H_

#include <stdlib.h>
#include <assert.h>

/* Enclave structs will go here */
typedef struct item {
    int key;
    int val;
} item;

typedef struct store {
    int size;
    item *entries;
} store;

#endif /* !_ENCLAVE_H_ */
