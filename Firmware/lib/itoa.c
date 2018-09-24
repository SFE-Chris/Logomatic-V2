/*
Print integers in a given base 2-16 (default 10)
*/
#include "itoa.h"

int itoa(int n, char str[], int b) {
	int i = convert(n, b, str, 0);
	str[i] = '\0';
	return i;
}

int convert(int n, int b, char str[], int i) {
	if (n/b > 0)
		i = convert(n/b, b, str, i);
	str[i++] = "0123456789ABCDEF"[n%b];
	return i;
}

