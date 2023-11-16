#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>

typedef struct metadata_t {
    char file_name[40];
    int height;
    int width;
    int size;
    int user_id;
    char last_modified[11];
    int link_count;
    char user_access[4];
    char group_access[4];
    char others_access[4];
}metadata_t;

void checkArguments(int number)
{
    if(number > 2)
    {
        perror("Too many arguments \n");
        exit(-1);
    }
    else if (number < 2)
    {
        perror("One argument expected \n");
        exit(-1);
    }
}

int openFile (const char *pathname, int oflag)
{
    int fileDescriptor = open(pathname, oflag);

    if(fileDescriptor == -1)
    {
        perror("Error opening file\n");
        exit(-1);
    }
    
    return fileDescriptor;
}

//unused
int checkFileExtension(const char *filename, const char *extension)
{
    const char *currentExtension = &filename[strlen(filename) - 3];
    if(strcmp(currentExtension, extension) == 0)
        return 1;
    return 0;
}

int fileIsBMP(int fd)
{
    char buff[2];
    if(read(fd, buff, 2) < 0) {
        perror("Error reading from file: ");
        exit(-1);
    }

    if((buff[0] == 'B') && (buff[1] == 'M'))
        return 1;
    
    return 0;
}

void checkFileType(const char *filename)
{
    struct stat file_status;
    if(stat(filename, &file_status) < 0)
    {
        perror("Error using stat on file \n");
        exit(-1);
    }

    if(!S_ISREG(file_status.st_mode))
    {
        perror("File is not good\n");
        exit(-1);
    }
}

void getImageHeightWidthSize(int file_descriptor, int *height, int *width, int *size)
{

    if(lseek(file_descriptor, 18, SEEK_SET) < 0) {
        perror("Error moving file cursor: ");
        exit(-1);
    }
    
    if(read(file_descriptor, width, 4) < 0) {
        perror("Error reading from file: ");
        exit(-1);
    }

    if(lseek(file_descriptor, 22, SEEK_SET) < 0) {
        perror("Error moving file cursor: ");
        exit(-1);
    }

    if(read(file_descriptor, height, 4) < 0) {
        perror("Error reading from file: ");
        exit(-1);
    }

    if(lseek(file_descriptor, 2, SEEK_SET) < 0) {
        perror("Error moving file cursor: ");
        exit(-1);
    }
    
    if(read(file_descriptor, size, 4) < 0) {
        perror("Error reading from file: ");
        exit(-1);
    }
}


void filePermissionToString(mode_t mode, char *str, char who)
{
    switch (who) {
        case 'u': // User
            str[0] = (mode & S_IRUSR) ? 'r' : '-';
            str[1] = (mode & S_IWUSR) ? 'w' : '-';
            str[2] = (mode & S_IXUSR) ? 'x' : '-';
            break;
        case 'g': // Group
            str[0] = (mode & S_IRGRP) ? 'r' : '-';
            str[1] = (mode & S_IWGRP) ? 'w' : '-';
            str[2] = (mode & S_IXGRP) ? 'x' : '-';
            break;
        case 'o': // Others
            str[0] = (mode & S_IROTH) ? 'r' : '-';
            str[1] = (mode & S_IWOTH) ? 'w' : '-';
            str[2] = (mode & S_IXOTH) ? 'x' : '-';
            break;
    }
    str[3] = '\0';
}

void getFilePermissions(struct stat *stats, metadata_t *md)
{
    filePermissionToString(stats->st_mode, md->user_access, 'u');
    filePermissionToString(stats->st_mode, md->group_access, 'g');
    filePermissionToString(stats->st_mode, md->others_access, 'o');
}

void getLastModifiedDate(struct stat *stats, metadata_t *md)
{
    char *date = malloc(20);
    strftime(date, 20, "%d.%m.%Y", localtime(&(stats->st_mtime)));
    strcpy(md->last_modified, date);
}

//insert all file stats into medatada struct
void getFileStats(char *filename, metadata_t *md)
{
    struct stat stats;
    if(stat(filename, &stats) < 0)
    {
        perror("Error using stat on file \n");
        exit(-1);
    }

    md->user_id = (int)stats.st_uid;
    md->link_count = (int)stats.st_nlink;
    
    getFilePermissions(&stats, md);
    getLastModifiedDate(&stats, md);
}

char* createMetadata(char *filename, metadata_t *md)
{
    char *statistics = malloc(200 * sizeof(char));

    sprintf(statistics, "Nume fisier: %s\nInaltime: %d\nLungime: %d\nDimensiune: %d\nIdentificatorul utilizatorului: %d\nTimpul ultimei modificari: %s\nContorul de legaturi: %d\nDrepturi de acces user: %s\nDrepturi de acces grup: %s\nDrepturi de acces altii: %s",
            filename,
            md->height,
            md->width,
            md->size,
            md->user_id,
            md->last_modified,
            md->link_count,
            md->user_access,
            md->group_access,
            md->others_access);

    return statistics;
}

void createStatisticFile(const char *filename, const char *statistics)
{
    int fileDescriptor = open(filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fileDescriptor < 0)
    {
        perror("Error creating statistics file\n");
        exit(-1);
    }

    if(write(fileDescriptor, statistics, strlen(statistics)) < 0)
    {
        perror("Error writing in statistics file\n");
        close(fileDescriptor);
        exit(-1);
    }

    close(fileDescriptor);
}

int main (int argc, char* argv[])
{
    metadata_t md;

    checkArguments(argc);
    checkFileType(argv[1]);
    int fileDescriptor = openFile(argv[1], O_RDONLY);

    //check if file is BMP Image
    if(fileIsBMP(fileDescriptor) == 0)
    {
        perror("File is not bmp\n");
        exit(-1);
    }

    getImageHeightWidthSize(fileDescriptor, &md.height, &md.width, &md.size);
    getFileStats(argv[1], &md);

    char *statistics = createMetadata(argv[1], &md);
    createStatisticFile("statistics.txt", statistics);

    close(fileDescriptor);
    return 0;
}