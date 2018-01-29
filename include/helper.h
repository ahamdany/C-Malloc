#ifndef HELPER_H
#define HELPER_H
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

extern int sbrk_counter;
extern int remaining_space;
extern int allocated_memory;



sf_free_header* find_space(size_t size);
void update_free_list_tail(void* header);
sf_footer* set_footer(sf_header* header);
sf_header* set_header(sf_header* header, size_t size);
sf_header* setup_new_page(void* page,size_t size);
void update_seg_free_list(sf_free_header* freeheader);
sf_header* set_free_header(sf_header* header, int remainingSpace);
int* find_header(sf_header* header, free_list* freelist);
void remove_header_from_freelist(free_list* freelist,int* cordinates );
int find_free_list_length(free_list* freelist,int index);
int compute_block_size(size_t size);
int* get_cordinates_list_length_1(int which_list,int list_index,free_list* freelist,sf_header* header,int* cordinates);
int* get_cordinates_list_length_n(int which_list,free_list* freelist,sf_header* header,int* cordinates);
sf_free_header* iterate(int which_list,int block_size);
void add_head(sf_free_header* freeheader,int which_list,int min_constraint,int max_constraint);
void add_tail(sf_free_header* freeheader,int which_list);
sf_header* set_allocated_block(sf_header* header, size_t size);
sf_header* set_free_block(sf_header* header, size_t remaining_space);
int validate(size_t size);
void* initialize_memory(size_t size);
void* request_memory(size_t block_size);
bool validate_size(size_t size, int rem_space);
void* use_free_space(sf_free_header* freeSpace, size_t size);
sf_header* get_prev_free_block(char* verification_ptr,int* actual_space_wanted,size_t size);
void* use_free_space_colaesce(sf_free_header* freeSpace, size_t size);
sf_header* de_allocate_block(sf_header* header);
sf_header* de_allocate_header(sf_header* header);
void coalesce(sf_header* current_head,sf_header* new_head);
void coalesce_down(void* ptr);
sf_footer* get_footer(sf_header* header);
void coalesce_down_before(void* ptr);
#endif
