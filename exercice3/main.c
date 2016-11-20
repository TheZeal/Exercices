//
//  main.c
//  gls
//
//  Created by Zeal Iskander on 09/11/2016.
//  Copyright © 2016 Zeal Iskander. All rights reserved.
//  19

#define _BSD_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <grp.h>
#include <time.h>
#include <sys/time.h>
#include <pwd.h>
#include <unistd.h>
#ifdef __APPLE__
#include <sys/xattr.h>
#include <sys/acl.h>
#else
#include <sys/sysmacros.h>
#endif

#ifndef S_ISVTX
#define S_ISVTX __S_ISVTX
#endif

#define st_mtimespec_tv_sec st_mtime
//#define st_mtimespec_tv_sec st_mtimespec.tv_sec

char *strdup( const char *p )
{
    return strcpy( malloc(strlen(p)+1), p );
}

#define MAX_FILES   1000
#define MAX_DEPTH   1000

typedef enum {
    INODE_NUMBER = 0,
    MODE,
    CHILDREN_NUMBER,
    USER_NAME,
    GROUP_NAME,
    SIZE,
    DATE,
    NAME
} PrintType;

#ifdef __APPLE__
acl_entry_t foo;
#endif

int isText[8] = {0, 0, 0, 1, 1, 0, 0, 1};

int compare_strings(const void *s1, const void *s2)
{
    return strcmp(*((const char **)s1), *((const char **)s2));
}

char * format_inode(struct stat *stat)
{
    char buf[256];
    sprintf(buf, "%llu", (unsigned long long)stat->st_ino);
    return strdup(buf);
}

char * format_mode(struct stat *stat, char * path)
{
    mode_t mode = stat->st_mode;
    char buf[12];
    buf[0] = S_ISBLK(mode) ? 'b' : S_ISCHR(mode) ? 'c' : (S_ISDIR(mode)) ? 'd' : S_ISLNK(mode) ? 'l' : '-';
    buf[1] =  (mode & S_IRUSR) ? 'r' : '-';
    buf[2] = (mode & S_IWUSR) ? 'w' : '-';
    buf[3] = (mode & S_IXUSR) ? ((mode & S_ISUID) ? 's' : 'x') : (mode & S_ISUID) ? 'S' : '-';
    buf[4] = (mode & S_IRGRP) ? 'r' : '-';
    buf[5] = (mode & S_IWGRP) ? 'w' : '-';
    buf[6] = (mode & S_IXGRP) ? ((mode & S_ISGID) ? 's' : 'x') : (mode & S_ISGID) ? 'S' : '-';
    buf[7] = (mode & S_IROTH) ? 'r' : '-';
    buf[8] = (mode & S_IWOTH) ? 'w' : '-';
    buf[9] = (mode & S_IXOTH) ? (mode & S_ISVTX) ? 't' : 'x' : (mode & S_ISVTX) ? 'T' : '-';
#ifdef __APPLE__
    buf[10] = listxattr(path, NULL, 0, XATTR_NOFOLLOW)>0 ? '@' : !acl_get_entry(acl_get_link_np(path, ACL_TYPE_EXTENDED),
                                                                                ACL_FIRST_ENTRY, &foo) ? '+' : ' ';
#else
    buf[10] = '\0';
#endif
    buf[11] = '\0';
    return strdup(buf);
}

char * format_nlink(struct stat *stat)
{
    char buf[256];
    sprintf(buf, "%u", (unsigned int)stat->st_nlink);
    return strdup(buf);
}

char * format_user(struct stat *stat)
{
    return strdup(getpwuid(stat->st_uid)->pw_name);
}

char * format_creation_date(struct stat *stat)
{
    char buf[14];
    time_t now = time(NULL);
    struct tm time_info = *localtime(&stat->st_mtimespec_tv_sec);
    if( now - stat->st_mtimespec_tv_sec > (365 / 2) * 86400)
    {
        strftime(buf, 14, "%b %e  %Y", &time_info);
    }
    else
    {
        strftime(buf, 14, "%b %e %R", &time_info);
    }
    return strdup(buf);
}

char * format_size(struct stat *stat)
{
    char buf[256];
    if(S_ISBLK(stat->st_mode) ||  S_ISCHR(stat->st_mode))
    {
        sprintf(buf, "% d,%4d", major(stat->st_rdev), minor(stat->st_rdev));
    }
    else
    {
        sprintf(buf, "%lld", (long long)stat->st_size);
    }
    return strdup(buf);
}


char * format_group(struct stat *stat)
{
    return strdup(getgrgid(stat->st_gid)->gr_name);
}

char * format_name(struct stat *stat, char *name)
{
    if(S_ISLNK(stat->st_mode))
    {
        char buf[512];
        long l = readlink(name, buf, sizeof(buf));
        if (l >= 0)
        {
            char lbuf[512];
            buf[l] = '\0';
            sprintf(lbuf, "%s -> %s", name, buf);
            return strdup(lbuf);
        }
    }
    return strdup(name);
}

struct file_description
{
    int i;
    char *fields[255];
    int dir;
};

void append(struct file_description *description, char *field)
{
    description->fields[description->i++] = field;
}

int isDir(struct file_description *description)
{
    return description->dir;
}

void setDir(struct file_description *description, int dir)
{
    description->dir = dir;
}

struct file_description * store_file_description(char *name, PrintType display_modes[], int display_mode_number, int *size)
{
    struct stat *buf = malloc(sizeof(struct stat));
    lstat(name, buf);
    (*size)+= buf->st_blocks;
    char * ret;
    struct file_description *description = malloc(sizeof(struct file_description));
    description->i = 0;
    for(int i = 0; i!= display_mode_number; i++)
    {
        switch (*display_modes++) {
            case INODE_NUMBER:
                ret = format_inode(buf);
                break;
            case MODE:
                ret = format_mode(buf, name);
                break;
            case CHILDREN_NUMBER:
                ret = format_nlink(buf);
                break;
            case DATE:
                ret = format_creation_date(buf);
                break;
            case GROUP_NAME:
                ret = format_group(buf);
                break;
            case NAME:
                ret = format_name(buf, name);
                break;
            case SIZE:
                ret = format_size(buf);
                break;
            case USER_NAME:
                ret = format_user(buf);
                break;
            default:
                break;
        }
        append(description, ret);
    }
    free(buf);
    setDir(description, S_ISDIR(buf->st_mode));
    return description;
}


int max(int a, int b)
{
    return a>b?a:b;
}

int get_max_size(struct file_description *descriptions[], int n_descriptions, int col)
{
    int max_size = -1;
    for(int i = 0; i!= n_descriptions; i++)
    {
        max_size = max(max_size,(int)strlen(descriptions[i]->fields[col]));
    }
    return max_size;
}

void display_description(struct file_description *description, int maxSize[], PrintType display_modes[], int col_number)
{
    for(int i = 0; i!= col_number; i++)
    {
        if (i == col_number -1)
        {
            printf("%s\n", description->fields[i]);
            return;
        }
        int len_diff = maxSize[i] - (int)strlen(description->fields[i]);
        if(isText[display_modes[i]])
        {
            printf("%s", description->fields[i]);
            printf("%*s ", len_diff + 1, "");
        }
        else
        {
            printf("%*s", len_diff, "");
            printf("%s ", description->fields[i]);
        }
    }
}

void free_description(struct file_description *description, int col_number)
{
    for(int i = 0; i!= col_number; i++)
    {
        free(description->fields[i]);
    }
    free(description);
}

int printdir(const char * argv[], char * path[] , int depth)
{
    int result = EXIT_FAILURE;
    DIR *d = opendir(".");
    if (!d)
    {
        result = EXIT_SUCCESS;
        printf( "%s: .: Permission denied\n", argv[0] );
        goto denied;
    }
    struct dirent *sd;
    char *f_names[MAX_FILES];
    struct file_description *descriptions[MAX_FILES];
    PrintType display_modes[] = {INODE_NUMBER, MODE, CHILDREN_NUMBER, USER_NAME, GROUP_NAME, SIZE, DATE, NAME};
    int n_files = 0;
    int n_descriptions = 0;
    while ((sd = readdir(d)))
    {
        if (sd->d_name[0] != '.')
        {
            f_names[n_files++] = strdup(sd->d_name);
        }
    }
    f_names[n_files] = NULL;
    qsort(f_names, n_files, sizeof(char *), compare_strings);
    char **p = f_names;
    int total_size = 0;
    while (*p)
    {
        descriptions[n_descriptions++] = store_file_description(*p++, display_modes, 8, &total_size);
    }
    result = EXIT_SUCCESS;
    int maxSize[8];
    for(int i = 0; i!= 8; i++)
    {
        maxSize[i] = get_max_size(descriptions, n_descriptions, i);
    }
    if(n_files != 0 )
        printf("total %i\n", total_size);
    for(int i = 0; i!= n_descriptions; i++)
    {
        display_description(descriptions[i], maxSize, display_modes, 8);
    }
    for (int i = 0; i!= n_descriptions; i++)
    {
        if(isDir(descriptions[i]))
        {
            free_description(descriptions[i], 8);
            printf("\n.");
            path[depth] = f_names[i];
            for(int j = 1; j!= depth+1; j++)
            {
                printf("/%s", path[j]);
            }
            printf(":\n");
            int res = chdir(f_names[i]);
            if(res != -1)
            {
                printdir(argv, path, depth + 1);
                chdir("..");
            }
            else
            {
                printf("%s: %s: Permission denied\n", argv[0], f_names[i]);
            }
        }
        else
        {
            free_description(descriptions[i], 8);
        }
    }
fail:
    p = f_names;
    while (*p)
    {
        free(*p++);
    }
    closedir(d);
denied:
    return result;
}


int main(int argc, const char * argv[])
{
    static char *path[MAX_DEPTH];
    printdir(argv, path, 1);
}



