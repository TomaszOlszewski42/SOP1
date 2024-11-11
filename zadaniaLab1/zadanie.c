#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

enum control{
    CREATE,
    INSTALL,
    DEINSTALL
};

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

int main(int argc, char **argv)
{
    if(argc < 2)
        ERR("wrong arguments");
    int c;
    char** environ_name;
    environ_name = (char**)malloc(sizeof(char*)*argc/2);
    for(int i = 0; i < argc/2; i++)
    {
        environ_name[i] = NULL;
    }
    int environ_num = 0;
    char* pack_name;
    char* pack_version;
    int option = -1;
    while ((c = getopt(argc, argv, "cv:i:r:")) != -1)
        switch (c)
        {
            case 'c':
                if(option != -1)
                    ERR("wrong arguments");
                option = CREATE;
                break;
            case 'v':
                environ_name[environ_num] = malloc(strlen(optarg)+1);
                strcpy(environ_name[environ_num],optarg);
                environ_name[environ_num][strlen(optarg)] = '\0';
                environ_num++;
                break;
            case 'i':
                if(option != -1)
                    ERR("wrong arguments");
                option = INSTALL;
                if(strstr(optarg,"==") == NULL)
                    ERR("package name error");
                pack_name = strtok(optarg,"==");
                pack_version = strtok(NULL,"==");
                if (strtok(NULL,"==") != NULL)
                    ERR("error in package name");
                break;
            case 'r':
                if(option != -1)
                    ERR("wrong arguments");
                option = DEINSTALL;
                pack_name = malloc(strlen(optarg)+1);
                sprintf(pack_name,"%s",optarg);
                break;
            case '?':
            default:
                ERR("wrong arguments");
        }

    if(option == -1)
        ERR("something went wrong");

    if(option == CREATE)
    {
        for(int i = 0; i < environ_num;i++)
        {
            if(-1 == mkdir(environ_name[i],0744))
                ERR("mkdir");
            char path[strlen(environ_name[i])+4+strlen("requirements")];
            sprintf(path,"./%s/requirements",environ_name[i]);
            FILE* f = fopen(path,"w+");
            fclose(f);
        }
    }
    if(option == INSTALL)
    {
        for(int i = 0; i < environ_num; i++)
        {
            char* path = malloc(strlen(environ_name[i])+4+strlen("requirements"));
            sprintf(path,"./%s/requirements",environ_name[i]);
            FILE* f;
            if((f = fopen(path,"r+")) == NULL)
                ERR("nonexisting environment");
            char* line = NULL;
            size_t n = 0;
            int found = 0;
            while(-1 != getline(&line,&n,f))
                if(strstr(line,pack_name) != NULL)
                {
                    fclose(f);
                    free(line);
                    free(path);
                    fprintf(stderr,"%s already installed in %s\n",pack_name,environ_name[i]);
                    found = 1;
                    break;
                }
            if(found == 1) continue;
            fseek(f,0,SEEK_END);
            fprintf(f,"%s %s\n",pack_name,pack_version);
            fclose(f);
            if(path)
                free(path);
            path = malloc(strlen(environ_name[i])+4+strlen(pack_name));
            sprintf(path,"./%s/%s",environ_name[i],pack_name);
            make_file(path,256,0444);
            if(path)
                free(path);
            if(line)
                free(line);
        }
    }
    if(option == DEINSTALL)
    {
        for(int i = 0; i < environ_num; i++)
        {
            char* path = malloc(strlen(environ_name[i])+4+strlen("requirements"));
            sprintf(path,"./%s/requirements",environ_name[i]);
            FILE* source;
            if((source = fopen(path,"r")) == NULL)
                ERR("nonexisting environment");
            char* line = NULL;
            size_t n = 0;
            int found = 0;
            while(-1 != getline(&line,&n,source))
                if(strstr(line,pack_name) != NULL)
                {
                    found = 1;
                    break;
                }
            if(found != 1)
            {
                fclose(source);
                if(line)
                    free(line);
                if(path)
                    free(path);
                fprintf(stderr,"%s not installed in %s\n",pack_name,environ_name[i]);
                continue;
            }
            FILE* new_req;
            char* path2 = malloc(strlen(environ_name[i])+4+strlen("requirements2"));
            sprintf(path2,"./%s/requirements2",environ_name[i]);
            new_req = fopen(path2,"a");
            while(-1 != getline(&line,&n,source))
                if(strstr(line,pack_name) == NULL)
                {
                    fprintf(new_req,"%s",line);
                }
            unlink(path);
            rename(path2,path);
            fclose(source);
            fclose(new_req);
            if(line)
                free(line);
            if(path)
                free(path);
            if(path2)
                free(path2);
            path2 = malloc(strlen(environ_name[i])+4+strlen(pack_name));
            sprintf(path2,"./%s/%s",environ_name[i],pack_name);
            unlink(path2);
            if(path2)
                free(path2);
        }
        if(pack_name)
            free(pack_name);
    }

    if (argc > optind)
        ERR("wrong arguments");
    
    for(int i = 0; i < environ_num; i++)
    {
        if(environ_name[i])
            free(environ_name[i]);        
    }
    if(environ_name)
        free(environ_name);

    return EXIT_SUCCESS;
}