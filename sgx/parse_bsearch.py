#!/usr/bin/python3

import re
import sys
from elftools.elf.elffile import ELFFile
import libnemesis as nem

IN_FILE          = 'data/bsearch_100_left_right_eq/out.txt'
TRACE_FILE       = 'latency_trace.txt'
ENCLAVE_FILE     = 'data/bsearch_100_left_right_eq/encl.so'

# objdump from linux-sgx/sdk/tlibc/stdlib/bsearch.c
#
#0000000000008710 <bsearch>:
#    0x0000:       41 57                   push   %r15
#    0x0002:       41 56                   push   %r14
#    0x0004:       41 55                   push   %r13
#    0x0006:       41 54                   push   %r12
#    0x0008:       55                      push   %rbp
#    0x0009:       53                      push   %rbx
#    0x000a:       48 83 ec 18             sub    $0x18,%rsp
#    0x000e:       48 85 d2                test   %rdx,%rdx
#    0x0011:       48 89 7c 24 08          mov    %rdi,0x8(%rsp)
#    0x0016:       74 53                   je     877b <bsearch+0x6b>
#    0x0018:       49 89 f4                mov    %rsi,%r12
#    0x001b:       48 89 d3                mov    %rdx,%rbx
#    0x001e:       48 89 cd                mov    %rcx,%rbp
#    0x0021:       4d 89 c5                mov    %r8,%r13
#    0x0024:       eb 1a                   jmp    8750 <bsearch+0x40>
#    0x0026:       66 2e 0f 1f 84 00 00    nopw   %cs:0x0(%rax,%rax,1)
#    0x002d:       00 00 00 
#
#    base = (char *)p + size; lim--;
#    0x0030:       48 83 eb 01             sub    $0x1,%rbx
#    0x0034:       4d 8d 24 2e             lea    (%r14,%rbp,1),%r12
#    0x0038:       48 d1 eb                shr    %rbx
#    0x003b:       48 85 db                test   %rbx,%rbx
#    0x003e:       74 2b                   je     877b <bsearch+0x6b>
#
#    for (lim = nmemb; lim != 0; lim >>= 1) {
#    0x0040:       49 89 df                mov    %rbx,%r15
#    0x0043:       48 8b 7c 24 08          mov    0x8(%rsp),%rdi
#    0x0048:       49 d1 ef                shr    %r15
#    0x004b:       4c 89 fa                mov    %r15,%rdx
#    0x004e:       48 0f af d5             imul   %rbp,%rdx
#    0x0052:       4d 8d 34 14             lea    (%r12,%rdx,1),%r14
#    0x0056:       4c 89 f6                mov    %r14,%rsi
#
#    cmp = (*compar)(key, p);
#    0x0059:       41 ff d5                callq  *%r13
#    0x005c:       83 f8 00                cmp    $0x0,%eax
#
#    if (cmp == 0)
#    0x005f:       74 1f                   je     8790 <bsearch+0x80>
#
#    if (cmp > 0) {  /* key > p: move right */
#    0x0061:       7f cd                   jg     8740 <bsearch+0x30>
#
#    } /* else move left */
#    0x0063:       4c 89 fb                mov    %r15,%rbx
#    0x0066:       48 85 db                test   %rbx,%rbx
#    0x0069:       75 d5                   jne    8750 <bsearch+0x40>
#
#    return (NULL);
#    0x006b:       48 83 c4 18             add    $0x18,%rsp
#    0x006f:       31 c0                   xor    %eax,%eax
#    0x0071:       5b                      pop    %rbx
#    0x0072:       5d                      pop    %rbp
#    0x0073:       41 5c                   pop    %r12
#    0x0075:       41 5d                   pop    %r13
#    0x0077:       41 5e                   pop    %r14
#    0x0079:       41 5f                   pop    %r15
#    0x007b:       c3                      retq   
#    0x007c:       0f 1f 40 00             nopl   0x0(%rax)
#
#    return ((void *)p);
#    0x0080:       48 83 c4 18             add    $0x18,%rsp
#    0x0084:       4c 89 f0                mov    %r14,%rax
#    0x0087:       5b                      pop    %rbx
#    0x0088:       5d                      pop    %rbp
#    0x0089:       41 5c                   pop    %r12
#    0x008b:       41 5d                   pop    %r13
#    0x008d:       41 5e                   pop    %r14
#    0x008f:       41 5f                   pop    %r15
#    0x0091:       c3                      retq   
#    0x0092:       66 2e 0f 1f 84 00 00    nopw   %cs:0x0(%rax,%rax,1)
#    0x0099:       00 00 00 
#    0x009c:       0f 1f 40 00             nopl   0x0(%rax)

BS_OFFSET_CALL      = 0x59
BS_OFFSET_RET1      = 0x7b
BS_OFFSET_RET2      = 0x91
BS_OFFSET_CMP       = 0x5c
BS_OFFSET_JE        = 0x5f
BS_OFFSET_MOV_EQ    = 0x84
BS_OFFSET_MOV_LEFT  = 0x63

(latencies, rips) = nem.parse_latencies(IN_FILE)

with open( ENCLAVE_FILE ,'rb') as f:
    elf = ELFFile(f)
    symtab  = elf.get_section_by_name('.symtab')
    bs_rip = symtab.get_symbol_by_name('bsearch')[0]['st_value']
    call_rip = bs_rip + BS_OFFSET_CALL
    ret1_rip = bs_rip + BS_OFFSET_RET1
    ret2_rip = bs_rip + BS_OFFSET_RET2
    cmp_rip   = bs_rip + BS_OFFSET_CMP
    je_rip   = bs_rip + BS_OFFSET_JE    # sgx-step somehow consistently skips this je

bs_idx_entry = rips.index(bs_rip)
try:
    bs_idx_ret = rips.index(ret1_rip)
except ValueError:
    bs_idx_ret = rips.index(ret2_rip)

print("parse_bsearch.py: found bsearch@{} (steps {}-{})".format(hex(bs_rip), bs_idx_entry, bs_idx_ret))

cmp_indices = [i for i,r in enumerate(rips) if r == cmp_rip]
cmp_latencies = []
cmp_labels = []

cmp_labels_eq    = ['cmp', 'add', 'mov',  'pop', 'pop', 'pop',  'pop', 'pop', 'pop', 'retq']
cmp_labels_left  = ['cmp', 'jg',  'mov', 'test', 'jne', 'mov',  'mov', 'shr', 'mov', 'imul', 'lea', 'mov',  'callq']
cmp_labels_right = ['cmp', 'jg',  'sub', 'lea',  'shr', 'test', 'je',  'mov', 'mov', 'shr',  'mov', 'imul', 'lea']

for i,cmp_idx in enumerate(cmp_indices):
    cmp_path_end = cmp_idx + len(cmp_labels_right)
    cmp_latencies.append(latencies[cmp_idx:cmp_path_end])
    print("parse_bsearch.py: found secret-dependent code path (steps {}-{})".format(cmp_idx, cmp_path_end))
    print([hex(x) for x in rips[cmp_idx:cmp_path_end]])

    if (rips[cmp_idx+2] == bs_rip+BS_OFFSET_MOV_EQ):
        print('--> EQ')
        cmp_labels.append(cmp_labels_eq)
    elif (rips[cmp_idx+2] == bs_rip+BS_OFFSET_MOV_LEFT):
        print('--> LEFT')
        cmp_labels.append(cmp_labels_left)
    else:
        print('--> RIGHT')
        cmp_labels.append(cmp_labels_right)

cmp_labels = zip(cmp_labels[0], cmp_labels[1], cmp_labels[2])
nem.boxplot(zip(cmp_latencies[0], cmp_latencies[1], cmp_latencies[2], cmp_labels))

nem.plot_latency_avg_trace(latencies, rips, bs_idx_entry, bs_idx_ret+1) #, cmp_indices)
