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
#include <unistd.h>
#include <syslog.h>
#include <sys/wait.h>

#include "smags.h"
#include "agros.h"

int main (){
    int pid = 0;
    char *commandline = NULL;
    char prompt[MAX_LINE_LEN];

    command_t cmd = {NULL, 0, {NULL}};
    config_t ag_config = {NULL, NULL, NULL, NULL, 0, 0, NULL, 0, 0};
    user_t ag_user = {NULL, NULL};

    /* Init ag_user */
    init_user (&ag_user);

    /* Opens the syslog file */
    openlog (ag_user.username, LOG_PID, LOG_USER);

    /* Parses the config files for data */
    parse_config (&ag_config, ag_user.username);


    /*
     *   Main loop:
     *   - print prompt
     *   - read input and parse it
     *   - either a built-in command ("cd", "?" or "exit)
     *   - or a system command, in which case the program forks and executes it with execvp()
     */

    if (ag_config.welcome_message != NULL && strlen (ag_config.welcome_message) > 0) {
        fprintf (stdout, "\n%s\n\n", ag_config.welcome_message);
    }

    while (AG_TRUE){
        /* Set the prompt */
        get_prompt(prompt, MAX_LINE_LEN, &ag_user);
        fprintf (stdout, "%s", prompt);

        commandline = read_input (MAX_LINE_LEN);
        parse_command (commandline, &cmd);

        switch (get_cmd_code (cmd.name)){
            case EMPTY_CMD:
                break;

            case CD_CMD:
                change_directory (cmd.argv[1], ag_config.loglevel, ag_user);
                break;

            case HELP_CMD:
                if (cmd.argc > 2)
                    fprintf (stdout, "Too many arguments\n");

                else if (cmd.argc == 1)
                    print_help (&ag_config, "-a");

                else
                    print_help(&ag_config, cmd.argv[1]);

                break;

            case SHORTHELP_CMD:
                print_help(&ag_config, "-s");
                break;

            case SETENV_CMD:
                if (cmd.argv[1] && cmd.argc == 2)
                    ag_setenv (cmd.argv[1]);
                break;

            case UNSETENV_CMD:
                if (cmd.argv[1] && cmd.argc == 2)
                    ag_unsetenv (cmd.argv[1]);
                break;

            case ENV_CMD:
                print_env (cmd.argv[1]);
                break;

            case EXIT_CMD:
                closelog ();
                return 0;

            case OTHER_CMD:

                /* Determines whether the command should run in the bg or not */
                pid = vfork();

                if (pid == 0){

                    if (check_validity (cmd, ag_config) == AG_TRUE){
                        if (ag_config.loglevel == 3)    syslog (LOG_NOTICE, "Using command: %s.", cmd.name);
                        execvp (cmd.argv[0], cmd.argv);

                        fprintf (stderr, "%s: Could not execute command!\nType '?' for help.\n", cmd.name);
                        if (ag_config.loglevel >= 2)    syslog (LOG_NOTICE, "Could not execute: %s.", cmd.name);

                    }else {
                        fprintf (stdout, "Not allowed! \n");
                        if (ag_config.warnings >= 0)    decrease_warnings (&ag_config);
                        if (ag_config.loglevel >= 1)    syslog (LOG_ERR, "Trying to use forbidden command: %s.", cmd.name);
                    }

                    free (commandline);
                    commandline = (char *)NULL;

                    _exit(EXIT_FAILURE);

                }else if (pid < 0){

                    fprintf (stderr, "Error! ... Negative PID. God knows what that means ...\n");
                    if (ag_config.loglevel >= 1) syslog (LOG_ERR, "Negative PID. Using command: %s.", cmd.name);

                }else {

                    /* Wait until the end of child process, then resumes the loop */
                    wait (0);
                    break;
                }
        }
    }

    closelog();

    return 0;
}
