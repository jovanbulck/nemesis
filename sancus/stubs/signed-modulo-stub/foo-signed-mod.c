#include "foo-signed-mod.h"
#include <sancus_support/sm_io.h>

DECLARE_SM(foo, 0x1234);

#define FOO_A_CST -0x3f
#define FOO_B_CST 0x6

int SM_DATA(foo) foo_a;
int SM_DATA(foo) foo_b;

void SM_ENTRY(foo) init_foo(void)
{
    foo_a = FOO_A_CST;
    foo_b = FOO_B_CST;
}

int SM_ENTRY(foo) leak_foo(void)
{
    volatile int c = foo_a % foo_b;
    return 0;
}
