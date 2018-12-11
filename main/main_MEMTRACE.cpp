#include "mbed.h"
#include "mbed_mem_trace.h"

DigitalOut led1(LED1);

void print_memory_info() {
    // allocate enough room for every thread's stack statistics
    int cnt = osThreadGetCount();
    mbed_stats_stack_t *stats = (mbed_stats_stack_t*) malloc(cnt * sizeof(mbed_stats_stack_t));

    cnt = mbed_stats_stack_get_each(stats, cnt);
    for (int i = 0; i < cnt; i++) {
        printf("Thread: 0x%lX, Stack size: %lu / %lu\r\n", stats[i].thread_id, stats[i].max_size, stats[i].reserved_size);
    }
    free(stats);

    // Grab the heap statistics
    mbed_stats_heap_t heap_stats;
    mbed_stats_heap_get(&heap_stats);
    printf("Heap size: %lu / %lu bytes\r\n", heap_stats.current_size, heap_stats.reserved_size);
}

// main() runs in its own thread in the OS
int main() {
	//view memory regions
	while(1){
		print_memory_info();

		// Rest of the program
		led1 = !led1;
		wait(0.5);
	}

	//debug heap allocation error

	print_memory_info();

	size_t malloc_size = 16 * 1024;
	while (true) {
	    wait(0.2);
	    // fill up the memory
	    void *ptr = malloc(malloc_size);
	    printf("Allocated %d bytes (success %d)\n", malloc_size, ptr != NULL);
	    if (ptr == NULL) {
	        malloc_size /= 2;
	        print_memory_info();
	    }
	    // and then allocate an object on the heap
	    DigitalOut* led = new DigitalOut(LED1);
	}


	//debug stack overflow error

	uint8_t big_arr[1024];
	for (size_t ix = 0; ix < sizeof(big_arr); ix++) big_arr[ix] = ix; // fill the memory
	print_memory_info();

	while (true) {
	    wait(1.0);
	    // allocate an array that does not fit on the stack
	    char another_array[2048 + (rand() % 1024)];
	    for (size_t ix = 0; ix < sizeof(another_array); ix++) another_array[ix] = ix;

	    // some random operations on the arrays to prevent them from being optimized away
	    another_array[rand() % sizeof(another_array)] = big_arr[rand() % sizeof(big_arr)];
	    printf("random number is %d\n", another_array[rand() % sizeof(another_array)]);
	}


}
