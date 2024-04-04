// R. Jesse Chaney
// rchaney@pdx.edu

#ifndef __VIKALLOC_H
# define __VIKALLOC_H

# include <stdint.h>
# include <string.h>
# include <unistd.h>
# include <errno.h>
# include <stddef.h>
# include <values.h>
# include <stdlib.h>
# include <stdio.h>

// enable the define below to disable assert.
//# define NDEBUG
# include <assert.h>

# ifndef MAX
#  define MAX(_a,_b) ((_a) > (_b) ? (_a) : (_b))
# endif // MAX

# ifndef MIN
#  define MIN(_a,_b) ((_a) < (_b) ? (_a) : (_b))
# endif // MIN

# ifndef TRUE
#  define TRUE 1
# endif // TRUE
# ifndef FALSE
#  define FALSE 0
# endif // FALSE

// Allows you you to specify the algorithm to be used when
//   finding blocks to use.
// See OSTEP Chap 17
typedef enum {
    FIRST_FIT
    , BEST_FIT
    , WORST_FIT
    , NEXT_FIT // the algorithm we are going to use
} vikalloc_fit_algorithm_t;

// This is the default value set to the variable min_sbrk_size. The
// variable min_sbrk_size can be changed with a call to vikalloc_set_min().
# ifndef MIN_SBRK_SIZE
//#  define MIN_SBRK_SIZE 2048
# define MIN_SBRK_SIZE 4096
# endif // MIN_SBRK_SIZE

// This the absolute mininum size that the variable min_sbrk_size can be
// be set to (by calling vikalloc_set_min()).
# ifndef SILLY_SBRK_SIZE
#  define SILLY_SBRK_SIZE 128
# endif // SILLY_SBRK_SIZE

typedef struct heap_block_s {
    size_t capacity;
    size_t size;

    struct heap_block_s *prev;
    struct heap_block_s *next;
} heap_block_t;

// The basic memory allocator.
// If you pass NULL or 0, then NULL is returned.
// If, for some reason, the system cannot allocate the requested
//   memory, set errno and return NULL.
// Do not allocate a chunk of memory (using sbrk()) smaller than
//   min_sbrk_size bytes.
// This could only happen if someone calls vikalloc_set_min() with
//   something really small. I don't plan to try that.
// You must use sbrk() or brk() in requesting more memory for your
//   vikalloc() routine to manage.
void *vikalloc(size_t size);

// A pointer returned from a prevous call to vikalloc() must
//   be passed.
// If a pointer is passed to a block than is already free, 
//   simply return. If you are in verbose mode, print a snarky message
//   before returning.
// Blocks must be coalesced, where possible, as they are free'ed.
void vikfree(void *ptr);

// This is like the regular calloc() call. See the man page for details.
// Allocate memory, using vikalloc and set the memory to all zeroes.
void *vikcalloc(size_t nmemb, size_t size);

// This is mostly like the regular realloc() call. See the man page for details.
// Pass in a pointer to already allocated memory and a new size you wish
// were allocated. If the new size exceeds the capacity of the existing
// block, a new block must be allocated, the old contents be copied
// into the new block, and the old block deallocated.
// If you pass NULL as the pointer to old memory, this will behave the
// same as vikalloc.
void *vikrealloc(void *ptr, size_t size);

// This is like the strdup() function call.
// You pass in a string or a string literal, vikstrdup() will allocate the
// exact amount necessary, copy into the allocated memory the string, and
// return a pointer to the allocated memory.
void *vikstrdup(const char *s);

// Output a map of the current state of the heap. I provide this to you.
void vikalloc_dump2(void *);

// Completely reset your heap back to zero bytes allocated.
// You are going to like being able to do this.
// Implementation can be done in as few as 1 line, though
//   you will probably use more to reset the stats you keep
//   about heap.
// After you've called this function, everything you had in
//   the heap is just __GONE__!!!
// You should be able to call vikalloc() after calling vikalloc_reset()
//   to restart building the heap again.
void vikalloc_reset(void);

// Set the fit algorithm.
// This should modify a variable that is static to your C module.
void vikalloc_set_algorithm(vikalloc_fit_algorithm_t);

// Set the verbosity of your vikalloc() code (and related functions).
// This should modify a variable that is static to your C module.
void vikalloc_set_verbose(uint8_t);

// Set the stream into which diagnostic information will be sent.
void vikalloc_set_log(FILE *);

// Allows you to set the chunk size. When calling sbrk(), the amount
// of memory requested will always be a multiple of the chunk size.
// THis sets the variable min_sbrk_size.
// Passing 0 returns the current chunk size.
size_t vikalloc_set_min(size_t);

#endif // __VIKALLOC_H
