#include "compat.h"
#include "libconfig.h"
#include "libconfig_private.h"
#include "conf_section.h"


#include <stdio.h>



#include <string.h>


int lc_process_conf_section(const char *appname, const char *configfile) {
	FILE *configfp = NULL;
	char linebuf[1024] = {0}, *linebuf_ptr = NULL;
	char qualifbuf[1024] = {0};
	char *cmd = NULL, *value = NULL, *sep = NULL, *cmdend = NULL;
	char *currsection = NULL;
	char *fgetsret = NULL;
	int lcpvret = -1;
	int invalid_section = 1, ignore_section = 0;
	int retval = 0;
	lc_err_t save_lc_errno = LC_ERR_NONE;

	if (appname == NULL || configfile == NULL) {
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

		/* Handle section header. */
		if (linebuf[0] == '[' && linebuf[strlen(linebuf) - 1] == ']') {
			linebuf[strlen(linebuf) - 1] = '\0';
			linebuf_ptr = &linebuf[1];

			/* If a section was open, close it. */
			if (currsection != NULL) {
				lcpvret = lc_process_var(currsection, NULL, NULL, LC_FLAGS_SECTIONEND);
				if (lcpvret < 0) {

					fprintf(stderr, "Invalid section terminating: \"%s\"\n", currsection);

				}
				free(currsection);
			}

			/* Open new section. */
			currsection = strdup(linebuf_ptr);
			lcpvret = lc_process_var(currsection, NULL, NULL, LC_FLAGS_SECTIONSTART);
			if (lcpvret < 0) {

				fprintf(stderr, "Invalid section: \"%s\"\n", currsection);

				invalid_section = 1;
				lc_errno = LC_ERR_INVSECTION;
				retval = -1;
			} else {
				invalid_section = 0;
				ignore_section = 0;
			}

			if (lcpvret == LC_CBRET_IGNORESECTION) {
				ignore_section = 1;
			}
			continue;
		}

		/* Remove leading spaces. */
		linebuf_ptr = &linebuf[0];
		while (*linebuf_ptr == ' ') {
			linebuf_ptr++;
		}

		/* Drop comments and blank lines. */
		if (*linebuf_ptr == ';' || *linebuf_ptr == '\0') {
			continue;
		}

		/* Don't handle things for a section that doesn't exist. */
		if (invalid_section == 1) {

			fprintf(stderr, "Ignoring line (because invalid section): %s\n", linebuf);

			continue;
		}

		/* Don't process commands if this section is specifically ignored. */
		if (ignore_section == 1) {

			fprintf(stderr, "Ignoring line (because ignored section): %s\n", linebuf);

			continue;
		}

		/* Find the command and the data in the line. */
		cmdend = sep = strpbrk(linebuf_ptr, "=");
		if (sep == NULL) {

			fprintf(stderr, "Invalid line: \"%s\"\n", linebuf);

			continue;
		}

		/* Delete space at the end of the command. */
		cmdend--; /* It currently derefs to the seperator.. */
		while (*cmdend <= ' ') {
			*cmdend = '\0';
			cmdend--;
		}

		cmd = linebuf_ptr;

		/* Delete the seperator char and any leading space. */
		*sep = '\0';
		sep++;
		while (*sep == ' ' || *sep == '\t') {
			sep++;
		}
		value = sep;

		/* Create the fully qualified variable name. */
		if (currsection == NULL) {
			strncpy(qualifbuf, cmd, sizeof(qualifbuf) - 1);
		} else {
			snprintf(qualifbuf, sizeof(qualifbuf) - 1, "%s.%s", currsection, cmd);
		}

		/* Call the parent and tell them we have data. */
		save_lc_errno = lc_errno;
		lc_errno = LC_ERR_NONE;
		lcpvret = lc_process_var(qualifbuf, NULL, value, LC_FLAGS_VAR);
		if (lcpvret < 0) {
			if (lc_errno == LC_ERR_NONE) {

				fprintf(stderr, "Invalid command: \"%s\"\n", cmd);

				lc_errno = LC_ERR_INVCMD;
			} else {

				fprintf(stderr, "Error processing command (command was valid, but an error occured, errno was set)\n");

			}
			retval = -1;
		} else {
			lc_errno = save_lc_errno;
		}
	}

	/* Close any open section, and clean-up. */
	if (currsection != NULL) {
		lcpvret = lc_process_var(currsection, NULL, NULL, LC_FLAGS_SECTIONEND);
		if (lcpvret < 0) {

			fprintf(stderr, "Invalid section terminating: \"%s\"\n", currsection);

		}
		free(currsection);
	}

	fclose(configfp);

	return(retval);
}
