/*
 * Copyright (C) 2011-2017 Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Intel Corporation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <string.h>
#include <sys/time.h>

#include <unistd.h>
#include <pwd.h>
#define MAX_PATH FILENAME_MAX

#include "sgx_urts.h"
#include "App.h"
#include "Enclave_u.h"

/* Global EID shared by multiple threads */
sgx_enclave_id_t global_eid = 0;

/* Handle SGX Errors & init enclave:
 *   Adapted from Intel's SampleEnclave Source */
typedef struct _sgx_errlist_t {
    sgx_status_t err;
    const char *msg;
    const char *sug; /* Suggestion */
} sgx_errlist_t;

/* Error code returned by sgx_create_enclave */
static sgx_errlist_t sgx_errlist[] = {
    {
        SGX_ERROR_UNEXPECTED,
        "Unexpected error occurred.",
        NULL
    },
    {
        SGX_ERROR_INVALID_PARAMETER,
        "Invalid parameter.",
        NULL
    },
    {
        SGX_ERROR_OUT_OF_MEMORY,
        "Out of memory.",
        NULL
    },
    {
        SGX_ERROR_ENCLAVE_LOST,
        "Power transition occurred.",
        "Please refer to the sample \"PowerTransition\" for details."
    },
    {
        SGX_ERROR_INVALID_ENCLAVE,
        "Invalid enclave image.",
        NULL
    },
    {
        SGX_ERROR_INVALID_ENCLAVE_ID,
        "Invalid enclave identification.",
        NULL
    },
    {
        SGX_ERROR_INVALID_SIGNATURE,
        "Invalid enclave signature.",
        NULL
    },
    {
        SGX_ERROR_OUT_OF_EPC,
        "Out of EPC memory.",
        NULL
    },
    {
        SGX_ERROR_NO_DEVICE,
        "Invalid SGX device.",
        "Please make sure SGX module is enabled in the BIOS, and install SGX driver afterwards."
    },
    {
        SGX_ERROR_MEMORY_MAP_CONFLICT,
        "Memory map conflicted.",
        NULL
    },
    {
        SGX_ERROR_INVALID_METADATA,
        "Invalid enclave metadata.",
        NULL
    },
    {
        SGX_ERROR_DEVICE_BUSY,
        "SGX device was busy.",
        NULL
    },
    {
        SGX_ERROR_INVALID_VERSION,
        "Enclave version was invalid.",
        NULL
    },
    {
        SGX_ERROR_INVALID_ATTRIBUTE,
        "Enclave was not authorized.",
        NULL
    },
    {
        SGX_ERROR_ENCLAVE_FILE_ACCESS,
        "Can't open enclave file.",
        NULL
    },
};

/* Check error conditions for loading enclave */
void print_error_message(sgx_status_t ret)
{
    size_t idx = 0;
    size_t ttl = sizeof sgx_errlist/sizeof sgx_errlist[0];

    for (idx = 0; idx < ttl; idx++) {
        if(ret == sgx_errlist[idx].err) {
            if(NULL != sgx_errlist[idx].sug)
                printf("Info: %s\n", sgx_errlist[idx].sug);
            printf("Error: %s\n", sgx_errlist[idx].msg);
            break;
        }
    }
    
    if (idx == ttl)
        printf("Error: Unexpected error occurred.\n");
}


/* Initialize the enclave:
 *   Step 1: try to retrieve the launch token saved by last transaction
 *   Step 2: call sgx_create_enclave to initialize an enclave instance
 *   Step 3: save the launch token if it is updated
 */
int initialize_enclave(void)
{
    char token_path[MAX_PATH] = {'\0'};
    sgx_launch_token_t token = {0};
    sgx_status_t ret = SGX_ERROR_UNEXPECTED;
    int updated = 0;
    
    /* Step 1: try to retrieve the launch token saved by last transaction 
     *         if there is no token, then create a new one.
     */
    /* try to get the token saved in $HOME */
    const char *home_dir = getpwuid(getuid())->pw_dir;
    
    if (home_dir != NULL && 
        (strlen(home_dir)+strlen("/")+sizeof(TOKEN_FILENAME)+1) <= MAX_PATH) {
        /* compose the token path */
        strncpy(token_path, home_dir, strlen(home_dir));
        strncat(token_path, "/", strlen("/"));
        strncat(token_path, TOKEN_FILENAME, sizeof(TOKEN_FILENAME)+1);
    } else {
        /* if token path is too long or $HOME is NULL */
        strncpy(token_path, TOKEN_FILENAME, sizeof(TOKEN_FILENAME));
    }

    FILE *fp = fopen(token_path, "rb");
    if (fp == NULL && (fp = fopen(token_path, "wb")) == NULL) {
        printf("[Untrusted]Warning: Failed to create/open the launch token file \"%s\".\n", token_path);
    }

    if (fp != NULL) {
        /* read the token from saved file */
        size_t read_num = fread(token, 1, sizeof(sgx_launch_token_t), fp);
        if (read_num != 0 && read_num != sizeof(sgx_launch_token_t)) {
            /* if token is invalid, clear the buffer */
            memset(&token, 0x0, sizeof(sgx_launch_token_t));
            printf("[Untrusted]Warning: Invalid launch token read from \"%s\".\n", token_path);
        }
    }
    /* Step 2: call sgx_create_enclave to initialize an enclave instance */
    /* Debug Support: set 2nd parameter to 1 */
    ret = sgx_create_enclave(ENCLAVE_FILENAME, SGX_DEBUG_FLAG, &token, &updated, &global_eid, NULL);
    if (ret != SGX_SUCCESS) {
        print_error_message(ret);
        if (fp != NULL) fclose(fp);
        return -1;
    }

    /* Step 3: save the launch token if it is updated */
    if (updated == FALSE || fp == NULL) {
        /* if the token is not updated, or file handler is invalid, do not perform saving */
        if (fp != NULL) fclose(fp);
        return 0;
    }

    /* reopen the file with write capablity */
    fp = freopen(token_path, "wb", fp);
    if (fp == NULL) return 0;
    size_t write_num = fwrite(token, 1, sizeof(sgx_launch_token_t), fp);
    if (write_num != sizeof(sgx_launch_token_t))
        printf("[Untrusted]Warning: Failed to save launch token to \"%s\".\n", token_path);
    fclose(fp);
    return 0;
}

/* OCalls */
// Prints string buffer. Overflow is handled by EDL definition [string]
void print(const char *str)
{
    printf("%s\n", str);
}

void iprint(int n)
{
    printf("%d\n", n);
}

/* Utility Functions & Global Variables */
// Store instruction buffers
static char **set_insts = NULL;
static char **get_insts = NULL;
static int set_row_ctr;

// Read set instructions into global buffer
void read_set_insts(uint8_t sz, const char *file)
{
    FILE *w_file;
    char *inst = new char[BUFFER_SZ];
    uint set_sz = WKLD_MULT * sz;
    
    printf("[Untrusted]Allocating %d MB for the incoming instructions.\n", \
    (set_sz * BUFFER_SZ)/(1024*1024));
    
    set_insts = new char *[set_sz];
    for (uint i = 0; i < set_sz; i++)
        set_insts[i] = new char[BUFFER_SZ];
    set_row_ctr = 0; // prepare for reading instrs

    // Read file
    w_file = fopen(file, "r");

    if (w_file == NULL)
    {
        printf("[Untrusted]File: %s", file);
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
        printf("[Untrusted]Closing file.\n");
        fclose(w_file);
    }
}

void clear_instructions(uint8_t sz)
{
    uint set_sz = WKLD_MULT * sz;

    for (int i = 0; i < set_sz; i++)
        delete[] set_insts[i];
    delete[] set_insts;
    printf("[Untrusted]Deallocate untrusted memory with instructions.\n");
}

/* Application Entry */
int SGX_CDECL main(int argc, char *argv[])
{
    (void)(argc);
    (void)(argv);

    // Initialize the enclave
    if(initialize_enclave() < 0){
        printf("[Untrusted]Enclave initialisation failed. Exiting...\n");
        return -1; 
    } else {
        printf("[Untrusted]Enclave initialised.\n");
    }

    struct timeval filestart, fileend, enc_start, enc_end;
	double file_diff, set_diff = 0;

    uint8_t workload_size = 1;
    gettimeofday(&filestart, NULL);
    read_set_insts(workload_size, "/home/am/kvs/workloads/set1.dat");
    gettimeofday(&fileend, NULL);
    // printf("%s\n", set_insts[1024]);


    // Trusted Enclave function calls
    printf("==========");
    sgx_status_t ret = init_store(global_eid, set_row_ctr);
    if (ret != SGX_SUCCESS) {
        print_error_message(ret);
        return -1;
    }
    gettimeofday(&enc_start, NULL);
    // Cast to uint because sizeof() returns lu but ctr is u
    ret = secure_store(global_eid, set_row_ctr * (int)sizeof(char), set_insts);
    gettimeofday(&enc_end, NULL);
    if (ret != SGX_SUCCESS) {
        print_error_message(ret);
        return -1;
    }

    // Get
    char cmd[] = "GET 2048\n";
    ret = get_from_store(global_eid, cmd);
    if (ret != SGX_SUCCESS) {
        print_error_message(ret);
        return -1;
    }

    ret = destroy_store(global_eid, set_row_ctr);
    if (ret != SGX_SUCCESS) {
        print_error_message(ret);
        return -1;
    }
    printf("==========\n");
    clear_instructions(workload_size);

    file_diff = (double)(fileend.tv_usec - filestart.tv_usec) / 1000 + (double)(fileend.tv_sec - filestart.tv_sec);
    set_diff = (double)(enc_end.tv_usec - enc_start.tv_usec) / 1000 + (double)(enc_end.tv_sec - enc_start.tv_sec);
    
    FILE *resfile = fopen("readings.txt", "a");
    fprintf(resfile, "2048 Instructions\tFile Read Time: %f ms\tEnclave Set Time:%f ms\n", file_diff, set_diff);

    // Destroy the enclave
    sgx_destroy_enclave(global_eid);

    return 0;
}
