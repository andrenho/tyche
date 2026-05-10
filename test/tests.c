#include "../lib/vm.c"

#include <assert.h>
#include <math.h>
#include <string.h>

#define EQ(a, b) (memcmp(a, b) == 0)

int main()
{
    // values
    assert(value_type(create_value_integer(42)) == TT_INTEGER);
    assert(value_integer(create_value_integer(-42)) == -42);
    assert(fabsf(value_real(create_value_real(42.4f)) - 42.4f) < 0.00001f);
    assert(value_idx(create_value_idx(TT_FUNCTION, 42)) == 42);
}