/**
* All functions you make for the assignment must be implemented in this file.
* Do not submit your assignment with a main function in this file.
* If you submit with a main function in this file, you will get a zero.
*/
#include "sfmm.h"
#include "helper.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

/**
* You should store the heads of your free lists in these variables.
* Doing so will make it accessible via the extern statement in sfmm.h
* which will allow you to pass the address to sf_snapshot in a different file.
*/
free_list seg_free_list[4] = {
{NULL, LIST_1_MIN, LIST_1_MAX},
{NULL, LIST_2_MIN, LIST_2_MAX},
{NULL, LIST_3_MIN, LIST_3_MAX},
{NULL, LIST_4_MIN, LIST_4_MAX}
};


int sbrk_counter =0;
int remaining_space =0;
int allocated_memory = 0;

void *sf_malloc(size_t size) {

    if(!validate(size)){
        return NULL;
    }

    if(initialize_memory(size)==NULL){
        if(validate_size(size,PAGE_SZ*4)){
            sf_header* header = set_allocated_block(setup_new_page(request_memory(size),size),size);
            remaining_space = sbrk_counter*PAGE_SZ - (header->block_size*16);
            sf_header* freeheader = set_free_block(header,remaining_space);
            update_seg_free_list((sf_free_header*)freeheader);
            sf_blockprint(header);
            return ((char*)header + SF_HEADER_SIZE/8);
        }else{
            sf_errno = ENOMEM;
            return NULL;
        }
    }else{
        sf_free_header* freeSpace = find_space(size);
        if(freeSpace){
            return use_free_space(freeSpace,size);
        }else{
            int x = 0;
            int* actual_space_wanted = &x;

            char* verification_ptr = (char*)get_heap_end() - SF_FOOTER_SIZE/8;
            sf_footer* verification_footer = (sf_footer*)verification_ptr;
            sf_header* place_holder ;

            if(verification_footer->allocated == 0 && find_header((sf_header*)((char*)verification_footer - (verification_footer->block_size*16 - SF_FOOTER_SIZE/8)),seg_free_list)!=NULL){
                place_holder =  get_prev_free_block(verification_ptr,actual_space_wanted,size);
            }else{
                place_holder = request_memory(size);
            }
            if(validate_size(size,*actual_space_wanted)){
                return use_free_space_colaesce((sf_free_header*)place_holder,size);
            }else{
                sf_errno = ENOMEM;
                return NULL;
            }
        }
    }

    sf_errno = ENOMEM;
    return NULL;
}

void *sf_realloc(void *ptr, size_t size) {
    if(ptr == NULL){
        errno = EINVAL;
        abort();
    }
    char* h = (char*)ptr - SF_HEADER_SIZE/8;
    sf_header* header = (sf_header*)h;
    sf_footer* footer = get_footer(header);

    if(footer->requested_size == size){
        return ptr;
    }
    if(size == 0){
        sf_free(ptr);
        return NULL;
    }
    if(header->allocated == 0){
        errno = EINVAL;
        return NULL;
    }

    if(!(compute_block_size(size) <= header->block_size*16)){
        void* return_ptr;
        if((return_ptr = sf_malloc(size))== NULL){
            sf_errno = ENOMEM;
            return NULL;
        }else{
            memcpy(return_ptr, ptr, size);
            sf_free(ptr);
            sf_errno = 0;
            return return_ptr;
        }

    }else{
        int free_space = header->block_size*16 - compute_block_size(size);
        if(free_space >= 32){
             sf_header* changed_header = set_allocated_block((sf_header*)header,size);
             sf_header* freeheader = set_free_block(changed_header,free_space);
             coalesce_down(freeheader);
        }
        return ptr;
    }

    return NULL;
}

void sf_free(void *ptr) {
    if(ptr == NULL){
        abort();
    }
    char* h = (char*)ptr - SF_HEADER_SIZE/8;
    sf_header* header = (sf_header*)h;

    sf_footer* footer = get_footer((sf_header*)h);

    if(header->allocated ==0 || footer->allocated ==0 ){
        abort();
    }

    if(header->allocated != footer->allocated){
        abort();
    }

    if(header->padded != footer->padded){
        abort();
    }

    if(header->two_zeroes != 0 || footer->two_zeroes !=0){
        abort();
    }

    if(header->two_zeroes != footer->two_zeroes){
        abort();
    }

    if(header->block_size != footer->block_size){
        abort();
    }

    if(header->block_size != (compute_block_size(footer->requested_size))/16){
        abort();
    }

    if((void*)h<(void*)get_heap_start() || (void*)footer<(void*)get_heap_start()){
        errno = EINVAL;
        abort();
    }

    if(find_header((sf_header*)h,seg_free_list) != NULL){
        errno = ENOMEM;
        abort();
    }

    de_allocate_block(header);
    coalesce_down(h);
}




