#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>

#define MAX_PATH 256
#define MAX_NAME 256
#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))


void scan_dir(FILE* output)
{
    DIR *dirp;
    struct dirent *dp;
    struct stat filestat;
    if ((dirp = opendir(".")) == NULL)
        ERR("opendir");
    fprintf(output,"LISTA PLIKOW:\n");
    do
    {
        errno = 0;
        if ((dp = readdir(dirp)) != NULL)
        {
            char name[MAX_NAME];
            strcpy(name,dp->d_name);
            lstat(dp->d_name, &filestat);
            int size = filestat.st_size;
            fprintf(output,"%s %d\n",name,size);
        }
    } while (dp != NULL);

    if (errno != 0)
        ERR("readdir");
    if (closedir(dirp))
        ERR("closedir");
}

int main(int argc, char **argv)
{
    char** paths;
    char path[MAX_PATH];
    FILE* output = stdout;
    if (getcwd(path, MAX_PATH) == NULL)
        ERR("getcwd");
    strcat(path,"\0");
    paths = (char**)malloc(sizeof(char*)*argc/2);
    int i;
    for(i = 0;i<argc/2;i++)
    {
        paths[i] = NULL;
    }
    int c;
    i = 0;
    while ((c = getopt(argc, argv, "p:o:")) != -1)
        switch (c)
        {
            case 'p':
                paths[i] = malloc(strlen(optarg) + 1);
                strcpy(paths[i],optarg);
                paths[i][strlen(optarg)] = '\0';
                i++;
                break;
            case 'o':
                output = fopen(optarg,"w+");
                break;
            case '?':
            default:
                ERR("wrong arguments");
        }
    if(i == 0)
        ERR("wrong arguments");
    for(i = 0; i < argc/2; i++)
    {
        if(paths[i] == NULL)
            break;
        if(chdir(paths[i]))
            ERR("chdir");
        fprintf(output,"SCIEZKA:\n%s\n",paths[i]);
        scan_dir(output);
        if (chdir(path))
            ERR("chdir");
    }
    for(i = 0; i < argc/2; i++)
    {
        if(paths[i])
            free(paths[i]);
    }
    if(paths)
        free(paths);

    if(output != stdout)
        fclose(output);

    return EXIT_SUCCESS;
}