#ifndef EX4_SIM_MEM_H
#define EX4_SIM_MEM_H

#include <fstream>
#include <fcntl.h>
#include <unistd.h>
#include <cmath>
#include <bitset>
#include <iostream>
#include <string>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <cstdio>

using namespace std;


#define EXTERNAL_TABLE_LENGTH 4
#define MEMORY_SIZE 200
extern char main_memory[MEMORY_SIZE];
extern int indexes[3];


//emptyFrameIndex
typedef struct Node {
    int emptyFrameIndex;
    Node *next;
} Node;

class LinkedList{
private:
    Node * head;

public:
    LinkedList();
    ~LinkedList();
    int popNode();
    void addNodeToEnd(int value);
    void addNodeToFirst(int value);
    bool isEmpty();
};


typedef struct page_descriptor {
    bool valid;
    int frame;
    bool dirty;
    int swap_index;
    int counter;
} page_descriptor;


class sim_mem{
    int swapfile_fd; //swap file fd
    int program_fd; //executable file fd
    int text_size;
    int data_size;
    int bss_size;
    int heap_stack_size;
    int page_size;
    int swapSize;
    int number_offset_bits;
    int clock;

    page_descriptor **page_table; //pointer to page table
public:
    sim_mem(char*, char*, int, int, int, int, int);
    ~sim_mem();
    void store(int, char);
    char load(int);
    void print_memory();
    void print_page_table();
    void print_swap();
    void cleanRam();
    int ramInsertGeneral();
    void copyFrom_F_To_R(int, int, int, int);
    int convertAddress(int, int*);
    void findMinCounter(int*, int*, int, int);
    void initializePageTable(int rangeLoop, int tableIndex );
};
#endif //EX4_SIM_MEM_H
