#include <stdio.h>

#include <stdlib.h>

#include <sys/stat.h>

#define CBT_IMPLEMENTATION
#define CBT_SELF_REBUILD_DEBUG_MODE
#define REBUILD_UNWIND
#include "cbt.h"

#if 0 // NOTE: IDEAL USAGE OF PROJECT
int main() {
    project p = create_project();
    project_add_dependencies(&p, ...); // NOTE: add other projects which use the same build system
    project_set_flags(&p, ...);
    project_set_sources(&p, ...);

    backup b = project_create_backup(p);
    ProjectBuildStatus status = project_build(p);
    if (status != PROJECT_BUILD_SUCCESS) {
        rollback_project(p, b);
    }
}
#endif

int main(int argc, char **argv) {
  enable_self_rebuild();

  bld_t b = bld_create("test/main", str_list_create("test/main.c", "test/example.c"));

  bld_run(b, "-O2", "-funsafe-math-optimizations");

  alloc_free();
  return EXIT_SUCCESS;
}
