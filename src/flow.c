#include "mp_table.h"
//#include <linux/kernel.h>


//static int mt_it=0;// iterator to map through the mapping table
Metadata * mp_table[MAX_SIZE];//the mapping table

void initializeMetadata()
{
	int i;
//	struct file_metadata mp_table[MAX_SIZE];
	for( i=0;i < MAX_SIZE;i++)
	{
		mp_table[i] = NULL;
		//mp_table[i].valid_data = false;
		//mp_table[i].count = 0;
	}
}

void ssdDelete(char * file)
{
	//Delete logic
	int key1=0;
	int i;
	Metadata * tmp;	
	char * orig_name;
	int wl_array[NUM_OF_CHUNKS];
	char* fname;
	int isUnique = 0;// to find unique SSD nos in wear level array assuming not more than 32 SSDs are used in array
	char* s1;
	char* s2;

	key1 = hashmap_find_filename(file,1);//1=find file and delete corresponding entry
	printf("ssdDelete: mt_it= %d\n",key1);
	if( key1 == -1)
	{
		printf("File %s not found\n", file);
		return ;
	}
	
	int *c = mp_table[key1]->count;
	/*Remove corresponding entry from md5 hashmap if this is unique data*/
	if(*c==1)
	{
		if(mp_table[key1]->orig_filename!=NULL)
			file=mp_table[key1]->orig_filename;
			
		/*Extract filename and extension*/
		s1= (char *)malloc(strlen(file));
		strcpy(s1,file);
		s2=strchr(s1, '.');
		if(s2 != NULL)
		{
			*s2='\0';
		}
			printf("flow.c: s1: %s\n", s1);
		s2= (char *)malloc(strlen(file));
		fname = strchr(file, '.');
		if (fname != NULL) {
			strcpy(s2, fname);
		}
		else
		{
			*s2='\0';
		}
			printf("flow.c: s2: %s\n", s2);
		fname= (char*)malloc(sizeof(char*)*(55+strlen(file)));//35= size of string mentioned in sprintf fname

		for(i=0;i<NUM_OF_CHUNKS;i++)
		{
			wl_array[i]=mp_table[key1]->chunk[i].SSD_no;
			if(isUnique & 1<<wl_array[i])
			{	//Do nothing
			}	
			else
			{
				isUnique |= 1<<wl_array[i];
				/*Delete from SSDs*/
			sprintf(fname,"rm -rf /media/newSSD%d/server/%s_*%s",wl_array[i],s1,s2);
			printf("flow.c: %s\n",fname);
			system(fname);	
			}
		}
		/*Delete metadata from destination folder*/
		sprintf(fname,"rm -rf Coding/%s_meta.txt",s1);
		printf("flow.c: %s\n",fname);
		system(fname);	
		hashmap_find_md5(mp_table[key1]->md5,1);
	free(c);
	free(s1);
	free(s2);
	free(fname);	
	}
	else
	{
		//mp_table[key1]->count--;
		(*c)--;//decrement count
			printf("flow.c delete\n");
		for(i=0;i<NUM_OF_CHUNKS;i++)
		{
			//some logic
		}		
	}	
	
	/*Deallocate memory corresponding to the file entry in mapping table*/
	orig_name = mp_table[key1]->orig_filename;
	if(orig_name != NULL)
		free(orig_name);
	tmp = mp_table[key1];
	mp_table[key1]=NULL;
	free(tmp);
	
}

void ssdRead(char * file)
{
	int key1=0;
	
	//printf("flow.c: File=%s\n",file);
	key1 = hashmap_find_filename(file,0);//0=Only find filename
	printf("ssdRead: mt_it= %d\n",key1);
	//char read_file[20];
	
	int i;
	
	if( key1 == -1)
	{
		printf("File %s not found", file);
		return ;
	}
	//strcpy(read_file,mp_table[key1].filename);// strcpy
	int wl_array[NUM_OF_CHUNKS];//={0,0,1,2,1,2,1,2};
	
	/*Populate wl array to be passed for decoding*/
	for(i=0;i<NUM_OF_CHUNKS;i++)
	{
		wl_array[i]=mp_table[key1]->chunk[i].SSD_no;
		//printf("flow.c: w=%d\n",wl_array[i]);	
	}
	
	if(mp_table[key1]->orig_filename != NULL)
		decoder(mp_table[key1]->orig_filename,file,wl_array);
	else
		decoder(file,NULL,wl_array);
}

void ssdWrite(char * file)
{

	int i;
	static int mt_it=0;
	int key1;
	char * orig_name;
        unsigned char c[MD5_DIGEST_LENGTH];
        MD5_CTX mdContext;
        int bytes;
        unsigned char data[MAX_READ_LENGTH];
	//printf("flow.c: File=%s\n",file);
	key1= hashmap_find_filename(file,0);//0= Only find filename
	if( key1 != -1)
	{
		printf("Cannot add two files with same name in the same disk!\n");
	}		
	else{
	
	 		FILE *inFile = fopen (file, "rb");
			if (inFile == NULL) {
        			printf ("%s can't be opened.\n", file);
        			//return 0;
    			}
			else{
    				printf ("File read: %s\n", file);
	
	        		MD5_Init (&mdContext);
	        		while ((bytes = fread (data, 1, MAX_READ_LENGTH, inFile)) != 0)
    				{
       	       				 MD5_Update (&mdContext, data, bytes);
    				}	
			        MD5_Final (c,&mdContext);
				fclose(inFile);
			}
	
		
		/*Find free slot in mapping table, for cases when some files are deleted*/
		while(mp_table[mt_it] != NULL)//blocking wait
		{	
			mt_it++;
			mt_it = mt_it % MAX_SIZE;
		}
		Metadata * tmpNode;
		tmpNode = (Metadata *) malloc(sizeof(Metadata) );//Allocate memory
		mp_table[mt_it]=tmpNode;
		strcpy(mp_table[mt_it]->filename,file);
		hashmap_add_filename(file,mt_it);//add filename to the filename - mp_table_index 
			//printf("\nflow.c: filehash inserted for file %s at mt_it= %d\n", file,mt_it );

		/*Put md5 entry into mapping table and then check if there is a duplicate or no */		
		for(i = 0; i < MD5_DIGEST_LENGTH; i++) {
			//printf("%02x", c[i]); //print the md5#
			mp_table[mt_it]->md5[i]=c[i];//copy md5 into structure
		}

		key1 = hashmap_find_md5(c,0);
		//printf("ssdWrite: md5Key = %d\n",key1 );	

		if( key1 != -1)//check if key is present mapping table
		{
		
		mp_table[mt_it]->dup_data = true;	
			printf("Duplicate MD5: %s found for file: %s\n",c , file );
			//duplicate data found
			//adjust pointers from the mapping table itself
			for( i = 0; i < NUM_OF_CHUNKS ; i++){
				//mp_table[mt_it].chunk[i].chunk_no = mp_table[key1].chunk[i].chunk_no;
				mp_table[mt_it]->chunk[i].SSD_no = mp_table[key1]->chunk[i].SSD_no;
			}
		
			strcpy(mp_table[mt_it]->file_text_created, mp_table[key1]->file_text_created);	
			int*cc = mp_table[key1]->count;
			(*cc)++;//increment count value for dup data
			printf("flow.c: New count= %d\n",*cc);
			//mp_table[key1]->count++;
			mp_table[mt_it]->count=mp_table[key1]->count;//Both entries must reflect same coutn and point to the same int pointer
			orig_name = (char*)malloc(sizeof(char)*FILE_NAME_SIZE);
			strcpy(orig_name,mp_table[key1]->filename);
			mp_table[mt_it]->orig_filename=orig_name;
			
		}
		else{
			mp_table[mt_it]->dup_data = false;	
			int *cc = (int *)malloc(sizeof(int)*1);
			*cc=1;
			mp_table[mt_it]->count=cc;
		//	mp_table[mt_it]->count=1;
			mp_table[mt_it]->orig_filename=NULL;
			
			/*add new(unique) md5 value to the hashtable with the mp_table iterator value*/
			hashmap_add_md5(c,mt_it);
			printf("\nMD5 inserted for file %s at mt_it= %d\n", file,mt_it );
			
			//int * wl; //wl = wearl level
			//get input from wear levelling to fill wl_array
			//wl = &wl_array[0];
			//*wl = getWearLevelingResultQueue();//result queue returns a array of SSD numbers to write to
			int wl_array[NUM_OF_CHUNKS]={1,2,1,2,1,2,1,2};
			
			for( i = 0; i < NUM_OF_CHUNKS ; i++){
				//mp_table[mt_it].chunk[i].chunk_no = mp_table[key1].chunk[i].chunk_no;
				mp_table[mt_it]->chunk[i].SSD_no = wl_array[i];
			}
			//call erasure encoding function from here
			//printf("flow.c Flag1\n");
			encode(mp_table,mt_it,file,wl_array);	
		}
		mt_it++;
	}
}



int main(int argc, char * argv[])
{
	initializeMetadata();
	//call wearleveliing;
	char file[FILE_NAME_SIZE];
	//int mt_it=0;//map_table iterator
       
	hashmap_init();
	char in_c;

	while(1){
		//printf("\nEnter r for read a file\nw for writing a file\nd for deleting a file\nEnter your choice: ");
		printf("\nEnter r / w / d: ");
		scanf("%c",&in_c);
		//fflush(stdin);
		while ( getchar() != '\n' );
		printf("\nEnter file name: ");
		scanf("%s",file);
		while ( getchar() != '\n' );

		//printf("Input selection= %c file= %s\n",in_c,file);

		switch (in_c) {
		case 'r':
			ssdRead(file);
			break;
		case 'w':
			ssdWrite(file);
			break;
		case 'd':
			ssdDelete(file);
			break;
		default:
			printf("\nInvalid selection\n");
		}
	}
	return 0;
}


