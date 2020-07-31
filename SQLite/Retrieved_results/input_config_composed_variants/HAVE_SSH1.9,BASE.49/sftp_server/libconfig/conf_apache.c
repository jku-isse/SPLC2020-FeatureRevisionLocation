#include "compat.h"
#include "libconfig.h"
#include "libconfig_private.h"
#include "conf_apache.h"


#include <ctype.h>



#include <stdio.h>



#include <string.h>



#include <sys/types.h>



#include <sys/stat.h>



#include <unistd.h>



#include <dirent.h>


static int lc_process_conf_apache_file(const char *configfile, const char *pathprefix);

static int lc_process_conf_apache_include(const char *pathname, const char *pathprefix) {
	struct stat pathinfo;
	struct dirent *dinfo = NULL;
	char includepath[1024] = {0};
	DIR *dh = NULL;
	int statret = -1, lcpcafret = -1;
	int retval = 0;

	statret = stat(pathname, &pathinfo);
	if (statret < 0) {
		return(-1);
	}

	if (S_ISDIR(pathinfo.st_mode)) {
		dh = opendir(pathname);
		if (dh == NULL) {
			return(-1);
		}

		while (1) {
			dinfo = readdir(dh);
			if (dinfo == NULL) {
				break;
			}

			/* Skip files that begin with a dot ('.') */
			if (dinfo->d_name[0] == '.') continue;

			snprintf(includepath, sizeof(includepath) - 1, "%s/%s", pathname, dinfo->d_name);
			lcpcafret = lc_process_conf_apache_include(includepath, pathprefix);
			if (lcpcafret < 0) {
				retval = -1;
				/* XXX: should we break here (abort further including of files from a directory if one fails ?) */
			}
		}

		closedir(dh);
	} else {
		lcpcafret = lc_process_conf_apache_file(pathname, pathprefix);
		if (lcpcafret < 0) {
			retval = -1;
		}
	}

	return(retval);
}

static int lc_process_conf_apache_file(const char *configfile, const char *pathprefix) {
	FILE *configfp = NULL;
	char linebuf[1024] = {0}, *linebuf_ptr = NULL, *tmp_ptr = NULL;
	char *lastsection = NULL;
	char qualifbuf[1024] = {0};
	char *cmd = NULL, *value = NULL, *sep = NULL, *cmdend = NULL;
	char *fgetsret = NULL;
	int lcpvret = -1, lpcafret = -1;
	int invalid_section = 0, ignore_section = 0;
	int retval = 0;
	lc_err_t save_lc_errno = LC_ERR_NONE;

	if (pathprefix != NULL) {
		/* Copy the prefix, if specified. */
		strncpy(qualifbuf, pathprefix, sizeof(qualifbuf) - 1);
	}

	if (configfile == NULL) {
		lc_errno = LC_ERR_INVDATA;
		return(-1);
	}

	configfp = fopen(configfile, "r");

	if (configfp == NULL) {
		lc_errno = LC_ERR_CANTOPEN;
		return(-1);
	}

	while (1) {
		fgetsret = fgets(linebuf, sizeof(linebuf) - 1, configfp);
		if (fgetsret == NULL) {
			break;
		}
		if (feof(configfp)) {
			break;
		}

		/* Remove trailing crap (but not spaces). */
		linebuf_ptr = &linebuf[strlen(linebuf) - 1];
		while (*linebuf_ptr < ' ' && linebuf_ptr >= linebuf) {
			*linebuf_ptr = '\0';
			linebuf_ptr--;
		}

		/* Remove leading spaces. */
		linebuf_ptr = &linebuf[0];
		while (*linebuf_ptr == ' ' || *linebuf_ptr == '\t') {
			linebuf_ptr++;
		}

		/* Handle section header. */
		if (linebuf_ptr[0] == '<' && linebuf_ptr[strlen(linebuf_ptr) - 1] == '>') {
			/* Remove < and > from around the data. */
			linebuf_ptr[strlen(linebuf_ptr) - 1] = '\0';
			linebuf_ptr++;

			/* Lowercase the command part of the section. */
			tmp_ptr = linebuf_ptr;
			while (*tmp_ptr != '\0' && *tmp_ptr != ' ') {
				*tmp_ptr = tolower(*tmp_ptr);
				tmp_ptr++;
			}

			/* If this is a close section command, handle it */
			if (linebuf_ptr[0] == '/') {
				linebuf_ptr++;
				cmd = linebuf_ptr; 

				/* Find the last section closed. */
				tmp_ptr = strrchr(qualifbuf, '.');
				if (tmp_ptr == NULL) {
					lastsection = qualifbuf;
					tmp_ptr = qualifbuf;
				} else {
					lastsection = tmp_ptr + 1;
				}

				if (strcmp(cmd, lastsection) != 0) {

					
					

					retval = -1;
					lc_errno = LC_ERR_BADFORMAT;

					/* For this error, we abort immediately. */
					break;
				}

				lcpvret = lc_process_var(qualifbuf, NULL, NULL, LC_FLAGS_SECTIONEND);
				if (lcpvret < 0) {

					

				}

				/* Remove the "lastsection" part.. */
				*tmp_ptr = '\0';

				/* We just sucessfully closed the last section opened,
				   we must be in a valid section now since we only open
				   sections from within valid sections. */
				invalid_section = 0;
				ignore_section = 0;

				continue;
			}
			/* Otherwise, open a new section. */

			/* Don't open a section from an invalid section. */
			if (invalid_section == 1 || ignore_section == 1) {
				continue;
			}

			/* Parse out any argument passed. */
			sep = strpbrk(linebuf_ptr, " \t");

			if (sep != NULL) {
				cmdend = sep;
				/* Delete space at the end of the command. */
				cmdend--; /* It currently derefs to the seperator.. */
				while (*cmdend <= ' ') {
					*cmdend = '\0';
					cmdend--;
				}

				/* Delete the seperator char and any leading space. */
				*sep = '\0';
				sep++;
				while (*sep == ' ' || *sep == '\t') {
					sep++;
				}
				value = sep;
			} else {
				/* XXX: should this be "" or NULL ? */
				value = "";
			}

			cmd = linebuf_ptr;

			if (qualifbuf[0] != '\0') {
				strncat(qualifbuf, ".", sizeof(qualifbuf) - strlen(qualifbuf) - 1);
			}
			strncat(qualifbuf, cmd, sizeof(qualifbuf) - strlen(qualifbuf) - 1);

			lcpvret = lc_process_var(qualifbuf, value, NULL, LC_FLAGS_SECTIONSTART);
			if (lcpvret < 0) {

				

				invalid_section = 1;
				lc_errno = LC_ERR_INVSECTION;
				retval = -1;
			}
			if (lcpvret == LC_CBRET_IGNORESECTION) {
				ignore_section = 1;
			}
			continue;
		}

		/* Drop comments and blank lines. */
		if (*linebuf_ptr == '#' || *linebuf_ptr == '\0') {
			continue;
		}

		/* Don't handle things for a section that doesn't exist. */
		if (invalid_section == 1) {

			

			continue;
		}
		if (ignore_section == 1) {

			

			continue;
		}

		/* Find the command and the data in the line. */
		sep = strpbrk(linebuf_ptr, " \t");
		if (sep != NULL) {
			cmdend = sep;

			/* Delete space at the end of the command. */
			cmdend--; /* It currently derefs to the seperator.. */
			while (*cmdend <= ' ') {
				*cmdend = '\0';
				cmdend--;
			}

			/* Delete the seperator char and any leading space. */
			*sep = '\0';
			sep++;
			while (*sep == ' ' || *sep == '\t') {
				sep++;
			}
			value = sep;
		} else {
			value = NULL;
		}

		cmd = linebuf_ptr;

		/* Handle special commands. */
		if (strcasecmp(cmd, "include") == 0) {
			if (value == NULL) {
				lc_errno = LC_ERR_BADFORMAT;
				retval = -1;

				

				continue;
			}

			lpcafret = lc_process_conf_apache_include(value, qualifbuf);
			if (lpcafret < 0) {

				

				retval = -1;
			}
			continue;
		}

		/* Create the fully qualified variable name. */
		if (qualifbuf[0] != '\0') {
			strncat(qualifbuf, ".", sizeof(qualifbuf) - strlen(qualifbuf) - 1);
		}
		strncat(qualifbuf, cmd, sizeof(qualifbuf) - strlen(qualifbuf) - 1);

		/* Call the parent and tell them we have data. */
		save_lc_errno = lc_errno;
		lc_errno = LC_ERR_NONE;
		lcpvret = lc_process_var(qualifbuf, NULL, value, LC_FLAGS_VAR);
		if (lcpvret < 0) {
			if (lc_errno == LC_ERR_NONE) {

				

				lc_errno = LC_ERR_INVCMD;
			} else {

				

			}
			retval = -1;
		} else {
			lc_errno = save_lc_errno;
		}

		/* Remove the "cmd" part of the buffer. */
		tmp_ptr = strrchr(qualifbuf, '.');
		if (tmp_ptr == NULL) {
			tmp_ptr = qualifbuf;
		}
		*tmp_ptr = '\0';
	}

	fclose(configfp);

	return(retval);
}

int lc_process_conf_apache(const char *appname, const char *configfile) {
	return(lc_process_conf_apache_file(configfile, NULL));
}
