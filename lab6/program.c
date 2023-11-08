#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#define BUFFER_SIZE 4096

void checkArguments(int number, char *arguments[])
{
    if(number > 4)
    {
        perror("Too many arguments \n");
        exit(-1);
    }
    else if (number < 4)
    {
        perror("Three arguments expected \n");
        exit(-1);
    }
}

int openFile (char *pathname, int oflag)
{
    int fileDescriptor = open(pathname, oflag);

    if(fileDescriptor == -1)
    {
        printf("Error opening file\n");
        exit(-1);
    }
    
    return fileDescriptor;
}

char* createStatistic(int fd, char *character)
{
    int countLowercase = 0;
    int countUppercase = 0;
    int countDigits = 0;
    int countChar = 0;
    char *output = malloc(100 * sizeof(char));

    char buff[BUFFER_SIZE];
    int readValue = 0;
    while((readValue = read(fd, buff, 4096)))
    {
        if(readValue == -1)
        {
            printf("Erro occured while trying to read the file\n");
            exit(-1);
        }

        for(int i = 0; i < readValue; i++)
        {
            if(isupper(buff[i]))
                countUppercase++;
            else if (islower(buff[i]))
                countLowercase++;
            else if (isdigit(buff[i]))
                countDigits++;
            
            char buffChar[2] = {buff[i], '\0'};
            if(strcmp(buffChar, character) == 0)
                countChar++;
        }
    }

    sprintf(output, "Uppercase: %d \nLowercase: %d \nDigits: %d \nCharacter count: %d \n", countUppercase, countLowercase, countDigits, countChar);

    return output;
} 

void writeInFile(int fd, char *str)
{
    int writeValue = write(fd, str, strlen(str));
    if(writeValue == -1)
    {
        printf("Error writing in file \n");
    }
}

char* getFileSize(char *filename)
{
    char *output = malloc(100 * sizeof(char));
    struct stat file_status;
    if(stat(filename, &file_status) < 0)
    {
        printf("Error using stat on file \n");
        exit(-1);
    }

    sprintf(output, "File size is %lld bytes", file_status.st_size);
    return output;
}

int main (int argc, char *argv[]) 
{
    checkArguments(argc, argv);

    int inputFileDescriptor = openFile(argv[1], O_RDONLY);

    char *output = createStatistic(inputFileDescriptor, argv[3]);

    int outputFileDescriptor = openFile(argv[2], O_WRONLY);

    writeInFile(outputFileDescriptor, output);

    char *fileSize = getFileSize(argv[1]);
    writeInFile(outputFileDescriptor, fileSize);

    close(inputFileDescriptor);
    close(outputFileDescriptor);
    return 0;
}