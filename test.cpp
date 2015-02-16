#include <stdio.h>
int add(int a, int b) {
    return a + b;
}
int main(int argc, char const* argv[])
{
    fprintf(stderr, "%d\n", add(10, 20));
    return 0;
}
