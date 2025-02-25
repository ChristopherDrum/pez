#include <stdio.h>

const char* origin = "\33[H";
const char* inverse = "\33[7m";
char spaces[] =  "                                               ";
char* location = "West of House";
char* score = "Score: 92  Moves:119";

int main(int argc, char* argv[]) {
    printf("\33[[2J");              //clear the screen
    printf("\33[?3l");              //80 column display
    printf("\33[2;24r");            //set scroll window to lines 2 - 24
    printf("%s%s %s%s%s \n", origin, inverse, location, spaces, score);

    printf("\33[0m");                //reset print attributes
    for (int i = 0; i < 30; i++) {
        printf("\33[2C%d\n", i);
    }
    printf("\33[10;20H");
    printf("OVERPRINT\n\n\n\n");
}