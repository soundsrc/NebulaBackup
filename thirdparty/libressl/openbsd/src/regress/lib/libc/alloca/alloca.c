/*	$OpenBSD: alloca.c,v 1.6 2003/09/02 23:52:16 david Exp $	*/

/*	Written by Michael Shalayeff, 2003, Public Domain.	*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int
main(int argc, char *argv[])
{
	char *q, *p;

	p = alloca(41);
	strlcpy(p, "hellow world", 41);

	q = alloca(53);
	strlcpy(q, "hellow world", 53);

	exit(strcmp(p, q));
}
