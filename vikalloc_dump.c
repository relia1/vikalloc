// R. Jesse Chaney
// rchaney@px.edu

void 
vikalloc_dump2(void *addr)
{
    heap_block_t *curr = NULL;
    unsigned i = 0;
    unsigned user_bytes = 0;
    unsigned capacity_bytes = 0;
    unsigned block_bytes = 0;
    unsigned used_blocks = 0;
    unsigned free_blocks = 0;

    fprintf(vikalloc_log_stream, "Heap map\n");
    fprintf(vikalloc_log_stream
            , "  %s\t%s\t%s\t%s\t%s" 
              "\t%s\t%s\t%s\t%s\t%s"
              "\n"
            , "blk no  "
            , "block add "
            , "next add  "
            , "prev add  "
            , "data add  "
            
            , "blk size "
            , "capacity "
            , "size     "
            , "excess   "
            , "status   "
        );
    for (curr = block_list_head, i = 0; curr != NULL; curr = curr->next, i++) {
        fprintf(vikalloc_log_stream
                , "  %u\t\t"
                  PTR_T PTR_T PTR_T PTR_T
                  "%9zu\t%9zu\t"
                  "%9zu\t%9zu\t%s\t%c"
                , i
                , (((void *) curr) - addr)
                , (curr->next ? ((void *) curr->next - addr) : 0x0)
                , (curr->prev ? ((void *) curr->prev - addr) : 0x0)
                , (BLOCK_DATA(curr) - addr)

                , (curr->capacity + BLOCK_SIZE)
                , curr->capacity
                , curr->size
                , (curr->capacity - curr->size)
                , IS_FREE(curr) ? "free  " : "in use"
                , IS_FREE(curr) ? '*' : ' '
            );
        if (NEXT_FIT == fit_algorithm) {
            if (curr == next_fit) {
                fprintf(vikalloc_log_stream, " <");
            }
            else {
                fprintf(vikalloc_log_stream, "  ");
            }
        }
        fprintf(vikalloc_log_stream, "\n");
        user_bytes += curr->size;
        capacity_bytes += curr->capacity;
        block_bytes += curr->capacity + BLOCK_SIZE;

        if (IS_FREE(curr)) {
            free_blocks++;
        }
        else {
            used_blocks++;
        }
    }
    fprintf(vikalloc_log_stream
            , "  %s\t\t\t\t\t\t\t\t"
              "%9u\t%9u\t%9u\t%9u\n"
            , "Total bytes used"
            , block_bytes
            , capacity_bytes
            , user_bytes
            , capacity_bytes - user_bytes
        );
    fprintf(vikalloc_log_stream
            , "  Used blocks: %4u  Free blocks: %4u  "
              "Min heap: " PTR "    Max heap: " PTR 
              "   Total bytes: %lu"
              "   Block size: %zu bytes\n"
            , used_blocks, free_blocks
            , (low_water_mark ? (low_water_mark - addr) : 0x0)
            , (high_water_mark ? (high_water_mark - addr) : 0x0)
            , (high_water_mark - low_water_mark)
            , BLOCK_SIZE
        );
    //fprintf(vikalloc_log_stream, "  next_fit = " PTR " ***\n", (long) (((void *) next_fit) - addr));
}
