/*
 *  This file is part of the SGX-Step enclave execution control framework.
 *
 *  Copyright (C) 2017 Jo Van Bulck <jo.vanbulck@cs.kuleuven.be>,
 *                     Raoul Strackx <raoul.strackx@cs.kuleuven.be>
 *
 *  SGX-Step is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  SGX-Step is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with SGX-Step. If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sgx_trts.h>

// see asm.S
extern void asm_microbenchmark(void*);
extern char secret_str;
extern void zigzag_bench(void);
extern void trigger_func(void);
extern int a, b;

void  *get_trigger_adrs( void )
{
    return trigger_func;
}

void *get_bench_adrs( void )
{
    return asm_microbenchmark;
}

void *get_bench_data_adrs( void )
{
    return &a;
}

void *get_zz_adrs(void)
{
    return zigzag_bench;
}

void *get_bsearch_adrs(void)
{
    return bsearch;
}

#define ARRAY_SIZE 11
int array[ARRAY_SIZE] = { 0, 5, 8, 9, 10, 25, 63, 78, 80, 100, 101 };

int my_comp(const void *p1, const void *p2)
{
    int a = *((int*) p1), b = *((int*) p2);

    if (a == b)
        return 0;
    else if (a > b)
        return 1;
    else
        return -1;
}

int do_bsearch(int a)
{
    int idx = 0;
    int *adrs = (int*) bsearch(&a, array, ARRAY_SIZE, sizeof(int), my_comp);    
    trigger_func();
    if (adrs)
       idx = adrs - array;
    return idx;
}

int dummy;

void do_inst_slide(void *p)
{
    asm_microbenchmark(&a);
    trigger_func();
}

void do_zigzagger(void)
{
    a = 1;
    b = 0;
    zigzag_bench();

    a = 0;
    b = 1;
    zigzag_bench();
    trigger_func();
}

int dummy;
volatile int *gv = &dummy;

void set_global_var_pt(void *p)
{
    gv = (int*) p;
}

/*
 * Compute n^-1 mod m by extended euclidian method.
 * (from https://github.com/pantaloons/RSA/blob/master/single.c)
 */
int inverse(int n, int modulus)
{
	int a = n, b = modulus;
	int x = 0, y = 1, x0 = 1, y0 = 0, q, temp;
	while (b != 0)
	{
		q = a / b;
		temp = a % b;
		a = b;
		b = temp;
		temp = x; x = x0 - q * x; x0 = temp;
		temp = y; y = y0 - q * y; y0 = temp;
	}
	if (x0 < 0) x0 += modulus;
	return x0;
}

/*
 * Computes a^b mod n.
 * (long long multiplication prevents overflow)
 */
int modpow(long long a, long long b, long long n)
{
    long long res = 1;
    uint16_t mask = 0x8000;

    for (int i=15; i >= 0; i--)
    {
        res = (res * res) % n;
        if (b & mask)
        {
            res = (res * a) % n;
        }
        mask = mask >> 1;
    }
    return res;
}

/*
 * Example RSA key with p = 137 and q = 421.
 *
 * Defining rsa_n as a precompiler constant
 * slightly increases length of compiler-
 * generated asm code for ( (a * b) % rsa_n ).
 *
 * Global var rsa_d allocated on data page.
 */
#define rsa_n   57677
int rsa_e =     11;
int rsa_d =     20771; //16'b0101000100100011

void set_rsa_key(int k)
{
    rsa_d = k;
}

int rsa_decode(int cipher)
{
    int i, mask, r = 0;
    long long res;
    
    /* Blinding with 16-bit random factor. */
    if (sgx_read_rand((unsigned char*) &r, 2)
            != SGX_SUCCESS) return 0;
    cipher = cipher * modpow(r, rsa_e, rsa_n);
    
    /* 
     * Decrypt blinded message with inlined
     * square and multiply algorithm.
     * res = modpow(cipher, rsa_d, rsa_n);
     */
    res = 1; mask = 0x8000;

    for (i=15; i >= 0; i--)
    {
        #if 0
            *gv = i; //XXX
            asm("rdrand %%rax\n\t"
                :::"rax");
        #endif

        res = (res * res) % rsa_n;
        if (rsa_d & mask)
        {
            res = (res * cipher) % rsa_n;
        }
        mask = mask >> 1;
    }
    *gv = 111;

    /* Unblind result. */
    return (res * inverse(r, rsa_n)) % rsa_n;
}

int rsa_encode(int plain)
{
    return modpow(plain, rsa_e, rsa_n);
}
