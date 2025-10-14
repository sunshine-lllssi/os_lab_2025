#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "revert_string.h"

int main() {
    char test1[] = "a";
    char test2[] = "ab";
    char test3[] = "abcd";
    char test4[] = "";
    RevertString(test1); printf("%s\n", test1); // a
    RevertString(test2); printf("%s\n", test2); // ba
    RevertString(test3); printf("%s\n", test3); // cba
    RevertString(test4); printf("%s\n", test4); // (пусто)
    return 0;
}
