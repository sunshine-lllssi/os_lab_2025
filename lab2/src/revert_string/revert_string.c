#include "revert_string.h"
#include <string.h>  

void RevertString(char *str)
{
	if (str == NULL) return;  // защита от NULL
    int len = strlen(str);
    int left = 0;
    int right = len - 1;
    while (left < right) {
        // Меняем местами str[left] и str[right]
        char temp = str[left];
        str[left] = str[right];
        str[right] = temp;
        left++;
        right--;
    }
}

