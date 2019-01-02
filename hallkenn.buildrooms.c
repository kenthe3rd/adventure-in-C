#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>

int isGraphFull(char* path, char* names[], int deck[]);

void addConnection(char* path, char* names[], int deck[]);

void main(){
	// i, j are general-purpose counters
	int i = 0;
	int j = 0;

	int seed = time(NULL);
	srand(seed);

	// make the directory to hold names
	char dirName[25] = "hallkenn.rooms.";
	char pidString[6];
	sprintf(pidString, "%d\0", getpid());
	strcat(dirName, pidString);
	mkdir(dirName, 0755);

	// define the 10 options for names
	char* namesArray[10];
	namesArray[0] = "kitchen";
	namesArray[1] = "study";
	namesArray[2] = "cellar";
	namesArray[3] = "bedroom";
	namesArray[4] = "attic";
	namesArray[5] = "laundry";
	namesArray[6] = "bathroom";
	namesArray[7] = "closet";
	namesArray[8] = "den";
	namesArray[9] = "hallway";
	
	// define the full filepath
	char path[255];
	getcwd(path, sizeof(path));
	strcat(path, "/");
	strcat(path, dirName);

	// build and shuffle a 'deck' of 10 room 'cards'
	// to randomize room selection
	i = 0;
	//build
	int deck[10];
	for(i; i<10; ++i){
		deck[i] = i;
	}
	//shuffle
	i = 0;
	for(i; i<7; ++i){
		int j = i + rand() % (10 - i);
		int temp = deck[i];
		deck[i] = deck[j];
		deck[j] = temp;
	}

	// create files in the dir specified above
	char newFile[255];
	i = 0;
	for(i; i<7; ++i){
		strcpy(newFile, path);
		strcat(newFile, "/");
		strcat(newFile, namesArray[deck[i]]);
		FILE* file_ptr = fopen(newFile, "w");
		fprintf(file_ptr, "ROOM NAME: %s\n", namesArray[deck[i]]);
		fclose(file_ptr);
	}

	// build the graph
	while(isGraphFull(path, namesArray, deck) == 0){
		addConnection(path, namesArray, deck);
	}

	// assign START, MID, END types
	i = 0;
	for(i; i<7; ++i){
		strcpy(newFile, path);
		strcat(newFile, "/");
		strcat(newFile, namesArray[deck[i]]);
		FILE* file_ptr = fopen(newFile, "a");
		if(i == 0){
			fprintf(file_ptr, "ROOM TYPE: START_ROOM\n");
		} else if (i == 6){
			fprintf(file_ptr, "ROOM TYPE: END_ROOM\n");
		} else {
			fprintf(file_ptr, "ROOM TYPE: MID_ROOM\n");
		}
		fclose(file_ptr);
	}
}

int isGraphFull(char* path, char* names[], int deck[]){
	int i = 0;
	char filename[255];
	char buff[255];
	for(i; i<7; ++i){
		strcpy(filename, path);
		strcat(filename, "/");
		strcat(filename, names[deck[i]]);
		FILE* file_ptr = fopen(filename, "r");
		while(1){
			fgets(buff, 255, file_ptr);
			//perform string slicing 
			char substring[13];
			int j = 0;
			for(j; j<12; ++j){
				substring[j] = buff[j];
			}
			substring[12] = '\0';
			if(strcmp(substring, "CONNECTION 3") == 0){
				fclose(file_ptr);	
				break;
			} else if (feof(file_ptr)){
				//graph is not full
				fclose(file_ptr);
				return 0;
			} 
		}
	}
	// graph is full
	return 1;
}

void addConnection(char* path, char* names[], int deck[]){
	int i = 0;
	char filename1[255];
	char filename2[255];
	char buff[255];
	int seed = time(NULL);
	srand(seed);
	while(1){
		int room1Full = 0;
		int room2Full = 0;
		
		// get two different rooms
		int room1 = 0;
		int room2 = 0;
		int alreadyConnected = 0;
		while(room1 == room2){
			room1 = rand() % 7;
			room2 = rand() % 7;
		}
		strcpy(filename1, path);
		strcat(filename1, "/");
		strcat(filename1, names[deck[room1]]);
		strcpy(filename2, path);
		strcat(filename2, "/");
		strcat(filename2, names[deck[room2]]);
		// check room1 capacity
		FILE* file_ptr1 = fopen(filename1, "r");
		while(1){
			fgets(buff, 255, file_ptr1);
			char substring[13];
			int j = 0;
			for(j; j<12; ++j){
				substring[j] = buff[j];
			}
			substring[12] = '\0';
			if(strcmp(substring, "CONNECTION 6") == 0){
				room1Full = 1;
				fclose(file_ptr1);
				break;
			} else if(feof(file_ptr1)){
				fclose(file_ptr1);
				break;
			}
		}
		if(!room1Full){
			// check room2 capacity
			FILE* file_ptr2 = fopen(filename2, "r");
			while(1){
				fgets(buff, 255, file_ptr2);
				char substring[13];
				int j = 0;
				for(j; j<12; ++j){
					substring[j] = buff[j];
				}
				substring[12] = '\0';
				if(strcmp(substring, "CONNECTION 6") == 0){
					room2Full = 1;
					fclose(file_ptr2);
					break;
				} else if(feof(file_ptr2)){
					fclose(file_ptr2);
					break;
				}
			}
		}

		if(room1Full || room2Full){
			continue;
		}
		
		// check for existing connection between room1, room2
		
		// calculate the size of room2's name
		int j = 0;
		while(1){
			if(names[deck[room2]][j] == '\0'){
				break;
			}
			j++;
		}
		file_ptr1 = fopen(filename1, "r");
		// skip past the ROOM NAME line
		fgets(buff, 255, file_ptr1);
		while(1){
			if(feof(file_ptr1)){
				fclose(file_ptr1);
				break;
			}
			fgets(buff, 255, file_ptr1);
			int k=0;
			char substring[20];
			for(k; k<j; ++k){
				// 'CONNECTION X: ' offset == 14
				substring[k] = buff[k+14];
			}
			substring[k] = '\0';
			if(strcmp(substring, names[deck[room2]]) == 0){
				fclose(file_ptr1);
				alreadyConnected = 1;
				break;
			}
		}
		if(!alreadyConnected){
			// add to room1
			file_ptr1 = fopen(filename1, "r+");
			int k = 0;
			while(1){
				fgets(buff, 255, file_ptr1);
				if(feof(file_ptr1)){
					fprintf(file_ptr1, "CONNECTION %d: %s\n", k, names[deck[room2]]);
					fclose(file_ptr1);
					break;
				}
				k++;
			}
			// add to room2
			FILE* file_ptr2 = fopen(filename2, "r+");
			k = 0;
			while(1){
				fgets(buff, 255, file_ptr2);
				if(feof(file_ptr2)){
					fprintf(file_ptr2, "CONNECTION %d: %s\n", k, names[deck[room1]]);
					fclose(file_ptr2);
					// whew! made a connection!
					return;
				}
				k++;
			}
		}
	}
}
