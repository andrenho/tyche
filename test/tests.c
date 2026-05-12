#include "../lib/vm.c"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#define EQ(a, b) (memcmp(a, b) == 0)

int main()
{
    {
        printf("### Values\n");
        assert(value_type(create_value_integer(42)) == TT_INTEGER);
        assert(value_integer(create_value_integer(-42)) == -42);
        assert(fabsf(value_real(create_value_real(42.4f)) - 42.4f) < 0.00001f);
        assert(value_idx(create_value_idx(TT_FUNCTION, 42)) == 42);
    }

    {
        printf("### Stack\n");

        Stack s;
        stack_init(&s);

        stack_push(&s, create_value_integer(10));
        stack_push(&s, create_value_integer(20));
        stack_push(&s, create_value_integer(30));

        VALUE v;
        assert(stack_len(&s) == 3);
        assert(stack_at(&s, 0, &v) == T_OK); assert(value_integer(v) == 10);
        assert(stack_at(&s, 1, &v) == T_OK); assert(value_integer(v) == 20);
        assert(stack_at(&s, -1, &v) == T_OK); assert(value_integer(v) == 30);
        assert(stack_at(&s, -2, &v) == T_OK); assert(value_integer(v) == 20);

        assert(stack_at(&s, 3, &v) == T_ERR_STACK_ACCESS_OUT_OF_RANGE);
        assert(stack_at(&s, -4, &v) == T_ERR_STACK_ACCESS_OUT_OF_RANGE);

        assert(stack_set(&s, 1, create_value_integer(99)) == T_OK);
        assert(stack_at(&s, 1, &v) == T_OK); assert(value_integer(v) == 99);
        assert(stack_at(&s, -2, &v) == T_OK); assert(value_integer(v) == 99);

        assert(stack_pop(&s, NULL) == T_OK);
        assert(stack_pop(&s, NULL) == T_OK);
        assert(stack_at(&s, -1, &v) == T_OK); assert(value_integer(v) == 10);
        assert(stack_pop(&s, NULL) == T_OK);
        assert(stack_len(&s) == 0);

        assert(stack_pop(&s, NULL) == T_ERR_STACK_UNDERFLOW);

        stack_finalize(&s);
    }

    {
        printf("### Stack with frame pointer\n");

        Stack s;
        stack_init(&s);

        stack_push(&s, create_value_integer(10));
        stack_push(&s, create_value_integer(20));
        stack_push_fp(&s);
        stack_push(&s, create_value_integer(30));
        stack_push(&s, create_value_integer(40));
        stack_push(&s, create_value_integer(50));

        VALUE v;
        assert(stack_len(&s) == 3);
        assert(stack_at(&s, 0, &v) == T_OK); assert(value_integer(v) == 30);
        assert(stack_at(&s, 1, &v) == T_OK); assert(value_integer(v) == 40);
        assert(stack_at(&s, -1, &v) == T_OK); assert(value_integer(v) == 50);
        assert(stack_at(&s, -2, &v) == T_OK); assert(value_integer(v) == 40);

        assert(stack_set(&s, -2, create_value_integer(99)) == T_OK);
        assert(stack_at(&s, -2, &v) == T_OK); assert(value_integer(v) == 99);

        assert(stack_at(&s, 3, &v) == T_ERR_STACK_ACCESS_OUT_OF_RANGE);
        assert(stack_at(&s, -4, &v) == T_ERR_STACK_ACCESS_OUT_OF_RANGE);

        stack_pop_fp(&s);

        assert(stack_len(&s) == 2);
        assert(stack_at(&s, 0, &v) == T_OK); assert(value_integer(v) == 10);
        assert(stack_at(&s, 1, &v) == T_OK); assert(value_integer(v) == 20);
        assert(stack_at(&s, -1, &v) == T_OK); assert(value_integer(v) == 20);
        assert(stack_at(&s, -2, &v) == T_OK); assert(value_integer(v) == 10);

        assert(stack_at(&s, 2, &v) == T_ERR_STACK_ACCESS_OUT_OF_RANGE);
        assert(stack_at(&s, -3, &v) == T_ERR_STACK_ACCESS_OUT_OF_RANGE);

        stack_finalize(&s);
    }

    // TODO - stack set
}