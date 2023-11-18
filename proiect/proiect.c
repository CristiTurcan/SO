#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <dirent.h>

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
    int targetFileSize;
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
        exit(EXIT_FAILURE);
    }
}

int openFile (const char *pathname, int oflag)
{
    int fileDescriptor = open(pathname, oflag);

    if(fileDescriptor == -1)
    {
        perror("Error opening file\n");
        exit(EXIT_FAILURE);
    }
    
    return fileDescriptor;
}

DIR* openDirectory (const char *dirname)
{
    DIR* pDir;
    pDir = opendir(dirname);
    if( pDir == NULL)
    {
        perror("Cannot open directory\n");
        exit(EXIT_FAILURE);
    }

    return pDir;

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

    if(lseek(fd, 0, SEEK_SET) < 0) // sa fie tot timpul de la inceputul fisierului citirea
    {
        perror("Error moving file cursor: ");
        exit(EXIT_FAILURE);
    }

    if(read(fd, buff, 2) < 0) {
        perror("Error reading from file: ");
        exit(EXIT_FAILURE);
    }

    if((buff[0] == 'B') && (buff[1] == 'M'))
        return 1;

    return 0;
}

int getFileType(const char *filename)
{
    struct stat file_status;
    if(lstat(filename, &file_status) < 0)
    {
        perror("Error using stat on file \n");
        exit(EXIT_FAILURE);
    }

    if(S_ISREG(file_status.st_mode))
        return 1;
    
    if(S_ISDIR(file_status.st_mode))
        return 2;
    
    if(S_ISLNK(file_status.st_mode))
        return 3;

    perror("File is not dir/symlink/regfile\n");
    exit(EXIT_FAILURE);
}

void getImageHeightWidth(int file_descriptor, int *height, int *width)
{

    if(lseek(file_descriptor, 18, SEEK_SET) < 0)
    {
        perror("Error moving file cursor: ");
        exit(EXIT_FAILURE);
    }
    
    if(read(file_descriptor, width, 4) < 0)
    {
        perror("Error reading from file: ");
        exit(EXIT_FAILURE);
    }

    if(lseek(file_descriptor, 22, SEEK_SET) < 0)
    {
        perror("Error moving file cursor: ");
        exit(EXIT_FAILURE);
    }

    if(read(file_descriptor, height, 4) < 0)
    {
        perror("Error reading from file: ");
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
    char *date = malloc(20);
    strftime(date, 20, "%d.%m.%Y", localtime(&(stats->st_mtime)));
    strcpy(md->last_modified, date);
    free(date);
}

//insert all file stats into medatada struct
void getFileStats(char *filename, metadata_t *md, int fd)
{
    struct stat stats;
    if(stat(filename, &stats) < 0)
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
    if(fileIsBMP(fd))
        getImageHeightWidth(fd, &md->height, &md->width);
}

void getDirStats(char *path, metadata_t *md)
{
    struct stat stats;
    if(stat(path, &stats) < 0)
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
    if(lstat(path, &stats) < 0)
    {
        perror("Error using stat on file \n");
        exit(-1);
    }

    strcpy(md->file_name, path);
    md->size = (int)stats.st_size;

    getFilePermissions(&stats, md);

    if(stat(path, &stats) < 0)
    {
        perror("Error using stat on file \n");
        exit(-1);
    }
    md->targetFileSize = (int)stats.st_size; // with stat you get target file statistics
}

char* createMetadata(metadata_t *md, int fd)
{
    char *statistics = (char*)malloc(200 * sizeof(char));
    if(fileIsBMP(fd))
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

char* createDirMetadata (metadata_t *md)
{
    char *statistics = (char*)malloc(200 * sizeof(char));
    sprintf(statistics, "Nume director: %s\nIdentificatorul utilizatorului: %d\nDrepturi de acces user: %s\nDrepturi de acces grup: %s\nDrepturi de acces altii: %s",
            md->file_name,
            md->user_id,
            md->user_access,
            md->group_access,
            md->others_access);

    return statistics;
}

char* createSymlinkMetadata (metadata_t *md)
{
    char *statistics = (char*)malloc(200 * sizeof(char));
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
    int fileDescriptor = open(filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fileDescriptor < 0)
    {
        perror("Error creating statistics file\n");
        exit(EXIT_FAILURE);
    }

    if(write(fileDescriptor, statistics, strlen(statistics)) < 0)
    {
        perror("Error writing in statistics file\n");
        close(fileDescriptor);
        exit(EXIT_FAILURE);
    }

    close(fileDescriptor);
}

char* getPath(const char *root, const char *current)
{
    char* path = (char*)malloc(50 * sizeof(char));
    sprintf(path, "%s/%s", root, current);
    return path;
}

int main (int argc, char* argv[])
{
    metadata_t md;
    struct dirent *pDirent;
    DIR *pDir;

    checkArguments(argc);
    pDir = openDirectory(argv[1]);

    while((pDirent = readdir(pDir)) != NULL)
    {
        if((strcmp(pDirent->d_name, ".") == 0) || (strcmp(pDirent->d_name, "..") == 0)) // skip 
            continue;
        
        char *path = getPath(argv[1], pDirent->d_name);

        char statisticsFileName[40];
        strcpy(statisticsFileName, pDirent->d_name);
        strcat(statisticsFileName, "Statistics.txt");

        int fileType = getFileType(path);
        switch (fileType)
        {
            case 1: //regular file
                {
                    int fileDescriptor = openFile(path, O_RDONLY);
                    getFileStats(path, &md, fileDescriptor);
                    char *statistics = createMetadata(&md, fileDescriptor);
                    createStatisticFile(statisticsFileName, statistics);
                    free(statistics);
                    close(fileDescriptor);
                    break;
                }
            
            case 2: //directory
                {
                    getDirStats(path, &md);
                    char *statistics = createDirMetadata(&md);
                    createStatisticFile(statisticsFileName, statistics);
                    free(statistics);
                    break;
                }

            case 3: //symlink
                {
                    getSymlinkStats(path, &md);
                    char *statistics = createSymlinkMetadata(&md);
                    createStatisticFile(statisticsFileName, statistics);
                    free(statistics);
                    break;
                }

            default:
                break;
        }

        free(path);
    }

    closedir(pDir);
    return 0;
}