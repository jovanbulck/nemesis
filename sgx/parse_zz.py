#!/usr/bin/python3

import re
import sys
from elftools.elf.elffile import ELFFile
import libnemesis as nem

import pandas as pd, numpy as np
import matplotlib.pyplot as plt

IN_FILE          = 'data/zz_100/out.txt'
TRACE_FILE       = 'latency_trace.txt'
ENCLAVE_FILE     = 'data/zz_100/encl.so'

(latencies, rips) = nem.parse_latencies(IN_FILE)

#dump_latency_avg_trace(latencies, TRACE_FILE)

with open( ENCLAVE_FILE ,'rb') as f:
    elf = ELFFile(f)
    symtab  = elf.get_section_by_name('.symtab')
    b0_rip = symtab.get_symbol_by_name('block0')[0]['st_value']
    zz4_rip = symtab.get_symbol_by_name('zz4')[0]['st_value']
    b5_rip  = symtab.get_symbol_by_name('block5')[0]['st_value']

b0_indices = [i for i,n in enumerate(rips) if n == b0_rip]
b5_indices = [i for i,n in enumerate(rips) if n == b5_rip]
zz_latencies = []

for i,b0_idx in enumerate(b0_indices):
    zz4_idx_start = b0_idx + rips[b0_idx:].index(zz4_rip)
    assert(rips[zz4_idx_start] == zz4_rip)
    zz4_idx_end   = zz4_idx_start + 1 + rips[zz4_idx_start+1:].index(zz4_rip)
    assert(rips[zz4_idx_end] == zz4_rip)
    zz_latencies.append(latencies[zz4_idx_start:zz4_idx_end])
    print("parse_zz.py: found secret-dependent code path zz4@{}->zz4 (steps {}-{})".format(hex(zz4_rip), zz4_idx_start, zz4_idx_end))
    print([hex(x) for x in rips[zz4_idx_start:zz4_idx_end]])

zz_labels = [('jmpq','jmpq'), ('nop','lea'), ('lea','lea'), ('jmp','cmp'), ('jmp','cmove'), ('jmp','jmp'), ('jmp','jmp'), ('jmp','jmp'), ('jmpq','jmpq')]
nem.boxplot2(zip(zz_latencies[0], zz_latencies[1], zz_labels))

exit(0)


b0_a_idx  = rips.index(b0_rip)
b5_a_idx  = rips.index(b5_rip)
zz4_a_idx = rips[b0_a_idx:b5_a_idx].index(zz4_rip)
print("parse_zz.py: found secret-dependent A=1 code path zz4@{}->b5@{} (steps {}-{})".format(hex(zz4_rip), hex(b5_rip), zz4_a_idx, b5_a_idx))

rips_b = rips[b5_a_idx:]
b0_b_idx  = rips_b.index(b0_rip)
b5_b_idx  = rips_b.index(b5_rip)
print(b0_b_idx  , b5_b_idx  )
print(rips_b[b0_b_idx:b5_b_idx])
zz4_b_idx = rips_b[b0_b_idx:b5_b_idx].index(zz4_rip)
print("parse_zz.py: found secret-dependent A=0 code path zz4@{}->b5@{} (steps {}-{})".format(hex(zz4_rip), hex(b5_rip), zz4_b_idx, b5_b_idx))
exit(0)


nem.boxplot2(latencies[zz_4_idx:b5_idx])

exit(0)

d = {}

for i in range(zz4_idx, b5_idx):
    d['step-{}'.format(i)] = latencies[i]

df = pd.DataFrame(data=d)
df = df.clip_upper(8600)
print(df)
df.boxplot()
plt.show()


# TODO from zz4 to b5: construct boxplots

exit(0)


prev = 0
run = 0

with open(IN_FILE, 'r') as fi, open(BENCH_FILE, 'w') as fb, open(TRACE_FILE, 'w') as ft:
    for line in fi:
        m = re.search('RIP=0x([0-9A-Fa-f]+); latency=([0-9]+)', line)
        if m:
            cur = int(m.groups()[0], base=16)
            latency = int(m.groups()[1])

            # collect latency measurements from zz execution
            if (cur == zz_start):
                run += 1
                in_zz = 1
                fired = 0
            if (in_zz):
                ft.write(str(latency) + '\n')
            if (cur == zz_end):
                in_zz = 0

            # collect secret-dependent inst latencies for later plotting
            if (prev == zz_secret_jmp and not fired):
                fb.write(str(latency) + '\n')
                fired = 1

            prev = cur
