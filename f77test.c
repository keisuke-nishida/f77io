#include <stdio.h>
#include "f77io.h"

int main(int argc, char *argv[])
{
	int fh;
	int a = 1;
	int b = 3;
	int c = 9;
	int d[2][3][4] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24};
	int e = 0;
	
		
	fh = f77open(NULL, STATUS_SCRATCH);
	printf("fh: %d\n", fh);
	e = f77write(fh, 4, P0(a), P0(b), P0(c), P3(d, 2, 3, 4));
	e = f77write(fh, 4, P0(a), P0(b), P0(c), P3(d, 2, 3, 4));
	printf("%d, %d, %d, %d, %d, %d, %d, %d, %d\n", a,b,c, d[0][0][0], d[0][1][1], d[0][2][2], d[1][0][3], d[1][1][0], d[1][2][1]);
	f77backspace(fh);
	/* e = f77read(fh, 4, P0(a), P0(b), P0(c), P3(d, 2, 3, 4)); */
	e = f77read(fh, 0);
	f77backspace(fh);
	f77write(fh, 1, P0(a));
	f77endfile(fh);
	printf("%d, %d, %d, %d, %d, %d, %d, %d, %d\n", a,b,c, d[0][0][0], d[0][1][1], d[0][2][2], d[1][0][3], d[1][1][0], d[1][2][1]);

	f77close(fh);
	return 0;
}
