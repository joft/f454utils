/*
 * startssh - "custom app" for bticino F454 to start dropbear server
 *
 * Copyright (C) 2013  Joachim Foerster <JOFT@gmx.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2, only - as published by the Free Software Foundation.
 * The usual option to use any later version is hereby excluded.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#define BTWDDIR "/var/tmp/bticino/bt_wd"
#define EXTRAMNT "/home/bticino/cfg/extra"

#define APPNAME "startssh"

#define BTWDFILE BTWDDIR "/" APPNAME
#define BTWDINT 15

#define DROPBEARKEY "/usr/sbin/dropbearkey"
#define DROPBEAR "/usr/sbin/dropbear"
#define SSHPORTSTR "1022"
#define RSAKEYFILE EXTRAMNT "/startssh_rsa_key"
#define DOTSSHAUTHKEY "/home/root/.ssh/authorized_keys"


int make_key()
{
	int rc;
	struct stat st;

	rc = stat(RSAKEYFILE, &st);
	if ((rc == -1) && (errno == ENOENT)) {
		rc = system(DROPBEARKEY " -t rsa -f " RSAKEYFILE);
		if (rc == -1)
			perror("system " DROPBEARKEY);
	} else if (rc == -1) {
		perror("stat " RSAKEYFILE);
	}

	return rc;
}

int fork_dropbear()
{
	int rc;
	struct stat st;
	int pid;
	char *dbargs[] = {
		DROPBEAR,
		"-F",
		"-r", RSAKEYFILE,
		"-p", SSHPORTSTR,
		"-s",
		NULL};

	/* If root has ~/.ssh/authorized_keys disable password login. */
	rc = stat(DOTSSHAUTHKEY, &st);
	if ((rc == -1) && (errno == ENOENT)) {
		dbargs[6] = NULL;
	} else if (rc == -1) {
		perror("stat " DOTSSHAUTHKEY);
		return -1;
	}

	pid = fork ();
	if (!pid) {
		/* child */
		execv(DROPBEAR, dbargs);
		_exit(EXIT_FAILURE);
	} else if (pid < 0) {
		return -1;
	}

	return pid;
}


int main(int argc, char *argv[])
{
	int rc;
	int pid;
	int jf;

	pid = -1;
	jf = 0;
	while (1) {
		if (pid < 0) {
			rc = make_key();
			if (!(rc < 0)) {
				pid = fork_dropbear();
				jf = 1;
			}

			sleep(5);
			continue;
		}

		if (waitpid(pid, NULL, WNOHANG) == pid) {
			pid = -1;
			continue;
		}

		sleep(BTWDINT - (jf ? 5 : 0));
		jf = 0;

		rc = system("touch " BTWDFILE);
		if (rc == -1)
			perror("system touch");
	}

	return 0;
}
