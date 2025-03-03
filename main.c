/*This is just a simple test to verify correctness of Cosmopolitan ABI
Just need a sanity check that they do in fact work, and that something else
is going wrong inside zap89*/

#include <stdio.h>
#include <time.h>

int main(int argc,char **argv) {
    time_t tt = time(NULL);
    struct tm *tms = localtime(&tt);

    printf("tt = %lld\n", tt);
    printf("%2d, %2d, %4d\n", tms->tm_mon+1, tms->tm_mday, 1900+tms->tm_year);

    return 0;
}