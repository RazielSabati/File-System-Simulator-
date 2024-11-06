#include "sim_mem.h"

int indexes[3] = {0, 0, 0};
LinkedList MainMemoryList;
LinkedList swapList;

sim_mem::sim_mem(char exe_file_name[], char swap_file_name[], int text_size, int data_size, int bss_size,
                 int heap_stack_size, int page_size) {
    program_fd = open(exe_file_name, O_RDONLY);
    if (program_fd == -1)// File could not be opened
    {
        perror("ERR");
        exit(1);
    }
    swapfile_fd = open(swap_file_name, O_RDWR | O_CREAT | O_TRUNC,0666);
    if (swapfile_fd == -1)// File could not be opened
    {
        close(program_fd);
        perror("ERR");
        exit(1);
    }
    this->text_size = text_size;
    this->data_size = data_size;
    this->bss_size = bss_size;
    this->heap_stack_size = heap_stack_size;
    this->page_size = page_size;
    this->number_offset_bits = (int) log2(page_size);
    this->clock = 0;
    swapSize = data_size + bss_size + heap_stack_size;

    //Building the page table with 2 levels according to the entered information
    page_table = (page_descriptor **) malloc(EXTERNAL_TABLE_LENGTH * sizeof(page_descriptor * ));
    if (page_table == nullptr) {
        close(program_fd);
        close(swapfile_fd);
        printf("ERR");
        exit(1);
    }
    page_table[0] = (page_descriptor *) malloc((text_size / page_size) * sizeof(page_descriptor));
    page_table[1] = (page_descriptor *) malloc((data_size / page_size) * sizeof(page_descriptor));
    page_table[2] = (page_descriptor *) malloc((bss_size / page_size) * sizeof(page_descriptor));
    page_table[3] = (page_descriptor *) malloc((heap_stack_size / page_size) * sizeof(page_descriptor));

    ////// check malloc



    //Initialize all pages in the table to default using an external function
    initializePageTable(text_size / page_size, 0);
    initializePageTable(data_size / page_size, 1);
    initializePageTable(bss_size / page_size, 2);
    initializePageTable(heap_stack_size / page_size, 3);


    //Initialize all main memory with zeroes
    for (char &i : main_memory)
        i = '0';

    //Initializing the list of free frame indexes in main memory
    int k = 0;
    for (int i = 0; i < MEMORY_SIZE;) {
        MainMemoryList.addNodeToEnd(k);
        i += page_size;
        k++;
    }

    //Initializing the list of free frame indexes in swapFile
    k = 0;
    for (int i = 0; i < swapSize;) {
        swapList.addNodeToEnd(k);
        i += page_size;
        k++;
    }

    //Initialize all swapFile with zeroes
    char buffer[1] = {'0'};
    for (int i = 0; i < swapSize; ++i) {
        write(swapfile_fd, buffer, 1);
    }
}

//A function that initializes an internal page table to the initial values
void sim_mem::initializePageTable(int rangeLoop, int tableIndex) {
    for (int i = 0; i < rangeLoop; i++) {
        page_table[tableIndex][i].valid = false;
        page_table[tableIndex][i].frame = -1;
        page_table[tableIndex][i].dirty = false;
        page_table[tableIndex][i].swap_index = -1;
        page_table[tableIndex][i].counter = -1;
    }
}


void sim_mem::store(int address, char value) {
    //case when the address illegal
    if (address > 4095 || address < 0 || convertAddress(address, indexes) == -1)
        return;

    // case when we try to write to the text code
    if (indexes[0] == 0) {
        printf("ERR\n");
        return;
    }
    //case when we need to do "malloc", stack/heap when it's the first time we store to them
    if ((indexes[0] == 3) && !page_table[indexes[0]][indexes[1]].dirty) {
        if (MainMemoryList.isEmpty())
            cleanRam();

        int emptyFrameIndex = MainMemoryList.popNode();

        for (int i = emptyFrameIndex * page_size; i < emptyFrameIndex * page_size + page_size; ++i)
            main_memory[i] = '0';

        page_table[indexes[0]][indexes[1]].frame = emptyFrameIndex;
        page_table[indexes[0]][indexes[1]].dirty = true;
        page_table[indexes[0]][indexes[1]].valid = true;
        page_table[indexes[0]][indexes[1]].counter = clock;
        clock++;

        // insert 0 on this page in the RAM when we get empty place and update the page table
        main_memory[(page_table[indexes[0]][indexes[1]].frame * page_size) + indexes[2]] = value;
        return;
    }

    //case when this page not in the main memory
    if (!page_table[indexes[0]][indexes[1]].valid)
        ramInsertGeneral();

    else {
        page_table[indexes[0]][indexes[1]].counter = clock;
        clock += 1;
    }
    page_table[indexes[0]][indexes[1]].dirty = true;
    main_memory[(page_table[indexes[0]][indexes[1]].frame * page_size) + indexes[2]] = value;
}

char sim_mem::load(int address) {
    //case when the address illegal
    if (address > 4095 || address < 0 || convertAddress(address, indexes) == -1)
        return '\0';

    //A case where the page is in main memory
    //And we will immediately return the desired index according to the address
    if (page_table[indexes[0]][indexes[1]].valid == 1) {
        page_table[indexes[0]][indexes[1]].counter = clock;
        clock++;
        return main_memory[(page_table[indexes[0]][indexes[1]].frame * page_size) + indexes[2]];
    } else {
        //case when we try to load heap/stack pages, but we didn't make the command store on that page
        //so this function will return -1 ,and we will return \0
        //Otherwise the function will insert the right page into
        if (ramInsertGeneral() == -1) {
            printf("ERR\n");
            return '\0';
        }
    }
    return main_memory[(page_table[indexes[0]][indexes[1]].frame * page_size) + indexes[2]];
}

//A function that receives a number in base 10 representing an address
//Converts it to a 12-bit binary representation
//Divides the address according to the numbers of bits.
//The first 2 bit from the left represent an index of the external table
//Then the bit numbers to the right of the offset will be a base 2 log of the page size
//And what remains are the bits that will represent the index in the internal table
int sim_mem::convertAddress(int addressC, int *updateAddress) {
    bitset < 12 > binary(addressC);
    string binaryAddress = binary.to_string();
    string externalIndex = binaryAddress.substr(0, 2);
    string internalIndex = binaryAddress.substr(2, (10 - number_offset_bits));
    string offset = binaryAddress.substr(12 - number_offset_bits, number_offset_bits);
    updateAddress[0] = stoi(externalIndex, nullptr, 2);
    updateAddress[1] = stoi(internalIndex, nullptr, 2);
    updateAddress[2] = stoi(offset, nullptr, 2);
    if ((updateAddress[0] == 0 && updateAddress[1] >= text_size / page_size) ||
        (updateAddress[0] == 1 && updateAddress[1] >= data_size / page_size) ||
        (updateAddress[0] == 2 && updateAddress[1] >= bss_size / page_size) ||
        (updateAddress[0] == 3 && updateAddress[1] >= heap_stack_size / page_size)) {
        printf("ERR\n");
        return -1;
    }
    return 1;
}

//function that enters main memory the correct page returns -1 if it failed.
//Knows how to enter the information according to the page data found in the indexes array
int sim_mem::ramInsertGeneral() {
    //case when we try to load heap/stack pages, but we didn't make the command store on that page
    if ((indexes[0] == 3) && !page_table[indexes[0]][indexes[1]].dirty)
        return -1;

    // case when we need to bring the correct page into the RAM - main memory
    // and there is no empty places in the RAM
    if (MainMemoryList.isEmpty())
        //function to clean places in the ram
        cleanRam();

    int emptyFrameIndex = MainMemoryList.popNode();

    //case when this address represent character from the text
    if (indexes[0] == 0) {
        copyFrom_F_To_R(emptyFrameIndex, program_fd, indexes[0], indexes[1]);
    }
        //case to bring the correct page in type DATA/BSS into the RAM and its isn't dirty yet -> from the text
    else if ((indexes[0] == 1 || indexes[0] == 2) && !(page_table[indexes[0]][indexes[1]].dirty)) {
        copyFrom_F_To_R(emptyFrameIndex, program_fd, indexes[0], indexes[1]);
    }
        //case when we try to load data/bss/heap/stack pages, and we make the command store on that page before
    else if (indexes[0] != 0 && page_table[indexes[0]][indexes[1]].dirty) {
        //**** code to insert from the swap to the ram*******
        off_t file_offset = page_table[indexes[0]][indexes[1]].swap_index * page_size;  // Starting index in the swapFile
        off_t memory_offset = emptyFrameIndex * page_size;  // Starting index in the main_memory array
        ssize_t bytes_to_read = page_size;  // Number of characters to copy

        ssize_t bytes_read = pread(swapfile_fd, main_memory + memory_offset, bytes_to_read, file_offset);
        if (bytes_read == -1)
            exit(1);

        // *** Inserting 0 inside the frame from which we copied in the swapFile ***
        char str[page_size];
        for (int i = 0; i < page_size; ++i)
            str[i] = '0';
        ssize_t bytesWritten = pwrite(swapfile_fd, str, page_size, file_offset);
        if (bytesWritten == -1)
            exit(1);
        // ***
        // Updating the list of free frames in the swapFile
        swapList.addNodeToFirst(page_table[indexes[0]][indexes[1]].swap_index);
        //update all the data of this page un the pageTable
        page_table[indexes[0]][indexes[1]].frame = emptyFrameIndex;
        page_table[indexes[0]][indexes[1]].valid = true;
        page_table[indexes[0]][indexes[1]].swap_index = -1;
        page_table[indexes[0]][indexes[1]].counter = clock;
        clock++;
    }
    // if we get here we do the ram insertion successfully
    return 1;
}

//A function whose job is to go through an internal page table
//And finally put in the array the indexes of the page with the lowest clock
void sim_mem::findMinCounter(int *minClockArray, int *minClockCounter, int rangeLoop, int tableIndex) {
    for (int i = 0; i < rangeLoop; i++) {
        if (page_table[tableIndex][i].valid) {
            if (page_table[tableIndex][i].counter < *minClockCounter) {
                minClockArray[0] = tableIndex;
                minClockArray[1] = i;
                *minClockCounter = page_table[tableIndex][i].counter;
            }
        }
    }
}

//A function whose job is to clean the RAM in case it is full
//Identify which page to clean and move it to the
// appropriate place - swap or delete because it already exists in the text file
void sim_mem::cleanRam() {
    int minClock[2];
    int minClockCounter = INT32_MAX;
    findMinCounter(minClock, &minClockCounter, text_size / page_size, 0);
    findMinCounter(minClock, &minClockCounter, data_size / page_size, 1);
    findMinCounter(minClock, &minClockCounter, bss_size / page_size, 2);
    findMinCounter(minClock, &minClockCounter, heap_stack_size / page_size, 3);

    //the data of the minimal counter page
    int externalTable = minClock[0];
    int internalTable = minClock[1];

    // case when it's not text file
    if (externalTable != 0) {
        // case when it's not text and the user already store to this address
        // we need to save it in swap
        if (page_table[externalTable][internalTable].dirty) {
            //******* insert swap********
            //get the first empty index in the swap
            int swapIndex = swapList.popNode() * page_size;

            off_t memory_offset = page_table[externalTable][internalTable].frame *page_size;  // Starting index in the main_memory array
            ssize_t bytesWritten = pwrite(swapfile_fd, main_memory + memory_offset, page_size, swapIndex);
            if (bytesWritten == -1)
                exit(1);

            page_table[externalTable][internalTable].swap_index = swapIndex / page_size;
        }
    }
    // if its text file or bss/data that we didn't store them we automatically arrived to this lines
    page_table[externalTable][internalTable].valid = false;
    MainMemoryList.addNodeToFirst(page_table[externalTable][internalTable].frame);
    page_table[externalTable][internalTable].frame = -1;
}

//function whose job is to insert a page in a text file into main memory
void sim_mem::copyFrom_F_To_R(int emptyFrameIndex, int fileD, int external, int internal) {
    //Calculation of the offset that needs to be done within the file to reach the correct location for copying
    int skip = 0;
    if (external == 1)
        skip = text_size;
    else if (external == 2)
        skip = text_size + data_size;

    off_t file_offset = internal * page_size + skip;  // Starting index in the file
    off_t memory_offset = emptyFrameIndex * page_size;  // Starting index in the main_memory array

    ssize_t bytes_read = pread(fileD, main_memory + memory_offset, page_size, file_offset);
    if (bytes_read == -1)
        exit(1);

    // update the data in this specific frame
    page_table[external][internal].valid = true;
    page_table[external][internal].frame = emptyFrameIndex;
    page_table[external][internal].counter = clock;
    clock++;
}




void sim_mem::print_memory() {
    int i;
    printf("\n Physical memory\n");
    for(i = 0; i < MEMORY_SIZE; i++) {
        printf("[%c]\n", main_memory[i]);
    }
}



void sim_mem::print_swap() {
    char *str = (char *) malloc(this->page_size * sizeof(char));
    int i;
    printf("\n Swap memory\n");
    lseek(swapfile_fd, 0, SEEK_SET); // go to the start of the file
    while (read(swapfile_fd, str, this->page_size) == this->page_size) {
        for (i = 0; i < page_size; i++) {
            printf("%d - [%c]\t", i, str[i]);
        }
        printf("\n");
    }
    free(str);
}







void sim_mem::print_page_table() {
    int i;
    printf("Valid\t Dirty\t Frame\t Swap index\n");
    for (i = 0; i < text_size / page_size; i++) {
        printf("[%d]\t[%d]\t[%d]\t[%d]\n",
               page_table[0][i].valid,
               page_table[0][i].dirty,
               page_table[0][i].frame,
               page_table[0][i].swap_index);
    }
    printf("Valid\t Dirty\t Frame\t Swap index\n");
    for (i = 0; i < data_size / page_size; i++) {
        printf("[%d]\t[%d]\t[%d]\t[%d]\n",
               page_table[1][i].valid,
               page_table[1][i].dirty,
               page_table[1][i].frame,
               page_table[1][i].swap_index);
    }
    printf("Valid\t Dirty\t Frame\t Swap index\n");
    for (i = 0; i < bss_size / page_size; i++) {
        printf("[%d]\t[%d]\t[%d]\t[%d]\n",
               page_table[2][i].valid,
               page_table[2][i].dirty,
               page_table[2][i].frame,
               page_table[2][i].swap_index);
    }
    printf("Valid\t Dirty\t Frame\t Swap index\n");
    for (i = 0; i < heap_stack_size / page_size; i++) {
        printf("[%d]\t[%d]\t[%d]\t[%d]\n",
               page_table[3][i].valid,
               page_table[3][i].dirty,
               page_table[3][i].frame,
               page_table[3][i].swap_index);
    }
}

//destructor of sim_mem class release the page table memory and close the files
sim_mem::~sim_mem() {
    for (int i = 0; i < 4; i++)
        free(page_table[i]);
    free(page_table);
    close(program_fd);
    close(swapfile_fd);
}

// Implementation of all the functions of the linked list class
//Creating a list, removing a node, inserting a node, destructor,empty?
LinkedList::LinkedList() {
    head = nullptr;
}

void LinkedList::addNodeToEnd(int value) {
    Node * newNode = new
            Node;
    newNode->emptyFrameIndex = value;
    newNode->next = nullptr;
    if (head == nullptr)
        head = newNode;
    else {
        Node *current = head;
        while (current->next != nullptr)
            current = current->next;
        current->next = newNode;
    }
}

void LinkedList::addNodeToFirst(int value) {
    Node * newNode = new
            Node;
    newNode->emptyFrameIndex = value;
    newNode->next = nullptr;
    if (head == nullptr) {
        head = newNode;
    } else {
        newNode->next = head;
        head = newNode;
    }
}

int LinkedList::popNode() {
    if (head == nullptr)
        return -1; // Assuming -1 is an invalid value

    int topValue = head->emptyFrameIndex;
    Node *temp = head;
    head = head->next;
    delete temp;
    return topValue;
}

bool LinkedList::isEmpty() {
    return head == nullptr;
}

LinkedList::~LinkedList() {
    // Start from the head of the linked list
    Node *current = head;
    Node *next = nullptr;

    // Traverse the linked list and delete each node
    while (current != nullptr) {
        next = current->next;
        delete current;
        current = next;
    }
}



