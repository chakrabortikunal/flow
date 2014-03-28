#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

#define MAX_POSSIBLE_ENTRIES 100
#define MD5_LENGTH 16

typedef struct node{
	char str[MD5_LENGTH];
	int val;
	struct node * link;

}Node;

int hashmap_init();
void hashmap_add_md5( char * str, int value );
int hashmap_find_md5( char * in_str, short del);
void hashmap_add_filename(char * str, int value);
int hashmap_find_filename(char * in_str, short del); 


