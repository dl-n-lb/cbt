#include <stdio.h>
#include <stdlib.h>

#define CBT_IMPLEMENTATION
#include "cbt.h"



int main(int argc, char **argv) {
    build b = create_build();
    setSources(&b, "test/main.c", "test/other_object.c");
    return EXIT_SUCCESS;
}
