/* -*- linux-c -*-
 *
 * stapio.c - SystemTap module io handler.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright (C) 2005-2012 Red Hat, Inc.
 *
 */

#include "staprun.h"
#include <pwd.h>
char *__name__ = "stapio";

int main(int argc, char **argv)
{
	/* Force libc to make our stderr messages atomic by enabling line
	   buffering since stderr is unbuffered by default. Without this, libc
	   is at liberty to split a single stderr message into multiple writes
	   to the fd while holding flockfile(stderr). POSIX only guarantees that
	   a single write(2) is atomic; chaining several write(2) calls together
	   won't be atomic, and we don't want libc to do that within a single
	   *fprintf(stderr) call since it'll mangle messages printed across
	   different processes (*not* threads). */
	setlinebuf(stderr);

#if ENABLE_NLS
	setlocale (LC_ALL, "");
	bindtextdomain (PACKAGE, LOCALEDIR);
	textdomain (PACKAGE);
#endif
	setup_signals();
	parse_args(argc, argv);

        /* If we inherited a relay_basedir_fd, we want to keep it to ourselves -
           i.e., FD_CLOEXEC the bad boy. */
        if (relay_basedir_fd >= 0) {
                int rc =  set_clexec(relay_basedir_fd);
                if (rc) 
                        exit(-1);
        }


	if (buffer_size)
		dbug(1, "Using a buffer of %u MB.\n", buffer_size);

	if (optind < argc) {
		parse_modpath(argv[optind++]);
		dbug(2, "modpath=\"%s\", modname=\"%s\"\n", modpath, modname);
	}

	if (optind < argc) {
		if (attach_mod) {
			err(_("Cannot have module options with attach (-A).\n"));
			usage(argv[0],1);
		} else {
			unsigned start_idx = 3;	/* reserve three slots in modoptions[] */
			while (optind < argc && start_idx + 1 < MAXMODOPTIONS)
				modoptions[start_idx++] = argv[optind++];
			modoptions[start_idx] = NULL;
		}
	}

	if (modpath == NULL || *modpath == '\0') {
		err(_("Need a module name or path to load.\n"));
		usage(argv[0],1);
	}

	if (init_stapio())
		exit(1);

	if (stp_main_loop()) {
		err(_("Couldn't enter main loop. Exiting.\n"));
		exit(1);
	}

	return 0;
}
