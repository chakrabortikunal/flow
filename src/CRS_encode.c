/* Examples/encoder.c
 * Catherine D. Schuman, James S. Plank

Jerasure - A C/C++ Library for a Variety of Reed-Solomon and RAID-6 Erasure Coding Techniques
Copright (C) 2007 James S. Plank

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

James S. Plank
Department of Electrical Engineering and Computer Science
University of Tennessee
Knoxville, TN 37996
plank@cs.utk.edu
*/

/*
 * $Revision: 1.2 $
 * $Date: 2008/08/19 17:53:34 $
 */

/* 

This program takes as input an inputfile, k, m, a coding 
technique, w, and packetsize.  It creates k+m files from 
the original file so that k of these files are parts of 
the original file and m of the files are encoded based on 
the given coding technique. The format of the created files 
is the file name with "_k#" or "_m#" and then the extension.  
(For example, inputfile test.txt would yield file "test_k1.txt".)

 */
#include <sys/time.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include "jerasure.h"
//#include "reed_sol.h"
#include "galois.h"
#include "cauchy.h"
//#include "liberation.h"
#include "mp_table.h"


//extern Metadata mp_table[MAX_SIZE];

#define N 10

//enum Coding_Technique {Reed_Sol_Van, Reed_Sol_R6_Op, Cauchy_Orig, Cauchy_Good, Liberation, Blaum_Roth, Liber8tion, RDP, EVENODD, No_Coding};

//char *Methods[N] = {"reed_sol_van", "reed_sol_r6_op", "cauchy_orig", "cauchy_good", "liberation", "blaum_roth", "liber8tion", "no_coding"};

/* Global variables for signal handler */
int readins, n;
//enum Coding_Technique method;

/* Function prototypes */
//int is_prime(int w);
void ctrl_bs_handler(int dummy);


int jfread(void *ptr, int size, int nmembers, FILE *stream)
{
  int nd;
  int *li, i;
  if (stream != NULL) return fread(ptr, size, nmembers, stream);

  nd = size/sizeof(int);
  li = (int *) ptr;
  for (i = 0; i < nd; i++) li[i] = mrand48();
  return size;
}


int encode (Metadata * mp_table,int mt_it, char *argv, int ssd_no[]) {
	FILE *fp, *fp2;				// file pointers
	char *memblock;				// reading in file
	char *block;				// padding file
	int size, newsize;			// size of file and temp size 
	struct stat status;			// finding file size

	
	//enum Coding_Technique tech;		// coding technique (parameter)
	int k = 5;//dividing original file into k blocks
	int m = 3;//redundancy
	int w = 4;//word size
	int packetsize = 1024;;		// parameters
	int buffersize = w*packetsize*k*sizeof(int);				// paramter
	int i, j;						// loop control variables
	int blocksize;					// size of k+m files
	int total;
	int extra;
	
	/* Jerasure Arguments */
	char **data;				
	char **coding;
	int *matrix;
	int *bitmatrix;
	int **schedule;
	int *erasure;
	int *erased;
	
	/* Creation of file name variables */
	char temp[5];
	char *s1, *s2;
	char *fname;
	int md;
	char *curdir;
	
	/* Timing variables */
	struct timeval t1, t2, t3, t4;
	struct timezone tz;
	double tsec;
	double totalsec;
	struct timeval start, stop;

	/* Find buffersize */
	int up, down;


	signal(SIGQUIT, ctrl_bs_handler);

	/* Start timing */
	gettimeofday(&t1, &tz);
	totalsec = 0.0;
	matrix = NULL;
	bitmatrix = NULL;
	schedule = NULL;
	//int ssd_no=1;//hardcoded value to be taken as input from flow.c

	//if(argc < 2)
	//{
	//	printf("Invalid usage. Enter file name\n");
	//	exit(0);
	//}	
	
	/* Set global variable method for signal handler */
	//method = tech;

	/* Get current working directory for construction of file names */
	curdir = (char*)malloc(sizeof(char)*1000);	
	getcwd(curdir, 1000);

        if (argv[0] != '-') {

		/* Open file and error check */
		fp = fopen(argv, "rb");
		if (fp == NULL) {
			fprintf(stderr,  "Unable to open file.\n");
			exit(0);
		}
	
		/* Create Coding directory */
		i = mkdir("Coding", S_IRWXU);
		if (i == -1 && errno != EEXIST) {
			fprintf(stderr, "Unable to create Coding directory.\n");
			exit(0);
		}
	
		/* Determine original size of file */
		stat(argv, &status);	
		size = status.st_size;
        } 

	newsize = size;
	
	/* Find new size by determining next closest multiple */
	if (packetsize != 0) {
		if (size%(k*w*packetsize*sizeof(int)) != 0) {
			while (newsize%(k*w*packetsize*sizeof(int)) != 0) 
				newsize++;
		}
	}
	/*
	else {
		if (size%(k*w*sizeof(int)) != 0) {
			while (newsize%(k*w*sizeof(int)) != 0) 
				newsize++;
		}
	}
	*/

	/* check if newsize is a multiple of buffersize	*/
	/*if (buffersize != 0) {
		while (newsize%buffersize != 0) {
			newsize++;
		}
	}*/

	/* Determine size of k+m files */
	blocksize = newsize/k;

	/* Allow for buffersize and determine number of read-ins */
	if (size > buffersize && buffersize != 0) {
			readins = newsize/buffersize;
		block = (char *)malloc(sizeof(char)*buffersize);
		blocksize = buffersize/k;
	}
	else {
		readins = 1;
		buffersize = size;
		block = (char *)malloc(sizeof(char)*newsize);  // here value of newsize and buffersize will be same fir size < buffersize
	}

	/* Break inputfile name into the filename and extension */	
	s1 = (char*)malloc(sizeof(char)*(strlen(argv)+10));
	s2 = strrchr(argv, '/');
	if (s2 != NULL) {
		s2++;
		strcpy(s1, s2);
	}
	else {
		strcpy(s1, argv);
	}
	s2 = strchr(s1, '.');
	if (s2 != NULL) {
		*s2 = '\0';
	}
	fname = strchr(argv, '.');
	s2 = (char*)malloc(sizeof(char)*(strlen(argv)+5));
	if (fname != NULL) {
		strcpy(s2, fname);
	}
	else{
		*s2='\0';
	}
	printf("CRS_encode.c: s1=%s s2=%s\n",s1,s2);	
	/* Allocate for full file name */
	fname = (char*)malloc(sizeof(char)*(strlen(argv)+strlen(curdir)+30));
	sprintf(temp, "%d", k);
	md = strlen(temp);
	
	/* Allocate data and coding */
	data = (char **)malloc(sizeof(char*)*k);
	coding = (char **)malloc(sizeof(char*)*m);
	for (i = 0; i < m; i++) {
		coding[i] = (char *)malloc(sizeof(char)*blocksize);
	}

	gettimeofday(&t3, &tz);
	
	matrix = cauchy_good_general_coding_matrix(k, m, w);
	bitmatrix = jerasure_matrix_to_bitmatrix(k, m, w, matrix);
	schedule = jerasure_smart_bitmatrix_to_schedule(k, m, w, bitmatrix);
	
	#if 1
	gettimeofday(&start, &tz);	
	gettimeofday(&t4, &tz);
	tsec = 0.0;
	tsec += t4.tv_usec;
	tsec -= t3.tv_usec;
	tsec /= 1000000.0;
	tsec += t4.tv_sec;
	tsec -= t3.tv_sec;
	totalsec += tsec;
	#endif
	
	/* Read in data until finished */
	n = 1;
	total = 0;

	while (n <= readins) {
		/* Check if padding is needed, if so, add appropriate 
		   number of zeros */
		if (total < size && total+buffersize <= size) {
			total += jfread(block, sizeof(char), buffersize, fp);
		}
		else if (total < size && total+buffersize > size) {
			extra = jfread(block, sizeof(char), buffersize, fp);
			for (i = extra; i < buffersize; i++) {
				block[i] = '0';
			}
		}
		else if (total == size) {
			for (i = 0; i < buffersize; i++) {
				block[i] = '0';
			}
		}
	
			
		/* Set pointers to point to file data */
		for (i = 0; i < k; i++) {
			data[i] = block+(i*blocksize);
		}
		
		gettimeofday(&t3, &tz);

    jerasure_schedule_encode(k, m, w, schedule, data, coding, blocksize, packetsize);
		
	gettimeofday(&t4, &tz);	
	
		/* Write data and encoded data to k+m files */
		for	(i = 1; i <= k; i++) {
			if (fp == NULL) {
				bzero(data[i-1], blocksize);
 			} else {
				//sprintf(fname, "%s/Coding/%s_k%0*d%s", curdir, s1, md, i, s2);
				sprintf(fname, "/media/newSSD%d/server/%s_k%0*d%s", ssd_no[i-1], s1, md, i, s2);
				//printf("Encoder: fname=%s\n",fname);
				mp_table[mt_it].chunk[i-1].SSD_no = ssd_no[i-1];//add the destination ssd_no to the mapping table entry
				if (n == 1) {
					fp2 = fopen(fname, "wb");
				}
				else {
					fp2 = fopen(fname, "ab");
				}
				fwrite(data[i-1], sizeof(char), blocksize, fp2);
				fclose(fp2);
			}
			
		}
		for	(i = 1; i <= m; i++) {
			if (fp == NULL) {
				bzero(data[i-1], blocksize);
 			} else {
				//sprintf(fname, "%s/Coding/%s_m%0*d%s", curdir, s1, md, i, s2);
				sprintf(fname, "/media/newSSD%d/server/%s_m%0*d%s", ssd_no[k+i-1], s1, md, i, s2);
				mp_table[mt_it].chunk[k+i-1].SSD_no = ssd_no[k+i-1];//add the destination ssd_no to the mapping table entry
				//printf("Encoder: fname=%s\n",fname);
				if (n == 1) {
					fp2 = fopen(fname, "wb");
				}
				else {
					fp2 = fopen(fname, "ab");
				}
				fwrite(coding[i-1], sizeof(char), blocksize, fp2);
				fclose(fp2);
			}
		}
		n++;
		#if 1
		/* Calculate encoding time */
		tsec = 0.0;
		tsec += t4.tv_usec;
		tsec -= t3.tv_usec;
		tsec /= 1000000.0;
		tsec += t4.tv_sec;
		tsec -= t3.tv_sec;
		totalsec += tsec;
		#endif
	}

	/* Create metadata file */
        if (fp != NULL) {
		sprintf(fname, "%s/Coding/%s_meta.txt", curdir, s1);
		fp2 = fopen(fname, "wb");
		fprintf(fp2, "%s\n", argv);
		fprintf(fp2, "%d\n", size);
		//fprintf(fp2, "%d %d %d %d %d\n", k, m, w, packetsize, buffersize);
		//fprintf(fp2, "%s\n", "cauchy_good");
		//fprintf(fp2, "%d\n", 3);
		//fprintf(fp2, "%d\n", readins);
		fclose(fp2);
	}

	/* Free allocated memory */
	free(s2);
	free(s1);
	free(fname);
	free(block);
	free(curdir);
	
	#if 1
	/* Calculate rate in MB/sec and print */
	gettimeofday(&t2, &tz);
	tsec = 0.0;
	tsec += t2.tv_usec;
	tsec -= t1.tv_usec;
	tsec /= 1000000.0;
	tsec += t2.tv_sec;
	tsec -= t1.tv_sec;
	printf("Encoding (MB/sec): %0.10f\n", (size/1024/1024)/totalsec);
	printf("En_Total (MB/sec): %0.10f\n", (size/1024/1024)/tsec);
	#endif
	printf("Encoding done\n");
}

/*
    //is_prime returns 1 if number if prime, 0 if not prime
int is_prime(int w) {
	int prime55[] = {2,3,5,7,11,13,17,19,23,29,31,37,41,43,47,53,59,61,67,71,
	    73,79,83,89,97,101,103,107,109,113,127,131,137,139,149,151,157,163,167,173,179,
		    181,191,193,197,199,211,223,227,229,233,239,241,251,257};
	int i;
	for (i = 0; i < 55; i++) {
		if (w%prime55[i] == 0) {
			if (w == prime55[i]) return 1;
			else { return 0; }
		}
	}
}
*/


/* Handles ctrl-\ event */
void ctrl_bs_handler(int dummy) {
	time_t mytime;
	mytime = time(0);
	fprintf(stderr, "\n%s\n", ctime(&mytime));
	fprintf(stderr, "You just typed ctrl-\\ in encoder.c.\n");
	fprintf(stderr, "Total number of read ins = %d\n", readins);
	fprintf(stderr, "Current read in: %d\n", n);
	fprintf(stderr, "Method: %s\n\n", "cauchy_good");	
	signal(SIGQUIT, ctrl_bs_handler);
}
