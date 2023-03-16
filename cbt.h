#ifndef CBT_H_
#define CBT_H_

#include <stdio.h>
#include <stdlib.h>

#define LINKED_LIST(name, type)                                                \
  typedef struct {                                                             \
    type *data;                                                                \
    int count;                                                                 \
    int capacity;                                                              \
  } name;                                                                      \
  name init_##name(int);

LINKED_LIST(Sources, char *);
LINKED_LIST(Flags, char *);

typedef struct {
  Sources sources;
  Flags flags;
} build;

build create_build(void);

#endif // CBT_H_
#ifdef CBT_IMPLEMENTATION

#define LINKED_LIST_IMPL(name, type)                                           \
  name init_##name(int initial_cap) {                                          \
    return (name){                                                             \
        .data = malloc(initial_cap * sizeof(type)),                            \
        .count = 0,                                                            \
        .capacity = initial_cap,                                               \
    };                                                                         \
  }

LINKED_LIST_IMPL(Sources, char *);
LINKED_LIST_IMPL(Flags, char *);

build create_build(void) { return (build){.sources = init_Sources(1), .flags = init_flags(4)}; }

#endif
