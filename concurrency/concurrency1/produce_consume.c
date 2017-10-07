/* Kristen Patterson & Kenneth Steinfelt
 * Producer and Consumer Concurrency
 * Due: 10/09/17
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "mt19937ar.c"

struct buffer_entry{
	int num; //random value, must not be 0 if "full"
	int work_time; //random time to consume the thread
};

struct buffer_arr{
	struct buffer_entry entries[32]; //size specified in assignment
	pthread_mutex_t stop; //locks buffer
};

struct buffer_arr buffer;

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

/* Name: random_num
 * Description: checks if system is compatible with rdrand and generates a number
 * Use: Produce random numbers for consumer/producer work time and entry in buffer
 * Prerequisite: None
 * Paramters: Bounds to generate a number
 */
int random_num(int min, int max)
{
	return 0;
	//Need to research inline assembly will complete later
}

int main()
{
	int test;
	test = check_buffer();
	return 0;
}
