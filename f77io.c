/* Sequential Read and Write binary file with Fortran77 unformatted form
 * r77rw.c
 * 02-Mar-2005 Keisuke Nishida
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif 

#include "f77io.h"

/* for non-windows */
#ifndef O_BINARY
#define O_BINARY 0x0
#endif

/* file permission */
#define PMODE S_IREAD | S_IWRITE

static int error(const char *fmt, ...);



/* Open or create a file
 * int f77open(const char *file, unsigned int flag)
 *
 *  file: specify file name or NULL (ignored in scratch status)
 *  flag: specify STATUS, POSITION, ACCESS, FORM, ACTION
 *
 *  return: scucess: file hundle (positive)
 *          failure: -1
*/
int f77open(const char *file, unsigned int flag)
{
	int fh;
	int oflag = O_BINARY;	/* binary mode on windows */
	char buf[512];
	
	if (flag & FORM_FORMATTED)
		error("f77open: FORMATTED FORM is not supported.");
	
	if (flag & ACCESS_DIRECT)
		error("f77open: DIRECT ACCESS is not supported.");
	
	
	if (flag & ACTION_READ) {
		if (flag & STATUS_SCRATCH)
			error("f77open: SCRATCH status for READ action is not supported.");
		
		if (flag & STATUS_NEW)
			error("f77open: NEW status for READ action is not supported.");
		
		if (flag & STATUS_REPLACE)
			error("f77open: REPLACE status for READ action is not supported.");
	}
	
	/* if STATUS_SCRATCH && !_WIN32, call mkstemp(3) and return */
#ifndef _WIN32
	if (flag & STATUS_SCRATCH) {
		strcpy (buf, "tmp.FXXXXXX");
		fh = mkstemp(buf);
		unlink(buf);
		return fh;
	}
#endif	/* !_WIN32 */
	
	/* getfilename */
	if (flag & STATUS_SCRATCH || file == NULL || file[0] =='\0') {
		/* create new file name */
		strcpy (buf, "tmp.FXXXXXX");
		if (mktemp(buf) == NULL)
			return -1;	/* fail to create new file name */
		
	} else {
		if (strlen(file) > sizeof(buf) - 1)
			error("f77open: path is too long");
		
		strcpy(buf, file);
	}
	
	switch (flag & _ACTION) {
		case ACTION_READWRITE:
			oflag |= O_RDWR;
			break;
		case ACTION_READ:
			oflag |= O_RDONLY;
			break;
		case ACTION_WRITE:
			oflag |= O_WRONLY;
			break;
		default:
			error("f77open: Unsupported ACTION is specified.");
			break;
	}
	
	switch (flag & _STATUS) {
		case STATUS_UNKNOWN:
			oflag |= O_CREAT;
			break;
		case STATUS_OLD:
			/* oflag |= 0; */
			break;
		case STATUS_NEW:
			oflag |= O_CREAT | O_EXCL;
			break;
		case STATUS_REPLACE:
			oflag |= O_CREAT | O_TRUNC;
			break;
#ifdef _WIN32
		case STATUS_SCRATCH:
			oflag |= O_CREAT | O_TEMPORARY;
			break;
#endif	/* _WIN32 */
		default:
			error("f77open: Unsupported STATUS is specified.");
			break;
	}
	
	fh = open(buf, oflag, PMODE);
	
	if (fh == -1)
		return -1;
	
	/* seek */
	if (flag & POSITION_APPEND)
		lseek(fh, 0L, SEEK_END);
	else
		lseek(fh, 0L, SEEK_SET);
	
	return fh;

}



/* Close file stream
 * int f77close(int fh)
 *
 *  fh: file handle
 *  return: scucess: 0
 *          failure: EOF
*/

int
f77close(int fh)
{
	return close(fh);
}



/* Write binary file with Fortran77 unformatted form
 * int f77write(int fh, size_t n, size_t size1, size_t countx1, size_t county1, size_t countz1, void *data1, ...)
 * 
 *  fh: file handle
 *  n: number of data
 *  size: size in bytes of each item that has to be written (read)
 *  countx1: number of items in MAJOR  dimension, each one with a size of size bytes
 *           If data is normal variable, 1D array, or 2D array, then contx1 = 1.
 *  county1: number of items in MIDDLE dimension, each one with a size of size bytes
 *           If data is normal variable or 1D array, then conty1 = 1.
 *  countz1: number of items in MINOR  dimension, each one with a size of size bytes
 *  data: pointer to data
 *
 *  return: number of data witten (read)
 *
 *  In Fortran77 unformatted form, major and minor dimension are exchanged from C form.
 */

size_t
f77write(int fh, size_t n, ...)
{
	va_list args;
	size_t result;
	
	va_start(args, n);
	result = f77rw(WRITE, fh, n, args);
	va_end(args);
	
	return result;
}



/* Read binary file with Fortran77 unformatted form
 *
 * int f77write(int fh, size_t n, size_t size1, size_t countx1, size_t county1, size_t countz1, void *data1, ...)
 *
 *  parameters and return value are same as the f77write function
 *
 *  If n == 0 , then skip next record.
 */
 
size_t
f77read(int fh, size_t n, ...)
{
	va_list args;
	size_t result;
	
	va_start(args, n);
	result = f77rw(READ, fh, n, args);
	va_end(args);
	
	return result;
}



/* Read/Write binary file with Fortran77 unformatted form
 *
 * int f77rw(enum _RW mode, int fh, size_t n, size_t size1, size_t countx1, size_t county1, size_t countz1, void *data1, ...)
 *
 *  This function is called by f77write or f77read.
 *  If mode == READ && n == 0, then skip next record.
 */
 
size_t
f77rw(enum RW mode, int fh, size_t n, va_list args)
{
	static int (*rwfunc[])() = {read, write};
	static void *data[MAXARGS];
	static size_t elementsize[MAXARGS];
	static size_t countx1[MAXARGS], county1[MAXARGS], countz1[MAXARGS];
	size_t i, x, y, z;
	size_t tmpsize, recordsize = 0;
	
	if (n > MAXARGS)
		error("f77rw: too many arguments.");
	
	/*
	if (n < 0)
		error("f77rw: invalid n.");
	*/
	
	for (i = 0; i < n; i++) {
		elementsize[i] = va_arg(args, size_t);
		countx1[i] = va_arg(args, size_t);
		county1[i] = va_arg(args, size_t);
		countz1[i] = va_arg(args, size_t);
		recordsize += elementsize[i] * countx1[i] * county1[i] * countz1[i];
		data[i] = va_arg(args, void *);
	}
	
	/* read or write header */
	tmpsize = recordsize;
	if ((*rwfunc[mode])(fh, &tmpsize, sizeof(tmpsize)) != sizeof(tmpsize)
		|| (n != 0 && tmpsize != recordsize))	/* when mode == WRITE, always tmpsize == recordsize */
		
		error("f77rw: Invalid record length near 0x%X.", lseek(fh, 0L, SEEK_CUR));
	
	
	if (mode == READ && n == 0) {
		/* if mode == READ && n == 0, then skip record */
		recordsize = tmpsize;
		
		if (lseek(fh, (long)recordsize, SEEK_CUR) == -1L)
			error("f77rw: fail in fseek near 0x%X.", lseek(fh, 0L, SEEK_CUR));
		
	} else {
		/* read or write record data */
		for (i = 0; i < n; i++)
			for (z = 0; z < countz1[i]; z++)
				for (y = 0; y < county1[i]; y++)
					for (x = 0; x < countx1[i]; x++)
						if ((*rwfunc[mode])(
								fh, (char *)data[i] + ((x * county1[i] + y) * countz1[i] + z) * elementsize[i], elementsize[i]
							) != (signed int)elementsize[i])
							
							error("f77rw: Write error near 0x%X.", lseek(fh, 0L, SEEK_CUR));
	}
	
	/* read or write footer */
	tmpsize = recordsize;
	if ((*rwfunc[mode])(fh, &tmpsize, sizeof(tmpsize)) != sizeof(tmpsize) || tmpsize != recordsize)
		error("f77rw: Invalid record length near 0x%X.", lseek(fh, 0L, SEEK_CUR));
	
	return n;
}



/* Moves the record position to the beginning of the previous record
 * int f77backspace(int fh)
 *
 *  fh: file handle
 *
 *  return: 0
 */

int
f77backspace(int fh)
{
	size_t recordsize, tmpsize;
	
	/* get previous record size */
	if (lseek(fh, -(long)sizeof(size_t), SEEK_CUR) == -1L)
		error("f77backspace: fail in fseek near 0x%X.", lseek(fh, 0L, SEEK_CUR));
	
	if (read(fh, &recordsize, sizeof(recordsize)) != sizeof(recordsize))
		error("f77backspace: Invalid record length near 0x%X.", lseek(fh, 0L, SEEK_CUR));
	
	/* seek to beginnig of record */
	if (lseek(fh, -(long)(sizeof(size_t) * 2 + recordsize), SEEK_CUR) == -1L)
		error("f77backspace: fail in fseek near 0x%X.", lseek(fh, 0L, SEEK_CUR));
	
	/* test record size */
	if (read(fh, &tmpsize, sizeof(tmpsize)) != sizeof(tmpsize) || tmpsize != recordsize)
		error("f77backspace: Invalid record length near 0x%X.", lseek(fh, 0L, SEEK_CUR));
	
	if (lseek(fh, -(long)sizeof(size_t), SEEK_CUR) == -1L)
		error("f77backspace: fail in fseek near 0x%X.", lseek(fh, 0L, SEEK_CUR));
	
	return 0;
}



/* Sets the record position to the beginning of the file
 * void f77rewind(int fh)
 *
 *  fh: file handle
 */

void
f77rewind(int fh)
{
	lseek(fh, 0L, SEEK_SET);
}



/* Writes an end-of-file marker after the current record,
 * in fact chsize or ftruncate at current position.
 * int f77endfile(int fh)
 *
 *  fh: file handle
 *  retrun: scucess: 0
 *          failure: -1
 */
int
f77endfile(int fh)
{
	#ifdef _WIN32
	return chsize(fh, lseek(fh, 0L, SEEK_CUR));
	#else
	return ftruncate(fh, lseek(fh, 0L, SEEK_CUR));
	#endif
}



/* Print error message and exit
 *
 * Arguments are same as printf(3)
 */

static int
error(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	fprintf(stderr, "error: ");
	vfprintf(stderr, fmt, args);
	fprintf(stderr, "\n");
	va_end(args);
	exit(1);
	
	return 0;
}
