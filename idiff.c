/*
 * idiff - interactive diff
 * @(#)idiff.c	1.3 (Kernighan & Pike) 88/03/31
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <assert.h>

#define HUGE 1000000L

char *progname;		/* every program should have one of these */
const char *diffout = "/tmp/idiff.XXXXXX";
const char *tempfile = "/tmp/idiff.XXXXXX";

FILE *
efopen(const char *fname, const char *fmode)
{
	FILE *f = fopen(fname, fmode);
	if (f == NULL) {
		fprintf(stderr, "Unable to open %s mode %s\n", fname, fmode);
		exit(1);
	}
	return f;
}

/* Interrupt handler */
void
onintr(const int signum)
{
#ifndef DEBUG
	(void) unlink(tempfile);
	(void) unlink(diffout);
#endif	/* DEBUG */
	exit(1);
}


void
parse(char *s, int* pfrom1, int* pto1, int* pcmd, int* pfrom2, int* pto2)
{
#define a2i(p)	while (isdigit(*s)) p = 10 * (p) + *s++ - '0'

	*pfrom1 = *pto1 = *pfrom2 = *pto2 = 0;
	a2i(*pfrom1);
	if (*s == ',') {
		s++;
		a2i(*pto1);
	} else
		*pto1 = *pfrom1;
	*pcmd = *s++;
	a2i(*pfrom2);
	if (*s == ',') {
		s++;
		a2i(*pto2);
	} else
		*pto2 = *pfrom2;
}

void
nskip(FILE *fin, int n)	/* skip n lines of file fin */
{
	char buf[BUFSIZ];

	while (n-- > 0)
		/* check for EOF in case called with HUGE */
		if (fgets(buf, sizeof buf, fin) == NULL)
			return;
}

void
ncopy(FILE *fin, int n, FILE *fout)	/* copy n lines from fin to fout */
{
	char buf[BUFSIZ];

	assert(fin != NULL);
	assert(fout != NULL);

	while (n-- > 0) {
		if (fgets(buf, sizeof buf, fin) == NULL)
			return;
		fputs(buf, fout);
	}
}

/* process diffs */
void
idiff(FILE *f1, FILE *f2, FILE *fin, FILE *fout)
{
	char buf[BUFSIZ], buf2[BUFSIZ], *ed, *getenv();
	FILE *ft, *efopen();
	int cmd, n, from1, to1, from2, to2, nf1, nf2;

	assert(f1 != NULL);
	assert(f2 != NULL);
	assert(fin != NULL);
	assert(fout != NULL);

	(void) mkstemp(strdup(tempfile));
	if ((ed=getenv("EDITOR")) == NULL)
		ed = "/bin/ed";

	nf1 = nf2 = 0;
	while (fgets(buf, sizeof buf, fin) != NULL) {
		parse(buf, &from1, &to1, &cmd, &from2, &to2);
		n = to1-from1 + to2-from2 + 1; /* #lines from diff */
		if (cmd == 'c')
			n += 2;
		else if (cmd == 'a')
			from1++;
		else if (cmd == 'd')
			from2++;
		printf("%s", buf);
		while (n-- > 0) {
			(void) fgets(buf, sizeof buf, fin);
			printf("%s", buf);
		}
		do {
			printf("? ");
			(void) fflush(stdout);
			(void) fgets(buf, sizeof buf, stdin);
			switch (buf[0]) {
			case '>':
				nskip(f1, to1-nf1);
				ncopy(f2, to2-nf2, fout);
				break;
			case '<':
				nskip(f2, to2-nf2);
				ncopy(f1, to1-nf1, fout);
				break;
			case 'e':
				ncopy(f1, from1-1-nf1, fout);
				nskip(f2, from2-1-nf2);
				ft = efopen(tempfile, "w");
				ncopy(f1, to1+1-from1, ft);
				fprintf(ft, "---\n");
				ncopy(f2, to2+1-from2, ft);
				(void) fclose(ft);
				(void) sprintf(buf2, "%s %s", ed, tempfile);
				(void) system(buf2);
				ft = efopen(tempfile, "r");
				ncopy(ft, HUGE, fout);
				(void) fclose(ft);
				break;
			case '!':
				(void) system(buf+1);
				printf("!\n");
				break;
			case 'q':
				switch (buf[1]) {
					case '>' :
						nskip(f1, HUGE);
						ncopy(f2, HUGE, fout);
				/* this can fail on immense files */
						goto out;
						break;
					case '<' :
						nskip(f2, HUGE);
						ncopy(f1, HUGE, fout);
				/* this can fail on immense files */
						goto out;
						break;
				default:
					fprintf(stderr, 
			"%s: q must be followed by a < or a >!\n", progname);
					break;
			}
			break;
			default:
				fprintf(stderr,
					"%s: >, q>, <, q<, ! or e only!\n",
					progname);
				break;
			}
		} while (buf[0] != '<' && buf[0]  != '>' 
				&& buf[0] != 'q' && buf[0] != 'e');
		nf1 = to1;
		nf2 = to2;
	}
out:
	ncopy(f1, HUGE, fout);	/* can fail on very large files */
	(void) unlink(tempfile);
}

/* main method */
int
main(int argc, char	*argv[])
{
	FILE *fin, *fout, *f1, *f2;
	char cmdBuf[1024], *inname1, *inname2;
	int c, errflg = 0;
	extern int optind;
	extern char *optarg;
	int use_b = 0;		/* true --> use diff -b */

	progname = argv[0];

	while ((c = getopt(argc, argv, "b")) != EOF)
		switch (c) {
		case 'b':
			use_b = 1;
			break;
		case '?':
		default:
			errflg++;
			break;
		}
	if (errflg) {
		(void) fprintf(stderr, "usage: %s xxx [file] ...\n", progname);
		exit(2);
	}
	if (argc-optind != 2) {
		fprintf(stderr, "usage: idiff file1 file2\n");
		exit(1);
	}
	if (signal(SIGINT, SIG_IGN) != SIG_IGN)
		(void) signal(SIGINT, onintr);
	inname1 = argv[optind+0];
	inname2 = argv[optind+1];
	f1 = efopen(inname1, "r");
	f2 = efopen(inname2, "r");
	fout = efopen("idiff.out", "w");
	(void) mktemp(strdup(diffout));
	(void) sprintf(cmdBuf, "diff %s %s %s >%s", 
		use_b ? "-b" : "",
		inname1, inname2, diffout);
	(void) system(cmdBuf);
	fin = efopen(diffout, "r");
	idiff(f1, f2, fin, fout);
	(void) unlink(diffout);
	printf("%s output in file idiff.out\n", progname);
	exit(0);
}

