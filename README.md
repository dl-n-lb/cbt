# C Build Tool (CBT)
An all-c build tool for building c projects.
The only thing you need is a compatible c compiler.

# Use
For the first compilation, run:
```console
CC cbt.c -o cbt
./cbt
```
And for all subsequent uses:
```console
./cbt
```
The tool will recompile itself if the build file is changed.
# Example cbt.c file
An example file might look like the following:
```c
#define CBT_IMPLEMENTATION
#define CBT_SELF_REBUILD_DEBUG_MODE
#define REBUILD_UNWIND
#include "cbt.h"

int main(int argc, char **argv) {
  enable_self_rebuild();

  bld_t b = bld_create("test/main", str_list_create("test/main.c", "test/example.c"));

  bld_run(b, "-O2", "-funsafe-math-optimizations");

  alloc_free();
  return EXIT_SUCCESS;
}
```
Note the use of the macro `enable_self_rebuild()` to allow the program to rebuild itself.
Other than that, the only notable call is to `alloc_free()` which manually frees the (arena allocated) build system memory. This would otherwise be done by the operating system in most cases, but the function is there in case.
