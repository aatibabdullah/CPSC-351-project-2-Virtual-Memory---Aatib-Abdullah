#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <errno.h>
// all 128 has been 256 before
#define PAGE_SIZE 256 // page size and entreis are BUFFlength
#define PAGE_ENTRIES 256
#define PAGE_NUM_BITS 8
#define FRAME_SIZE 256 // Frame size and entries are part of frame size
#define FRAME_ENTRIES 256 // originally 256
#define MEM_SIZE (FRAME_SIZE * FRAME_ENTRIES)
#define TLB_ENTRIES 16

#define ARGC_ERROR 1
#define FILE_ERROR 2

int logical;
int page_num;
int offset;
int physical;
int frame_num;
int val;
int page_table[PAGE_ENTRIES];
int tlb[TLB_ENTRIES][2];
int front = -1;
int back = -1;
char phsmemory[MEM_SIZE];
int mem_index=0;

int fcount;
int tlbcount; // will count how many hits there where
int addcount;
float fault_rate;
float tlb_rate; // will count the rate

int getphysical(int logical);
int getoffset(int logical);
int get_page_number(int logical);
void init_page_table(int n);
void init_tlb(int n);
int consult_pt(int page_num);
int consult_tlb(int page_num);
void update_tlb(int page_num, int frame_num);
int getframe();

int main(int argc, char *argv[]) {

    char* in_file;

    char* store_file;
    char* store_data;
    int store_fd;
    char line[8];
    FILE* in_ptr;


    init_page_table(-1);
    init_tlb(-1);

    if (argc != ARGC_ERROR) {
       printf("Enter ./virtual_mem addresses.txt!");
       exit(EXIT_FAILURE);
   }

    
    if (argc != FILE_ERRO) {
       printf("Enter addresses.txt!");
       exit(EXIT_FAILURE);
   }

   else {

       in_file = argv[1];
       //out_file = argv[2];
       store_file = "BACKING_STORE.bin"; // internal initizlization

       if((in_ptr = fopen(in_file, "r"))==NULL){ // this initializes the in_ptr, however it also is part of condition
       
       printf("The file cannot be read");
           exit(EXIT_FAILURE);
       }
       

       store_fd = open(store_file, O_RDONLY);
        store_data = mmap(0, MEM_SIZE, PROT_READ, MAP_SHARED, store_fd, 0);

       while (fgets(line, sizeof(line), in_ptr)) {
         logical = atoi(line);
         addcount++;
         page_num = get_page_number(logical);
         offset=getoffset(logical);
         frame_num = consult_tlb(page_num);
         if (frame_num != -1) {

                physical = frame_num + offset;
               val = phsmemory[physical];
              }
          else{
            frame_num = consult_pt(page_num);
            if (frame_num != -1) {

                    physical = frame_num + offset;

                    update_tlb(page_num, frame_num);

                    val = phsmemory[physical];
                  }
            else{
              // This is where the page fault is handled and it usess the backing_Store.bin
               int page_address = page_num * PAGE_SIZE;
               if(mem_index != -1)
               {
                 memcpy(phsmemory + mem_index, store_data + page_address, PAGE_SIZE);
                frame_num = mem_index;
                physical = frame_num + offset;
                val = phsmemory[physical];
                page_table[page_num] = mem_index;
                update_tlb(page_num, frame_num);
                if (mem_index < MEM_SIZE - FRAME_SIZE) {
                            mem_index += FRAME_SIZE;
                        }
                        else {

                            mem_index = -1;
                            }
               }
               else{ //failed
            }
          }
        }
            printf("logical: %5u (page:%3u, offset:%3u) ---> physical: %5u -- passed\n", logical, page_num, offset, physical);
          if(addcount%5 == 0){printf("\n");}
        }
        fault_rate = (float) fcount / (float) addcount;
        tlb_rate = (float) tlbcount / (float) addcount;

        printf("Number of Translated Addresses = %d\n", addcount);
        printf("Page Faults = %d\n", fcount);
        printf("Page Fault Rate = %.3f\n", fault_rate);
        printf("TLB Hits = %d\n", tlbcount);
        printf("TLB Hit Rate = %.3f\n", tlb_rate);

        fclose(in_ptr);
        close(store_fd);
      }
      return EXIT_SUCCESS;
    }

    int getphysical(int logical)
    {
      physical = get_page_number(logical) + getoffset(logical);
      return physical;
    }

    int get_page_number(int logical)
    {
      return(logical >> PAGE_NUM_BITS);
    }

    int getoffset(int logical)
    {
      int mask = 255;
      return logical & mask;
    }

    void init_page_table(int n)
    {
      for(int i=0; i<PAGE_ENTRIES; i++)
      {
        page_table[i]=n;
      }
    }
    void init_tlb(int n) {
        for (int i = 0; i < TLB_ENTRIES; i++) {
            tlb[i][0] = -1;
            tlb[i][1] = -1;
        }
    }
    int consult_pt(int page_num)
    {
      if(page_table[page_num]==-1)
      { fcount++;}
      return page_table[page_num];
    }
    int consult_tlb(int page_num)
    {
      for(int i =0; i< TLB_ENTRIES; i++)
      {
        if(tlb[i][0] == page_num)
        {
          tlbcount++;
          return tlb[i][1];
        }
      }
      return -1;
    }
    void update_tlb(int page_num, int frame_num)
    {
      if(front == -1)
      {
        front = 0;
        back = 0;

        tlb[back][0] = page_num;
        tlb[back][1]= frame_num;
      }
      else {
        // I used FIFO
      /*  Use circular array to implement queue. */
    front = (front + 1) % TLB_ENTRIES;
       back = (back + 1) % TLB_ENTRIES;

        /*Insert new TLB entry in the back. */
      tlb[back][0] = page_num;
      tlb[back][1] = frame_num;
    }
// The LRU algorithm is this part
/*
    if(front == -1)
    {
      front = 0;
      back = 0;

      tlb[front][0] = page_num;
      tlb[front][1]= frame_num;
    }
    else
    {
      front = (front + 1) % TLB_ENTRIES;
      back = (back + 1) % TLB_ENTRIES;

      tlb[front][0] = page_num;
      tlb[front][1] = frame_num;
    }

*/
       // LRU page replacement
      return;
    }
