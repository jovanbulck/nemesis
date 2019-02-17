#ifndef FOO_H_INC
#define FOO_H_INC

#include <sancus/sm_support.h>

extern struct SancusModule foo;

void SM_ENTRY(foo) init_foo(void);
int SM_ENTRY(foo) leak_foo(void);

#endif
