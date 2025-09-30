#include "swap.h"

void Swap(char *a, char *b)
{
	char temp = *a;
	*a = *b;
	*b = temp;
}

