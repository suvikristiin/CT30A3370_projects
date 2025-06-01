#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>    
#include <sys/stat.h>   
#include <fcntl.h>      
#include <unistd.h>      
#include <pthread.h>     
#include <string.h>      
#include <sys/sysinfo.h> 


//A lot of reference and code implementation constraints was used from this repository: https://github.com/Saggarwal9/Parallel-ZIP/blob/master/pzip.c


typedef struct {
    char *data;        // Pointer to input data
    size_t start;      // Start of data segment
    size_t end;        // End of data segment
    int *counts;       // Dynamic array to store counts of the RLE
    char *chars;       // Dynamic array to store corresponding characters (RLE)
    int num_runs;      // Number of runs produced by this thread
} thread_arg_t;

/*
 Thread function that compresses a segment of the concatenated data.
 Run-Length Encoding overview: https://www.geeksforgeeks.org/run-length-encoding/
 POSIX threads: https://man7.org/linux/man-pages/man3/pthread_create.3.html
 */
void* compress_segment(void *arg) {
    //Initializing variables
    thread_arg_t *targ = (thread_arg_t*) arg;
    size_t i = targ->start;
    
    // Print thread ID and segment boundaries.
    fprintf(stderr, "Thread %lu: processing segment [%zu, %zu)\n",
            (unsigned long)pthread_self(), targ->start, targ->end);
    

    //If there is nothing to process, exit
    if (i >= targ->end) {
        targ->num_runs = 0;
        return NULL;
    }
    
    //Allocate memory for the RLE results
    int capacity = 16;
    targ->counts = malloc(capacity * sizeof(int)); //Allocates memory to store RLE results
    targ->chars = malloc(capacity * sizeof(char)); //Allocates memory to store RLE results
    targ->num_runs = 0; //From start is zero
    

    //Assume that one character appears
    int run_count = 1;

    //Current character
    char current_char = targ->data[i];
    //Loops via each chartacter. 
    for (i = targ->start + 1; i < targ->end; i++) {
        char c = targ->data[i];

        //If matches the previous, increase the count
        if (c == current_char) {
            run_count++;

            //If not stpre the previous segment
        } else {
            if (targ->num_runs >= capacity) {
                capacity *= 2;
                targ->counts = realloc(targ->counts, capacity * sizeof(int)); //Dynamically store
                targ->chars = realloc(targ->chars, capacity * sizeof(char)); //Dynamically store
            }

            //Store the run-length data, in the arrays
            targ->counts[targ->num_runs] = run_count;
            targ->chars[targ->num_runs] = current_char;
            targ->num_runs++; //New RLE entry is added
            current_char = c; //Start tracking new sequence
            run_count = 1; //Reset the run_count
        }
    }
    
    // Store the final run, if num_run will exceed the caacity, it will mean that arrays are full --> allocate more memory
    if (targ->num_runs >= capacity) {
        capacity++; //Increase the capacity
        targ->counts = realloc(targ->counts, capacity * sizeof(int)); //Expand the array
        targ->chars = realloc(targ->chars, capacity * sizeof(char)); //Expand the array
    }

    //Storing new run,
    targ->counts[targ->num_runs] = run_count; //Storing number of times character has appeared in the sequence
    targ->chars[targ->num_runs] = current_char; //Index starts at 0, ensure that new run is stored in the correct position
    targ->num_runs++; // Run is stored, increase the counter --> next run stored in the next available slot
    
    // Print thread completion with run count.
    fprintf(stderr, "Thread %lu: finished with %d runs\n",
            (unsigned long)pthread_self(), targ->num_runs);
    
    return NULL;
}

/*

Entry point for the parallel zip (pzip) program.

Memory Mapping (mmap): https://www.geeksforgeeks.org/memory-mapping/
get_nprocs(): https://man7.org/linux/man-pages/man3/get_nprocs.3.html
pthread_join(): https://man7.org/linux/man-pages/man3/pthread_join.3.html
fstat(): https://pubs.opengroup.org/onlinepubs/009696699/functions/fstat.html
munmap(): https://pubs.opengroup.org/onlinepubs/000095399/functions/munmap.html

 */

int main(int argc, char *argv[]) {

    //Check command-line for enough arguments (files are needed)
    if (argc < 2) {
        fprintf(stderr, "pzip: file1 [file2 ...]\n");
        exit(1);
    }
    // Compute total size for all input files.
    size_t total_size = 0; //Hold all file data here, from single or multiple files

    //Iterate
    for (int i = 1; i < argc; i++) {
        //Opens the file, with O_RDONLY
        int fd = open(argv[i], O_RDONLY);

        //Check if oppening is possible
        if (fd < 0) {
            perror("pzip: cannot open file");
            exit(1);
        }
        struct stat sb;
        if (fstat(fd, &sb) < 0) { //Fetches the file's metadata 
            perror("pzip: fstat error"); //Error
            exit(1);
        }

        //Adds the size to total_size
        total_size += sb.st_size;
        close(fd); //Closes the file
    }
    
    // Allocate a big buffer and copy all files into it.

    char *big_buffer = malloc(total_size); //Large memory block, large enough to hold all file contents
    if (!big_buffer) { //Error with malloc
        perror("pzip: malloc failed");
        exit(1);
    }
    //Load files into the previosly allocated memory block
    size_t offset = 0;
    for (int i = 1; i < argc; i++) {
        int fd = open(argv[i], O_RDONLY);
        if (fd < 0) {
            perror("pzip: cannot open file");
            exit(1);
        }
        struct stat sb;
        if (fstat(fd, &sb) < 0) {
            perror("pzip: fstat error");
            exit(1);
        }
        size_t fsize = sb.st_size;
        // Maps the file into memory using nmap()
        //PROT_READ --> Allows reading
        //MAP_PRIVATE --> Changes are not visible to other processes
        char *filedata = mmap(NULL, fsize, PROT_READ, MAP_PRIVATE, fd, 0);
        if (filedata == MAP_FAILED) {
            //Error
            perror("pzip: mmap failed");
            exit(1);
        }
        // Copy file contents into big_buffer.
        memcpy(big_buffer + offset, filedata, fsize); //Copying
        offset += fsize;

        munmap(filedata, fsize); //Unamps the file, after copying is done
        close(fd);
    }
    
    //Determine number of threads based on available processors.
    int num_threads = get_nprocs();
    if (num_threads > total_size)
        num_threads = total_size; // Do not create more threads than bytes.
    
    // Create threads to process segments of big_buffer.
    pthread_t *threads = malloc(num_threads * sizeof(pthread_t)); //Allocate the memory for thread IDs
    thread_arg_t *targs = malloc(num_threads * sizeof(thread_arg_t)); //Allocate memory for thread arguments
    size_t segment_size = total_size / num_threads; //Dividing buffer into equal-sized segments, for parrallel processing
    //Creating threads
    for (int i = 0; i < num_threads; i++) {
        targs[i].data = big_buffer; //Assign data pointers
        targs[i].start = i * segment_size; //Calculate the start index for the thread
        targs[i].end = (i == num_threads - 1) ? total_size : (i + 1) * segment_size; //Calculate the end index of the thread
        targs[i].counts = NULL; //Initialize the result storage
        targs[i].chars = NULL; // Initialize the result storage
        targs[i].num_runs = 0; // Initialize the result storage
        pthread_create(&threads[i], NULL, compress_segment, &targs[i]); //Create the thread
    }
    
    // Thread synchronization with pthread_join
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL); //pthread_join blocks execution until corresponding thread completes
        //NULL means that return valuye from compress_segment is not returning (store the result in the targs[i])
    }
    
    // Allocate memory for merging.
    //This block is merging the results from RLE, from multiple threads
    int merged_capacity = 128; //Intially 128, can grow dynamically
    int merged_count = 0; //Track of number stored in runs
    int *merged_counts = malloc(merged_capacity * sizeof(int)); //Run lenghts
    char *merged_chars = malloc(merged_capacity * sizeof(char)); //Characters
    

    //Iterate over threads
    for (int i = 0; i < num_threads; i++) { //Iteration over each thread's result
        for (int j = 0; j < targs[i].num_runs; j++) { //Iteration over each RLE result inside the thread
            int count = targs[i].counts[j]; //Extract the count and characters
            char ch = targs[i].chars[j];
            // If previous run exists and characters match, combine them.
            if (merged_count > 0 && merged_chars[merged_count - 1] == ch) { //Check if last character matches ch, if true, merges the runs by adding count to the last stored run
                merged_counts[merged_count - 1] += count; //if true, merges the runs by adding count to the last stored run

                //If previous character is different, a new run is stored
            } else {
                //Check if enough memory
                if (merged_count >= merged_capacity) {
                    merged_capacity *= 2; //Expand memory
                    merged_counts = realloc(merged_counts, merged_capacity * sizeof(int));//Expand memory
                    merged_chars = realloc(merged_chars, merged_capacity * sizeof(char));//Expand memory
                }
                //New run is stored
                merged_counts[merged_count] = count;
                merged_chars[merged_count] = ch;
                merged_count++; //Track number of stored runs
            }
        }

        //Free allocated memory, which were allocated by threads
        free(targs[i].counts);
        free(targs[i].chars);
    }
    
    // Write merged runs as binary output: each run is a 4-byte integer followed by a 1-byte character.
    for (int i = 0; i < merged_count; i++) {
        fwrite(&merged_counts[i], sizeof(int), 1, stdout);
        fwrite(&merged_chars[i], sizeof(char), 1, stdout);
    }
    
    // Free all allocated memory
    free(merged_counts);
    free(merged_chars);
    free(threads);
    free(targs);
    free(big_buffer);
    
    return 0;
}
