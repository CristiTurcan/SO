#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <dirent.h>

#define BUFFER_SIZE 4096

typedef struct metadata_t
{ 
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
    int targetFileSize;
} metadata_t;

void checkArguments(int number)
{
    if (number > 3)
    {
        perror("Too many arguments \n");
        exit(-1);
    }
    else if (number < 3)
    {
        perror("One argument expected \n");
        exit(EXIT_FAILURE);
    }
}

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

DIR *openDirectory(const char *dirname)
{
    DIR *inputDir;
    inputDir = opendir(dirname);
    if (inputDir == NULL)
    {
        perror("Cannot open directory\n");
        exit(EXIT_FAILURE);
    }

    return inputDir;
}

char* allocateMemory (int n)
{
    char *value = (char*)malloc(n * sizeof(char));
    if(value == NULL)
    {
        printf("Couldn't allocate memory correctly\n");
        free(value);
        exit(EXIT_FAILURE);
    }

    return value;
}

int checkFileExtension(const char *filename, const char *extension)
{
    const char *currentExtension = &filename[strlen(filename) - strlen(extension)];
    if (strcmp(currentExtension, extension) == 0)
        return 1;
    return 0;
}

int fileIsBMP(int fd)
{
    char buff[2];

    if (lseek(fd, 0, SEEK_SET) < 0) // cursor always at the start of file
    {
        perror("Error moving file cursor: ");
        close(fd);
        exit(EXIT_FAILURE);
    }

    if (read(fd, buff, 2) < 0)
    {
        perror("Error reading from file while checking if it's BMP ");
        close(fd);
        exit(EXIT_FAILURE);
    }

    if ((buff[0] == 'B') && (buff[1] == 'M'))
        return 1;

    return 0;
}

int getFileType(const char *filename)
{
    struct stat file_status;
    if (lstat(filename, &file_status) < 0)
    {
        perror("Error using stat on file \n");
        exit(EXIT_FAILURE);
    }

    if (S_ISREG(file_status.st_mode))
        return 1;

    if (S_ISDIR(file_status.st_mode))
        return 2;

    if (S_ISLNK(file_status.st_mode))
        return 3;

    // nu cred ca am nevoie de astea, cel putin sa faca exit, mai vad pe viitor
    // perror("File is not dir/symlink/regfile\n");
    // exit(EXIT_FAILURE);
    return 0;
}

void getImageHeightWidth(int file_descriptor, int *height, int *width)
{
    if (lseek(file_descriptor, 18, SEEK_SET) < 0)
    {
        perror("Error moving file cursor: ");
        exit(EXIT_FAILURE);
    }

    if (read(file_descriptor, width, 4) < 0)
    {
        perror("Error reading from file: ");
        close(file_descriptor);
        exit(EXIT_FAILURE);
    }

    if (lseek(file_descriptor, 22, SEEK_SET) < 0)
    {
        perror("Error moving file cursor: ");
        exit(EXIT_FAILURE);
    }

    if (read(file_descriptor, height, 4) < 0)
    {
        perror("Error reading from file: ");
        close(file_descriptor);
        exit(EXIT_FAILURE);
    }
}

void filePermissionToString(mode_t mode, char *str, char who)
{
    switch (who)
    {
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
    char *date = allocateMemory(20);
    strftime(date, 20, "%d.%m.%Y", localtime(&(stats->st_mtime)));
    strcpy(md->last_modified, date);
    free(date);
}

// insert all file stats into medatada struct
void getFileStats(char *filename, metadata_t *md, int fd)
{
    struct stat stats;
    if (stat(filename, &stats) < 0)
    {
        perror("Error using stat on file \n");
        exit(-1);
    }

    strcpy(md->file_name, filename);
    md->user_id = (int)stats.st_uid;
    md->link_count = (int)stats.st_nlink;
    md->size = (int)stats.st_size;

    getFilePermissions(&stats, md);
    getLastModifiedDate(&stats, md);
    if (fileIsBMP(fd))
        getImageHeightWidth(fd, &md->height, &md->width);
}

void getDirStats(char *path, metadata_t *md)
{
    struct stat stats;
    if (stat(path, &stats) < 0)
    {
        perror("Error using stat on file \n");
        exit(-1);
    }

    strcpy(md->file_name, path);
    md->user_id = (int)stats.st_uid;
    getFilePermissions(&stats, md);
}

void getSymlinkStats(char *path, metadata_t *md)
{
    struct stat stats;
    if (lstat(path, &stats) < 0)
    {
        perror("Error using stat on file \n");
        exit(-1);
    }

    strcpy(md->file_name, path);
    md->size = (int)stats.st_size;

    getFilePermissions(&stats, md);

    if (stat(path, &stats) < 0)
    {
        perror("Error using stat on file \n");
        exit(-1);
    }
    md->targetFileSize = (int)stats.st_size; // with stat you get target file statistics
}

char *createMetadata(metadata_t *md, int fd)
{
    char *statistics = allocateMemory(200);
    if (fileIsBMP(fd))
    {
        sprintf(statistics, "Nume fisier: %s\nInaltime: %d\nLungime: %d\nDimensiune: %d\nIdentificatorul utilizatorului: %d\nTimpul ultimei modificari: %s\nContorul de legaturi: %d\nDrepturi de acces user: %s\nDrepturi de acces grup: %s\nDrepturi de acces altii: %s",
                md->file_name,
                md->height,
                md->width,
                md->size,
                md->user_id,
                md->last_modified,
                md->link_count,
                md->user_access,
                md->group_access,
                md->others_access);
    }
    else
    {
        sprintf(statistics, "Nume fisier: %s\nDimensiune: %d\nIdentificatorul utilizatorului: %d\nTimpul ultimei modificari: %s\nContorul de legaturi: %d\nDrepturi de acces user: %s\nDrepturi de acces grup: %s\nDrepturi de acces altii: %s",
                md->file_name,
                md->size,
                md->user_id,
                md->last_modified,
                md->link_count,
                md->user_access,
                md->group_access,
                md->others_access);
    }

    return statistics;
}

char *createDirMetadata(metadata_t *md)
{
    char *statistics = allocateMemory(200);
    sprintf(statistics, "Nume director: %s\nIdentificatorul utilizatorului: %d\nDrepturi de acces user: %s\nDrepturi de acces grup: %s\nDrepturi de acces altii: %s",
            md->file_name,
            md->user_id,
            md->user_access,
            md->group_access,
            md->others_access);

    return statistics;
}

char *createSymlinkMetadata(metadata_t *md)
{
    char *statistics = allocateMemory(200);
    sprintf(statistics, "Nume legatura: %s\nDimensiune legatura: %d\nDimensiunea fisierului target: %d\nDrepturi de acces user: %s\nDrepturi de acces grup: %s\nDrepturi de acces altii: %s",
            md->file_name,
            md->size,
            md->targetFileSize,
            md->user_access,
            md->group_access,
            md->others_access);

    return statistics;
}

void createStatisticFile(const char *filename, const char *statistics)
{
    int fileDescriptor;
    if ((access(filename, F_OK) == 0) && (strcmp(filename, "statistics.txt") == 0)) // statistics file is the only one that needs append
    {
        fileDescriptor = open(filename, O_WRONLY | O_APPEND);
    }
    else
    {
        fileDescriptor = open(filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        if (fileDescriptor < 0)
        {
            perror("Error creating statistics file\n");
            exit(EXIT_FAILURE);
        }
    }

    if (write(fileDescriptor, statistics, strlen(statistics)) < 0)
    {
        perror("Error writing in statistics file\n");
        close(fileDescriptor);
        exit(EXIT_FAILURE);
    }

    close(fileDescriptor);
}

char *getPath(const char *root, const char *current)
{
    char *path = allocateMemory(50);
    sprintf(path, "%s/%s", root, current);
    return path;
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

char *createFileName(const char *outputFile, const char *current)
{
    char *statisticsFileName = allocateMemory(100);
    strcpy(statisticsFileName, "./");
    strcat(statisticsFileName, outputFile);
    strcat(statisticsFileName, "/");
    strcat(statisticsFileName, current);
    strcat(statisticsFileName, "_statistics.txt");

    return statisticsFileName;
}

int lineCount(const char *filename)
{
    int fd = openFile(filename, O_RDONLY);
    int lines = 0;

    char buff[BUFFER_SIZE];
    int readValue;
    while ((readValue = read(fd, buff, BUFFER_SIZE)))
    {
        if (readValue == -1)
        {
            printf("Error occured while reading file %s\n", filename);
            close(fd);
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < readValue; i++)
        {
            if (buff[i] == '\n')
                lines++;
        }
    }

    lines++; // count last line
    close(fd);
    return lines;
}

char* intToChar (int number)
{
    char *str = allocateMemory(5);
    sprintf(str, "%d\n", number);
    return str;
}

void waitAllChildProcess()
{
    int w = 0;
    int wstatus = 0;
    
    while((w = wait(&wstatus)) > 0)
    {
        // cazul w == -1 nu o sa se intample niciodata
        if (w == -1)
        {
            perror("waitpid");
            exit(EXIT_FAILURE);
        }


        if (WIFEXITED(wstatus))
        {   
            char *number = intToChar(WEXITSTATUS(wstatus));
            createStatisticFile("statistics.txt", number);
            free(number);
            printf("exited pid=%d, status=%d\n", w, WEXITSTATUS(wstatus));
        }
    }
}

void getImagePixelData(const char *filename)
{   
    int fd = openFile(filename, O_RDONLY);
    int height = 0, width= 0;
    getImageHeightWidth(fd, &height, &width);
    int size = height * width * 3;

    unsigned char* colorTable = (unsigned char*)malloc(size);
    if(colorTable == NULL)
    {
        printf("Couldn't allocate memory for BMP pixel data\n");
        free(colorTable);
        close(fd);
        exit(EXIT_FAILURE);
    }
    
    while((read(fd, colorTable, size)) < 0)
    {
        perror("Could not read pixel data\n");
        free(colorTable);
        close(fd);
        exit(EXIT_FAILURE);
    }

    for(int i = 0; i < size; i += 3)
    {
        unsigned char blue = colorTable[i];
        unsigned char green = colorTable[i + 1];
        unsigned char red = colorTable[i + 2];

        unsigned char greyscale = (unsigned char)(0.299 * red + 0.587 * green + 0.114 * blue);
        
        colorTable[i] = greyscale;
        colorTable[i + 1] = greyscale;
        colorTable[i + 2] = greyscale;
    }

    close(fd);
    fd = openFile(filename, O_WRONLY);

    if (lseek(fd, 54, SEEK_SET) < 0)
    {
        perror("Error moving file cursor: ");
        close(fd);
        exit(EXIT_FAILURE);
    }

    if (write(fd, colorTable, size) < 0)
    {
        perror("Error writing BMP image data\n");
        close(fd);
        exit(EXIT_FAILURE);
    }


    free(colorTable);
    close(fd);
}

int main(int argc, char *argv[])
{
    struct dirent *pDirent;
    metadata_t md;
    DIR *inputDir;
    int pid = 0;
    int lines = 0;

    checkArguments(argc);
    inputDir = openDirectory(argv[1]);

    while ((pDirent = readdir(inputDir)) != NULL)
    {
        if ((strcmp(pDirent->d_name, ".") == 0) || (strcmp(pDirent->d_name, "..") == 0)) // skip
            continue;

        char *path = getPath(argv[1], pDirent->d_name);
        char *statisticsFileName = createFileName(argv[2], pDirent->d_name);

        int fileType = getFileType(path);

        switch (fileType)
        {
        case 1: // regFile
        {
            if ((pid = newProcess()) == 0)
            {
                int fileDescriptor = openFile(path, O_RDONLY);  
                getFileStats(path, &md, fileDescriptor);
                char *statistics = createMetadata(&md, fileDescriptor);
                createStatisticFile(statisticsFileName, statistics);
                free(statistics);
                close(fileDescriptor);
                lines = lineCount(statisticsFileName);
                exit(lines);
            }
            if(checkFileExtension(path, ".bmp"))
            {
                int fileDescriptor = openFile(path, O_RDONLY);  
                if(fileIsBMP(fileDescriptor))
                    if((pid = newProcess()) == 0)
                    {
                        // printf("%s\n", path);
                        getImagePixelData(path);
                        exit(0);
                    }
                close(fileDescriptor);
            }
        }
        break;
        case 2: // directory
        {
            if ((pid = newProcess()) == 0)
            {
                getDirStats(path, &md);
                char *statistics = createDirMetadata(&md);
                createStatisticFile(statisticsFileName, statistics);
                free(statistics);
                lines = lineCount(statisticsFileName);
                exit(lines);
            }
        }
        break;
        case 3: // symlink
        {
            if ((pid = newProcess()) == 0)
            {
                getSymlinkStats(path, &md);
                char *statistics = createSymlinkMetadata(&md);
                createStatisticFile(statisticsFileName, statistics);
                free(statistics);
                lines = lineCount(statisticsFileName);
                exit(lines);
            }
        }
        break;
        default:
            break;
        }

        free(statisticsFileName);
        free(path);
    }

    waitAllChildProcess();
    closedir(inputDir);
    return 0;
}