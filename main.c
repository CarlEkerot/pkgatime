#include <alpm.h>
#include <alpm_list.h>
#include <stdio.h>
#include <linux/limits.h>
#include <sys/stat.h>
#include <time.h>
#include <string.h>
#include "main.h"
#include "slist.h"

pkgatime_t* get_pkg_stat(alpm_pkg_t *pkg)
{
	alpm_filelist_t*	filelist;
	alpm_file_t		file;
	struct stat		buf;
	size_t			i;
	char			filename[PATH_MAX] = {0};
	pkgatime_t*		pkgatime;

	filelist = alpm_pkg_get_files(pkg);
	if (filelist->count == 0) {
		return NULL;
	}

	pkgatime = malloc(sizeof(pkgatime_t));
	pkgatime->time = 0; // Not optimal, but... working?
	pkgatime->pkgname = alpm_pkg_get_name(pkg);

	for(i = 0; i < filelist->count; i++) {
		file = filelist->files[i];
		snprintf(filename, PATH_MAX, "/%s", file.name);

		stat(filename, &buf);
		if (!S_ISREG(buf.st_mode)) {
			continue;
		}

		if (pkgatime->time < buf.st_atime) {
			pkgatime->time = buf.st_atime;
			strncpy(pkgatime->filename, filename, PATH_MAX - 1);
		}
	}

	return pkgatime;
}

int main(int argc, char** argv)
{
	alpm_handle_t*		handle;
	alpm_db_t*		db;
	alpm_list_t*		pkglist;
	alpm_pkg_t*		pkg;
	enum _alpm_errno_t	err;
	pkgatime_t*		pkgatime;
	slist_t*		l = NULL;
	slist_t*		p;
	size_t			i;
	unsigned long int	numpkgs = 20;
	size_t			listlen;
	int			exit_code = 0;

	if (argc > 1) {
		numpkgs = strtoul(argv[1], NULL, 10);
	}

	if ((handle = alpm_initialize("/", "/var/lib/pacman", &err)) == NULL) {
		fprintf(stderr, "Error initializing alpm handle: %s\n",
			alpm_strerror(err));
		exit(EXIT_FAILURE);
	}

	db = alpm_get_localdb(handle);

	pkglist = alpm_db_get_pkgcache(db);
	printf("Number of installed packages: %zu\n", alpm_list_count(pkglist));

	for (; pkglist; pkglist = alpm_list_next(pkglist)) {
		pkg = pkglist->data;
		pkgatime = get_pkg_stat(pkg);
		if (pkgatime == NULL) {
			continue;
		}
		slist_insert(&l, pkgatime->time, pkgatime);
	}

	listlen = slist_length(l);
	if (numpkgs > listlen) {
		fprintf(stderr, "Trying to print too many packages: %lu > %zu\n",
			numpkgs,
			listlen);
		exit_code = EXIT_FAILURE;
	} else {
		p = l;
		printf("Listing %lu least commonly used packages:\n", numpkgs);
		printf("%32s\t%s\n", "Package", "Last used");
		for (i = 0; i < numpkgs; i++) {
			print_pkg(p->data);
			p = p->succ;
		}
	}

	p = l;
	for (i = 0; i < listlen; i++) {
		pkgatime = p->data;
		free(pkgatime);
		p = p->succ;
	}
	free_slist(&l);
	alpm_release(handle);

	return exit_code;
}

void print_pkg(pkgatime_t* pkgatime)
{
	printf("%32s\t%s",
		pkgatime->pkgname,
		ctime(&pkgatime->time));
}
