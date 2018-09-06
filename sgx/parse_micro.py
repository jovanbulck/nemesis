#!/usr/bin/python3

import re
import sys
from elftools.elf.elffile import ELFFile
import libnemesis as nem

MAX_MEASUREMENTS    = 10000
    
def parse_bench_file(f, skip=0):
    latencies = []
    irq_cnt = 0
    to_skip = skip

    with open(f, 'r') as fi:
        for line in fi:
            m = re.search('RIP=0x([0-9A-Fa-f]+); ACCESSED=([0-9]); latency=([0-9]+)', line)
            if m:
                if not to_skip:
                    latencies.append(int(m.groups()[2]))
                    to_skip = skip
                    irq_cnt += 1
                else:
                    to_skip -= 1

            if irq_cnt >= MAX_MEASUREMENTS:
                break

    # last two instructions call trigger_func  and disable single stepping
    latencies.pop()
    latencies.pop()

    print('parse_micro.py: extracted', len(latencies), 'measurements')
    return latencies

#nem.hist( parse_bench_file('out.txt', 0) )
#exit(0)

l0 = parse_bench_file('data/ubench/nop.txt')
l1 = parse_bench_file('data/ubench/nop-pte-flush.txt')
l2 = parse_bench_file('data/ubench/load-encl.txt')
l3 = parse_bench_file('data/ubench/load-pte-data-flush.txt')
l4 = parse_bench_file('data/ubench/load-pte-code-data-flush.txt')
nem.hist([l0,l1,l2,l3,l4], ['nop baseline  : nop',
                            'code PTE flush: nop',
                            'load baseline      : mov (%rdi), %rax',
                            'data PTE flush     : mov (%rdi), %rax',
                            'code+data PTE flush: mov (%rdi), %rax'],
         legend_ncol=2, filename='pte')
#exit(0)

l0 = parse_bench_file('data/ubench/nop.txt')
l1 = parse_bench_file('data/ubench/add.txt')
l2 = parse_bench_file('data/ubench/lfence.txt')
l3 = parse_bench_file('data/ubench/fscale.txt')
l4 = parse_bench_file('data/ubench/rdrand.txt')
nem.hist([l0,l1,l2,l3,l4], ['no-operation      : nop',
                            'register increment: add $0x1, %rax',
                            'load serialization  : lfence',
                            'floating point scale: fscale',
                            'generate random num : rdrand'],
         legend_loc='upper left', legend_ncol=2, filename='inst')

l0 = parse_bench_file('data/ubench/load-encl.txt')
l1 = parse_bench_file('data/ubench/load-enclave-clflush.txt', 1)
l2 = parse_bench_file('data/ubench/store-encl.txt')
l3 = parse_bench_file('data/ubench/movnti-encl.txt')
nem.hist([l0,l1,l2,l3], ['load cache hit    : mov    (%rdi), %rax',
                         'load cache miss   : mov    (%rdi), %rax',
                         'store             : mov    %rax, (%rdi)',
                         'store non-temporal: movnti %rax, (%rdi)'],
         filename='cache')

l0 = parse_bench_file('data/ubench/div/div-rax-min.txt', 3)
l1 = parse_bench_file('data/ubench/div/div-rdx-min.txt', 3)
l2 = parse_bench_file('data/ubench/div/div-rdx-half.txt', 3)
l3 = parse_bench_file('data/ubench/div/div-rdx-max.txt', 3)
nem.hist([l0,l1,l2,l3], ['rdx=0x0000000000000000; rax=0x0000000000000000',
                         'rdx=0x0000000000000000; rax=0xffffffffffffffff',
                         'rdx=0x00000000ffffffff; rax=0xffffffffffffffff',
                         'rdx=0x0fffffffffffffff; rax=0xffffffffffffffff'],
         filename='div')
