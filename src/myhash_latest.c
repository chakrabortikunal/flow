#include "myhash_latest.h"

/*map_md5 is an array of Node* pointers*/
static Node* map_md5[MAX_POSSIBLE_ENTRIES];

/*map_filename is an array of Node* pointers*/
static Node* map_filename[MAX_POSSIBLE_ENTRIES];


int hashmap_init()
{
	int j;
	for(j=0;j<MAX_POSSIBLE_ENTRIES;j++)
	{
		map_md5[j]=NULL;
		map_filename[j]=NULL;
	}
	return 1;
}

/*Function: hashmap_add_md5
	Maps md5 with index value of mp_table	
*/
void hashmap_add_md5( char * str, int value )
{
	//char c;
	//c= *str;
	unsigned int key_index=0;
	Node * tmp;
	tmp = (Node *) malloc( sizeof(Node) );  // allocate memory
	tmp->val = value; //copy value
	tmp->link = NULL;

    int i=0;
    for( i=0; i<MD5_LENGTH ;i++)
	{
	    //printf("%c",str[i]);
		tmp->str[i]=str[i];
		key_index += str[i];
	}
	Node *collision_it; //collision iterator to iterate thru the linked list

    key_index = key_index%MAX_POSSIBLE_ENTRIES;

    //printf("\nkey_index add md5= %d\n",key_index);

	if(map_md5[key_index] == NULL)
		map_md5[key_index]= tmp;
	else{
		collision_it = map_md5[key_index];

		while(collision_it->link != NULL)  //find out the last node in the linked list
			collision_it = 	collision_it->link;
		collision_it->link = tmp; // assign val to the end of the list
	}

}

/*
hashmap_find_md5:
This function returns the interger value mapped with the md5.
If no entry for this md5 is stored then -1 is retuned.
Parameter del determines if we want to remove the entry for this md5 from the mapping table. 
del = 0 -> only find and return value
del = 1 ->find and return value and remove corresponding entry

Return value is the int value mapped to this filename
*/
int hashmap_find_md5( char * in_str, short del)
{
    int i;
	unsigned int key_index=0;

	for(i=0;i < MD5_LENGTH;i++)
	{
	    	//printf("%c",in_str[i]);
		key_index += in_str[i];
	}
    key_index = key_index % MAX_POSSIBLE_ENTRIES;
    //printf("\nkey_index: find: md5 = %d\n",key_index);

    if ( map_md5[key_index] == NULL)
    {
        return -1;
    }

    Node *temp= map_md5[key_index];
    Node *prev=NULL;
    bool flag=true;
    char c1,c2;
	while(temp != NULL){
            flag=true; //reset flag for next node
        	for(i=0; i<MD5_LENGTH ; i++)
        	{
        	    c1=temp->str[i];
        	    c2=in_str[i];
        	    if( c1 != c2 )
        	    {
        	        flag=false;
        	    }
        	}
	
        	if(flag == false){
		    prev=temp;
        	    temp=temp->link;
		}
	        else{
        	    i= temp->val;  //return val corresponding to md5
		    if(del == 1)
		    {
			if(prev==NULL)
				map_md5[key_index]=NULL;
			else
				prev->link=temp->link;

			free(temp);
		    }
		    return i;
		}
	}

    return -1;
}

void hashmap_add_filename( char * str, int value )
{
	unsigned int key_index=0;
	Node * tmp;
	tmp = (Node *) malloc( sizeof(Node) );  // allocate memory
	tmp->val = value; //copy value
	tmp->link = NULL;

    	int i=0;
    	//for( i=0; i<strlen(str) ;i++)
    	for( i=0; str[i]!='\0' ;i++)
	{
		//printf("%c",str[i]);
		tmp->str[i]=str[i];
		key_index += str[i];
	}
	tmp->str[i]='\0';
	Node *collision_it; //collision iterator to iterate thru the linked list

	key_index = key_index%MAX_POSSIBLE_ENTRIES;

	//printf("\nmyhash.c: key_index add file index=%d val=%d\n",key_index,tmp->val);

	if(map_filename[key_index] == NULL)
		map_filename[key_index]= tmp;
	else{
		collision_it = map_filename[key_index];

		while(collision_it->link != NULL)  //find out the last node in the linked list
			collision_it = 	collision_it->link;
		collision_it->link = tmp; // assign val to the end of the list
	}

}

/*
hashmap_find_filename:
This function returns the interger value mapped with the filename.
If no entry for this file name is stored then -1 is retuned.
Parameter del determines if we want to remove the entry for this filename from the mapping table. 
del = 0 -> only find and return value
del = 1 ->find and return value and remove corresponding entry

Return value is the int value mapped to this filename
*/
int hashmap_find_filename( char * in_str, short del)
{
    int i;
	unsigned int key_index=0;

	//for(i=0;i < strlen(in_str);i++)
	for(i=0;in_str[i] !='\0';i++)
	{
		key_index += in_str[i];
	}
    key_index = key_index % MAX_POSSIBLE_ENTRIES;
    //printf("\nmyhash.c: key_index find file= %d\n",key_index);

    if ( map_filename[key_index] == NULL)
    {
        return -1;
    }
    Node *temp= map_filename[key_index];
    Node* prev=NULL;
    bool flag=true;
    char c1,c2;
    while(temp != NULL){

	if(strcmp(temp->str,in_str)){
		prev=temp;
		temp=temp->link;
	}
	else{
		i= temp->val;
		if(del==1)
		{
			if(prev==NULL)
				map_filename[key_index]=NULL;
			else
				prev->link=temp->link;

			free(temp);//delete node for this filename
		}
		return i;
	}
    }

    return -1;
}


