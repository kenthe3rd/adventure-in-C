#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <dirent.h>
#include <time.h>
#include <pthread.h>

pthread_mutex_t myMutex = PTHREAD_MUTEX_INITIALIZER;
char path[255];

void* timeKeeper(){
    //lock access to shared resources (path variable) while this thread is executing
    pthread_mutex_lock(&myMutex);
    char timeFile[255];
    strcpy(timeFile, path);
    strcat(timeFile, "currentTime.txt");
    // open the file and write the current time to it
    FILE* file_ptr = fopen(timeFile, "w");
    time_t currentTime = time(NULL);
    struct tm* tmp;
    tmp = localtime(&currentTime);
    char formattedTime[255];
    strftime(formattedTime, sizeof(formattedTime), "%I:%M %p, %A, %B %d, %Y", tmp);
    fprintf(file_ptr, formattedTime);
    fclose(file_ptr);
    pthread_mutex_unlock(&myMutex);
    return;
}

int main(){
    // lock down the path variable
    pthread_mutex_lock(&myMutex);
    char buffer[255];

    // get current directory in path
    getcwd(path, sizeof(path));
	strcat(path, "/");

    // read the contents of the current directory
    // and id the newest 'rooms' directory
    int newestTime = 0;
    char newestDir[255];
    DIR* dirp;
    struct dirent* direntp;
    struct stat fileStat;
    dirp = opendir(path);
    if(dirp == NULL){
        return 1;
    } else {
        while(1){
            direntp = readdir(dirp);
            if(direntp == NULL){
                break;
            }
            sprintf(buffer, direntp->d_name);
            if(stat(buffer, &fileStat) == -1){
                // unable to open the dir indicated by path
                closedir(dirp);
                return 1;
            }
            // read the first 13 characters to see if the file
            // is a room file
            char substring[255];
            int i = 0;
            for(i; i<13; ++i){
                substring[i] = buffer[i];
            }
            substring[i] = '\0';
            // check to see if the file is a room dir
            if(strcmp(substring, "hallkenn.room") == 0){
                // check to see if the file is the newest hotness
                if(fileStat.st_mtime > newestTime){
                    newestTime = fileStat.st_mtime;
                    strcpy(newestDir, buffer);
                }
            }
        }
        closedir(dirp);
    }

    // build the path to the selected rooms dir
    // and save to newestDir
    strcpy(buffer, newestDir);
    strcpy(newestDir, path);
    strcat(newestDir, buffer);

    // open newestDir and read through the files, 
    // looking for the START_ROOM
    char roomFile[255];
    char roomName[255];
    int lookingForStartRoom = 1;
    dirp = opendir(newestDir);
    if(dirp == NULL){
        return 1;
    } else {
        while(lookingForStartRoom){
            direntp = readdir(dirp);
            if(direntp == NULL){
                break;
            } else {
                // build the path to the room
                sprintf(roomName, direntp->d_name);
                if(roomName[0] == '.'){
                    // not what we're looking for...skip it
                    continue;
                } else {
                    strcpy(roomFile, newestDir);
                    strcat(roomFile, "/");
                    strcat(roomFile, roomName);
                }
                // open the file for reading and check for START_ROOM
                FILE* file_ptr = fopen(roomFile, "r");
                while(1){
                    fgets(buffer, 255, file_ptr);
                    if(strcmp(buffer, "ROOM TYPE: START_ROOM\n") == 0){
                        fclose(file_ptr);
                        lookingForStartRoom = 0;
                        break;
                    } else if(feof(file_ptr)){
                        fclose(file_ptr);
                        break;
                    }
                }
            }
        }
        if(!lookingForStartRoom){
            // play the game
            int stepsCounter = 0;
            char roomsHistory[1000];
            strcpy(roomsHistory, "Start: ");
            char connections[6][255];
            char input[255];
            int gameOver = 0;
            int validRoom = 0;
            char substring[255];
            int i = 0;
            int j = 0;
            while(!gameOver){
                // check to see if the player is in the end room
                FILE* file_ptr = fopen(roomFile, "r");
                while(1){
                    fgets(buffer, 255, file_ptr);
                    if(strcmp(buffer, "ROOM TYPE: END_ROOM\n") == 0){
                        strcat(roomsHistory, "End: ");
                        strcat(roomsHistory, input);
                        fclose(file_ptr);
                        gameOver = 1;
                        break;

                    } else if(feof(file_ptr)){
                        fclose(file_ptr);
                        break;
                    }
                }
                if(gameOver){
                    printf("\nYOU HAVE FOUND THE END ROOM. CONGRATULATIONS!\n");
                    printf("YOU TOOK %d STEPS. YOUR PATH TO VICTORY WAS:\n", stepsCounter);
                    printf("%s\n", roomsHistory);
                    break;
                }
                file_ptr = fopen(roomFile, "r");
                // print the current room info to the user
                fgets(buffer, 255, file_ptr);
                strcpy(roomName, buffer + 11);
                printf("\nCURRENT LOCATION: %s", roomName);
                //clean out old connections data
                i = 0;
                for(i; i<6; i++){
                    strcpy(connections[i], "\0");
                }
                // build the connections list, then
                // print the connections info to the user
                j = 0;
                while(1){
                    fgets(buffer, 255, file_ptr);
                    i = 0;
                    for(i; i<7; ++i){
                        substring[i] = buffer[i];
                    }
                    substring[i] = '\0';
                    if(strcmp(substring, "CONNECT") == 0){
                        strcpy(buffer, buffer + 14);
                        //strip out newline character
                        buffer[strlen(buffer)-1] = '\0';
                        strcpy(connections[j], buffer);
                        j++;
                    } else if(feof(file_ptr)){
                        break;
                    }
                }
                i = 0;
                printf("POSSIBLE CONNECTIONS: ");
                for(i; i<6; ++i){
                    if('\0' != connections[i][0]){
                        if(i>0){
                        printf(", %s", connections[i]);
                        } else {
                        printf("%s", connections[i]);
                        }

                    }
                }
                printf(".\n");
                fclose(file_ptr);
                // prompt for next room
                printf("WHERE TO? >");
                fgets(input, 64, stdin);
                // strip out newline characyer
                input[strlen(input)-1] = '\0';

                // check input against possible connections
                i = 0;
                validRoom = 0;
                if(strcmp(input, "time") == 0){
                    // unlock the path variable and start the time keeper
                    pthread_t timeKeeperThread;
                    pthread_create(&timeKeeperThread, NULL, &timeKeeper, NULL);
                    pthread_mutex_unlock(&myMutex);
                    pthread_join(timeKeeperThread, NULL);
                    pthread_mutex_lock(&myMutex);
                    // read time from the time file
                    char timePath[255];
                    strcpy(timePath, path);
                    strcat(timePath, "currentTime.txt");
                    file_ptr = fopen(timePath, "r");
                    fgets(buffer, 255, file_ptr);
                    fclose(file_ptr);
                    printf("\n%s\n", buffer);
                    continue;
                }
                for(i; i<6; ++i){
                    if('\0' != connections[i][0] && strcmp(connections[i], input) == 0){
                        validRoom = 1;
                        break;
                    }
                }
                if(!validRoom){
                    printf("\nHUH? I DON'T UNDERSTAND THAT ROOM. TRY AGAIN.\n\n");
                    continue;
                } else {
                    // take a step and add the previous room to the history
                    stepsCounter++;
                    strcat(roomsHistory, roomName);
                    strcpy(roomFile, newestDir);
                    strcat(roomFile, "/");
                    strcat(roomFile, input);
                    continue;
                }
                break;
            }

        } else {
            printf("Couldn't find a start room.\n");
        }
    }
    closedir(dirp);
    return 0;
}