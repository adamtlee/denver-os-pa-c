/*
 * Created by Ivo Georgiev on 2/9/16.
 * c programming assignment
 * Adam Lee
 * Group members:  Michael Palme; William King; Mahesh;
 */

#include <stdlib.h>
#include <assert.h>
#include <stdio.h> // for perror()

#include "mem_pool.h"

/*************/
/*           */
/* Constants */
/*           */
/*************/
//static const float      MEM_FILL_FACTOR                 = 0.75;
//static const unsigned   MEM_EXPAND_FACTOR               = 2;

static const unsigned   MEM_POOL_STORE_INIT_CAPACITY    = 20;
static const float      MEM_POOL_STORE_FILL_FACTOR      = 0.75;
static const unsigned   MEM_POOL_STORE_EXPAND_FACTOR    = 2;

static const unsigned   MEM_NODE_HEAP_INIT_CAPACITY     = 40;
static const float      MEM_NODE_HEAP_FILL_FACTOR       = 0.75;
static const unsigned   MEM_NODE_HEAP_EXPAND_FACTOR     = 2;

static const unsigned   MEM_GAP_IX_INIT_CAPACITY        = 40;
static const float      MEM_GAP_IX_FILL_FACTOR          = 0.75;
static const unsigned   MEM_GAP_IX_EXPAND_FACTOR        = 2;



/*********************/
/*                   */
/* Type declarations */
/*                   */
/*********************/
typedef struct _node {
    alloc_t alloc_record;
    unsigned used;
    unsigned allocated;
    struct _node *next, *prev; // doubly-linked list for gap deletion
} node_t, *node_pt;

typedef struct _gap {
    size_t size;
    node_pt node;
} gap_t, *gap_pt;

typedef struct _pool_mgr {
    pool_t pool;
    node_pt node_heap;
    unsigned total_nodes;
    unsigned used_nodes;
    gap_pt gap_ix;
    unsigned gap_ix_capacity;
} pool_mgr_t, *pool_mgr_pt;



/***************************/
/*                         */
/* Static global variables */
/*                         */
/***************************/
static pool_mgr_pt *pool_store = NULL; // an array of pointers, only expand
static unsigned pool_store_size = 0;
static unsigned pool_store_capacity = 0;



/********************************************/
/*                                          */
/* Forward declarations of static functions */
/*                                          */
/********************************************/
static alloc_status _mem_resize_pool_store();
static alloc_status _mem_resize_node_heap(pool_mgr_pt pool_mgr);
static alloc_status _mem_resize_gap_ix(pool_mgr_pt pool_mgr);
static alloc_status
        _mem_add_to_gap_ix(pool_mgr_pt pool_mgr,
                           size_t size,
                           node_pt node);
static alloc_status
        _mem_remove_from_gap_ix(pool_mgr_pt pool_mgr,
                                size_t size,
                                node_pt node);
static alloc_status _mem_sort_gap_ix(pool_mgr_pt pool_mgr);



/****************************************/
/*                                      */
/* Definitions of user-facing functions */
/*                                      */
/****************************************/
alloc_status mem_init(){
    // ensure that it's called only once until mem_free
    // allocate the pool store with initial capacity
    // note: holds pointers only, other functions to allocate/deallocate
    //---------------------------------------------------------------------------------
    if(pool_store != NULL){
        return ALLOC_CALLED_AGAIN;
    }
    pool_store = (pool_mgr_pt*) calloc (MEM_POOL_STORE_INIT_CAPACITY, sizeof(pool_mgr_pt));
    pool_store_size = 0;
    pool_store_capacity = MEM_POOL_STORE_INIT_CAPACITY;
    if(pool_store != NULL) return ALLOC_OK;
    else return ALLOC_FAIL;
}//End mem_init

alloc_status mem_free() {
    // ensure that it's called only once for each mem_init
    if(pool_store == NULL){
        printf("pool_store is NULL \n");
        return ALLOC_CALLED_AGAIN;
    }
    // make sure all pool managers have been deallocated
    for(int de_alloc = 0; de_alloc < pool_store_size; de_alloc++){
        if(pool_store[de_alloc] != NULL){
            return ALLOC_FAIL;
        }
    }

    //free the pool store array
    free(pool_store);

    // update static variables
    pool_store = NULL;
    pool_store_size = 0;
    pool_store_capacity = 0;
    return ALLOC_OK;

}//End mem_free

pool_pt mem_pool_open(size_t size, alloc_policy policy) {
    // make sure there the pool store is allocated
    // expand the pool store, if necessary
    // allocate a new mem pool mgr
    // check success, on error return null
    // allocate a new memory pool
    // check success, on error deallocate mgr and return null
    // allocate a new node heap
    // check success, on error deallocate mgr/pool and return null
    // allocate a new gap index
    // check success, on error deallocate mgr/pool/heap and return null
    // assign all the pointers and update meta data:
    //   initialize top node of node heap
    //   initialize top node of gap index
    //   initialize pool mgr
    //   link pool mgr to pool store
    // return the address of the mgr, cast to (pool_pt)
    //assert(pool_store);

//---------------------------------------------------------------------------------
    // make sure there the pool store is allocated
    if(pool_store == NULL) {
        return NULL;
    }
    // expand the pool store, if necessary
    _mem_resize_pool_store();

    // allocate a new mem pool mgr
    pool_mgr_pt pool_manager = calloc(1, sizeof(pool_mgr_t));

    // check success, on error return null
    if(pool_manager == NULL) {
        return NULL;
    }

    // allocate a new memory pool
    (*pool_manager).pool.mem = (char*) calloc(size, sizeof(char));

    // check success, on error return null
    if((*pool_manager).pool.mem == NULL) {
        free(pool_manager);
        return NULL;
    }

    // allocating a new node heap
    (*pool_manager).node_heap = (node_pt)
            calloc(MEM_NODE_HEAP_INIT_CAPACITY, sizeof(node_t));

    // check success, on error deallocate mgr/pool and return null
    if((*pool_manager).node_heap == NULL) {
        free((*pool_manager).pool.mem);
        free(pool_manager);
        return NULL;
    }

    // allocate a new gap index
    (*pool_manager).gap_ix = (gap_pt)
            calloc(MEM_GAP_IX_INIT_CAPACITY, sizeof(gap_t));

    // check success, on error deallocate mgr/pool/heap and return null
    if((*pool_manager).gap_ix == NULL) {
        free((*pool_manager).node_heap);
        free((*pool_manager).pool.mem);
        free(pool_manager);
        return NULL;
    }

    /* assign all the pointers and update meta data:
    initialize top node of node heap */
    (*pool_manager).node_heap[0].alloc_record.size = size;
    (*pool_manager).node_heap[0].alloc_record.mem = (*pool_manager).pool.mem;
    (*pool_manager).node_heap[0].used = 1;
    (*pool_manager).node_heap[0].allocated = 0;
    (*pool_manager).used_nodes = 1;
    (*pool_manager).total_nodes = MEM_NODE_HEAP_INIT_CAPACITY;

    //initialize top node of gap index
    (*pool_manager).gap_ix[0].node = &(*pool_manager).node_heap[0];
    (*pool_manager).gap_ix[0].size =
            (*pool_manager).node_heap[0].alloc_record.size;
    (*pool_manager).gap_ix_capacity = MEM_GAP_IX_INIT_CAPACITY;

    //initialize pool mgr
    (*pool_manager).pool.policy = policy;
    (*pool_manager).pool.total_size = size;
    (*pool_manager).pool.alloc_size = 0;
    (*pool_manager).pool.num_allocs = 0;
    (*pool_manager).pool.num_gaps = 1;

    //link pool mgr to pool store
    pool_store[pool_store_size] = pool_manager;
    pool_store_size++;

    //return the address of the mgr, cast to (pool_pt)
    return (pool_pt) pool_manager;

}//End mem_pool_open

alloc_status mem_pool_close(pool_pt pool) {
    // get mgr from pool by casting the pointer to (pool_mgr_pt)
    // check if this pool is allocated
    // check if pool has only one gap
    // check if it has zero allocations
    // free memory pool
    // free node heap
    // free gap index
    // find mgr in pool store and set to null
    // note: don't decrement pool_store_size, because it only grows
    // free mgr
    //---------------------------------------------------------------------------------

    // get mgr from pool by casting the pointer to (pool_mgr_pt)
    pool_mgr_pt pool_manger = (pool_mgr_pt) pool;
    // check if this pool is allocated
    if(pool_manger == NULL){
        return ALLOC_NOT_FREED;
    }

    // check if pool has only one gap
    if((*pool_manger).pool.num_gaps != 1){
        return ALLOC_NOT_FREED;
    }

    // check if it has zero allocations
    if((*pool_manger).pool.num_allocs != 0){
        return ALLOC_NOT_FREED;
    }

    //free memory pool
    free((*pool).mem);

    //free node heap
    free((*pool_manger).node_heap);
    (*pool_manger).node_heap = NULL;

    //free gap index
    free((*pool_manger).gap_ix);
    (*pool_manger).gap_ix = NULL;

    //find mgr in pool store and set to null
    for(int parser = 0; parser < pool_store_capacity; parser++){
        if(pool_store[parser] == pool_manger) {
            pool_store[parser] = NULL;
            parser = pool_store_capacity;
        }
    }

    //free pool_manager
    free(pool_manger);

    return ALLOC_OK;
}//End mem_pool_close

alloc_pt mem_new_alloc(pool_pt pool, size_t size) {
    // get mgr from pool by casting the pointer to (pool_mgr_pt)
    // check if any gaps, return null if none
    // expand heap node, if necessary, quit on error
    // check used nodes fewer than total nodes, quit on error
    // get a node for allocation:
    // if FIRST_FIT, then find the first sufficient node in the node heap
    // if BEST_FIT, then find the first sufficient node in the gap index
    // check if node found
    // update metadata (num_allocs, alloc_size)
    // calculate the size of the remaining gap, if any
    // remove node from gap index
    // convert gap_node to an allocation node of given size
    // adjust node heap:
    //   if remaining gap, need a new node
    //   find an unused one in the node heap
    //   make sure one was found
    //   initialize it to a gap node
    //   update metadata (used_nodes)
    //   update linked list (new node right after the node for allocation)
    //   add to gap index
    //   check if successful
    // return allocation record by casting the node to (alloc_pt)
    //---------------------------------------------------------------------------------

    //get mgr from pool by casting the pointer to (pool_mgr_pt)
    pool_mgr_pt pool_manager = (pool_mgr_pt) pool;

    //check if any gaps, return null if none
    if((*pool_manager).pool.num_gaps == 0) {
        return NULL;
    }

    //expand heap node, if necessary, quit on error
    _mem_resize_node_heap(pool_manager);

    //get a node for allocation:
    node_pt alloc_node = NULL;

    //FIRST_FIT
    if((*pool_manager).pool.policy == FIRST_FIT) {
        //find the first sufficient node in the node heap
        for(int parser = 0; parser < (*pool_manager).total_nodes; parser++) {
            alloc_node = &(*pool_manager).node_heap[parser];
            if((*alloc_node).used && !(*alloc_node).allocated &&
               (*alloc_node).alloc_record.size >= size) {
                parser = (*pool_manager).total_nodes;
            }
        }
        if((*alloc_node).alloc_record.size < size) {
            alloc_node = NULL;
        }
    }
        //BEST_FIT
    else if((*pool_manager).pool.policy == BEST_FIT){
        //find the first sufficient node in the gap index
        for(int parser=0; parser < (*pool_manager).gap_ix_capacity; parser++) {
            if((*pool_manager).gap_ix[parser].size >= size) {
                alloc_node = (*pool_manager).gap_ix[parser].node;
                parser = (*pool_manager).gap_ix_capacity;
            }
        }
    }

    //check if node found
    if(alloc_node == NULL) {
        return NULL;
    }

    //update metadata (num_allocs, alloc_size)
    (*pool_manager).pool.num_allocs++;
    (*pool_manager).pool.alloc_size += size;

    //calculate the size of the remaining gap, if any
    size_t remaining_gap_size = (*alloc_node).alloc_record.size - size;

    //remove node from gap index
    _mem_remove_from_gap_ix(pool_manager, size, alloc_node);

    //convert gap_node to an allocation node of given size
    (*alloc_node).allocated = 1;
    (*alloc_node).alloc_record.size = size;

    //adjust node heap:
    if(remaining_gap_size != 0) {
        //   if remaining gap, need a new node
        node_pt unused_node = NULL;
        for(int parser = 0; parser < (*pool_manager).total_nodes; parser++) {
            //   find an unused one in the node heap
            if((*pool_manager).node_heap[parser].used == 0) {
                unused_node = &(*pool_manager).node_heap[parser];
                parser = (*pool_manager).total_nodes;
            }
        }

        // make sure it found one
        if(unused_node == NULL) {
            return NULL;
        }

        //initialize it to a gap node
        (*unused_node).alloc_record.size = remaining_gap_size;
        (*unused_node).alloc_record.mem =
                (*alloc_node).alloc_record.mem + size;
        (*unused_node).allocated = 0;
        (*unused_node).used = 1;

        //update metadata (used_nodes)
        (*pool_manager).used_nodes++;

        // update linked list (new node right after the node for allocation)
        (*unused_node).prev = alloc_node;
        (*unused_node).next = (*alloc_node).next;
        if((*alloc_node).next != NULL) {
            (*(*alloc_node).next).prev = unused_node;
        }
        (*alloc_node).next = unused_node;

        //add to gap index
        _mem_add_to_gap_ix(pool_manager, remaining_gap_size, unused_node);
    }

    //return allocation record by casting the node to (alloc_pt)
    return (alloc_pt) alloc_node;
}//End mem_new_alloc

alloc_status mem_del_alloc(pool_pt pool, alloc_pt alloc) {
    // get mgr from pool by casting the pointer to (pool_mgr_pt)
    // get node from alloc by casting the pointer to (node_pt)
    // find the node in the node heap
    // this is node-to-delete
    // make sure it's found
    // convert to gap node
    // update metadata (num_allocs, alloc_size)
    // if the next node in the list is also a gap, merge into node-to-delete
    //   check success
    //   add the size to the node-to-delete
    //   update node as unused
    //   update metadata (used nodes)
    //   update linked list:
    /*
                    if (next->next) {
                        next->next->prev = node_to_del;
                        node_to_del->next = next->next;
                    } else {
                        node_to_del->next = NULL;
                    }
                    next->next = NULL;
                    next->prev = NULL;
     */

    // this merged node-to-delete might need to be added to the gap index
    // but one more thing to check...
    // if the previous node in the list is also a gap, merge into previous!
    //   remove the previous node from gap index
    //   check success
    //   add the size of node-to-delete to the previous
    //   update node-to-delete as unused
    //   update metadata (used_nodes)
    //   update linked list
    /*
                    if (node_to_del->next) {
                        prev->next = node_to_del->next;
                        node_to_del->next->prev = prev;
                    } else {
                        prev->next = NULL;
                    }
                    node_to_del->next = NULL;
                    node_to_del->prev = NULL;
     */
    //   change the node to add to the previous node!
    // add the resulting node to the gap index
    // check success
//-----------------------------------------------------------------------------------------


    //get mgr from pool by casting the pointer to (pool_mgr_pt)
    pool_mgr_pt pool_manager = (pool_mgr_pt) pool;

    //get node from alloc by casting the pointer to (node_pt)
    node_pt node_to_delete = (node_pt) alloc;

    //convert to gap node
    (*node_to_delete).allocated = 0;

    //update metadata (num_allocs, alloc_size)
    (*pool_manager).pool.num_allocs--;
    (*pool_manager).pool.alloc_size -= (*alloc).size;

    if((*node_to_delete).next != NULL &&
            //the next node in the list is also a gap, merge into node-to-delete
            //remove the next node from gap index
       (*(*node_to_delete).next).allocated == 0) {
        _mem_remove_from_gap_ix(pool_manager,
                                (*(*node_to_delete).next).alloc_record.size,
                                (*node_to_delete).next);

        //add the size to the node-to-delete
        (*alloc).size += (*(*node_to_delete).next).alloc_record.size;

        //update node as unused
        (*(*node_to_delete).next).used = 0;
        (*(*node_to_delete).next).alloc_record.size = 0;
        (*(*node_to_delete).next).alloc_record.mem = NULL;

        //update metadata (used nodes)
        (*pool_manager).used_nodes--;

        //update linked list:
        if((*(*node_to_delete).next).next != NULL) {
            //there exists a node after the next node
            (*(*(*node_to_delete).next).next).prev = node_to_delete;
            (*node_to_delete).next = (*(*node_to_delete).next).next;
        } else {
            //the next node is the last node
            (*(*node_to_delete).next).prev = NULL;
            (*node_to_delete).next = NULL;
        }
    }

    //this merged node-to-delete might need to be added to the gap index
    //but one more thing to check...

    //the previous node in the list is also a gap, merge into previous!
    if((*node_to_delete).prev && (*(*node_to_delete).prev).allocated == 0) {
        node_pt prev_node = (*node_to_delete).prev;

        //remove the previous node from gap index
        alloc_status remove_status =
                _mem_remove_from_gap_ix(pool_manager,
                                        (*prev_node).alloc_record.size,
                                        prev_node);

        //check success
        if(remove_status == ALLOC_FAIL) {
            return ALLOC_FAIL;
        }

        //add the size of node-to-delete to the previous
        (*prev_node).alloc_record.size += (*alloc).size;

        //update node-to-delete as unused
        (*node_to_delete).used = 0;
        (*node_to_delete).alloc_record.size = 0;
        (*node_to_delete).alloc_record.mem = NULL;

        //update metadata (used_nodes)
        (*pool_manager).used_nodes--;

        //update linked list
        if((*node_to_delete).next != NULL) {
            //Delete around this node
            (*prev_node).next = (*node_to_delete).next;
            (*(*node_to_delete).next).prev = prev_node;
        } else {
            //Delete this node
            (*prev_node).next = NULL;
        }
        (*node_to_delete).next = NULL;
        (*node_to_delete).prev = NULL;

        node_to_delete = prev_node;  //   change the node to add to the previous node!
    }

    //add the resulting node to the gap index
    alloc_status add_status = _mem_add_to_gap_ix(pool_manager,
                                                 (*node_to_delete).alloc_record.size,
                                                 node_to_delete);

    //check success
    return add_status;
}//End mem_del_alloc

void mem_inspect_pool(pool_pt pool,
                      pool_segment_pt *segments,
                      unsigned *num_segments) {
    // get the mgr from the pool
    // allocate the segments array with size == used_nodes
    // check successful
    // loop through the node heap and the segments array
    //    for each node, write the size and allocated in the segment
    // "return" the values:
    /*
                    *segments = segs;
                    *num_segments = pool_mgr->used_nodes;
     */
    //--------------------------------------------------------------------------------

    //get the mgr from the pool
    pool_mgr_pt pool_magager = (pool_mgr_pt) pool;

    //allocate the segments array with size == used_nodes
    pool_segment_pt segs = (pool_segment_pt)
            calloc((*pool_magager).used_nodes, sizeof(pool_segment_t));

    //check successful
    if(segs == NULL) {
        return;
    }

    node_pt current_node = (*pool_magager).node_heap;
    int index = 0;
    //loop through the node heap and the segments array
    //for each node, write the size and allocated in the segment
    while(current_node != NULL) {
        segs[index].size = (*current_node).alloc_record.size;
        segs[index].allocated = (*current_node).allocated;
        index++;
        current_node = (*current_node).next;
    }

    //"return" values:
    *segments = segs;
    *num_segments = (*pool_magager).used_nodes;

}//End mem_inspect_pool



/***********************************/
/*                                 */
/* Definitions of static functions */
/*                                 */
/***********************************/
static alloc_status _mem_resize_pool_store(){
    // check if necessary
    /*
                if (((float) pool_store_size / pool_store_capacity)
                    > MEM_POOL_STORE_FILL_FACTOR) {...}
     */
    // don't forget to update capacity variables
//-----------------------------------------------------------------------------------

    float size_used_percent = (float)
                                      pool_store_size / pool_store_capacity;
    if (size_used_percent > MEM_POOL_STORE_FILL_FACTOR) {
        //pool_store is getting full and needs to expand
        unsigned new_cap = MEM_POOL_STORE_EXPAND_FACTOR*pool_store_capacity;
        pool_store = (pool_mgr_pt*)
                realloc(pool_store, new_cap * sizeof(pool_mgr_pt));
        pool_store_capacity = new_cap;
    }
    return ALLOC_OK;
}//End _mem_resize_pool_store

static alloc_status _mem_resize_node_heap(pool_mgr_pt pool_mgr) {

    float nodes_used_percent = (float)
                                       (*pool_mgr).used_nodes / (*pool_mgr).total_nodes;
    if (nodes_used_percent > MEM_NODE_HEAP_FILL_FACTOR) {
        //node_heap is getting full and needs to expand
        unsigned new_cap =
                MEM_NODE_HEAP_EXPAND_FACTOR*(*pool_mgr).total_nodes;
        (*pool_mgr).node_heap = (node_pt)
                realloc((*pool_mgr).node_heap, new_cap * sizeof(node_t));
        (*pool_mgr).total_nodes = new_cap;
    }
    return ALLOC_OK;
}//End _mem_resize_node_heap

static alloc_status _mem_resize_gap_ix(pool_mgr_pt pool_mgr) {
    float active_gaps_percent = (float)
                                        (*pool_mgr).pool.num_gaps / (*pool_mgr).gap_ix_capacity;
    if (active_gaps_percent > MEM_GAP_IX_FILL_FACTOR) {
        //gap_ix is getting full and needs to expand
        unsigned new_cap =
                MEM_GAP_IX_EXPAND_FACTOR*(*pool_mgr).gap_ix_capacity;
        (*pool_mgr).gap_ix = (gap_pt)
                realloc((*pool_mgr).gap_ix, new_cap * sizeof(gap_t));
        (*pool_mgr).gap_ix_capacity = new_cap;
    }
    return ALLOC_OK;
}//End _mem_resize_gap_ix

static alloc_status _mem_add_to_gap_ix(pool_mgr_pt pool_mgr,
                                       size_t size,
                                       node_pt node) {
    // expand the gap index, if necessary (call the function)
    // add the entry at the end
    // update metadata (num_gaps)
    // sort the gap index (call the function)
    // check success
    //-------------------------------------------------------------

    //expand the gap index, if necessary (call the function)
    _mem_resize_gap_ix(pool_mgr);

    //add the entry at the end
    gap_pt new_gap = &(*pool_mgr).gap_ix[(*pool_mgr).pool.num_gaps];
    (*new_gap).size = size;
    (*new_gap).node = node;

    //update metadata (num_gaps)
    (*pool_mgr).pool.num_gaps++;

    //sort the gap index (call the function)
    alloc_status status = _mem_sort_gap_ix(pool_mgr);

    if(status == ALLOC_OK) {
        return ALLOC_OK; // check success
    }
    return ALLOC_FAIL;
}//End _mem_add_to_gap_ix

static alloc_status _mem_remove_from_gap_ix(pool_mgr_pt pool_mgr,
                                            size_t size,
                                            node_pt node) {
    // find the position of the node in the gap index
    // loop from there to the end of the array:
    //    pull the entries (i.e. copy over) one position up
    //    this effectively deletes the chosen node
    // update metadata (num_gaps)
    // zero out the element at position num_gaps!
    // -------------------------------------------------------------------------------------
    int position = -1;
    for(int parser = 0; parser < (*pool_mgr).gap_ix_capacity; parser++) {
        // find the position of the node in the gap index
        if((*pool_mgr).gap_ix[parser].node == node) {
            position = parser;
            parser = (*pool_mgr).gap_ix_capacity;
        }
    }
    if(position < 0) {
        return ALLOC_FAIL; //didn't find the node in the gap index
    }

    for(int parser=position; parser < (*pool_mgr).gap_ix_capacity; parser++) {
        /*
         * loop from there to the end of the array:
         * pull the entries (i.e. copy over) one position up
         * this effectively deletes the chosen node
         */
        (*pool_mgr).gap_ix[parser] = (*pool_mgr).gap_ix[parser + 1];
    }

    //update metadata (num_gaps)
    (*pool_mgr).pool.num_gaps--;

    //zero out the element at position num_gaps!
    gap_pt to_delete = &(*pool_mgr).gap_ix[(*pool_mgr).pool.num_gaps];
    (*to_delete).size = 0;
    (*to_delete).node = NULL;
    return ALLOC_OK;
}//End _mem_remove_from_gap_ix

// note: only called by _mem_add_to_gap_ix, which appends a single entry
static alloc_status _mem_sort_gap_ix(pool_mgr_pt pool_mgr){
// the new entry is at the end, so "bubble it up"
// loop from num_gaps - 1 until but not including 0:
//    if the size of the current entry is less than the previous (u - 1)
//    or if the sizes are the same but the current entry points to a
//    node with a lower address of pool allocation address (mem)
//       swap them (by copying) (remember to use a temporary variable)
// -----------------------------------------------------------------------

    //the new entry is at the end, so "bubble it up"
    for(int parser = (*pool_mgr).pool.num_gaps - 1; parser > 0; parser--) {
        // loop from num_gaps - 1 until but not including 0:
        if((*pool_mgr).gap_ix[parser].size <
           (*pool_mgr).gap_ix[parser - 1].size) {
            /*
             * if the size of the current entry is less than the previous (u - 1)
             * swap them (by copying)
             */
            gap_t temp_gap_swapper = (*pool_mgr).gap_ix[parser];
            (*pool_mgr).gap_ix[parser] = (*pool_mgr).gap_ix[parser - 1];
            (*pool_mgr).gap_ix[parser - 1] = temp_gap_swapper;
        }
        else if((*pool_mgr).gap_ix[parser].size ==
                (*pool_mgr).gap_ix[parser - 1].size) {
            //    or if the sizes are the same
            // a node with a lower address of pool allocation address (mem)
            if((*(*pool_mgr).gap_ix[parser].node).alloc_record.mem <
               (*(*pool_mgr).gap_ix[parser - 1].node).alloc_record.mem) {
                //and the current entry points to a node with a lower address
                // of pool allocation address (mem)
                //       swap them (by copying)
                gap_t temp_gap_swapper = (*pool_mgr).gap_ix[parser];
                (*pool_mgr).gap_ix[parser] = (*pool_mgr).gap_ix[parser - 1];
                (*pool_mgr).gap_ix[parser - 1] = temp_gap_swapper;
            }

        }
    }
    return ALLOC_OK;
}//End _mem_sort_gap_ix
