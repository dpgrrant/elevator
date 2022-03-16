#include <unistd.h>

int main() {
    access("/usr", F_OK);
    getgid();
    getpid();
    char cwd[256]="";
    getcwd(cwd, sizeof(cwd));
    return 0;
}

