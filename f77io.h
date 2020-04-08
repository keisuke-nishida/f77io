/* Sequential Read and Write binary file with Fortran77 unformatted form
 * r77rw.h
 * 02-Mar-2005 Keisuke Nishida
 */

#include <stdarg.h>
#include <limits.h>

#ifndef _F77RW_H_
#define _F77RW_H_

#define MAXARGS 100

/* Macro for f77write and f77read
 * To write single variable a
 *   f77write(fp, 1, P0(a));
 * To read 2D array b[2][4]
 *   f77read(fp, 1, P2(b, 2, 4));
 */

/* For single variable */
#define P0(a)          sizeof(a),          1, 1, 1, &(a)
/* For 1D array */
#define P1(a, z)       sizeof(a[0]),       1, 1, z, a
/* For 2D array */
#define P2(a, y, z)    sizeof(a[0][0]),    1, y, z, a
/* For 3D array */
#define P3(a, x, y, z) sizeof(a[0][0][0]), x, y, z, a


#define _STATUS           0xf
#define _POSITION         0xf0
#define _FORM             0xf000
#define _ACTION           0xf0000

/* STATUS */
#define STATUS_UNKNOWN    0x0		/* default */
#define STATUS_OLD        0x1
#define STATUS_NEW        0x2
#define STATUS_REPLACE    0x4
#define STATUS_SCRATCH    0x8

/* POSITION */
#define POSITION_ASIS     0x00		/* default */
#define POSITION_REWIND   0x10
#define POSITION_APPEND   0x20

/* ACCESS */
#define _ACCESS           0xf00
#define ACCESS_SEQUENTIAL 0x000		/* default */
#define ACCESS_DIRECT     0x100

/* FORM */
#define FORM_UNFORMATTED  0x0000	/* default */
#define FORM_FORMATTED    0x1000

/* ACTION */
#define ACTION_READWRITE  0x00000	/* default */
#define ACTION_READ       0x10000
#define ACTION_WRITE      0x20000

enum RW {READ, WRITE};


/* functions */

int f77open(const char *file, unsigned int flag);
int f77close(int fh);
size_t f77write(int fh, size_t n, ...);
size_t f77read(int fh, size_t n, ...);
size_t f77rw(enum RW mode, int fh, size_t n, va_list args);
int f77backspace(int fh);
void f77rewind(int fh);
int f77endfile(int fh);

#endif /* !_F77RW_H_ */
