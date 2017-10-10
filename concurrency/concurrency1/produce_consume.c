/* Kristen Patterson & Kenneth Steinfeldt
 * Producer and Consumer Concurrency
 * Due: 10/09/17
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include "mt19937ar.c"

#define BUFFSIZE 32

struct buffer_entry{
	int num; //random value, must not be 0 if "full"
	int work_time; //random time to consume the thread
};
typedef struct buffer_entry buffer_entry;

struct buffer_arr{
	struct buffer_entry entries[BUFFSIZE]; //size specified in assignment
	int size; // current end of buffer struct array
	pthread_mutex_t stop; //locks buffer
};
typedef struct buffer_arr buffer_arr;

/* Name: push
 * Description: Pushes a buffer_entry struct onto the buffer_arr stack
 * Use: To add to buffer
 * Prerequisite: initialized buffer_arr object
 * Parameters: buffer array object
 */
void push(buffer_arr *b, buffer_entry e)
{
	if (b->size < BUFFSIZE){ // make sure that buffer won't overflow
		b->entries[b->size++] = e; // place element in next open array spot
	}
    else
        printf("error, stack full");
}

/* Name: Pop
 * Description: Returns top of stack
 * Use: Get top of stack
 * Prerequisite: initialized buffer_arr object
 * Parameters: buffer array object
 */
buffer_entry pop(buffer_arr *b)
{
	if (b->size > 0){
        b->size--;
		return b->entries[b->size];
	}
    else
        printf("cannot pop. empty stack");
        return b->entries[b->size];
}

/* Name: asm_rand
 * Description: generates a random number using rdrand
 * Use: gets random number for systems capable of rdrand
 * Prerequisite: registers set and integer passed
 * Paramters: integer to store random number
 */
int asm_rand(int *tmp)
{
	//https://stackoverflow.com/questions/11407103/how-i-can-get-the-random-number-from-intels-processor-with-assembler
	unsigned char check;
	asm volatile(
		"rdrand %0 ; setc %1"
	       	: "=r"(*tmp), "=qm"(check)
	);
	return (int) check;
}

/* Name: random_num
 * Description: checks if system is compatible with rdrand and generates a number
 * Use: Produce random numbers for consumer/producer work time and entry in buffer
 * Prerequisite: None
 * Paramters: Bounds to generate a number
 */
int random_num(int min, int max)
{
	int eax, ebx, ecx, edx, tmp = 0;
	//flag for rdrand
	//https://codereview.stackexchange.com/questions/147656/checking-if-cpu-supports-rdrand
	const unsigned int flag_RDRAND = (1 << 30);

	//https://software.intel.com/en-us/forums/intel-isa-extensions/topic/381804
	eax = 0x01;

	//https://stackoverflow.com/questions/11407103/how-i-can-get-the-random-number-from-intels-processor-with-assembler
	asm volatile(
		"cpuid;"
		: "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
		: "a"(eax)
	);

	if (ecx & flag_RDRAND)
		asm_rand(&tmp);
	else
		tmp = (int)genrand_int32();

	tmp = abs(tmp);
	tmp = tmp % (max - min);
	if (tmp < min)
		tmp = min;

	return tmp;
}

/* Name: Produce
 * Description: randomly fills buffer_arr stack with buffer_entry objects
 * Parameters: buffer_arr object to be cast in function
 */
void *produce(void *buffer)
{
    int wait;
    buffer_arr *b = (buffer_arr *)buffer;
    
    while (1) {
        buffer_entry e;
        wait = random_num(3, 7);
        sleep(wait);
        
        e.work_time = random_num(2, 9);
        e.num = random_num(0, 10);
        
        pthread_mutex_lock(&b->stop);
        if (b->size < BUFFSIZE) {
            push(b, e);
            printf("produced number: %d, wait time: %d\n", e.num, wait);
        }
        pthread_mutex_unlock(&b->stop);
    }
    return NULL;
}

/* Name: Consume
 * Description: consumes buffer_entry objects out of buffer_arr stack
 * Parameters: buffer_arr object to be cast in function
 */
void *consume(void *buffer)
{
    buffer_arr *b = (buffer_arr *)buffer;
    
    while (1){
        pthread_mutex_lock(&b->stop);
        if (b->size > 0){
            buffer_entry e = pop(b);
            sleep(e.work_time);
            printf("waited: %d seconds, consumed the number: "
                   "%d\n", e.work_time, e.num);
        }
        pthread_mutex_unlock(&b->stop);
    }
    return NULL;
}

int main()
{
    printf("start\n");
    
	buffer_arr buffer;
    buffer.size = 0;

    pthread_t p;
	pthread_t c;

	pthread_mutex_init(&buffer.stop, NULL);

	pthread_create(&p, NULL, produce, &buffer);
	pthread_create(&c, NULL, consume, &buffer);
    
    pthread_join(p, NULL);
    pthread_join(c, NULL);
     
	return 0;
}
