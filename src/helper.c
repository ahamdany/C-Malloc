#include "sfmm.h"
#include "helper.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

int user_requested_size =0;

int validate(size_t size){
    if(allocated_memory > PAGE_SZ*sbrk_counter){
        sf_errno = ENOMEM;
        return 0;
    }
    if(size < 1) {
        sf_errno = EINVAL;
        return 0;
    }else if (size > PAGE_SZ*4) {
        sf_errno = EINVAL;
        return 0;
    }else if (size == PAGE_SZ*4) {
        sf_errno = ENOMEM;
        return 0;
    }else {
        sf_errno =0;
        return 1;
    }
}

void* initialize_memory(size_t size){
    void* heap_start = get_heap_start();
    if(heap_start == NULL){
        return NULL;
    }else{
        return heap_start;
    }
}

void* request_memory(size_t size){

    int block_size =compute_block_size(size);

    if( block_size <= 1*PAGE_SZ){
        sbrk_counter=1 + sbrk_counter;
        return sf_sbrk();
    }else if(block_size <= 2*PAGE_SZ){
        sbrk_counter=2 + sbrk_counter;
        void* page = sf_sbrk();
        sf_sbrk();
        return page;
    }else if(block_size <= 3*PAGE_SZ){
        sbrk_counter=3 + sbrk_counter;
        void* page = sf_sbrk();
        sf_sbrk();
        sf_sbrk();
        return page;
    }else if(block_size <= 4*PAGE_SZ){
        sbrk_counter=4 + sbrk_counter;
        void* page = sf_sbrk();
        sf_sbrk();
        sf_sbrk();
        sf_sbrk();
        return page;
    }else{
        return NULL;
    }
}

bool validate_size(size_t size, int rem_space){
    int block_size = compute_block_size(size);

    if(sbrk_counter == 4 && block_size <= rem_space + remaining_space){
        return true;
    }
    else if(sbrk_counter == 3 && block_size <= rem_space + remaining_space){
        return true;
    }else if(sbrk_counter == 2 && block_size <= rem_space + remaining_space){
        return  true;
    }else if(sbrk_counter == 1 && block_size <= rem_space + remaining_space){
        return  true;
    }else if(sbrk_counter == 0){
        return true;
    }else{
        return false;
    }
}

sf_free_header* find_space(size_t size){
    
    int block_size = compute_block_size(size);
    sf_free_header* temp_header;

    if((temp_header = iterate(0,block_size)) != NULL){
        return temp_header;
    }
    if((temp_header = iterate(1,block_size)) != NULL){
        return temp_header;
    }
    if((temp_header = iterate(2,block_size)) != NULL){
        return temp_header;
    }
    if((temp_header = iterate(3,block_size)) != NULL){
        return temp_header;
    }

    return 0;
}

sf_free_header* iterate(int which_list,int block_size){
    
    sf_free_header* temp_header = seg_free_list[which_list].head;
    if(temp_header != NULL){
        while(temp_header->next !=NULL){
            if(temp_header->header.block_size*16 >= block_size){
                return temp_header;
            }
            temp_header = temp_header->next;
        }
        if(temp_header->header.block_size*16 >= block_size){
                return temp_header;
        }
    }
    return NULL;
}

void* use_free_space(sf_free_header* freeSpace, size_t size){
    
    int free_block_size = freeSpace->header.block_size*16;
    int *cordinates = find_header((sf_header*)freeSpace,seg_free_list);
    remove_header_from_freelist(seg_free_list,cordinates);
    remaining_space = free_block_size - compute_block_size(size);

    if(remaining_space>=32){
        sf_header* header = set_allocated_block((sf_header*)freeSpace,size);
        sf_header* freeheader = set_free_block(header,remaining_space);
        update_seg_free_list((sf_free_header*)freeheader);
        return ((char*)header + SF_HEADER_SIZE/8);
    }else{
        sf_header* header = set_allocated_block((sf_header*)freeSpace,size + remaining_space);
        return ((char*)header + SF_HEADER_SIZE/8);
    }
}

void* use_free_space_colaesce(sf_free_header* freeSpace, size_t size){

    int free_block_size = freeSpace->header.block_size*16;
        int *cordinates = find_header((sf_header*)freeSpace,seg_free_list);
        remove_header_from_freelist(seg_free_list,cordinates);
        remaining_space = free_block_size - compute_block_size(size);

        if(remaining_space>=32){
           sf_header* header = set_allocated_block((sf_header*)freeSpace,size);
           sf_header* freeheader = set_free_block(header,remaining_space);
           update_seg_free_list((sf_free_header*)freeheader);
           return ((char*)header + SF_HEADER_SIZE/8);
        }else{
           sf_header* header = set_allocated_block((sf_header*)freeSpace,free_block_size);
          return ((char*)header + SF_HEADER_SIZE/8);
        }
}

sf_header* get_prev_free_block(char* verification_ptr,int* actual_space_wanted,size_t size){
    
    sf_footer* verification_footer = (sf_footer*)verification_ptr;
    char* prev_free_block_header = (char*)verification_footer - (verification_footer->block_size*16 - SF_FOOTER_SIZE/8);
    int prev_block_size = verification_footer->block_size;
    *actual_space_wanted = compute_block_size(size) - prev_block_size*16;
    sf_header* verification_header = (sf_header*)prev_free_block_header;
    int new_sbrk_counter = sbrk_counter;
    request_memory(*actual_space_wanted);
    new_sbrk_counter = sbrk_counter - new_sbrk_counter;
    verification_header->block_size = (verification_header->block_size*16 + (PAGE_SZ*new_sbrk_counter))/16;
    remaining_space = verification_header->block_size*16;
    return verification_header;
}

void update_free_List(void* header){

}

sf_header* set_header(sf_header* header, size_t size){
    
    user_requested_size = size;
    int total = 0;
    int padding = 0;
    total = size + SF_HEADER_SIZE/8 + SF_FOOTER_SIZE/8;

    while(total%16 != 0){
        padding++;
        total++;
    }
    header->block_size = total/16;
    if(padding > 0)
        header->padded = 1;
    else
        header->padded= 0;
    header->allocated=1;
    return header;
}


sf_footer* set_footer(sf_header* header){
    
    char* ptr;
    ptr = (char*)header + ((header->block_size*16) - SF_FOOTER_SIZE/8);
    sf_footer* footer = (sf_footer*)ptr;
    footer->allocated = header->allocated;
    footer->padded = header->padded;
    footer->block_size = header->block_size;
    if(header->allocated){
        footer->requested_size = user_requested_size;
    }else{
        footer->requested_size = 0;
    }
    return footer;
}

sf_header* setup_new_page(void* page, size_t size){
    sf_header* header = set_free_header(page,size);
    set_footer(header);
    return header;
}

void add_head(sf_free_header* freeheader,int which_list,int min_constraint,int max_constraint){
    free_list freelist = {freeheader,min_constraint,max_constraint};
    seg_free_list[which_list] = freelist;
    seg_free_list[which_list].head->next = NULL;
    seg_free_list[which_list].head->prev = NULL;
}

void add_tail(sf_free_header* freeheader,int which_list){
    sf_free_header* temp_header = seg_free_list[which_list].head;
    while(temp_header->next !=NULL){
        temp_header = temp_header->next;
    }
    temp_header->next = freeheader;
    sf_free_header* next = temp_header->next;
    next->next =NULL;
    next->prev = temp_header;
}

void update_seg_free_list_tail(sf_free_header* freeHeader){
     sf_free_header* freeheader = freeHeader;
     int block_size = (freeHeader->header.block_size)*16;

     if(block_size>=LIST_1_MIN && block_size<=LIST_1_MAX){
        if(seg_free_list[0].head == NULL){
            add_head(freeheader,0,LIST_1_MIN,LIST_1_MAX);
        }else{
            add_tail(freeheader,0);
        }
    }else if(block_size>=LIST_2_MIN && block_size<=LIST_2_MAX){
        if(seg_free_list[1].head == NULL){
            add_head(freeheader,1,LIST_2_MIN,LIST_2_MAX);
        }else{
            add_tail(freeheader,1);
        }
    }else if(block_size>=LIST_3_MIN && block_size<=LIST_3_MAX){
        if(seg_free_list[2].head == NULL){
            add_head(freeheader,2,LIST_3_MIN,LIST_3_MAX);
        }else{
            add_tail(freeheader,2);
        }
    }else if(block_size>=LIST_4_MIN){
        if(seg_free_list[3].head == NULL){
            add_head(freeheader,3,LIST_4_MIN,LIST_4_MAX);
        }else{
            add_tail(freeheader,3);
        }

     }else{

     }
}

void push_head(sf_free_header* freeheader,int which_list,int min,int MAX){
    sf_free_header* head = seg_free_list[which_list].head;
    sf_free_header* head_prev = seg_free_list[which_list].head->prev;
    seg_free_list[which_list].head = freeheader;
    freeheader->next = head;
    head_prev = freeheader;
    freeheader->prev = NULL;
}

void update_seg_free_list(sf_free_header* freeHeader){
    
    sf_free_header* freeheader = freeHeader;
     int block_size = (freeHeader->header.block_size)*16;

     if(block_size>=LIST_1_MIN && block_size<=LIST_1_MAX){
        if(seg_free_list[0].head == NULL){
            add_head(freeheader,0,LIST_1_MIN,LIST_1_MAX);
        }else{
            push_head(freeheader,0,LIST_1_MIN,LIST_1_MAX);

        }
    }else if(block_size>=LIST_2_MIN && block_size<=LIST_2_MAX){
        if(seg_free_list[1].head == NULL){
            add_head(freeheader,1,LIST_2_MIN,LIST_2_MAX);
        }else{
            push_head(freeheader,1,LIST_2_MIN,LIST_2_MAX);

        }
    }else if(block_size>=LIST_3_MIN && block_size<=LIST_3_MAX){
        if(seg_free_list[2].head == NULL){
            add_head(freeheader,2,LIST_3_MIN,LIST_3_MAX);
        }else{
            push_head(freeheader,2,LIST_3_MIN,LIST_3_MAX);

        }
    }else if(block_size>=LIST_4_MIN){
        if(seg_free_list[3].head == NULL){
            add_head(freeheader,3,LIST_4_MIN,LIST_4_MAX);
        }else{
            push_head(freeheader,3,LIST_4_MIN,LIST_4_MAX);

        }

     }else{

     }
}

sf_header* set_free_header(sf_header* header, int remainingSpace){

    int total = 0;
    int padding = 0;
    total = remainingSpace;
    header->block_size = total/16;
    header->allocated=0;
    header->padded = padding;

    return header;
}

void remove_header_from_freelist(free_list* freelist,int* cordinates){

    if(find_free_list_length(freelist,cordinates[0]) ==1){
        freelist[cordinates[0]].head = NULL;
        return;
    }

    sf_free_header* temp = freelist[cordinates[0]].head;
    for(int i =0; i<cordinates[1]; i++){
        temp = temp->next;
    }

    if(temp->prev !=NULL && temp->next != NULL){
        (temp->next)->prev = temp->prev;
        (temp->prev)->next = temp->next;
    }else if(temp->prev == NULL){
        (temp->next)->prev = NULL;
    }else{
        (temp->prev)->next = NULL;
    }
}

int* get_cordinates_list_length_1(int which_list,int list_index,free_list* freelist,sf_header* header,int* cordinates){
    if(&freelist[which_list].head->header == header){
            cordinates[0] = which_list;
            cordinates[1] = list_index;
            return cordinates;
        }
        return NULL;
}

int* get_cordinates_list_length_n(int which_list,free_list* freelist,sf_header* header,int* cordinates){
    sf_free_header* temp_header = freelist[which_list].head;
    int counter = 0;
    int list_index = 0;
    while(temp_header !=NULL && temp_header->next !=NULL){
        if(&temp_header == (void*)header){
            list_index = counter;
            cordinates[0] = which_list;
            cordinates[1] = list_index;
            return cordinates;
        }
        temp_header = temp_header->next;
        counter++;
    }
    return NULL;
}

int* find_header(sf_header* header,free_list* freelist){

    int* cordinates;
    static int array[2] = {0,0};
    if(find_free_list_length(freelist,0) == 1){
        if((cordinates = get_cordinates_list_length_1(0,0,freelist,header,array)) != NULL){
            return cordinates;
        }
    }
    if((cordinates = get_cordinates_list_length_n(0,freelist,header,array)) != NULL){
            return cordinates;
        }

    if(find_free_list_length(freelist,1) == 1){
        if((cordinates = get_cordinates_list_length_1(1,0,freelist,header,array)) != NULL){
            return cordinates;
        }
    }
    if((cordinates = get_cordinates_list_length_n(1,freelist,header,array)) != NULL){
            return cordinates;
        }

    if(find_free_list_length(freelist,2) == 1){
        if((cordinates = get_cordinates_list_length_1(2,0,freelist,header,array)) != NULL){
            return cordinates;
        }
    }
    if((cordinates = get_cordinates_list_length_n(2,freelist,header,array)) != NULL){
            return cordinates;
        }

    if(find_free_list_length(freelist,3) == 1){
        if((cordinates = get_cordinates_list_length_1(3,0,freelist,header,array)) != NULL){
            return cordinates;
        }
    }
    if((cordinates = get_cordinates_list_length_n(3,freelist,header,array)) != NULL){
            return cordinates;
        }
    return NULL;
}

int find_free_list_length(free_list* freelist,int index){
    int count = 0;

    if(freelist[index].head == NULL){
        return 0;
    }

    if(freelist[index].head != NULL && freelist[index].head->next ==NULL){
        return 1;
    }

     sf_free_header* temp_header = freelist[index].head;
     while(temp_header->next !=NULL){
        count++;
        temp_header = temp_header->next;
        }

    return count;
}

int compute_block_size(size_t size){
    int total = 0;
    int padding = 0;
    total = size + SF_HEADER_SIZE/8 + SF_FOOTER_SIZE/8;

    while(total%16 != 0){
        padding++;
        total++;
    }

    return total;
}

sf_header* set_allocated_block(sf_header* header, size_t size){
    sf_header* allocated_header = set_header(header,size);
    set_footer(allocated_header);
    allocated_memory = allocated_memory + compute_block_size(size);
    return allocated_header;
}

sf_header* set_free_block(sf_header* header, size_t remaining_space){

     char* ptr = (char*)header + header->block_size*16;
     sf_header* freeHeader = (sf_header*)ptr;
     sf_header* free_header = set_free_header(freeHeader,remaining_space);
     set_footer(free_header);
    return free_header;
}

void coalesce_down_before(void* ptr){

    sf_header* new_head = (sf_header*)ptr;
    int block_size = new_head->block_size;
    char* current_header_ptr = (char*)ptr + block_size*16;
    sf_header* current_head = (sf_header*)current_header_ptr;
    if((void*)current_head == (void*)seg_free_list[0].head){
        coalesce(current_head,new_head);
    }else if((void*)current_head == (void*)seg_free_list[1].head){
        coalesce(current_head,new_head);

    }else if((void*)current_head == (void*)seg_free_list[2].head){
        coalesce(current_head,new_head);

    }else if((void*)current_head == (void*)seg_free_list[3].head){
        coalesce(current_head,new_head);

    }else{
        update_seg_free_list((sf_free_header*)new_head);
    }
}

void coalesce_down(void* ptr){

    sf_header* new_head = (sf_header*)ptr;
    int block_size = new_head->block_size;
    char* current_header_ptr = (char*)ptr + block_size*16;
    sf_header* current_head = (sf_header*)current_header_ptr;
    int* cordinates;
    if((cordinates = find_header(current_head,seg_free_list)) != NULL){
        coalesce(current_head,new_head);
    }else{
        update_seg_free_list((sf_free_header*)new_head);

    }
}

void coalesce(sf_header* current_head,sf_header* new_head){
        int* cordinates = find_header(current_head,seg_free_list);
        int block_size = (new_head->block_size*16 + (current_head->block_size*16));
        new_head->block_size = block_size/16;
        sf_header* freeheader = set_free_header(new_head,block_size);
        set_footer(freeheader);
        remove_header_from_freelist(seg_free_list,cordinates);
        update_seg_free_list((sf_free_header*)freeheader);
}

sf_header* de_allocate_block(sf_header* header){
    sf_header* de_allocated_header = de_allocate_header(header);
    set_footer(de_allocated_header);
    allocated_memory = allocated_memory - header->block_size;
    return de_allocated_header;
}

sf_header* de_allocate_header(sf_header* header){
    header->padded = 0;
    header->allocated=0;
    return header;
}

sf_footer* get_footer(sf_header* header){
    char* ptr;
    ptr = (char*)header + ((header->block_size*16) - SF_FOOTER_SIZE/8);
    sf_footer* footer = (sf_footer*)ptr;
    return footer;
}


