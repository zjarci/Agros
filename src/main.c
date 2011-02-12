/*
 *    AGROS - The new Limited Shell
 *
 *    Author: Joe "rahmu" Hakim Rahme <joe.hakim.rahme@gmail.com>
 *
 *
 *    This file is part of AGROS.
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <syslog.h>
#include "agros.h"

#ifndef CONFIG_FILE
#define CONFIG_FILE "agros.conf"
#endif



int main (int argc, char** argv, char** envp){
    int pid = 0;
    command_t cmd;
    char commandline[MAX_LINE_LEN];
    int loglevel = 0;
    GKeyFile* gkf;

    /* Opens a log connection. AGROS relies on underlying Syslog to deal with logging issues.
       Log file manipulations such as compressions, purging or backup are left for the user
       to deal with. Until I find a better way to do it, that is :-) */
    openlog ("[AGROS]", LOG_CONS, LOG_USER);


    /* Gets the value of loglevel, from CONFIG_FILE */
    gkf = g_key_file_new();
    if (!g_key_file_load_from_file (gkf, CONFIG_FILE, G_KEY_FILE_NONE, NULL)){
        fprintf (stderr, "Could not read config file %s\nTry using another shell or contact an administrator.\n", CONFIG_FILE);
        syslog (LOG_USER, "<%s> Could not read config file. \n", getenv("USER"));
        exit (EXIT_FAILURE);
    }
    loglevel = g_key_file_get_integer (gkf, "General", "loglevel", NULL);
    g_key_file_free (gkf);

    /* Opens a log connection. AGROS relies on underlying Syslog to deal with logging issues.
       Log file manipulations such as compressions, purging or backup are left for the user
       to deal with. Until I find a better way to do it, that is :-) */
    openlog ("[AGROS]", LOG_CONS, LOG_USER);

    /*
     *   Main loop:
     *   - print prompt
     *   - read input and parse it
     *   - either a built-in command ("cd", "?" or "exit)
     *   - or a system command, in which case the program forks and executes it with execvp()
     */

    while (1){
        print_prompt();
        read_input (commandline, MAX_LINE_LEN);
        parse_command (commandline, &cmd);

        switch (get_cmd_code (cmd.name)){
            case EMPTY_CMD:
                break;

            case CD_CMD:
                change_directory (concat_spaces(cmd.argv), loglevel);
                break;

            case HELP_CMD:
                print_help();
                break;

            case ENV_CMD:
                print_env (cmd.argv[1]);
                break;

            case EXIT_CMD:
                closelog();
                return 0;

            case OTHER_CMD:
                pid = fork();
                if (pid == 0){
                    execvp (cmd.name, cmd.argv);
                    fprintf (stderr, "%s: Could not execute command!\nType '?' for help.\n", cmd.name);
                    if (loglevel >= 2)
                        syslog (LOG_USER, "#LGLVL2# <%s> %s: Could not execute command. \n", getenv("USER"), cmd.name);
                    kill(getpid(), SIGTERM);
                    break;
                }else if (pid < 0){
                    fprintf (stderr, "Error! ... Negative PID. God knows what that means ...\n");
                }else {
                    wait(0);
                }
                break;
        }
    }

    /* Close your log when you're done. Always. */
    closelog();

    return 0;
}
