#define BUFFER_SZ 64
#define WKLD_MULT 16384

typedef struct item {
    int key;
    int val;
} item;

typedef struct store {
    int size;
    item *entries;
} store;