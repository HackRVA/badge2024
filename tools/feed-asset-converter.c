#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <libgen.h>

/* This program is meant to help you construct the yaml file that the stupid
   asset converter seems to need.
 */

__attribute__((noreturn)) void usage(char *programname)
{
	fprintf(stderr, "usage: %s directory app-name\n", programname);
	fprintf(stderr, "\n\nThis program is meant to help you create the ridiculous YAML file\n");
	fprintf(stderr, "that asset_converter.py insists upon having.\n");
	fprintf(stderr, "\nIf you have a directory full of png files, mypngdir, you can run:\n\n");
	fprintf(stderr, "    ./feed_asset_converter mypngdir myapp > myoutput.yaml\n\n");
	fprintf(stderr, "and it will spit out the necessary YAML.\n");
	fprintf(stderr, "It assumes you want 8-bits per pixel of color information.\n");
	fprintf(stderr, "If you supply a 3rd argument, it will use that for the bits\n");
	fprintf(stderr, "per color.  It does assume all files in the directory should\n");
	fprintf(stderr, "use the same bits per color value.  If you need multiple, then\n");
	fprintf(stderr, "you'll have to manually edit the YAML file.\n");
	exit(1);
}

static int dir_filter(const struct dirent *entry)
{
	return (!!strstr(entry->d_name, ".png"));
}

int main(int argc, char *argv[])
{
	int rc;
	struct dirent **namelist;
	int bits = 8;

	if (argc < 3) {
		usage(argv[0]);
		__builtin_unreachable();
	}

	if (argc >= 4) {
		int rc = sscanf(argv[3], "%d", &bits);
		if (rc != 1) {
			fprintf(stderr, "Failed to parse '%s' as an integer\n", argv[3]);
			usage(argv[0]);
		}
	}

	char *directory = argv[1];

	rc = chdir(directory);
	if (rc) {
		fprintf(stderr, "Cannot chdir to '%s': %s\n", directory, strerror(errno));
		exit(1);
	}

	int n = scandir(".", &namelist, dir_filter, alphasort); 
	if (n < 0) {
		fprintf(stderr, "scandir failed: %s\n", strerror(errno));
		exit(1);
	}

	printf("name: %s\n", argv[2]);
	printf("images:\n");

	for (int i = 0; i < n; i++) {
		char *filename = strdup(namelist[i]->d_name);
		char *c = strstr(filename, ".png");
		if (c)
			*c = '\0';
		printf("  - name: \"%s\"\n", filename);
		printf("    filename: \"%s\"\n", namelist[i]->d_name);
		printf("    bits: %d\n", bits);
		free(filename);
	}

	for (int i = 0; i < n; i++)
		free(namelist[i]);
	free(namelist);
	return 0;
}
