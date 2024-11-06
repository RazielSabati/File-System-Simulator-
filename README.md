# Memory Simulator

**Authored by:** Raziel Sabati  

## Description

The program simulates processor approaches to memory (RAM) using the "paging" mechanism, which allows running a program when only a portion of it is in memory. The virtual memory is divided into pages, and these pages are brought to the main memory (RAM) as needed. 

In this simulation, we use one process as the virtual memory. The simulation will be implemented using two main approaches:
- Loading an address into the main memory.
- Storing an address in the main memory by using the hard disk.

### Program Database:
The database structure contains several sub-databases:

1. **page_table** - An array of structs. The page table serves as a table of contents, providing information about the RAM, address, and swap file.
2. **swap_fd** - A file descriptor that holds access to the swap file, simulating the hard disk.
3. **program_fd** - A file descriptor that holds access to the executable file, simulating a process.
4. **main_memory** - An array of 200 characters, simulating the RAM (Random Access Memory).
5. Two linked lists containing the free indexes in the main memory and the swap file.

### Functions:
There are two main functions in the simulation:

1. **load** - This method receives an address and a database, ensuring that the requested address is loaded into the main memory.
2. **store** - Similar to the "load" function, this method receives an address, a database, and a value. It allocates memory (like `malloc`) and stores the value in the RAM.
3. **convertAddress** - A function that receives a base-10 number representing an address, converts it to a 12-bit binary representation, and divides it based on the number of bits. The first 2 bits represent the index of the external table, and the remaining bits represent the index in the internal table.
4. **cleanRam** - A function that cleans the RAM when it is full. It identifies which page to clean and moves it to the appropriate place, either the swap file or deletes it if it already exists in the text file.
5. **ramInsertGeneral** - This function inserts the correct page into the main memory. It returns -1 if it fails and handles the data according to the page's index array.

## Program Files
- **mem_sim.c** - Contains the functions of the simulation.
- **mem_sim.h** - A header file containing structs and function declarations.
- **main.c** - Contains the main function only.
- **Makefile** - A Makefile to compile the program.

## How to Compile
To compile the program, run:

```bash
g++ main.cpp sim_mem.cpp -o ex4
```

## To run the program, use

```bash
./ex4
```
