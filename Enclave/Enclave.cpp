#include <stdarg.h>
#include <stdio.h>

#include "Enclave.h"
#include "Enclave_t.h"

/* 
 * hello():
 *   Says hello world and prints arg passed
 */
void hello(int a)
{
    print("Hello World!");
    iprint(a);
}
