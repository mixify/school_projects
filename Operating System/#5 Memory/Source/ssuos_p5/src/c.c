#include <stdio.h>
struct sibal{
    int a:1;
    int b:16;
    int c:23;
};
int main(int argc, char *argv[])
{
    printf("size = %u\n", sizeof(struct sibal));
    return 0;
}
