#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define READSIZE	1024

#define MAX(a, b)	((a) > (b) ? (a) : (b))

static void	usage(void);
static void	dump_reader(int fd, const char *path, int jammed);
static void	simple_reader(int fd, const char *path, int jammed);

int
main(int argc, char *argv[])
{
	int c, i;
	int fd, nfds, ready;
	fd_set fds, rfds;
	char **pathv;
	void (*reader)(int, const char *, int);

	reader = simple_reader;
	while ((c = getopt(argc, argv, "ds")) != -1)
		switch (c) {
		case 's':
			reader = simple_reader;
			break;
		case 'd':
			reader = dump_reader;
			break;
		case '?':
		default:
			usage();
			/* NOTREACHED */
		}
	argc -= optind;
	argv += optind;
	if (argc < 1)
		usage();

	nfds = 0;
	FD_ZERO(&fds);
	pathv = NULL;
	for (; *argv != NULL; argv++) {
		if ((fd = open(*argv, O_RDONLY | O_NOCTTY)) == -1)
			err(EXIT_FAILURE, "%s", *argv);
		nfds = MAX(fd+1, nfds);
		FD_SET(fd, &fds);

		if (!isatty(fd))
			warn("%s", *argv);

		if ((pathv = realloc(pathv, nfds * sizeof(*pathv))) == NULL)
			err(EXIT_FAILURE, "malloc");
		pathv[fd] = *argv;
	}

	while (/* CONSTCOND */ 1) {
		(void)memcpy(&rfds, &fds, sizeof(rfds));
		if ((ready = select(nfds, &rfds, NULL, NULL, NULL)) == -1)
			err(EXIT_FAILURE, "select");
		for (i = 0; i < nfds; i++)
			if (FD_ISSET(i, &rfds))
				(*reader)(i, pathv[i], ready > 1);
	}

	return -1;
}

static void
dump_reader(int fd, const char *path, int jammed)
{
	ssize_t nread;
	char buf[READSIZE];
	int i;

	if ((nread = read(fd, buf, sizeof(buf))) == -1)
		err(EXIT_FAILURE, "%s", path);

	(void)printf("%s%s (%zd):\t", jammed ? "[JAM]" : "", path, nread);
	for (i = 0; i < nread; i++)
		printf("%02X ", buf[i] & 0xFF);
	putchar('\n');
}

static void
simple_reader(int fd, const char *path, int jammed)
{
	ssize_t nread;
	char buf[READSIZE];
	int i;

	if ((nread = read(fd, buf, sizeof(buf))) == -1)
		err(EXIT_FAILURE, "%s", path);

	(void)printf("%s%s (%zd):\n", jammed ? "[JAM]" : "", path, nread);
	for (i = 0; i < nread; i++)
		putchar(buf[i]);
	putchar('\n');
}

static void
usage(void)
{
	(void)fprintf(stderr, "usage: showtoku line [...]\n");
	exit(EXIT_FAILURE);
}
