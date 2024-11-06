#include "sim_mem.h"
char main_memory[MEMORY_SIZE];
int main()
{
    char file1[]="exe_file";
    char file2[]="swap_file";
    char val;
    sim_mem mem_sm(file1, file2 ,16, 16, 32,32, 8);
    mem_sm.store(98,'X');
    val = mem_sm.load (8);
    cout << val;
    mem_sm.print_memory();
    mem_sm.print_swap();
}
