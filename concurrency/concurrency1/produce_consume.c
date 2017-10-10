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

struct buffer_arr{
	struct buffer_entry entries[BUFFSIZE]; //size specified in assignment
	int size; // current end of buffer struct array
	pthread_mutex_t stop; //locks buffer
};

struct buffer_arr buffer;

/* Name: push
 * Description: Pushes a buffer_entry struct onto the buffer_arr stack
 * Use: To add to buffer
 * 
 *
 */
void push(struct buffer_arr *b, struct buffer_entry e)
{
	if (b->size < BUFFSIZE){ // make sure that buffer won't overflow
		b->entries[b->size] = e; // place element in next open array spot
		b->size++; // iterate forward by 1
	}
}

/* Name: Pop
 * Description: Returns top of stack
 * Use: Get top of stack
 *
 *
 */
struct buffer_entry pop(struct buffer_arr *b)
{
	if (b->size > 0){
		return b->entries[b->size--];
	}
    else
        return b->entries[b->size];
}

/* Name: check_buffer
 * Description: Checks buffer entries, returns 1 if empty, 2 if full, 0 if neither
 * Use: Conditional to block consumers if empty and block producers if full
 * Prerequisite: buffer must be defined
 * Parameters: N/A
 */
int check_buffer()
{
	int x, temp, empty = 0;
	for(x = 0; x < 32; x++){
		if(buffer.entries[x].num == 0)
			empty++;
	}

	if(empty == 32)
		temp = 1;
	else if(empty == 0)
		temp = 2;
	else
		temp = 0;

	return temp;
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

void *produce(void *buffer)
{
    printf("produce called\n");
    int wait;
    struct buffer_arr *b = (struct buffer_arr *)buffer;
    while (1) {
        printf("produce while loop entered\n");
        struct buffer_entry e;
        wait = random_num(3, 7);
        printf("produce wait = %d\n", wait);
        sleep(wait);
        
        e.work_time = random_num(2, 9);
        e.num = random_num(0, 10);
        
        pthread_mutex_lock(&b->stop);
        int test = b->size;
        pthread_mutex_unlock(&b->stop);
        
        printf("b->size = %d\n", test);
        
        if (b->size < BUFFSIZE) {
            pthread_mutex_lock(&b->stop);
            push(b, e);
            pthread_mutex_unlock(&b->stop);
            printf("number: %d, wait time: %d\n", e.num, wait);
        }
    }
    return NULL;
}

void *consume(void *buffer)
{
    printf("consume called\n");
    struct buffer_arr *b = (struct buffer_arr *)buffer;
    printf("consume b->size = %d\n", b->size);
    while (1){
        if (b->size > 0){
            pthread_mutex_lock(&b->stop);
            struct buffer_entry e = pop(b);
            pthread_mutex_unlock(&b->stop);
            sleep(e.work_time);
            printf("waited: %d seconds, consumed the number: "
                   "%d\n", e.work_time, e.num);
        }
    }
    return NULL;
}

int main()
{
	struct buffer_arr buffer;
    buffer.size = 0;

    pthread_t p;
	pthread_t c;

	pthread_mutex_init(&buffer.stop, NULL);
    
    printf("start\n");

	pthread_create(&p, NULL, produce, &buffer);
	pthread_create(&c, NULL, consume, &buffer);

	//int test;
	//test = check_buffer();
	return 0;
}
