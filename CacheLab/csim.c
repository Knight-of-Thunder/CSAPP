#include "cachelab.h"
#include <stdio.h>



int main(int argc, char *argv[])
{
    parseArgs(argc, argv);

    initCache();



    FILE * fp;
    char buffer[1024];
    fp = fopen(args.t, "r");
    if(fp == NULL){
        perror("can not open trace file");
        return 1;
    }

    while (fgets(buffer, sizeof(buffer), fp))
    {
        excuteAccess(buffer);
    }
    
    printSummary(hits, misses, evictions);
    freeCache();
    return 0;
}
