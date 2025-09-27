#include <err.h>
#include <fcntl.h>
#include <dirent.h>
#include <string.h>
#include <sys/errno.h>

#include "../../external/cbs/cbs.c"

#define MAXBUILTINS 50

int main(void) {
	int listfd, d;
	DIR *dir;
	char *srcs[MAXBUILTINS + 2 + 1], **src, *decl;
	struct dirent *entry;

	build("./");

	if ((listfd = open("list.c", O_WRONLY | O_CREAT | O_TRUNC, 0644)) == -1)
		err(EXIT_FAILURE, "Unable to open/create `list.c'");
	if (!(dir = opendir("./")))
		err(EXIT_FAILURE, "Unable to open current directory");

	dprintf(listfd, "#include <stddef.h>\n\n#include \"builtin.h\"\n"
	        "#include \"list.h\"\n\n");

	src = srcs;
	errno = 0;
	while ((entry = readdir(dir))) {
		if (strcmp(entry->d_name, "build.c") == 0
		    || !(*src = strrchr(entry->d_name, '.')) || strcmp(*src, ".c") != 0)
			continue;
		if (!(*src = strdup(entry->d_name)))
			err(EXIT_FAILURE, "Unable to duplicate directory entry");
		(*src)[strlen(*src) - 2] = '\0';
		if (src - srcs == 2 + MAXBUILTINS + 1)
			errx(EXIT_FAILURE, "Unable to add %s built-in, maximum reached (%d)",
			     *src, MAXBUILTINS);
		if (strcmp(*src, "builtin") != 0 && strcmp(*src, "list") != 0)
			dprintf(listfd, "BUILTIN(%s);\n", *src);
		++src;
	}
	if (errno) err(EXIT_FAILURE, "Unable to read from current directory");
	*src = NULL;

	decl = "struct builtin builtins[] = {";
	d = (int)strlen(decl);
	dprintf(listfd, "\n%s", decl);
	for (src = srcs; *src; ++src)
		if (strcmp(*src, "builtin") != 0 && strcmp(*src, "list") != 0)
			dprintf(listfd, "{\"%s\", %s},\n%*s", *src, *src, d, "");
	dprintf(listfd, "{NULL}};\n");

	if (closedir(dir) == -1)
		err(EXIT_FAILURE, "Unable to close current directory");
	if (close(listfd) == -1) err(EXIT_FAILURE, "Unable to close `list.c'");

	cflags = LIST("-I../");
	for (src = srcs; *src; ++src) compile(*src);
	load('s', "../builtins", srcs);

	for (src = srcs; *src; ++src) free(*src);

	return EXIT_SUCCESS;
}
