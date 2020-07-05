#include <iostream>
#include <unistd.h>

int main(int argc, char *argv[]) {
    const char* optionString { "b:T:p:s:fd" };

    while (-1 != getopt(argc, argv, optionString)) {
        switch (optarg[0]) {
            case 'b':
                break;
            case 'T':
                break;
            case 'p':
                break;
            case 's':
                break;
            case 'f':
                break;
            case 'd':
                break;
            default:
                break;
        }
    }

    return 0;
}
