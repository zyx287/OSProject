#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#define MAX_BUFFER 1024    // max line buffer
#define MAX_ARGS 64        // max # args
#define SEPARATORS " \t\n" // token sparators
#define README "readme"    // help file name

struct shellstatus_st
{
    int foreground;  // foreground execution flag
    char *infile;    // input redirection flag & file
    char *outfile;   // output redirection flag & file
    char *outmode;   // output redirection mode
    char *shellpath; // full pathname of shell
};
typedef struct shellstatus_st shellstatus;

extern char **environ;

void check4redirection(char **, shellstatus *); // check command line for i/o redirection
void errmsg(char *, char *);                    // error message printout
void execute(char **, shellstatus);             // execute command from arg array
char *getcwdstr(char *, int);                   // get current work directory string
FILE *redirected_op(shellstatus);               // return required o/p stream
char *stripath(char *);                         // strip path from filename
void syserrmsg(char *, char *);                 // system error message printout

/*******************************************************************/

int main(int argc, char **argv)
{
    FILE *ostream = stdout;   // (redirected) o/p stream
    FILE *instream = stdin;   // batch/keyboard input
    char linebuf[MAX_BUFFER]; // line buffer
    char cwdbuf[MAX_BUFFER];  // cwd buffer
    char *args[MAX_ARGS];     // pointers to arg strings
    char **arg;               // working pointer thru args
    char *prompt = "==>";     // shell prompt
    char *readmepath;         // readme pathname
    shellstatus status;       // status structure

    // parse command line for batch input
    switch (argc)
    {
    case 1:
    {
        // keyboard input
        // TODO
    }

    case 2:
    {
        // possible batch/script
        // TODO
    }
    default: // too many arguments
        fprintf(stderr, "%s command line error; max args exceeded\n"
                        "usage: %s [<scriptfile>]",
                stripath(argv[0]), stripath(argv[0]));
        exit(1);
    }

    // get starting cwd to add to readme pathname
    // TODO

    // get starting cwd to add to shell pathname
    // TODO

    // set SHELL= environment variable, malloc and store in environment
    // TODO

    // prevent ctrl-C and zombie children
    signal(SIGINT, SIG_IGN);  // prevent ^C interrupt
    signal(SIGCHLD, SIG_IGN); // prevent Zombie children

    // keep reading input until "quit" command or eof of redirected input
    while (!feof(instream))
    {
        // (re)initialise status structure
        status.foreground = TRUE;

        // set up prompt
        // TODO
        printf("%s",getenv("PWD"));
        printf(prompt);
        fflush(stdout);

        // get command line from input
        if (fgets(linebuf, MAX_BUFFER, instream))
        {
            // read a line
            // tokenize the input into args array
            arg = args;
            *arg++ = strtok(linebuf, SEPARATORS); // tokenize input
            while ((*arg++ = strtok(NULL, SEPARATORS)))
                ;

            // last entry will be NULL
            if (args[0])
            {
                // check for i/o redirection
                check4redirection(args, &status);

                // check for internal/external commands
                // "cd" command
                if (!strcmp(args[0], "cd"))
                {
                    // TODO
                    char *new_dir;
                    if (args[1]==NULL){
                        //No arguments, print current working directory
                        new_dir=getcwd(NULL,0);
                        printf("%s\n",new_dir);
                        //Releasing space
                        free(new_dir);
                    }
                    //Change to specified directory
                    new_dir=args[1];
                    if (chdir(new_dir)==-1){
                        //Specified directory wasn't existed
                        perror(new_dir);
                    }
                    //Update Environment variable PWD
                    char cwd[MAX_BUFFER];
                    if (getcwd(cwd,sizeof(cwd))==NULL){
                        perror("getcwd");
                        exit(EXIT_FAILURE);
                    }
                    setenv("PWD",cwd,TRUE);
                }
                // "clr" command
                else if (!strcmp(args[0], "clr"))
                {
                    // TODO
                    pid_t pid;
                    if ((pid=fork())<0){
                        syserrmsg("fork","failed");
                    }
                    else if (pid==0){
                        //child
                        execlp("clear","clear",NULL);
                        syserrmsg("execlp","failed");
                        exit(1);
                    }
                    else {
                        //parent
                        waitpid(pid,NULL,0);
                    }
                }
                // "dir" command
                else if (!strcmp(args[0], "dir"))
                {
                    // TODO
                    pid_t pid=fork();
                    if (pid<0){
                        perror("Error forking");
                        exit(1);
                    }
                    else if (pid==0){
                        //child process, execute ls -al command to list directory contents
                        //execute in the specific directory or in the cwd
                        char *dirpath=args[1] ? args[1] : ".";
                        char *cmd[]={"/bin/ls","-al",dirpath,NULL};
                        execvp(cmd[0],cmd);
                        //execvp returns when error
                        perror("Error executing ls");
                        exit(1);
                    }
                    else{
                        //parent process
                        int status;
                        waitpid(pid,&status,0);
                        if(WIFSIGNALED(status)){
                            printf("dir command terminated abnormally with signal %d\n",WTERMSIG(status));
                        }
                    }
                }
                // "echo" command
                else if (!strcmp(args[0], "echo"))
                {
                    // TODO
                    //Start at args[1]
                    for (int i=1;args[i]!=NULL;i++){
                        char* word=args[i];
                        //需要检查输入的是否为具体命令吗？
                        //Print comment, compressed spaces and tabs
                        for (int j=0;j<strlen(word);j++){
                            if (word[j]==" "||word[j]=="\t"){
                                //Print space when there were no previous spaces or tabs
                                if (j==0||(word[j-1]!=" "&&word[j-1]!="\t")){
                                    printf(" ");
                                }
                            }
                            else{
                                printf("%c",word[j]);
                            }
                        }
                        //Add a space between words
                        printf(" ");
                    }
                    //New line for next comment
                    printf("\n");
                }
                // "environ" command
                else if (!strcmp(args[0], "environ"))
                {
                    // TODO
                }
                // "help" command
                else if (!strcmp(args[0], "help"))
                {
                    args[0] = "more";
                    args[1] = readmepath;
                    args[2] = NULL;
                }
                // "pause" command - note use of getpass - this is a made to measure way of turning off
                //  keyboard echo and returning when enter/return is pressed
                else if (!strcmp(args[0], "pause"))
                {
                    // TODO
                }
                // "quit" command
                else if (!strcmp(args[0], "quit"))
                {
                    break;
                }
                // else pass command on to OS shell
                // TODO
                else{
                    pid_t pid=fork();
                    if (pid<0){
                        perror("fork");
                    }
                    else if (pid==0){
                        //child
                        execvp(args[0],args);
                        perror("execvp");
                        exit(EXIT_FAILURE);
                    }
                    else {
                        //parent
                        int status;
                        waitpid(pid,&status,0);
                    }
                }
            }
        }
    }
    return 0;
}

/***********************************************************************

void check4redirection(char ** args, shellstatus *sstatus);

check command line args for i/o redirection & background symbols
set flags etc in *sstatus as appropriate

***********************************************************************/

void check4redirection(char **args, shellstatus *sstatus)
{
    sstatus->infile = NULL; // set defaults
    sstatus->outfile = NULL;
    sstatus->outmode = NULL;

    while (*args)
    {
        // input redirection
        if (!strcmp(*args, "<"))
        {
            // TODO
        }
        // output direction
        else if (!strcmp(*args, ">") || !strcmp(*args, ">>"))
        {
            // TODO
        }
        else if (!strcmp(*args, "&"))
        {
            // TODO
        }
        args++;
    }
}

/***********************************************************************

  void execute(char ** args, shellstatus sstatus);

  fork and exec the program and command line arguments in args
  if foreground flag is TRUE, wait until pgm completes before
    returning

***********************************************************************/

void execute(char **args, shellstatus sstatus)
{
    int status;
    pid_t child_pid;
    char tempbuf[MAX_BUFFER];

    switch (child_pid = fork())
    {
    case -1:
        syserrmsg("fork", NULL);
        break;
    case 0:
        // execution in child process
        // reset ctrl-C and child process signal trap
        signal(SIGINT, SIG_DFL);
        signal(SIGCHLD, SIG_DFL);

        // i/o redirection */
        // TODO

        // set PARENT = environment variable, malloc and put in nenvironment
        // TODO

        // final exec of program
        execvp(args[0], args);
        syserrmsg("exec failed -", args[0]);
        exit(127);
    }

    // continued execution in parent process
    // TODO
}

/***********************************************************************

 char * getcwdstr(char * buffer, int size);

return start of buffer containing current working directory pathname

***********************************************************************/

char *getcwdstr(char *buffer, int size)
{
    // TODO
    return buffer;
}

/***********************************************************************

FILE * redirected_op(shellstatus ststus)

  return o/p stream (redirected if necessary)

***********************************************************************/

FILE *redirected_op(shellstatus status)
{
    FILE *ostream = stdout;

    // TODO

    return ostream;
}

/*******************************************************************

  char * stripath(char * pathname);

  strip path from file name

  pathname - file name, with or without leading path

  returns pointer to file name part of pathname
            if NULL or pathname is a directory ending in a '/'
                returns NULL
*******************************************************************/

char *stripath(char *pathname)
{
    char *filename = pathname;

    if (filename && *filename)
    {                                      // non-zero length string
        filename = strrchr(filename, '/'); // look for last '/'
        if (filename)                      // found it
            if (*(++filename))             //  AND file name exists
                return filename;
            else
                return NULL;
        else
            return pathname; // no '/' but non-zero length string
    }                        // original must be file name only
    return NULL;
}

/********************************************************************

void errmsg(char * msg1, char * msg2);

print an error message (or two) on stderr

if msg2 is NULL only msg1 is printed
if msg1 is NULL only "ERROR: " is printed
*******************************************************************/

void errmsg(char *msg1, char *msg2)
{
    fprintf(stderr, "ERROR: ");
    if (msg1)
        fprintf(stderr, "%s; ", msg1);
    if (msg2)
        fprintf(stderr, "%s; ", msg2);
    return;
    fprintf(stderr, "\n");
}

/********************************************************************

  void syserrmsg(char * msg1, char * msg2);

  print an error message (or two) on stderr followed by system error
  message.

  if msg2 is NULL only msg1 and system message is printed
  if msg1 is NULL only the system message is printed
 *******************************************************************/

void syserrmsg(char *msg1, char *msg2)
{
    errmsg(msg1, msg2);
    perror(NULL);
    return;
}
