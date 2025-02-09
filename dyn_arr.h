#include <stdlib.h>
#include <strings.h>

#define GEN_ARRAY_FUNCTIONS(T, NAME, DEFAULT_CAP)                              \
  typedef struct {                                                             \
    T *array;                                                                  \
    size_t size;                                                               \
    size_t capacity;                                                           \
  } NAME;                                                                      \
  void NAME##_deinit(NAME *arr) { free(arr->array); }                          \
  void delete_##NAME(NAME *arr) {                                              \
    NAME##_deinit(arr);                                                        \
    free(arr);                                                                 \
  }                                                                            \
  void NAME##_init(NAME *arr) {                                                \
    arr->size = 0;                                                             \
    arr->capacity = DEFAULT_CAP;                                               \
    arr->array = (T *)calloc(DEFAULT_CAP, sizeof(T));                          \
    if (arr->array == NULL)                                                    \
      exit(1);                                                                 \
  }                                                                            \
  NAME *new_##NAME(void) {                                                     \
    NAME *res = (NAME *)malloc(sizeof(NAME));                                  \
    if (res == NULL)                                                           \
      exit(1);                                                                 \
    NAME##_init(res);                                                          \
    return res;                                                                \
  }                                                                            \
  void NAME##_ensure_capacity(NAME *arr, size_t required_cap) {                \
    if (required_cap < arr->capacity)                                          \
      return;                                                                  \
    do {                                                                       \
      size_t old_capacity = arr->capacity;                                     \
      arr->capacity <<= 1;                                                     \
      arr->array = realloc(arr->array, sizeof(T) * arr->capacity);             \
      bzero(arr->array + old_capacity, arr->capacity - old_capacity);          \
      if (arr->array == NULL)                                                  \
        exit(1);                                                               \
    } while (required_cap > arr->capacity);                                    \
  }                                                                            \
  void NAME##_add_tail(NAME *arr, T elem) {                                    \
    NAME##_ensure_capacity(arr, arr->size + 1);                                \
    arr->array[arr->size] = elem;                                              \
    arr->size++;                                                               \
  }                                                                            \
  void NAME##_fill(NAME *arr, size_t count, T filler) {                        \
    for (size_t i = 0; i < count; i++)                                         \
      NAME##_add_tail(arr, filler);                                            \
  }                                                                            \
  T NAME##_del_tail(NAME *arr) { return arr->array[--arr->size]; }             \
  T NAME##_get(NAME *arr, size_t index) { return arr->array[index]; }          \
  T *NAME##_get_pointer(NAME *arr, size_t index) {                             \
    return arr->array + index;                                                 \
  }                                                                            \
  T NAME##_get_or(NAME *arr, size_t index, T _default) {                       \
    if (index >= arr->size)                                                    \
      return _default;                                                         \
    return NAME##_get(arr, index);                                             \
  }                                                                            \
  void NAME##_set(NAME *arr, size_t index, T elem) {                           \
    arr->array[index] = elem;                                                  \
  }                                                                            \
  T NAME##_del(NAME *arr, size_t index) {                                      \
    memmove(arr->array + index, arr->array + index + 1,                        \
            sizeof(T) * (arr->capacity - index - 1));                          \
    return NAME##_del_tail(arr);                                               \
  }
