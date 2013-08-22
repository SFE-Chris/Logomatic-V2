/*
Print integers in a given base 2-16 (default 10)
*/
//#include <stdio.h>
//#include <stdlib.h>

int convert(int n, int b, char str[], int i) {
	if (n/b > 0)
		i = convert(n/b, b, str, i);
	str[i++] = "0123456789ABCDEF"[n%b];
	return i;
}

int itoa(int n, int b, char str[]) {
	int i = convert(n, b, str, 0);
	str[i] = '\0';
	return i;
}

