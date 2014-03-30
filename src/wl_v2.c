/*
 * wl.c
 *
 *  Created on: Mar 21, 2014
 *      Author: Mihir
 */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#define __WL_DEBUG__            0

#define TOTAL_SSD_IN_ARRAY 	2
#define ERASURE_CODE_PARAM 	0.6
#define TEMP_THRESHOLD 	   	55
#define WEAR_DEPTH	        8

/* Shell command list to extract values from output of smartctl generated file */
#define GET_LBA_READ  "grep \^242 ssdlog.txt | awk '{ print $NF }' > values"
#define GET_LBA_WRITTEN  "grep \^241 ssdlog.txt | awk '{ print $NF }' >> values"
#define GET_SSD_TEMP  "grep \^194 ssdlog.txt | awk '{ print $(NF-2) }' >> values"
#define GET_SPACE_AVAIL "grep \^\/ ssdspace.txt | awk '{ print $(NF-3) }' >> values"


/* Structure to store SSD parameters */
typedef struct st_wear_level {
	int ssd_id;
	int ssd_valid;
	int ssd_blocks_written;
	int ssd_blocks_read;
	int ssd_total_blocks;
	int ssd_used_space;
	int ssd_free_space;
	int ssd_temperature;
}st_wear_level;

void wl_initialize_array(struct st_wear_level* p_a_wear_level);
int wl_sort_ssd(struct st_wear_level* p_a_wear_level, int* p_wl_ssd_id_queue);

int ssd_file_size;
int ssd_array_total_space_avail;

int getWearlevel(int *p_wl_ssd_id_queue )
{
	struct st_wear_level* p_a_wear_level = NULL;
	//int* p_wl_ssd_id_queue = NULL;

	p_a_wear_level = (struct st_wear_level*)malloc(TOTAL_SSD_IN_ARRAY *
						 sizeof(struct st_wear_level));
	//p_wl_ssd_id_queue = (int *)malloc(WEAR_DEPTH*sizeof(int));

	wl_initialize_array(p_a_wear_level);

	if(wl_sort_ssd(p_a_wear_level, p_wl_ssd_id_queue) < 0){
		return -1;
	}
	free(p_a_wear_level);
	//free(p_wl_ssd_id_queue);
	return 0;
}


#if __WL_DEBUG__
void wl_initialize_array(struct st_wear_level* p_a_wear_level)
{
	    p_a_wear_level[0].ssd_id = 1;
		p_a_wear_level[0].ssd_blocks_read = 96035;
		p_a_wear_level[0].ssd_blocks_written = 6426658;
		p_a_wear_level[0].ssd_temperature = 48;
		p_a_wear_level[0].ssd_free_space = 16632448;

		p_a_wear_level[1].ssd_id = 2;
		p_a_wear_level[1].ssd_blocks_read = 96035;
		p_a_wear_level[1].ssd_blocks_written = 6426658;
		p_a_wear_level[1].ssd_temperature = 48;
		p_a_wear_level[1].ssd_free_space = 16632448;

		p_a_wear_level[2].ssd_id = 3;
		p_a_wear_level[2].ssd_blocks_read = 96035;
		p_a_wear_level[2].ssd_blocks_written = 5426658;
		p_a_wear_level[2].ssd_temperature = 66;
		p_a_wear_level[2].ssd_free_space = 16632448;


		ssd_array_total_space_avail = p_a_wear_level[0].ssd_free_space + p_a_wear_level[1].ssd_free_space + p_a_wear_level[2].ssd_free_space;
}
#else

void wl_initialize_array(struct st_wear_level* p_a_wear_level)
{
	int i;
	char wl_buff1[100];
	char wl_buff2[100];
	FILE *fp;

	for(i=0; i<TOTAL_SSD_IN_ARRAY; i++)
	{
	sprintf(wl_buff1,"smartctl -A \/dev\/sd%c1 > ssdlog.txt",i+'a');
	system(wl_buff1);
	system(GET_LBA_READ);
	system(GET_LBA_WRITTEN);
	system(GET_SSD_TEMP);

	//edit here
	//sprintf(wl_buff2,"df \/dev\/sda%d > ssdspace.txt",i+1);
	sprintf(wl_buff2,"df \/dev\/sd%c1 > ssdspace.txt",i+'a');//k
	printf("df \/dev\/sd%c1 > ssdspace.txt",i+'a');//k
	system(wl_buff2);
	system(GET_SPACE_AVAIL);

	fp=fopen("values","r");
	fscanf(fp,"%d %d %d %d",
		&p_a_wear_level[i].ssd_blocks_read,
		&p_a_wear_level[i].ssd_blocks_written,
		&p_a_wear_level[i].ssd_temperature,
		&p_a_wear_level[i].ssd_free_space);

	p_a_wear_level[i].ssd_id = i+1;

	ssd_array_total_space_avail+= p_a_wear_level[i].ssd_free_space;

//	printf("LBR: %d \nLBW: %d\nTEMP: %d\nFSR: %d\n",
//		p_a_wear_level[i].ssd_blocks_read,
//		p_a_wear_level[i].ssd_blocks_written,
//		p_a_wear_level[i].ssd_temperature,
//		p_a_wear_level[i].ssd_free_space);

	fclose(fp);
	}
}

#endif

int wl_sort_ssd(struct st_wear_level* p_a_wear_level, int* p_wl_ssd_id_queue)
{
	int total_file_size 	    = 0;
	int total_space_req         = 0;
	int total_space_per_ssd_req = 0;
	int i = 0;
	int j = 0;
	int total_ssd_valid_count = 0;
	int temp = 0;
	int count = 0;
	int a_temp_ssd_id[TOTAL_SSD_IN_ARRAY];
	int a_temp_ssd_lbw[TOTAL_SSD_IN_ARRAY];

	ssd_file_size = 153402;

	/* Converting size to KB resolution, since "du" command returns size in KB */
	ssd_file_size = (ssd_file_size / 1000) + 1;

	/* Total file size of erasure coding output */
	total_file_size = ssd_file_size + ((ERASURE_CODE_PARAM) * ssd_file_size);

	total_space_req	= total_file_size;
	
	printf("totalspcereq=%d totaval=%d\n",total_space_req,ssd_array_total_space_avail) ;

	/* If SSD array free space less than erasure coded file size, return error*/
	if(total_space_req > ssd_array_total_space_avail){
		printf("Error: Insufficient space in SSD array\n");
		return -1;
	}

	/* Divide total size by number of SSD, to get data required per SSD */
	total_space_per_ssd_req = total_space_req/TOTAL_SSD_IN_ARRAY;

	/* Run for loop to find which SSDs have available space */
	/* Invalidate ones that do not have sufficient space */
	for(i=0; i<TOTAL_SSD_IN_ARRAY; i++)
	{
		if((p_a_wear_level[i].ssd_free_space <= total_space_per_ssd_req) ||
			(p_a_wear_level[i].ssd_temperature > TEMP_THRESHOLD))
		{
			p_a_wear_level[i].ssd_valid = 0;
			a_temp_ssd_id[i] = -1;
			a_temp_ssd_lbw[i] = INT_MAX;
		}
		else
		{
			p_a_wear_level[i].ssd_valid = 1;
	                a_temp_ssd_id[i] = p_a_wear_level[i].ssd_id;
			a_temp_ssd_lbw[i] = p_a_wear_level[i].ssd_blocks_written;
			total_ssd_valid_count++;
		}
	}
    // TBD	/* Taking care of external fragmentation */
	if((total_ssd_valid_count * total_space_per_ssd_req) < (total_space_req-1))
	{
		printf("Error: Request cannot be completed\n");
		return -1;
	}

	/* Bubble sort ssd_id based on logical block written values */
	for(i=0; i<TOTAL_SSD_IN_ARRAY; i++)
	{
		for(j=0; j<(TOTAL_SSD_IN_ARRAY-1); j++)
		{
			if(a_temp_ssd_lbw[j] > a_temp_ssd_lbw[j+1])
			{
				temp = a_temp_ssd_lbw[j];
				a_temp_ssd_lbw[j] = a_temp_ssd_lbw[j+1];
				a_temp_ssd_lbw[j+1] = temp;

				temp = a_temp_ssd_id[j];
				a_temp_ssd_id[j] = a_temp_ssd_id[j+1];
				a_temp_ssd_id[j+1] = temp;
			}
		}
	}

	/* Generate queue of size WEAR_DEPTH, considering only valid ssd's */
	i = 0;
	while(count < WEAR_DEPTH)
	{
		if(a_temp_ssd_id[i] != -1)
		{
			p_wl_ssd_id_queue[count] = a_temp_ssd_id[i];
			count++;
			i++;
			if(i == TOTAL_SSD_IN_ARRAY){
					i = 0;
			}
		}
		else
		{
			i=0;
		}
	}

	/* Print wear level queue list*/
	printf("Output wear leveling queue\n");
	for(i=0;i<WEAR_DEPTH;i++){
		printf("%d ",p_wl_ssd_id_queue[i]);
	}
	printf("\n");
	system("rm ssdlog.txt ssdspace.txt values");
	return 0;
}





