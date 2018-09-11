#ifndef SM_IO_WRAP_H
#define SM_IO_WRAP_H

#include <stdio.h>
#include <sancus/sm_support.h>

    int __attribute__((noinline)) putchar(int c);
    void __attribute__((noinline)) printf0(const char* str);
    void __attribute__((noinline)) printf1(const char* fmt, int arg1);
    void __attribute__((noinline)) printf2(const char* fmt, int arg1, int arg2);
    void __attribute__((noinline)) printf3(const char* fmt, int arg1, int arg2, int arg3);
    
    #if __SANCUS_SIM
        #define sm_io_init()
        #define dump_sm_layout(sm)
    #else
        void sm_io_init(void);
        void dump_sm_layout(struct SancusModule *sm);
    #endif
    
#endif /* SM_IO_WRAP_H */
