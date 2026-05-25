#include <stdio.h>

#include "tyche.h"

int main()
{
    TycheVM* T = tyc_new();
    tyc_destroy(T);

    printf("This is not implemented yet.\n");
    return 1;
}