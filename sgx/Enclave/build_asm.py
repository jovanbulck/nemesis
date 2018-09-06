#!/usr/bin/python3

import string
import sys

if (len(sys.argv) != 2):
    print("usage: build_asm.py expects one argument <inst_slide_len>")
    exit(1)

NB_INST     = int(sys.argv[1])

ASM_INST    = "nop"
#ASM_INST    = "add $0x1, %rax"
#ASM_INST    = "fscale"
#ASM_INST    = "rdrand %rax"
#ASM_INST    = "lfence"

#ASM_INST    = "mov (%rdi),%rax"
#ASM_INST    = "mov %rax,(%rdi)"
#ASM_INST    = "movnti %rax,(%rdi)"
#ASM_INST    = "clflush (%rdi)\n mov (%rdi),%rax"

#ASM_INST    = "mov $0x0000000000000000, %rdx\n mov $0x0000000000000000,%rax\n mov $0xffffffffffffffff, %rbx\n div %rbx"
#ASM_INST    = "mov $0x0000000000000000, %rdx\n mov $0xffffffffffffffff,%rax\n mov $0xffffffffffffffff, %rbx\n div %rbx"
#ASM_INST    = "mov $0x00000000ffffffff, %rdx\n mov $0xffffffffffffffff,%rax\n mov $0xffffffffffffffff, %rbx\n div %rbx"
#ASM_INST    = "mov $0x0fffffffffffffff, %rdx\n mov $0xffffffffffffffff,%rax\n mov $0xffffffffffffffff, %rbx\n div %rbx"

template = string.Template('''
    /* ====== auto generated asm code from Python script ======= */

    .text
    .align 0x1000 /* 4 KiB */
    .global asm_microbenchmark
    asm_microbenchmark:
    $asmCode

    call trigger_func

    asm_microbenchmark_done:
    retq
''')

asm = (ASM_INST + '\n') * (NB_INST)
code = template.substitute(asmCode=asm)

with open('asm_micro.S', 'w') as the_file:
    the_file.write(code)
