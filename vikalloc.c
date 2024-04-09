// R. Jesse Chaney
// rchaney@pdx.edu

#include "vikalloc.h"
#include <string.h>
// Returns the size of the structure, in bytes.
#define BLOCK_SIZE (sizeof(heap_block_t))

// Returns a pointer to the data within a block, as a void *.
#define BLOCK_DATA(__curr) (((void *) __curr) + (BLOCK_SIZE))

// Returns a pointer to the structure containing the data
#define DATA_BLOCK(__curr) (((void *) __curr) - (BLOCK_SIZE))

// Returns 0 (false) if the block is NOT free, else 1 (true).
#define IS_FREE(__curr) ((__curr -> size) == 0)

// A noce macro for formatting pointer values.
#define PTR "0x%07lx"
#define PTR_T PTR "\t" // just a tab

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

    // printf("size %ld\n", size);

    if (0 == size) {
        return NULL;
    }

    if(low_water_mark == NULL) {
	low_water_mark = sbrk(0);
    }

    // There will always be at least 1 block requested
    // Here we take advantage of integer division to determine the number
    // of additional blocks needed
    size_to_request = 1 + ((size + BLOCK_SIZE) / min_sbrk_size);

    // Check if our data structure is NULL and initialize it if so
    // This will involve a system call to sbrk()
    
    // If after traversal there isn't a place that meets the size requirements
    // needed, then we have to make a system call to sbrk() for more memory
    if(block_list_head == NULL) {
	new_heap_node = sbrk(size_to_request * min_sbrk_size);
	new_heap_node->capacity = (size_to_request * min_sbrk_size) - BLOCK_SIZE;
	new_heap_node->size = size;
	next_fit = new_heap_node;
	new_heap_node->next = NULL;
	new_heap_node->prev = NULL;
	block_list_head = new_heap_node;
	block_list_tail = block_list_head;
	high_water_mark = sbrk(0);
	return BLOCK_DATA(next_fit);
    }

    // Traverse the data structure to see if there is enough memory already we
    // can use
    // If there is a spot that already exists that can fufill our request we
    // need to perform a split
    do {
	if((curr->capacity - curr->size) >= (size + BLOCK_SIZE)){
	    // perform split
	    next_fit = (void *)curr + BLOCK_SIZE + curr->size;
	    next_fit->next = curr->next;
	    next_fit->prev = curr;
	    next_fit->size = size;
	    next_fit->capacity = curr->capacity - curr->size - BLOCK_SIZE;
	    if(next_fit->next == NULL) { 
		block_list_tail = next_fit;
	    } else {
		next_fit->next->prev = next_fit;
	    }

	    curr->capacity = curr->size;
	    curr->next = next_fit;
	    data_block = BLOCK_DATA(next_fit);
	    // break;
	    return data_block;
	}

	if(curr->next == NULL) {
	    curr = block_list_head;
	} else {
	    curr = curr->next;
	}
    } while(curr != next_fit);

    if(data_block == NULL) {
	// wasn't a space to add our data, make a system call to sbrk to
	// have more allocated
	new_heap_node = sbrk(size_to_request * min_sbrk_size);
	new_heap_node->next = NULL;
	new_heap_node->prev = block_list_tail;
	new_heap_node->capacity = (size_to_request * min_sbrk_size) - BLOCK_SIZE;
	new_heap_node->size = size;
	next_fit = new_heap_node;
	block_list_tail->next = new_heap_node;
	block_list_tail = new_heap_node;
	/*
	if(new_heap_node->next != NULL) {
	    new_heap_node->next->prev = new_heap_node;
	}
	if(curr == block_list_tail) {
	    block_list_tail = new_heap_node;
	}
	*/
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
    // TODO check about next_fit
    // next_fit = curr;


    // coalesce
    // TODO check if I need to coalesce including the wrap around from tail
    // to head and from head to tail
    if(curr->next != NULL && IS_FREE(curr->next)) {
        coalesce_up(curr);
    }

    if(curr->prev != NULL && IS_FREE(curr->prev)) {
        coalesce_up(curr->prev);
    }

    if (isVerbose) {
        fprintf(vikalloc_log_stream, "<< %d: %s exit: ptr = %p\n", __LINE__, __FUNCTION__, ptr);
    }
}


void coalesce_up(heap_block_t * ptr) {
    heap_block_t *curr = ptr->next;

    if(curr == next_fit) {
	next_fit = ptr;
    }

    /*
    if(curr == block_list_tail) {
	block_list_tail = ptr;
    }
    */
    if(curr->next != NULL) {
	curr->next->prev = ptr;
    }

    ptr->next = curr->next;
    curr->prev = NULL;
    curr->next = NULL;
    ptr->capacity += BLOCK_SIZE + curr->capacity;
    
    /*
    // coalesce with previous and next blocks if they are free
    if((curr->prev != NULL && IS_FREE(curr->prev)) && (curr->next != NULL && IS_FREE(curr->next))) {
        heap_block_t * prev = curr->prev;
        heap_block_t * next = curr->next;
        size_t curr_mem_reclaimed = curr->capacity + BLOCK_SIZE;
        size_t next_mem_reclaimed = next->capacity + BLOCK_SIZE;
        prev->capacity += curr_mem_reclaimed + next_mem_reclaimed;

        prev->next = next->next;
        if(next->next != NULL) {
            next->next->prev = prev;
        }
        if(next == block_list_tail) {
            block_list_tail = prev;
            prev->next = NULL;
        }
    }// else
    
    if(curr->next != NULL && IS_FREE(curr->next)) {
        heap_block_t * next = curr->next;
        curr->capacity += next->capacity + BLOCK_SIZE;
        curr->next = next->next;
        if(next->next != NULL) {
            next->next->prev = curr;
        } else {
            block_list_tail = curr;
        }
        next->next = NULL;
        next->prev = NULL;
        next->size = 0;
        next->capacity = 0;
    }

    if(curr->prev != NULL && IS_FREE(curr->prev)) {
        heap_block_t * prev = curr->prev;
        prev->capacity += curr->capacity + BLOCK_SIZE;
        prev->next = curr->next;
        if(curr->next != NULL) {
            curr->next->prev = prev;
        }
        curr->next = NULL;
        curr->prev = NULL;
        curr->size = 0;
        curr->capacity = 0;
    }
    */
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
    void * heap_node_data = NULL;
    size_t old_size = 0;

    if(ptr == NULL) {
	return vikalloc(size);
    }

    if(size == 0) {
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

    // memset(new_heap_node, 0, size);
    memcpy(new_heap_node, ptr, MIN(size, curr->size));
    vikfree(ptr);
    return new_heap_node;
}

void * vikstrdup(const char *s)
{
    // return s? strcpy(vikalloc(strlen(s) + 1), s) : NULL;
    return strcpy(vikalloc(strlen(s)+1), s);
}

// This is unbelievably ugly.
#include "vikalloc_dump.c"
