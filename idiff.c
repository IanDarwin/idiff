/*
 * idiff - interactive diff
 * @(#)idiff.c	1.3 (Kernighan & Pike) 88/03/31
 */

#include <stdio.h>
#include <ctype.h>
#include <signal.h>

#define HUGE 10000

char *progname;		/* every program should have one of these */
char *diffout = "/tmp/idiff.XXXXXX";
char *tempfile = "/tmp/idiff.XXXXXX";

main(argc, argv)
int	argc;
char	*argv[];
{
	FILE *fin, *fout, *f1, *f2, *efopen();
	char buf[BUFSIZ], *mktemp();
	extern onintr();

	progname = argv[0];
	if (argc != 3) {
		fprintf(stderr, "usage: idiff file1 file2\n");
		exit(1);
	}
	if (signal(SIGINT, SIG_IGN) != SIG_IGN)
		(void) signal(SIGINT, onintr);
	f1 = efopen(argv[1], "r");
	f2 = efopen(argv[2], "r");
	fout = efopen("idiff.out", "w");
	(void) mktemp(diffout);
	(void) sprintf(buf, "diff %s %s >%s", argv[1], argv[2], diffout);
	(void) system(buf);
	fin = efopen(diffout, "r");
	idiff(f1, f2, fin, fout);
	(void) unlink(diffout);
	printf("%s output in file idiff.out\n", progname);
	exit(0);
}

idiff(f1, f2, fin, fout)	/* process diffs */
FILE *f1, *f2, *fin, *fout;
{
	char buf[BUFSIZ], buf2[BUFSIZ], *mktemp(), *ed, *getenv();
	FILE *ft, *efopen();
	int cmd, n, from1, to1, from2, to2, nf1, nf2;

	(void) mktemp(tempfile);
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

nskip(fin, n)	/* skip n lines of file fin */
FILE *fin;
{
	char buf[BUFSIZ];

	while (n-- > 0)
		(void) fgets(buf, sizeof buf, fin);
}

ncopy(fin, n, fout)	/* copy n lines from fin to fout */
FILE *fin, *fout;
{
	char buf[BUFSIZ];

	while (n-- > 0) {
		if (fgets(buf, sizeof buf, fin) == NULL)
			return;
		fputs(buf, fout);
	}
}

parse(s, pfrom1, pto1, pcmd, pfrom2, pto2)
char *s;
int *pcmd, *pfrom1, *pto1, *pfrom2, *pto2;
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

onintr()
{
#ifndef DEBUG
	(void) unlink(tempfile);
	(void) unlink(diffout);
#endif	/* DEBUG */
	exit(1);
}

