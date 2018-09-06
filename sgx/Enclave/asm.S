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

    .text
    .global trigger_func
    .align 0x1000   /* 4KiB */
trigger_func:
    retq
    .space 0x1000 /* 4 KiB */

    .data
    .global a
    .global b
    .align 0x1000   /* 4KiB */
a:
    .space 0x8
b:
    .space 0x8
    .space 0x1000 /* 4 KiB */

.macro lea_pic symbol, reg
    lea   \symbol(%rip), \reg
.endm

    .text
    .global zigzag_bench
    .align 0x1000   /* 4KiB */
    .type zigzag_bench,@function
/*
 * Code snippet from
 * https://www.usenix.org/system/files/conference/usenixsecurity17/sec17-lee-sangho.pdf
 */
zigzag_bench:
block0:     lea_pic block1, %r15
            lea_pic block2, %r14
            cmp $0, a(%rip)
            cmove %r14, %r15
block0.j:   jmp zz1

/*
 * NOTE: explicitly align secret-dependent code snippet to harden it against
 * known page-granular (4KiB) plus cache line-granular (64B) attacks.
 */
/* --------------------  BEGIN PAGE/CACHELINE -------------------- */
.align 0x40   /* 64B */
block1:     nop
            lea_pic block5, %r15
block1.j:   jmp zz2
block2:     lea_pic block3, %r15
            lea_pic block4, %r14
            cmp $0, b(%rip)
            cmove %r14, %r15
block2.j:   jmp zz3
block3:     nop
            lea_pic block5, %r15
block3.j:   jmp zz4
block4:     nop
block5:     nop

zigzag_bench_ret:
            retq

zz1:        jmp block1.j
zz2:        jmp block2.j
zz3:        jmp block3.j
zz4:        jmpq *%r15

/* --------------------  END CACHELINE -------------------- */
.space 0x1000   /* 4KiB */
