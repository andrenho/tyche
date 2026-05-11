#include "../lib/vm.c"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#define EQ(a, b) (memcmp(a, b) == 0)

int main()
{
    // values
    printf("### Values\n");
    assert(value_type(create_value_integer(42)) == TT_INTEGER);
    assert(value_integer(create_value_integer(-42)) == -42);
    assert(fabsf(value_real(create_value_real(42.4f)) - 42.4f) < 0.00001f);
    assert(value_idx(create_value_idx(TT_FUNCTION, 42)) == 42);

    // values
    printf("### Stack\n");
    Stack s;
    stack_init(&s);
    stack_push(&s, create_value_integer(10));
    stack_push(&s, create_value_integer(20));
    stack_push(&s, create_value_integer(30));

    assert(stack_len(&s) == 3);

    stack_finalize(&s);
}