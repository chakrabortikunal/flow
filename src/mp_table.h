#ifndef __mp_table.H__
#define __mp_table.H__

#include <stdio.h>
#include <unistd.h>

#include <string.h>
#include <sys/time.h>
#include <openssl/md5.h>
#include <stdbool.h>

#include "myhash_latest.h"

#define MAX_SIZE 1000
#define NUM_OF_CHUNKS 8
#define FILE_NAME_SIZE 25
#define TOTAL_FILE_ENTRIES_SUPPORTED 5
#define MAX_READ_LENGTH 1410

typedef struct output_chunk{

	short SSD_no;
	//char chunk_name[];

}op_chunk;


typedef struct file_metadata{
	bool dup_data;
	char filename[FILE_NAME_SIZE];
	unsigned char md5[MD5_DIGEST_LENGTH]; //MD5_DIGEST_LENGTH=16 defined in header md5.h
	op_chunk chunk[NUM_OF_CHUNKS];
	char file_text_created[FILE_NAME_SIZE];
	int *count; // to prevent deletion
	char *orig_filename;//for dedup save name of orig file chunks to decode
	
}Metadata;
;
#endif
