#include "txt_clrs.h"

void print_text(const char *string, int color, _Bool bold) {
    if (bold) {
        switch (color) {
            default:
                break;

            // default
            case 0:
                printf("\033[1m");
                printf("%s",string);
                printf("\033[0m");
                break;

            // green
            case 1:
                printf("\033[1;32m");
                printf("%s",string);
                printf("\033[0m");
                break;

            // red
            case 2:
                fprintf(stderr,"\033[1;31m");
                fprintf(stderr,"%s",string);
                fprintf(stderr,"\033[0m");
                break;

            // blue
            case 3:
                printf("\033[1;34m");
                printf("%s",string);
                printf("\033[0m");
                break;

            // yellow
            case 4:
                printf("\033[1;33m");
                printf("%s",string);
                printf("\033[0m");
                break;

        }
    } else{
        switch (color) {
            default:
                break;

            // default
            case 0:
                printf("%s",string);
                break;

            // green
            case 1:
                printf("\033[0;32m");
                printf("%s",string);
                printf("\033[0m");
                break;

            // red
            case 2:
                fprintf(stderr,"\033[0;31m");
                fprintf(stderr,"%s",string);
                fprintf(stderr,"\033[0m");
                break;

            // blue
            case 3:
                printf("\033[0;34m");
                printf("%s",string);
                printf("\033[0m");
                break;

            // yellow
            case 4:
                printf("\033[0;33m");
                printf("%s",string);
                printf("\033[0m");
                break;
        }
    }

}
