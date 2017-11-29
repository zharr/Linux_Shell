#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/time.h>

typedef struct process {
  int pid;
  int jid;
  struct process *next;
  char *args;
} job;

void
free_all(char** a, job* r, job* c, job* h, int b, int f) {
  free(a);
  // free linked list
  r = h;
  job *prev;
  while (r != c) {
    prev = r;
    r = r->next;
    free(prev->args);
    free(prev);
  }
  free(c->args);
  free(c);

  if (b == 1) {
    close(f);
  }
}


int
main(int argc, char *argv[]) {
  char prompt[] = "mysh> ";
  char *command = malloc(514 * sizeof(char));
  char **arg = malloc(256 * sizeof(char *));
  int rc;
  char file_error[] = "Error: Cannot open file ";
  int jid = 0;
  int batch = 0;
  int file = 0;
  job *head = malloc(sizeof(job));
  head->next = NULL;
  job *cur;
  cur = head;
  job *runner;
  runner = head;

  // check if in batch mode
  if (argc == 2) {
    close(STDIN_FILENO);
    // open file from command line
    int file;
    file = open(argv[1], O_RDONLY);

    // check to see if opened successfully
    if (file < 0) {
      write(STDERR_FILENO, file_error, sizeof(file_error) - 1);
      write(STDERR_FILENO, argv[1], sizeof(argv[1]) - 2);
      write(STDERR_FILENO, "\n", 1);
      exit(1);
    }
    batch = 1;
  }

  if (argc > 2) {
    write(STDERR_FILENO, "Usage: mysh [batchFile]\n", 24);
    exit(1);
  }

  // check for batch or interactive
  // if (argc == 1) {
  // in interactive mode (wait for command)
  while (1) {
    if (batch == 0) {
      write(1, prompt, sizeof(prompt) - 1);
    }

    if (fgets(command, 514, stdin)) {
      char n[] = "\n";
      if (strcmp(n, command) != 0) {
        int i = 0;
        int space = 0;
        while (command[i] != '\0') {
          if (command[i] == '\n') {
            command[i] = '\0';
            break;
          }
          if (command[i] == ' ') {
            space++;
          }
          i++;
        }

        if (batch == 1) {
          // display command to stdout
          write(1, command, i);
          write(1, "\n", 1);
        }
        // check for tabs
        int t;
        for (t = 0; t < i; t++) {
          if (command[t] == '\t') {
            command[t] = ' ';
          }
        }
        int count = 0;
        const char *d = " ";

        char printed[514];
        strncpy(printed, command, strlen(command) + 1);

        arg[0] = strtok(command, d);
        while (arg[count]) {
          count++;
          arg[count] = strtok(NULL, d);
        }

        arg[count + 1] = NULL;
        char ex[] = "exit";
        // check if should be in the background
        char b[] = "&";
        int bg = 0;
        // added count check for whitespace3
        if (strcmp(b, arg[count - 1]) == 0 /*&& count > 1*/) {
          bg = 1;

          // check here for whitespac test
          if (command[0] != '&') {
            if (strcmp(ex, arg[0]) != 0) {
              arg[count - 1] = NULL;
            }
          }
          int k;
          for (k = strlen(printed); k > -1; k--) {
            if (printed[k] == '&') {
              printed[k] = '\0';
              // don't know if this is right
              if (printed[k - 1] == ' ') {
                printed[k - 1] = '\0';
              }
              break;
            }
          }
        } else {
          // just added this if for whitespace 3
          // if (arg[0][0] != '&' && count > 1) {
          int m;
          for (m = strlen(arg[count - 1]); m > -1; m--) {
            if (arg[count - 1][m] == '&') {
              bg = 1;
              arg[count - 1][m] = '\0';

              int n;
              for (n = strlen(printed); n > -1; n--) {
                if (printed[n] == '&') {
                  printed[n] = '\0';
                  break;
                }
              }
              break;
            }
          }
         // }
        }

        // check to see if exit command
        // char ex[] = "exit";
        if (strcmp(ex, arg[0]) == 0) {
          if (count == 1) {
            free(command);
            free_all(arg, runner, cur, head, batch, file);
            exit(0);
          } else {
            char amps[] = "&";
            if (strcmp(amps, arg[1]) == 0) {
              free(command);
              free_all(arg, runner, cur, head, batch, file);
              exit(0);
            }
          }
        }
        if ((strcmp(arg[0], "j") == 0) || (strcmp(arg[0], "myw") == 0)) {
          // j command
          if (strcmp(arg[0], "j") == 0) {
                runner = head->next;
                while (runner != NULL) {
                  if (waitpid(runner->pid, NULL, WNOHANG) == 0) {
                    fprintf(stdout, "%d : %s\n", runner->jid, runner->args);
                    fflush(stdout);
                  }
                  runner = runner->next;
                }
              runner = head;
          } else {
            // myw command
            // traverse linked list until found
            if (count == 2) {
              runner = head;
              int c_jid = atoi(arg[1]);
              int found = 0;
              // case for only one node
              while (runner != NULL && found != 1) {
                if (runner->jid == c_jid) {
                  struct timeval start, end;
                  struct timezone tzp;
                  gettimeofday(&start, &tzp);

                  found = 1;
                  rc = waitpid(runner->pid, NULL, 0);
                  if (rc == runner->pid) {
                    gettimeofday(&end, &tzp);

                    fprintf(stdout, "%lu : Job %d terminated\n", (end.tv_sec -
                      start.tv_sec)*1000000, c_jid);
                    fflush(stdout);
                  }
                }
                runner = runner->next;
              }
              // if job not found
              if (found != 1) {
                fprintf(stderr, "Invalid jid %d\n", c_jid);
                fflush(stderr);
              }
            } else {
              fprintf(stderr, "%s: Command not found\n", arg[0]);
              fflush(stderr);
            }
          }
        } else {
          jid++;
          rc = fork();

          if (rc == 0) {
            execvp(arg[0], arg);
            write(2, arg[0], strlen(arg[0]));
            write(2, ": Command not found\n", 20);
            exit(1);
          } else if (rc > 0) {
            cur->next = malloc(sizeof(job));
            cur->next->pid = rc;
            cur->next->jid = jid;
            cur->next->next = NULL;
            // not sure about + 1
            cur->next->args = malloc(strlen(printed) + 1);
            strncpy(cur->next->args, printed, i);
            cur = cur->next;
            if (bg == 0) {
              (void) waitpid(rc, NULL, 0);
            }
          } else {
            write(2, "Error in calling fork().\n", 25);
            exit(1);
          }
        }
      } else {
        // user just hit enter so if in batch mode print newline
        if (batch == 1) {
          write(1, command, 1);
        }
      }
    } else {
      break;
    }
  }

  if (batch == 1) {
    close(file);
  }
  free(arg);
  // free linked list
  runner = head;
  job *prev;
  while (runner != cur) {
    prev = runner;
    runner = runner->next;
    free(prev->args);
    free(prev);
  }
  free(cur->args);
  free(cur);
  free(command);
  return 0;
}
