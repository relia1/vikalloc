// Robert Elia 2024
// relia@pdx.edu

#include "vikalloc.h"

// Returns the size of the structure, in bytes.
#define BLOCK_SIZE (sizeof(heap_block_t))

// Returns a pointer to the data within a block, as a void *.
#define BLOCK_DATA(__curr) (((void *) __curr) + (BLOCK_SIZE))

// Returns a pointer to the structure containing the data
#define DATA_BLOCK(__curr) (((void *) __curr) - (BLOCK_SIZE))

// Returns 0 (false) if the block is NOT free, else 1 (true).
#define IS_FREE(__curr) ((__curr -> size) == 0)

// A nice macro for formatting pointer values.
#define PTR "0x%07lx"
#define PTR_T PTR "\t" // just a tab

#define CURR_EXCESS_CAPACITY(__curr) (__curr->capacity - __curr->size)

// Function prototypes
// Recursive function that combines adjacent free blocks
void coalesce_up(heap_block_t * ptr);

// Some variables that repesent (internally) global pointers to the
// list in the heap.
static heap_block_t *block_list_head = NULL;
static heap_block_t *block_list_tail = NULL;
static void *low_water_mark = NULL;
static void *high_water_mark = NULL;

// only used in next-fit algorithm
// *************************************************************
// *************************************************************
// This is the variable you'll use to keep track of where the
// next_fit algorithm left off when searching through the linked
// list of heap_block_t sturctures.
// *************************************************************
// *************************************************************
static heap_block_t *next_fit = NULL;

static uint8_t isVerbose = FALSE;
static vikalloc_fit_algorithm_t fit_algorithm = NEXT_FIT;
static FILE *vikalloc_log_stream = NULL;

// Some gcc magic to initialize the diagnostic stream at startup.
static void init_streams(void) __attribute__((constructor));

// This is the variable that hols the smallist amount of memory
// you should use when calling sbrk().
// When you call sbrk(), you must call with a multiple of this
// variable. The value of min_sbrk_size can be changed with a
// call to vikalloc_set_min().
static size_t min_sbrk_size = MIN_SBRK_SIZE;

// This allows all diagnostic messages to go to a file.
static void init_streams(void)
{
    // Don't change this.
    vikalloc_log_stream = stderr;
}

size_t vikalloc_set_min(size_t size)
{
    // Don't change this.
    if (0 == size) {
	// just return the current value
	return min_sbrk_size;
    }
    if (size < (BLOCK_SIZE + BLOCK_SIZE)) {
	// In the event that it is set to something silly small.
	size = MAX(BLOCK_SIZE + BLOCK_SIZE, SILLY_SBRK_SIZE);
    }
    min_sbrk_size = size;

    return min_sbrk_size;
}

void vikalloc_set_algorithm(vikalloc_fit_algorithm_t algorithm)
{
    // Don't change this.
    fit_algorithm = algorithm;
    if (isVerbose) {
	switch (algorithm) {
	    case FIRST_FIT:
		fprintf(vikalloc_log_stream, "** First fit selected\n");
		break;
	    case BEST_FIT:
		fprintf(vikalloc_log_stream, "** Best fit selected\n");
		break;
	    case WORST_FIT:
		fprintf(vikalloc_log_stream, "** Worst fit selected\n");
		break;
	    case NEXT_FIT:
		fprintf(vikalloc_log_stream, "** Next fit selected\n");
		break;
	    default:
		fprintf(vikalloc_log_stream, "** Algorithm not recognized %d\n"
			, algorithm);
		fit_algorithm = FIRST_FIT;
		break;
	}
    }
}

void vikalloc_set_verbose(uint8_t verbosity)
{
    // Don't change this.
    isVerbose = verbosity;
    if (isVerbose) {
	fprintf(vikalloc_log_stream, "Verbose enabled\n");
    }
}

void vikalloc_set_log(FILE *stream)
{
    // Don't change this.
    vikalloc_log_stream = stream;
}

void * vikalloc(size_t size)
{
    heap_block_t * curr = next_fit;
    size_t size_to_request = 0;
    void * data_block = NULL;
    heap_block_t * new_heap_node = NULL;
    if (isVerbose) {
	fprintf(vikalloc_log_stream, ">> %d: %s entry: size = %lu\n"
		, __LINE__, __FUNCTION__, size);
    }


    if (0 == size) {
	return NULL;
    }

    if(low_water_mark == NULL) {
	low_water_mark = sbrk(0);
    }

    // There will always be at least 1 block requested
    // Here we take advantage of integer division to determine the number
    // of additional blocks needed
    size_to_request = ((size + BLOCK_SIZE) / min_sbrk_size);
    if((size + BLOCK_SIZE) % min_sbrk_size != 0) {
	size_to_request++;
    }


    // Check if our data structure is NULL and initialize it if so
    // This will involve a system call to sbrk()
    if(block_list_head == NULL) {
	block_list_head = sbrk(size_to_request * min_sbrk_size);
	if(new_heap_node == (void *)-1) {
	    if(isVerbose) {
		fprintf(vikalloc_log_stream, "<< %d: %s sbrk failure", __LINE__, __FUNCTION__);
	    }
	    errno = ENOMEM;
	    return NULL;
	}
	block_list_head->capacity = (size_to_request * min_sbrk_size) - BLOCK_SIZE;
	block_list_head->size = size;
	next_fit = block_list_head;
	block_list_tail = block_list_head;
	block_list_head->next = NULL;
	block_list_head->prev = NULL;
	high_water_mark = sbrk(0);
	return BLOCK_DATA(next_fit);
    }

    // Traverse the data structure to see if there is enough memory already we
    // can use
    // If there is a spot that already exists that can fufill our request we
    // need to perform a split
    do {
	if((CURR_EXCESS_CAPACITY(curr)) >= (size + BLOCK_SIZE)) {
	    // There exists an already freed heap node, so we can use this
	    // without needing to split
	    if(0 == curr->size) {
		curr->size = size;
		next_fit = curr;
		return BLOCK_DATA(curr);
	    } else {
		// perform split
		next_fit = (void *)curr + BLOCK_SIZE + curr->size;
		next_fit->next = curr->next;
		next_fit->prev = curr;
		next_fit->size = size;
		next_fit->capacity = CURR_EXCESS_CAPACITY(curr) - BLOCK_SIZE;
		if(next_fit->next == NULL) { 
		    block_list_tail = next_fit;
		} else {
		    next_fit->next->prev = next_fit;
		}

		curr->capacity = curr->size;
		curr->next = next_fit;
		return BLOCK_DATA(next_fit);
	    }
	} else {
	    if(curr->next == NULL) {
		curr = block_list_head;
	    } else {
		curr = curr->next;
	    }
	}
    } while(curr != next_fit);

    if(data_block == NULL) {
	// wasn't a space to add our data, make a system call to sbrk to
	// have more allocated
	new_heap_node = sbrk(size_to_request * min_sbrk_size);
	if(new_heap_node == (void *)-1) {
	    if(isVerbose) {
		fprintf(vikalloc_log_stream, "<< %d: %s sbrk failure", __LINE__, __FUNCTION__);
	    }
	    errno = ENOMEM;
	    return NULL;
	}
	new_heap_node->next = NULL;
	new_heap_node->prev = block_list_tail;
	new_heap_node->capacity = (size_to_request * min_sbrk_size) - BLOCK_SIZE;
	new_heap_node->size = size;
	block_list_tail->next = new_heap_node;
	block_list_tail = new_heap_node;
	data_block = BLOCK_DATA(new_heap_node);
    }

    high_water_mark = sbrk(0);


    if (isVerbose) {
	fprintf(vikalloc_log_stream, "<< %d: %s exit: size = %lu\n", __LINE__, __FUNCTION__, size);
    }

    return data_block;
}



void vikfree(void *ptr)
{
    heap_block_t *curr = NULL;

    if (ptr == NULL) {
	return;
    }

    // Get the data structure that holds the passed in data
    curr = DATA_BLOCK(ptr);

    if(curr == NULL) {
	return;
    }

    if (IS_FREE(curr)) {
	if (isVerbose) {
	    fprintf(vikalloc_log_stream, "Block is already free: ptr = " PTR "\n"
		    , (long) (ptr - low_water_mark));
	}
	return;
    }

    curr->size = 0;
    next_fit = curr;

    // Check for the three scenarios which we coalesce
    //   If curr->next and curr->prev both are of size 0
    //   Mark curr->next->size with a dummy value we can use to stop our recursive traversal
    //   Recursively coalesce up from curr->prev
    //
    //   If curr->next is of size 0
    //   Mark curr->next->size with a dummy value we can use to stop our recursive traversal
    //   Recursively coalesce up from curr
    //
    //   If curr->previous is of size 0
    //   Mark curr->size with a dummy value we can use to stop our recursive traversal
    //   Recursively coalesce up from curr->prev
    if((curr->next != NULL && IS_FREE(curr->next)) && (curr->prev != NULL && IS_FREE(curr->prev))) {
	curr->next->size = 1;
	next_fit = curr->prev;
	coalesce_up(curr->prev);
    } else if(curr->next != NULL && IS_FREE(curr->next)) {
	curr->next->size = 1;
	next_fit = curr;
	coalesce_up(curr);
    } else if(curr->prev != NULL && IS_FREE(curr->prev)) {
	curr->size = 1;
	next_fit = curr->prev;
	coalesce_up(curr->prev);
    }
    
    if (isVerbose) {
	fprintf(vikalloc_log_stream, "<< %d: %s exit: ptr = %p\n", __LINE__, __FUNCTION__, ptr);
    }
}


void coalesce_up(heap_block_t * ptr) {
    heap_block_t * next = NULL;
    if(ptr->size != 0) {
	return;
    } else {
	coalesce_up(ptr->next);
    }

    next = ptr->next;
    ptr->capacity += next->capacity + BLOCK_SIZE;
    if(next->next) {
	next->next->prev = ptr;
    } else {
	block_list_tail = ptr;
    }

    ptr->next = next->next;
}


///////////////

void vikalloc_reset(void)
{
    if (low_water_mark != NULL) {
	if (isVerbose) {
	    fprintf(vikalloc_log_stream, "*** Resetting all vikalloc space ***\n");
	}

	brk(low_water_mark);
	high_water_mark = low_water_mark;

	block_list_head = NULL;
	block_list_tail = NULL;
	next_fit = NULL;
    }
}

void * vikcalloc(size_t nmemb, size_t size)
{
    void *ptr = vikalloc(nmemb * size);
    if(ptr == NULL) {
	return NULL;
    }

    memset(ptr, 0, nmemb * size);
    return ptr;
}

void * vikrealloc(void *ptr, size_t size)
{
    heap_block_t *curr = NULL;
    void * new_heap_node = NULL;

    if(ptr == NULL) {
	return vikalloc(size);
    }

    if(0 == size) {
	vikfree(ptr);
	return NULL;
    }

    curr = DATA_BLOCK(ptr);
    if(size <= curr->capacity) {
	curr->size = size;
	return ptr;
    }

    new_heap_node = vikalloc(size);
    if(new_heap_node == NULL) {
	return NULL;
    }

    memmove(new_heap_node, ptr, MIN(size, curr->size));
    vikfree(ptr);
    return new_heap_node;
}

void * vikstrdup(const char *s)
{
    return strcpy(vikalloc(strlen(s)+1), s);
}

// This is unbelievably ugly.
#include "vikalloc_dump.c"
