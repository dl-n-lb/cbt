#ifndef CBT_H_
#define CBT_H_

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CC "gcc"

#define LIST(name, type)                                                       \
  typedef struct {                                                             \
    type *data;                                                                \
    int count;                                                                 \
    int capacity;                                                              \
  } name;                                                                      \
  name init##name(int);                                                        \
  // void append##name(name *, type);

LIST(Sources, char *);
LIST(Flags, char *);

typedef struct {
  Sources sources;
  Flags flags;
} build;

const char *concat_flags(Flags, char);

build create_build(void);
void setSources(build *, ...);

void run_build(build);

#endif // CBT_H_
#ifdef CBT_IMPLEMENTATION

#define LIST_IMPL(name, type)                                                  \
  name init##name(int initial_cap) {                                           \
    return (name){                                                             \
        .data = malloc(initial_cap),                                           \
        .count = 0,                                                            \
        .capacity = initial_cap,                                               \
    };                                                                         \
  }                                                                            \
  static void append##name(name *ls, type elem) {                              \
    if (ls->count < ls->capacity) {                                            \
      ls->data[ls->count++] = elem;                                            \
      return;                                                                  \
    }                                                                          \
    int nc = ls->capacity * 2;                                                 \
    typeof(ls->data) new_data = malloc(nc * sizeof(ls->data[0]));              \
    memcpy(new_data, ls->data, ls->count * sizeof(ls->data[0]));               \
    ls->data = new_data;                                                       \
    ls->capacity = nc;                                                         \
    ls->data[ls->count++] = elem;                                              \
  }
// TODO: handle case where count >= capacity

LIST_IMPL(Sources, char *);
LIST_IMPL(Flags, char *);

build create_build(void) {
  return (build){
      .sources = initSources(1),
      .flags = initFlags(3),
  };
}

const char *concat_flags(Flags flags, char sep) {
  if (flags.count == 0)
    return "";
  size_t ln = 0;
  for (size_t i = 0; i < flags.count; ++i)
    ln += strlen(flags.data[i]);
  const size_t res_len = (flags.count - 1) + ln + 1;
  char *res = malloc(sizeof(char) * res_len);
  if (res == NULL)
    assert(0 && "RIP BOZO");

  size_t idx = 0;

  for (size_t i = 0; i < flags.count; ++i) {
    size_t sl = strlen(flags.data[i]);
    memcpy(res + idx, flags.data[i], sl);
    idx += sl;
    if (i < flags.count - 1) {
      memcpy(res + (++idx), &sep, 1);
    }
  }
  return res;
}

void setSources(build *b, ...) {
  va_list args;
  va_start(args, b);
  for (char *next = va_arg(args, char *); next != NULL; next = va_arg(args, char *)) {
    appendSources(&b->sources, next);
  }
  va_end(args);
}

void run_build(build b) {

  // FILE *cmd = popen(cmd, "r");
  // pclose(cmd);
}

#endif
