#ifndef CBT_H_
#define CBT_H_

// TODO: PANIC AND CRASH REVERTS BUILD
// TODO: VARIADIC JOIN FOR LISTS
// TODO: PANIC UNWINDS BUILD STACK TO REVERT CHANGES
// TODO: IT WOULD BE COOL IF I DIDNT NEED TO SPECIFY DEPENDENCIES AND THE BUILD
// TOOL JUST FINDS THEM ALL FOR ME
//       OBV. STILL NEED TO SPECIFY LIB LOCATIONS
// TODO: ASYNC COMMAND EXECUTION
// TODO: EXIT GRACEFULLY WHEN ENCOUNTERING ERROR (REVERT)

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <assert.h>
#include <errno.h>
#include <string.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define UNIMPLEMENTED assert(0 && "unimplemented");

#define CC "gcc"

// ALLOCATOR

#define PAGE_SIZE 16384

typedef struct {
  void *data;
  int end;
} page_t;

static struct {
  void *data;
  size_t cur_idx;
} allocator;

void *alloc(size_t n_bytes);
void free_page(size_t idx);
void alloc_free(void);

// LIST

#define LIST(name, type)                                                       \
  typedef struct {                                                             \
    type *data;                                                                \
    size_t count;                                                              \
    size_t capacity;                                                           \
  } name##_t;                                                                  \
  name##_t name##_init(size_t);                                                \
  void name##_append(name##_t *, type);                                        \
  name##_t name##_join(name##_t, name##_t);

typedef const char *str_t;
LIST(str_list, str_t)

str_t str_list_concat(str_list_t, str_t);

str_list_t str_list_create_impl(int ignore, ...);
#define str_list_create(...) str_list_create_impl(0, __VA_ARGS__, NULL)

// LOG

void flog(FILE *, str_t, str_t, va_list);
void info(str_t, ...); //  all logs print msg to stderr
void warn(str_t, ...);
void error(str_t, ...); // NOTE: should error be different to warn?
void panic(str_t, ...); // TODO: needs to error, rollback build, exit
void sys_panic(void);

// CMD

int exec_cmd(str_list_t);
int exec_cmd_fd(str_list_t, int, int);

#define mkdirs(dirs) exec_cmd(str_list_create("mkdir", dirs, "-p"));
#define cc(...) exec_cmd(str_list_create(CC, __VA_ARGS__));

bool file_exists(str_t f);

// BLD COMMANDS (SINGLE CALL TO CC)

#define CACHE_DIRECTORY "./.cache" // TODO: if .cache doesnt exist, create it

typedef struct {
  str_t target;
  str_list_t srcs;
} bld_t;

#define BLD_STACK_SIZE 1024

typedef struct {
  bld_t *stack;
  size_t count;
} bld_stack_t;

static bld_stack_t bld_stack;

void add_to_stack(bld_t);

bld_t pop_stack(void);

void unwind_stack(void);

bld_t bld_create(str_list_t srcs, str_t tgt);

void bld_set_flags_impl(bld_t *, ...);
#define bld_set_flags(b, ...) bld_set_flags_impl(b, __VA_ARGS__, NULL)

// va args for flags (TODO: should this be a str_list_t?)
void bld_run_impl(bld_t, ...);
#define bld_run(b, ...) bld_run_impl(b, __VA_ARGS__, NULL)

bool need_to_rebuild(str_t, str_t);

void self_rebuild_impl(int, char **, str_t);
#define enable_self_rebuild() self_rebuild_impl(argc, argv, __FILE__)

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
  name##_t name##_init(size_t initial_cap) {                                   \
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
    size_t nc = xs->capacity * 2;                                              \
    typeof(xs->data) new_data = alloc(nc * sizeof(xs->data[0]));               \
    memcpy(new_data, xs->data, xs->count * sizeof(xs->data[0]));               \
    xs->data = new_data;                                                       \
    xs->capacity = nc;                                                         \
    xs->data[xs->count++] = elem;                                              \
  }                                                                            \
  name##_t name##_join(name##_t a, name##_t b) {                               \
    name##_t res = name##_init(a.count + b.count);                             \
    for (size_t i = 0; i < a.count; ++i) {                                     \
      name##_append(&res, a.data[i]);                                          \
    }                                                                          \
    for (size_t i = 0; i < b.count; ++i) {                                     \
      name##_append(&res, b.data[i]);                                          \
    }                                                                          \
    return res;                                                                \
  }

LIST_IMPL(str_list, str_t)

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
  res[res_len - 1] = '\0';
  return res;
}

// LOG

void flog(FILE *f, str_t ty, str_t fmt, va_list args) {
  fprintf(f, "[%s] ", ty);
  fprintf(f, fmt, args);
  fprintf(f, "\n");
}

void info(str_t fmt, ...) {
  va_list args;
  va_start(args, fmt);
  flog(stderr, "info", fmt, args);
  va_end(args);
}

void warn(str_t fmt, ...) {
  va_list args;
  va_start(args, fmt);
  flog(stderr, "warn", fmt, args);
  va_end(args);
}

void error(str_t fmt, ...) {
  va_list args;
  va_start(args, fmt);
  flog(stderr, "error", fmt, args);
  va_end(args);
}

void panic(str_t fmt, ...) {
  va_list args;
  va_start(args, fmt);
  flog(stderr, "panic", fmt, args);
  va_end(args);
  // TODO: unwind build stack
  unwind_stack();
  exit(errno);
}

void sys_panic() {
  panic("sys error, potentially caused by: %s", strerror(errno));
}

#define CHECK(ret)                                                             \
  if (ret < 0)                                                                 \
  sys_panic()

// CMD

int exec_cmd(str_list_t cmd) {
  return exec_cmd_fd(cmd, STDIN_FILENO, STDOUT_FILENO);
}

bool file_exists(str_t f) {
  struct stat buffer;
  return stat(f, &buffer) == 0;
}

int exec_cmd_fd(str_list_t cmd, int fdin, int fdout) {
  info(str_list_concat(cmd, " "));
  str_list_append(&cmd, NULL);
  int pid = fork();
  if (pid < 0)
    assert(0 && "Failed to create fork ;-;");
  if (pid == 0) {
    if (fdin != STDIN_FILENO && fdout != STDOUT_FILENO) {
      CHECK(close(fdin));
      CHECK(dup2(fdout, STDOUT_FILENO));
      CHECK(close(fdout));
    }
    if (execvp(cmd.data[0], (char *const *)cmd.data) < 0)
      panic("Failed to exec command: %s", strerror(errno));
  }
  int status;
  waitpid(pid, &status, 0);
  return status;
}

// returns the length of the substring of the file which contains the directory
static int get_dir_name(str_t file) {
  int fds[2];
  CHECK(pipe(fds));
  exec_cmd_fd(str_list_create("dirname", file), fds[0], fds[1]);
  close(fds[1]);
  ssize_t readlen;
  char fp_dir[256];
  while ((readlen = read(fds[0], fp_dir, sizeof(fp_dir))) != 0) {
    fp_dir[readlen] = '\0';
  }
  return (int)strlen(fp_dir) - 1;
}

// BLD

void add_to_stack(bld_t b) {
  if (bld_stack.stack == NULL) {
    bld_stack.stack = malloc(sizeof(bld_t) * BLD_STACK_SIZE);
    bld_stack.count = 0;
  }
  bld_stack.stack[bld_stack.count++] = b;
}

bld_t pop_stack(void) { return bld_stack.stack[--bld_stack.count]; }

void unwind_stack(void) {
  while (bld_stack.count > 0) {
    bld_t b = pop_stack();
    str_t fp = str_list_concat(str_list_create(CACHE_DIRECTORY, b.target), "/");
    exec_cmd(str_list_create("mv", fp, b.target));
  }
  exec_cmd(str_list_create("rm", "-r", CACHE_DIRECTORY));
}

bld_t bld_create(str_list_t srcs, str_t target) {
  return (bld_t){
      .srcs = srcs,
      .target = target,
  };
}

// TODO:
// move old files into cache directory
// if build fails move old files back (using build stack)
void bld_run_impl(bld_t b, ...) {
  if (file_exists(b.target)) {
    add_to_stack(b);
    mkdirs(CACHE_DIRECTORY);
    str_t fp = str_list_concat(str_list_create(CACHE_DIRECTORY, b.target), "/");
    // dirname
    int end_idx = get_dir_name(fp);
    char *fp_dir = malloc(strlen(fp) * sizeof(fp));
    strcpy(fp_dir, fp);
    fp_dir[end_idx] = '\0';
    exec_cmd(str_list_create("mkdir", "-p", fp_dir));
    exec_cmd(str_list_create("mv", b.target, fp));
    free(fp_dir);
  }
  va_list args;
  va_start(args, b);
  str_list_t flags = str_list_init(1);
  for (str_t flag = va_arg(args, str_t); flag != NULL;
       flag = va_arg(args, str_t)) {
    str_list_append(&flags, flag);
  }
  va_end(args);

  str_list_t cmd = str_list_init(1);
  str_list_append(&cmd, CC);
  str_list_append(&cmd, "-o");
  str_list_append(&cmd, b.target);
  str_list_t cmd_f = str_list_join(cmd, str_list_join(b.srcs, flags));

  int res = exec_cmd(cmd_f);
  if (res != 0) panic("failed to build file >.<");
}

bool need_to_rebuild(str_t src, str_t tgt) {
  struct stat ss, st;
  stat(src, &ss);
  stat(tgt, &st);
  return ss.st_mtime > st.st_mtime;
}

#ifdef CBT_SELF_REBUILD_DEBUG_MODE
#define SELF_REBUILD_WARNING_FLAGS                                             \
  "-Werror", "-pedantic", "-Wall", "-Wextra", "-Wconversion", "-Wshadow",      \
      "-Wpointer-arith", "-Wstrict-prototypes", "-Wunreachable-code",          \
      "-Wwrite-strings", "-Wbad-function-cast", "-Wcast-align",                \
      "-Wswitch-default", "-Winline", "-Wundef", "-Wfloat-equal",              \
      "-fno-common", "-fstrict-aliasing", "-fanalyzer", "--coverage",          \
      "-fsanitize=address", "-O0"
#else
#define SELF_REBUILD_WARNING_FLAGS                                             \
  "-O3", "-Werror", "-pedantic", "-Wall", "-Wextra", "-Wconversion", "-Wshadow"
#endif

void self_rebuild_impl(int argc, char **argv, str_t file) {
  if (need_to_rebuild(file, argv[0])) {
    info("Rebuilding cbt...");
    char new_filename[256];
    CHECK(sprintf(new_filename, "%s.new", argv[0]));
    int x = exec_cmd(str_list_create("gcc", "-o", new_filename, file,
                                     SELF_REBUILD_WARNING_FLAGS));
    if (x != 0) {
      exec_cmd(str_list_create("rm", "-f", new_filename));
      error("rebuild failed"); // NOTE: should be fine not to panic here, there
                               // shouldn't be a stack to unwind
      exit(x);
    }
    exec_cmd(str_list_create("mv", "cbt.new", "cbt"));
    info("Succesfully rebuilt");
    str_list_t argv_l = str_list_init((size_t)argc);
    argv_l.data = (str_t *)argv;
    argv_l.count = (size_t)argc;
    int res = exec_cmd(argv_l);
    exit(res);
  }
}

#endif
