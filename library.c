#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>

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

void scan_dir(const char* path)
{
    //fprintf(stderr,"IN SCAN-DIR: %s\n",path);
    DIR *dirp;
    struct dirent *dp;
    struct stat filestat;
    if ((dirp = opendir(path)) == NULL)
        ERR("opendir");
    do
    {
        errno = 0;
        if ((dp = readdir(dirp)) != NULL)
        {
            char fullpath[MAX_PATH];
            snprintf(fullpath, MAX_PATH + 1, "%s/%s", path, dp->d_name);
            if(lstat(fullpath, &filestat) != 0)
                fprintf(stderr,"DUPA\n");
            char name[MAX_NAME];
            strcpy(name,dp->d_name);
            int size = filestat.st_size;
            //fprintf(stderr, "IN WHILE: %s\n",name);
            int found = 0;
            for(int i = 0; i < n; i++)
            {
                if(strstr(name,extensions[i]) != NULL && strcmp(&name[strlen(name) - strlen(extensions[i])],extensions[i]) == 0)
                    found = 1;
            }

            if(!S_ISDIR(filestat.st_mode) && (found == 1 || n == 0))
                fprintf(output,"%s %d\n",name,size);
        }
    } while (dp != NULL);

    if (errno != 0)
        ERR("readdir");
    if (closedir(dirp))
        ERR("closedir");
}

void make_file(char *name, ssize_t size, mode_t perms)
{
    FILE *s1;
    int i;
    umask(~perms & 0777);
    if ((s1 = fopen(name, "w+")) == NULL)
        ERR("fopen");
    for (i = 0; i < size; i++)
    {
        fprintf(s1, "%c", 'A' + (i % ('Z' - 'A' + 1)));
    }
    if (fclose(s1))
        ERR("fclose");
}