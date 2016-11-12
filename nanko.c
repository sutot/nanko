/*
 * Copyright (C) 2016 @sutot
 * Licensed under the Apache License, Version 2.0
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

/* 
 * exit status
 */
#define SUCCESS	0
#define FAILURE	1

/* 
 * define
 */
#define LINEBUF 4096
#define VERSION "0.1"
#define CREATEDATE "2016-11-12"

/* 
 * define and arguments struct
 */
#define OPT_UNKNOWN_OPTION	1
#define OPT_PATTERN_NOTHING	2
#define OPT_FILE_NOTHING	4
typedef struct {
	bool i;
	bool h;
	bool v;
	char *pattern;
	char **file;
	int  filecnt;
	int  result;
} CMD_OPTION;

/* 
 * get_cmd_option()
 * Get a structure that stores the argument.
 */
CMD_OPTION* get_cmd_option(void) {
	CMD_OPTION* opts;
	opts = malloc(sizeof(CMD_OPTION));
	/* initialize */
	opts->i = false;
	opts->h = false;
	opts->v = false;
	opts->pattern = NULL;
	opts->file = NULL;
	opts->filecnt = 0;
	opts->result = 0;

	return opts;
}

/* 
 * free_cmd_option()
 * Release a structure that stores the argument.
 */
void free_cmd_option(CMD_OPTION* opts) {
	if (opts == NULL) { return; }
	if (opts->file != NULL) { free(opts->file); }
	if (opts != NULL) { free(opts); }
}

/* 
 * isPipe()
 * if stdin is true.
 */
bool isPipe(void)
{
	if (isatty(fileno(stdin))) { return false; }
	return true;
}

/* 
 * isDir()
 * if directory is true.
 */
bool isDir(char* f)
{
	struct stat st;
	if (stat(f, &st) == 0) {
		if ((st.st_mode & S_IFMT) == S_IFDIR) { return true; }
	}
	return false;
}

/* 
 * check_opts()
 * Check arguments.
 */
bool check_opts(int argc, char** argv, CMD_OPTION *opts)
{
	int o;
	while ((o = getopt(argc, argv, "ihv")) != -1) {
		switch (o) {
			case 'i':
				opts->i = true;
				break;
			case 'h':
				opts->h = true;
				break;
			case 'v':
				opts->v = true;
				break;
			default:
				return false;
		}
	}
	/* pattern */
	if (optind < argc) {
		opts->pattern = argv[optind++];
	} else {
		opts->result += OPT_PATTERN_NOTHING;
	}
	/* set target filename(s) */
	if (optind < argc) {
		int i, n;
		n = argc - optind;
		opts->filecnt = n;
		opts->file = malloc(sizeof(char*) * n);
		for (i=0; i < n; i++) {
			opts->file[i] = argv[optind];
			optind++;
		}
	} else {
		opts->result += OPT_FILE_NOTHING;
	}

	return true;
}

/* 
 * usage()
 * Show usage.
 */
void usage(char* argv0)
{
	char* cmd = basename(argv0);
	char* help =
		"Usage: %s [OPTION] PATTERN FILE\n"
		"       stdout | %s [OPTION] PATTERN\n"
		"\n"
		"This program checks how many specified character strings exist.\n"
		"\n"
		"  -h display this help and exit\n"
		"  -i ignore case distinctions\n"
		"  -v output version information\n"
		"\n";
	printf(help, cmd, cmd);
}

/* 
 * version()
 * Show version.
 */
void version(char* argv0)
{
	char* cmd = basename(argv0);
	char* ver = "\n"
				"This is %s, version %s (%s)\n"
				"Copyright 2016, IT SUPPORT SAKURA CO., Ltd.\n"
				"\n"
				"Written by SUTO Takayuki\n";
	printf(ver, cmd, VERSION, CREATEDATE);
}

/* 
 * nanko()
 * Searches for the specified patttern
 * and returns the number of patterns.
 */
int nanko(FILE* fp, char* pattern, bool ignore)
{
	char buf[LINEBUF];
	char* pbuf = NULL;
	char* p;
	int n = 0;

	while (fgets(buf, LINEBUF,fp) != NULL) {
		/* read one line */
		if (strchr(buf,'\n') == NULL) {
			pbuf = malloc(strlen(buf) + 1);
			strcpy(pbuf, buf);
			while (fgets(buf, LINEBUF,fp) != NULL) {
				if (strchr(buf,'\n') == NULL) {
					pbuf = realloc(pbuf, strlen(pbuf) + strlen(buf) + 1);
					strcat(pbuf, buf);
				}
			}
			p = pbuf;
		} else {
			p = buf;
		}
		/* search pattern and count */
		while(p != NULL) {
			if (ignore) {
				p = strcasestr(p, pattern);
			} else {
				p = strstr(p, pattern);
			}
			if (p != NULL) {
				p = &p[strlen(pattern)];
				n++;
			}
		}
		/* free malloc/realloc buffer */
		if (pbuf != NULL) {
			free(pbuf);
			pbuf = NULL;
		}
	}

	return n;
}

/* 
 * main()
 */
int main(int argc, char* argv[])
{
	char *cmdname;
	FILE *fp = NULL;
	CMD_OPTION* opts;

	cmdname = basename(argv[0]);
	opts = get_cmd_option();

	if (!check_opts(argc, argv, opts)) {
		fprintf(stderr, "Try '%s -h' for more information.\n", cmdname);
		exit(FAILURE);
	}
	/* check stdin */
	if (isPipe()) { fp = stdin; }

	/* check arguments */
	if (opts->h) {
		usage(argv[0]);
		opts->result = 0;
	} else if (opts->v) {
		version(argv[0]);
		return SUCCESS;
	} else {
		if (opts->result & OPT_PATTERN_NOTHING) {
			fprintf(stderr,"%s : Please enter a PATTERN\n", cmdname);
			exit(FAILURE);
		}
		if (fp == NULL) {	/* not stdin */
			if ((opts->result & OPT_FILE_NOTHING)) {
				fprintf(stderr,"%s : Please enter a FILE\n", cmdname);
				exit(FAILURE);
			}
		} else {
			opts->result -= OPT_FILE_NOTHING;
		}
	}

	/* read file/stdin */
	int f, n;
	if (fp == NULL) {	/* file */
		for (f=0; f < opts->filecnt; f++) {
			fp = fopen(opts->file[f], "r");
			if (isDir(opts->file[f])) {
				printf("%s : Is a directory\n", opts->file[f]);
				continue;
			}
			if (fp == NULL) {
					printf("%s : %s\n", opts->file[f], strerror(errno));
			} else {
				n = nanko(fp, opts->pattern, opts->i);
				if (opts->filecnt == 1) {
					printf("%d\n", n);
				} else {
					printf("%s : %d\n", opts->file[f], n);
				}
			}
			if (fp != NULL) { fclose(fp); }
		}
		fp = NULL;
	} else {	/* stdin */
		n = nanko(fp, opts->pattern, opts->i);
		printf("%d\n", n);
	}

	free_cmd_option(opts);

	if (opts->result) { exit(FAILURE); }

	return SUCCESS;
}
