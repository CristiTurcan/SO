#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>

#define BUFFER_SIZE 4096

int openFile(const char *pathname, int oflag)
{
    int fileDescriptor = open(pathname, oflag);

    if (fileDescriptor == -1)
    {
        perror("Error opening file %s\n");
        exit(EXIT_FAILURE);
    }

    return fileDescriptor;
}

int newProcess()
{
    int pid = 0;

    if ((pid = fork()) < 0)
    {
        perror("Error using fork\n");
        exit(EXIT_FAILURE);
    }

    return pid;
}

void waitAllChildProcess(int process_no)
{
    int w = 0;
    int wstatus = 0;

    for(int i = 0; i < process_no; i++)
    {
        w = wait(&wstatus);
        if (w == -1)
        {
            perror("waitpid");
            exit(EXIT_FAILURE);
        }

        if (WIFEXITED(wstatus))
        {
            // printf("exited pid %d status %d\n", w, WEXITSTATUS(wstatus));
        }

    }
}

void deleteUppercase(char arr[])
{
    int i = 0, j = 0;
    while (arr[i] != '\0') {
            if (!isupper(arr[i]))
                    arr[j++] = arr[i];
            i++;
    }
    arr[j] = '\0';
}

int createFile(char* filename)
{
    int fileDescriptor = open(filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fileDescriptor < 0)
    {
        perror("Error creating statistics file\n");
        exit(EXIT_FAILURE);
    }
    return fileDescriptor;
}

int write_occurences_in_file(char* str, int fd)
{
    int counts[256] = { 0 };
    int i;
    int distinctChar = 0;
    char *output;
    output = (char*)malloc(500*sizeof(char));
    char temp[100];
    size_t len = strlen(str);
    for (i = 0; i < len; i++)
    {
        if(str[i] >= 97 && str[i] <= 122)
        {
            counts[(int)(str[i])]++;
        }
    }
    for (i = 0; i < 256; i++)
    {
        if(counts[i]>0)
        {
            distinctChar++;
            sprintf(temp, "%c occurs %d times.\n", i , counts[i]);
            strcat(output, temp);
        }
        
    }
    if((write(fd, output, (strlen(output)))) != (strlen(output)))
            {
                perror("Error writing in statistics\n");
                exit(EXIT_FAILURE);
            }
    
    return distinctChar;
}

int main (void)
{
    int fd = 0, readValue = 0;
    int pfd1[2], pfd2[2], pfd3[2];
    char buff[BUFFER_SIZE];

    pid_t c1pid = 0, c2pid = 0, pid = 0;
    
    //creeaza pipes
    if ( (pipe(pfd1) < 0) || (pipe(pfd2) < 0) || (pipe(pfd3) < 0) )
    {
        perror("Error opening pipe\n");
        exit(EXIT_FAILURE);
    }

    //procesul parinte
    if((pid = newProcess()) == 0)
    {
        //close reading end
        close(pfd1[0]);
        
        fd = openFile("date.txt", O_RDONLY);
        while((readValue = read(fd, buff, BUFFER_SIZE - 1)))
        {
            if (readValue == -1)
            {
                printf("Error occured while reading file");
                close(fd);
                exit(EXIT_FAILURE);
            }

            buff[readValue] = '\0';

            if((write(pfd1[1], buff, (strlen(buff) + 1))) != (strlen(buff) + 1))
            {
                perror("Error writing in file\n");
                exit(EXIT_FAILURE);
            }
        }

        //proces copil 1
        if((c1pid = newProcess()) == 0)
        {
            close(pfd1[1]);

            readValue = read(pfd1[0], buff, sizeof(buff));
            if (readValue == -1)
                {
                    perror("Error occured while reading file proces");
                    close(fd);
                    exit(EXIT_FAILURE);
                }

            deleteUppercase(buff);

            //write text without uppercase in pipe1
            close(pfd2[0]);
            if((write(pfd2[1], buff, (strlen(buff) + 1))) != (strlen(buff) + 1))
            {
                perror("Error writing in pipe2\n");
                exit(EXIT_FAILURE);
            }

            close(pfd2[1]);
            close(pfd1[0]);
            exit(0);
        }

        //proces copil 2
        if((c2pid = newProcess()) == 0)
        {
            //citirea din pipe2
            close(pfd2[1]);
            readValue = read(pfd2[0], buff, sizeof(buff));
            if (readValue == -1)
                {
                    perror("Error occured while reading file proces");
                    close(fd);
                    exit(EXIT_FAILURE);
                }
            close(pfd2[0]);

            int fd = createFile("statistics.txt");
            int distinctLetterCount =  write_occurences_in_file(buff, fd);
            close(fd);

            char distinctLetters[30];
            sprintf(distinctLetters, "%d", distinctLetterCount);

            //write number of occurences in pipe3
            close(pfd3[0]);
            if((write(pfd3[1], distinctLetters, sizeof(distinctLetters)) != sizeof(distinctLetters)))
            {
                perror("Error writing in pipe3");
                printf("\n");
                exit(EXIT_FAILURE);
            }
            close(pfd3[1]);

            exit(0);
        }

        close(pfd1[1]);
        
        //print chlid2 result from pipe3 (print number of occurences calculated in chlid2)
        close(pfd3[1]);
        readValue = read(pfd3[0], buff, sizeof(buff));
        if (readValue == -1)
        {
            perror("Error occured while reading pipe3");
            close(fd);
            exit(EXIT_FAILURE);
        }
        printf("%s\n", buff);
        close(pfd3[0]);

        waitAllChildProcess(2);     //inchide cele 2 procese copil
        exit(0);
    }

    waitAllChildProcess(1);     //inchide procesul parinte
    close(pfd1[0]);
    close(pfd1[1]);
    close(pfd2[0]);
    close(pfd2[1]);
    close(pfd3[0]);
    close(pfd3[1]);
    close(fd);
    return 0;
}