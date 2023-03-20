#ifndef CBT_H_
#define CBT_H_

// TODO: REDIRECT FORKED STDOUT
// TODO: LOGGING SYSTEM (+ERRNO)
// TODO: ASYNC COMMAND EXECUTION
// TODO: VARIADIC JOIN FOR LISTS
// TODO: RECOMPILE SELF - WIP (cbt.c)
// TODO: ONLY RECOMPILE WHEN FILE CHANGED
// TODO: EXIT GRACEFULLY WHEN ENCOUNTERING ERROR (REVERT)

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define CC "gcc"

// ALLOCATOR

#define PAGE_SIZE 16384

typedef struct {
  void *data;
  int end;
} page;

static struct {
  void *data;
  int cur_idx;
} allocator;

void *alloc(size_t n_bytes);
void free_page(size_t idx);
void alloc_free();

#define LIST(name, type)                                                       \
  typedef struct {                                                             \
    type *data;                                                                \
    int count;                                                                 \
    int capacity;                                                              \
  } name##_t;                                                                  \
  name##_t name##_init(int);                                                   \
  void name##_append(name##_t *, type);                                        \
  name##_t name##_join(name##_t, name##_t);

typedef const char *str_t;
LIST(str_list, str_t);

typedef struct {
  str_list_t sources;
  str_list_t flags;
} build;

str_t str_list_concat(str_list_t, str_t);

str_list_t str_list_create_impl(int ignore, ...);
#define str_list_create(...) str_list_create_impl(0, __VA_ARGS__, NULL)

void exec_cmd(str_list_t);

build create_build(void);

void set_flags_impl(build *, ...);
#define set_flags(b, ...) set_flags_impl(b, __VA_ARGS__, NULL);

void set_sources_impl(build *, ...);
#define set_sources(b, ...) set_sources_impl(b, __VA_ARGS__, NULL);

void run_build(build);

void run_rebuild_impl(int, char **, const char *);

#define run_rebuild() run_rebuild_impl(argc, argv, __FILE__)

#endif // CBT_H_
#ifdef CBT_IMPLEMENTATION

void *alloc(size_t n_bytes) {
  if (allocator.data == NULL) {
    allocator.data = malloc(PAGE_SIZE);
    allocator.cur_idx = 0;
  }
  size_t idx = (size_t)allocator.data + allocator.cur_idx;
  allocator.cur_idx += n_bytes;
  assert(allocator.cur_idx < PAGE_SIZE && "Allocator full >.<");
  return (void *)idx;
}

void alloc_free() { free(allocator.data); }

#define LIST_IMPL(name, type)                                                  \
  name##_t name##_init(int initial_cap) {                                      \
    return (name##_t){                                                         \
        .data = alloc(initial_cap * sizeof(type)),                             \
        .count = 0,                                                            \
        .capacity = initial_cap,                                               \
    };                                                                         \
  }                                                                            \
  void name##_append(name##_t *xs, type elem) {                                \
    if (xs->count < xs->capacity) {                                            \
      xs->data[xs->count++] = elem;                                            \
      return;                                                                  \
    }                                                                          \
    int nc = xs->capacity * 2;                                                 \
    typeof(xs->data) new_data = alloc(nc * sizeof(xs->data[0]));               \
    memcpy(new_data, xs->data, xs->count * sizeof(xs->data[0]));               \
    xs->data = new_data;                                                       \
    xs->capacity = nc;                                                         \
    xs->data[xs->count++] = elem;                                              \
  }                                                                            \
  name##_t name##_join(name##_t a, name##_t b) {                               \
    name##_t res = name##_init(a.count + b.count);                             \
    for (int i = 0; i < a.count; ++i) {                                        \
      name##_append(&res, a.data[i]);                                          \
    }                                                                          \
    for (int i = 0; i < b.count; ++i) {                                        \
      name##_append(&res, b.data[i]);                                          \
    }                                                                          \
    return res;                                                                \
  }

LIST_IMPL(str_list, str_t);

str_list_t str_list_create_impl(int ignore, ...) {
  str_list_t res = str_list_init(4);
  va_list args;
  va_start(args, ignore);
  for (char *arg = va_arg(args, char *); arg != NULL;
       arg = va_arg(args, char *)) {
    str_list_append(&res, arg);
  }
  va_end(args);
  return res;
}

build create_build(void) {
  return (build){
      .sources = str_list_init(4),
      .flags = str_list_init(4),
  };
}

str_t str_list_concat(str_list_t xs, str_t sep) {
  if (xs.count == 0)
    return "";
  size_t ln = 0;
  for (size_t i = 0; i < xs.count; ++i) {
    ln += strlen(xs.data[i]);
  }
  const size_t res_len = (xs.count - 1) * strlen(sep) + ln + 1;
  char *res = alloc(sizeof(char) * res_len);
  if (res == NULL)
    assert(0 && "RIP BOZO");

  size_t idx = 0;

  for (size_t i = 0; i < xs.count; ++i) {
    size_t sl = strlen(xs.data[i]);
    memcpy(res + idx, xs.data[i], sl);
    idx += sl;
    if (i < xs.count - 1) {
      memcpy(res + idx, sep, strlen(sep));
      idx += strlen(sep);
    }
  }
  return res;
}

void set_sources_impl(build *b, ...) {
  va_list args;
  va_start(args, b);
  for (char *next = va_arg(args, char *); next != NULL;
       next = va_arg(args, char *)) {
    str_list_append(&b->sources, next);
  }
  va_end(args);
}

void set_flags_impl(build *b, ...) {
  va_list args;
  va_start(args, b);
  for (char *next = va_arg(args, char *); next != NULL;
       next = va_arg(args, char *)) {
    str_list_append(&b->flags, next);
  }
  va_end(args);
}

void exec_cmd(str_list_t cmd) {
  printf("[RUNNING COMMAND] %s\n", str_list_concat(cmd, " "));
  str_list_append(&cmd, NULL);
  int pid = fork();
  if (pid < 0)
    assert(0 && "Failed to create fork ;-;");
  if (pid == 0) {
    int c = execvp(cmd.data[0], (char *const *)cmd.data);
  }
  int t;
  waitpid(pid, &t, 0);
}

void run_build(build b) {
  // argv[0] must be the name of the command (i hate this)
  // execvp doesnt force this, so the caller must do it
  // this is v ugly code tho >.<
  str_list_t cmd = str_list_init(1);
  str_list_append(&cmd, CC);
  str_list_t res = str_list_join(cmd, str_list_join(b.sources, b.flags));

  exec_cmd(res);
}

void run_rebuild_impl(int argc, char **argv, const char *file) {
  struct stat sf, sb;
  stat(file, &sf);
  stat(argv[0], &sb);
  if (sf.st_mtime > sb.st_mtime) {
    printf("Rebuilding cbt...\n");
    exec_cmd(str_list_create("gcc", "-o", "cbt", "cbt.c"));
    str_list_t argv_l = str_list_init(argc);
    argv_l.data = (const char **)argv;
    argv_l.count = argc;
    exec_cmd(argv_l);
    exit(EXIT_SUCCESS);
  }
}

#endif
