#include <stdio.h>

int main()
{

	char str[100];
	printf("sscanf return %d\n", (int)sscanf("\n", "%s", str));
	printf("str=%s\n", str);

}
