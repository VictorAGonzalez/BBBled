/*
 ============================================================================
 Name        : MCodeHW.c
 Author      : Alejandro Dominguez
 Version     : 1
 Copyright   : 
 Description : BBB LED Morse Code Driver
 ============================================================================
 */
// UserTest.c -w 'Hello'
// UserTest.c -s 'Hello World'
// 	0      1	2

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>

#define BUFFER_LENGTH   256
#define DEVICE_LOCATION /dev/DriverMorse

char 	    **MorseString;
static char receive[BUFFER_LENGTH];
static char send[BUFFER_LENGTH];

extern inline char *mcodestring(int asciicode);

int main(int argc, char **argv){
	if(argc < 3)
		printf("[UserTest]: Too few calling arguments.");
		
	else if(argv[1][0] == '-'){
		if((argv[1][1] == 'w') || (argv[1][1] == 's')){
			for(i = 0; i != NULL; i++){
				if(argv[2][i] != NULL)
					MorseString[i] = mcodestring((int)argv[2][i]);
				else i = NULL;
			}
			
			int open_flag, write_flag;
			open_flag = open(DEVICE_LOCAION, O_RDWR);
			if(open_flag < 0){
				int i = 0;
				
				do{
					if(MorseString[i] == NULL)
						i=NULL;
					else{
						write_flag = write(open_flag, MorseString[i], strlen(MorseString[i]));
						if(write_flag < 0)
							printf("[UserTest]: Writing to the device failed.");
						else printf("[UserTest]: Writing to the device succesful.");
						i++;
					}
				}while(i!=NULL)
				
				
			}
			else printf("[UserTest]: Failed to open device.");
		}
	}
	
	else printf("[UserTest]: Calling parameters not recognized");
	
	return 0;
}


#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>

int main(int argc, char **argv){
	
	if(argc < 2)
		printf("[UserTest]: Too few calling arguments.");
		
	else if(argv[1][0] == '-'){
		if(argv[1][1] == 'a'){
	
	char option;
	option = argv[1][1];
	
	switch(option){
		case 'a':
			argv[2][] != '\0'
			
			%.2%f
			
			
			break;
	}
	return 0;
}