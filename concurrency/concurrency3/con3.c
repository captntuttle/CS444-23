/** Made by Kristen Patterson & Kenny Steinfeldt
 * Date: 11/26/2017
 * File: con3.c
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>

pthread_mutex_t search_lock;
pthread_mutex_t insert_lock;
pthread_mutex_t delete_lock;

struct linked_list list;
int searchers, inserters, deleters;

int linked_list_size();
void *search();
void *insert();
void *delete();

//linked list used for this assignment
struct linked_list {
	struct linked_list *next;
	int val;
} *head;

//checks for size of linked list
int linked_list_size() {
	struct linked_list *size;
	size = head;
	int i = 0;
	while (size != NULL) {
		size = size->next;
		i++;
	}

	return i;
}

//inserts an item in the list following the assignment constraints
void *insert() {
	int rand_num;
	struct linked_list *insert_list, **end;

	while (1) {
		if (linked_list_size() < 25) {
			if (!pthread_mutex_trylock(&insert_lock)) {

				rand_num = rand() % 10;
				insert_list = (struct linked_list *)malloc(sizeof(struct linked_list));

				printf("\n New item inserted to list:\n %d\n", rand_num);
				insert_list->val = rand_num;
				insert_list->next = NULL;
				end = &head;

				if (head == NULL) {
					head == insert_list;
				}
				else {
					while (*end != NULL) {
						end = &((*end)->next);
					}

					*end = insert_list;
				}

				pthread_mutex_unlock(&insert_lock);
				sleep(3);
			}
		}
	}
}

//deletes an item in the list following the assignment constraints
void *delete() {
	int delete_node;
	struct linked_list *delete_list, *prev;

	while (1) {

		if (linked_list_size() > 1) {

			if (!pthread_mutex_trylock(&insert_lock)) {
				if (!pthread_mutex_trylock(&search_lock)) {

					delete_list = head;
					delete_node = rand() % 10;

					while (delete_list != NULL) {
						if (delete_list->val == delete_node) {
							printf("\n Value deleted from list:\n %d\n", delete_node);

							if (delete_list == head) {
								head = delete_list->next;
								free(delete_list);
								break;
							}
							else {
								prev->next = delete_list->next;
								free(delete_list);
								break;
							}
						}
						else {
							prev = delete_list;
							delete_list = delete_list->next;
						}
					}
					pthread_mutex_unlock(&search_lock);
				}
				pthread_mutex_unlock(&insert_lock);
			}
			sleep(3);
		}
	}
}

//searches through the list following the assignment constraints
void *search() {
	struct linked_list *search_list;

	while (1) {
		if (!pthread_mutex_trylock(&search_lock)) {
			search_list = head;

			if (search_list == NULL) {
				printf("\n List is empty\n");
				continue;
			
			}
			else {
				printf("\n Searching through the list: \n");
				while (search_list != NULL) {
					printf("%d ", search_list->val);
					search_list = search_list->next;
				}
				printf("\n");
			}
			pthread_mutex_unlock(&search_lock);
		}
		sleep(3);
	}
}

int main(int argc, char const *argv[]) {

	time_t t;
	srand((unsigned) time(&t));

	pthread_t searching[3], inserting[3], deleting[3];

	struct linked_list *main;
	main = (struct linked_list *)malloc(sizeof(struct linked_list));
	main->val = rand() % 10;
	head = main;
	head->next = NULL;

	int i;
	for (i = 0; i < 3; i++) {
		pthread_create(&searching[i], NULL, search, NULL);
		pthread_create(&inserting[i], NULL, insert, NULL);
		pthread_create(&deleting[i], NULL, delete, NULL);
	}

	for (i = 0; i < 3; i++) {
		pthread_join(searching[i], NULL);
		pthread_join(inserting[i], NULL);
		pthread_join(deleting[i], NULL);
	}

	return 0;
}
