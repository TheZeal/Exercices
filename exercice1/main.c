//
//  main.c
//  gls
//
//  Created by Zeal Iskander on 09/11/2016.
//  Copyright © 2016 Zeal Iskander. All rights reserved.
//  19

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>

int compare_strings(const void *s1, const void *s2)
{
    return strcmp(*((const char **)s1), *((const char **)s2));
}
#define MAX_FILES   1000000

int main(int argc, const char * argv[])
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
    static char *f_names[MAX_FILES];
    int n_files = 0;
    while ((sd = readdir(d)))
    {
        if (sd->d_name[0] != '.')
        {
            char *p = malloc(sd->d_namlen + 1);
            if (!p)
                goto fail;
            f_names[n_files++] = strcpy(p, sd->d_name);
        }
    }
    qsort(f_names, n_files, sizeof(char *), compare_strings);
    char **p = f_names;
    while (*p)
    {
        printf("%s\n", *p++);
    }
    result = EXIT_SUCCESS;
    
fail:
    p = f_names;
    while (*p)
    {
        free(*p++);
    }
denied:
    return result;
}
