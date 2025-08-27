#include <err.h>
#include <fcntl.h>
#include <dirent.h>
#include <string.h>
#include <sys/errno.h>

#include "../../external/cbs/cbs.c"

#define MAXBUILTINS 50

int main(void) {
	int listfd, l;
	DIR *dir;
	char *src[MAXBUILTINS + 2 + 1], **p;
	struct dirent *entry;

	build("./");

	if ((listfd = open("list.c", O_WRONLY | O_CREAT | O_TRUNC, 0644)) == -1)
		err(EXIT_FAILURE, "Unable to open/create `list.c'");
	if (!(dir = opendir("./")))
		err(EXIT_FAILURE, "Unable to open current directory");

	dprintf(listfd, "#include <stddef.h>\n\n#include \"builtin.h\"\n"
	        "#include \"list.h\"\n\n");

	p = src;
	errno = 0;
	while ((entry = readdir(dir))) {
		if (strcmp(entry->d_name, "build.c") == 0
		    || !(*p = strrchr(entry->d_name, '.')) || strcmp(*p, ".c") != 0)
			continue;
		if (!(*p = strdup(entry->d_name)))
			err(EXIT_FAILURE, "Unable to duplicate directory entry");
		(*p)[strlen(*p) - 2] = '\0';
		if (p - src == 2 + MAXBUILTINS + 1)
			errx(EXIT_FAILURE, "Unable to add %s built-in, maximum reached (%d)",
			     *p, MAXBUILTINS);
		if (strcmp(*p, "builtin") != 0 && strcmp(*p, "list") != 0)
			dprintf(listfd, "extern BUILTIN(%s);\n", *p);
		++p;
	}
	if (errno) err(EXIT_FAILURE, "Unable to read from current directory");

	*p = "struct builtin builtins[] = {";
	l = (int)strlen(*p);
	dprintf(listfd, "\n%s", *p);
	*p = NULL;
	for (p = src; *p; ++p)
		if (strcmp(*p, "builtin") != 0 && strcmp(*p, "list") != 0)
			dprintf(listfd, "{\"%s\", %s},\n%*s", *p, *p, l, "");
	dprintf(listfd, "{NULL}};\n");

	if (closedir(dir) == -1)
		err(EXIT_FAILURE, "Unable to close current directory");
	if (close(listfd) == -1) err(EXIT_FAILURE, "Unable to close `list.c'");

	cflags = LIST("-I../");
	for (p = src; *p; ++p) compile(*p);
	load('s', "../builtins", src);

	for (p = src; *p; ++p) free(*p);

	return EXIT_SUCCESS;
}
