#include "stdio.h"

int add(int a, int b, int c)
{
	return a + b + c;
}

void invoke()
{
	int num = add(1, 2, 3);
	printf("num = %d\n", num);
}

int main(void)
{
	invoke();
	return 0;
}
