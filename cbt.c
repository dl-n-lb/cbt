#include <stdio.h>
#include <stdlib.h>

#include <sys/stat.h>

#define CBT_IMPLEMENTATION
#include "cbt.h"

#if 0 // NOTE: IDEAL USAGE OF BUILD
int main() {
    build b = create_build;
    file file;
    FOR_FILES_IN_DIR(file, "./", {
            if (IS_SOURCE_FILE(file)) build_add_source(&b, file);
        });
    build_set_flags(&b, ...);

    BuildStatus status = build_run(b);
    return status;
}
#endif

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
  run_rebuild();

  build b = create_build();
  set_sources(&b, "test/main.c", "test/example.c");
  set_flags(&b, "-o", "test/main", "-O2");

  run_build(b);

  alloc_free();
  return EXIT_SUCCESS;
}
