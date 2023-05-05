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
    int bg;
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
        break;
    }
    case 2:
    {
        // possible batch/script
        // TODO
        FILE *batchfile;
        char buf[MAX_BUFFER];
        char *args[MAX_BUFFER / 2 + 1];
        int i, nargs;
        // get starting cwd to add to readme pathname
        char readme_cwd[MAX_BUFFER];
        sprintf(readme_cwd,"%s/readme",getcwd(NULL,0));
        readmepath=readme_cwd;
        // get starting cwd to add to shell pathname
        char shell_cwd[MAX_BUFFER];
        char *shellname;
        char *parentpath;
        sprintf(shell_cwd,"SHELL=%s/myshell",getcwd(NULL,0));
        shellname=shell_cwd;
        parentpath=shell_cwd;
        // set SHELL= environment variable, malloc and store in environment
        putenv(shellname);
        // check if file exists
        if (access(argv[1], F_OK) != -1)
        {
            // open batchfile
            if ((batchfile = fopen(argv[1], "r")) == NULL)
            {
                syserrmsg("batchfile open failed", argv[1]);
                exit(EXIT_FAILURE);
            }

            // process batchfile
            while (!feof(batchfile))
            {
                if (fgets(buf, MAX_BUFFER, batchfile) != NULL)
                {
                    buf[strlen(buf) - 1] = '\0'; // overwrite \n at end of line

                    // parse arguments
                    nargs = 0;
                    args[nargs] = strtok(buf, " ");
                    while (args[nargs] != NULL)
                    {
                        nargs++;
                        args[nargs] = strtok(NULL, " ");
                    }

                    // execute command
                    execute(args, status);
                }
            }

            fclose(batchfile);

            // exit shell
            exit(EXIT_SUCCESS);
        }
    }

    default: // too many arguments
        fprintf(stderr, "%s command line error; max args exceeded\n"
                        "usage: %s [<scriptfile>]",
                stripath(argv[0]), stripath(argv[0]));
        exit(1);
    }

    // get starting cwd to add to readme pathname
    char readme_cwd[MAX_BUFFER];
    sprintf(readme_cwd,"%s/readme",getcwd(NULL,0));
    readmepath=readme_cwd;
    // get starting cwd to add to shell pathname
    char shell_cwd[MAX_BUFFER];
    char *shellname;
    char *parentpath;
    sprintf(shell_cwd,"SHELL=%s/myshell",getcwd(NULL,0));
    shellname=shell_cwd;
    parentpath=shell_cwd;
    // set SHELL= environment variable, malloc and store in environment
    putenv(shellname);


    // prevent ctrl-C and zombie children
    signal(SIGINT, SIG_IGN);  // prevent ^C interrupt
    signal(SIGCHLD, SIG_IGN); // prevent Zombie children

    // keep reading input until "quit" command or eof of redirected input
    while (!feof(instream))
    {
        // (re)initialise status structure
        status.foreground = TRUE;

        // set up prompt
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
                    char *new_dir;
                    if (args[1]==NULL){//No directory is spcified, print current working directory
                        new_dir=getcwd(NULL,0);
                        printf("%s\n",new_dir);
                    }
                    else {//Directory is specified, change to specified directory
                        new_dir=args[1];
                        if (chdir(new_dir)==-1){//Specified directory wasn't existed
                            perror(new_dir);
                        }
                        //Update Environment variable PWD by using putenv()
                        char env_var[MAX_ARGS];
                        sprintf(env_var,"PWD=%s",getcwd(NULL,0));//Prepare stdin for putenv() to update but not add the environment variable PWD
                        putenv(env_var);
                    }
                }
                // "clr" command
                else if (!strcmp(args[0], "clr"))
                {
                    pid_t pid;
                    pid=fork();
                    if (pid<0){//Failed in create a child process
                        syserrmsg("fork","failed");
                    }
                    else if (pid==0){//Using execvp to execute clear in child process
                        putenv(parentpath);
                        char *cmd[]={"clear",NULL};
                        execvp(cmd[0],cmd);
                        syserrmsg("execvp","failed");//Report error when execvp broken
                        exit(1);//Return a none-zero int to parent process when execvp failed
                    }
                    else {
                        //In parent process, waiting for the stop of any one child process
                        waitpid(pid,NULL,WUNTRACED);
                    }
                }
                // "dir" command
                else if (!strcmp(args[0], "dir"))
                {
                    pid_t pid=fork();
                    if (pid<0){//Failed in create a child process
                        syserrmsg("fork","failed");
                    }
                    else if (pid==0){//Using execvp to execute ls -al in child process
                        putenv(parentpath);
                        char *dirpath=args[1] ? args[1] : ".";//execute in the specific directory or in the PWD
                        char *cmd[]={"ls","-al",dirpath,NULL};
                        execvp(cmd[0],cmd);
                        syserrmsg("execvp","failed");//Report error when execvp broken
                        exit(1);//Return a none-zero int to parent process when execvp failed
                    }
                    else{
                        //parent process
                        waitpid(pid,NULL,WUNTRACED);
                    }
                }
                // "echo" command
                else if (!strcmp(args[0], "echo"))
                {
                    for (int i=1;args[i]!=NULL;i++){//Start at args[1], reading all words, spaces and tabs, return a single space
                        char *word=args[i];
                        for (int j=0;j<strlen(word);j++){
                            if (word[j]==' '||word[j]=='\t'){
                                if (j==0||(word[j-1]!=' '&&word[j-1]!='\t')){//Print space when there were no previous spaces or tabs
                                    printf(" ");
                                }
                            }
                            else{
                                printf("%c",word[j]);
                            }
                        }
                        printf(" ");////Add a space between words
                    }
                    printf("\n");//New line for next comment
                }
                // "environ" command
                else if (!strcmp(args[0], "environ"))
                {
                    char **env=environ;
                    while (*env){
                        printf("%s\n",*env);
                        env++;
                    }
                }
                // "help" command
                else if (!strcmp(args[0], "help"))
                {
                    args[0] = "more";
                    args[1] = readmepath;
                    args[2] = NULL;
                    pid_t pid;
                    pid=fork();
                    if (pid<0){
                        syserrmsg("fork","failed");
                    }
                    else if (pid==0){
                        putenv(parentpath);
                        execvp(args[0], args);
                    }
                    else {
                        waitpid(pid,NULL,WUNTRACED);
                    }
                }
                // "pause" command - note use of getpass - this is a made to measure way of turning off
                //  keyboard echo and returning when enter/return is pressed
                else if (!strcmp(args[0], "pause"))
                {
                    printf("Press 'Enter' to continue");//Stop, until the user press "Enter"
                    fflush(stdout);
                    char *input=getpass("");//Ignore the user's input
                    printf("\n");
                }
                // "quit" command
                else if (!strcmp(args[0], "quit"))
                {
                    break;
                }
                // else pass command on to OS shell
                else{
                    pid_t pid=fork();
                    if (pid<0){
                        perror("fork");
                    }
                    else if (pid==0){
                        //child
                        putenv(parentpath);
                        execvp(args[0],args);
                        perror("execvp");
                        exit(EXIT_FAILURE);
                    }
                    else {
                        //parent
                        waitpid(pid,NULL,WUNTRACED);
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
            if (*(args+1)) {
                sstatus->infile = *(args+1);
                args++;
            }
        }
        // output direction
        else if (!strcmp(*args, ">") || !strcmp(*args, ">>"))
        {
            if (*(args+1)) {
                sstatus->outfile = *(args+1);
                sstatus->outmode = (!strcmp(*args, ">")) ? "w" : "a";
                args++;
            }
        }
        else if (!strcmp(*args, "&"))
        {
            sstatus->bg = 1;
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
    check4redirection(args,&sstatus);

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
        if (sstatus.infile) {
            FILE *fin = freopen(sstatus.infile, "r", stdin);
            if (fin == NULL) {
                errmsg("could not open file", sstatus.infile);
                exit(EXIT_FAILURE);
            }
        }

        if (sstatus.outfile) {
            FILE *fout;
            //if (strcmp(sstatus.outmode, ">") == 0) {
            if (sstatus.outmode==">") {
                fout = fopen(sstatus.outfile, "w");
            } else if (strcmp(sstatus.outmode, ">>") == 0) {
                fout = fopen(sstatus.outfile, "a");
            } else {
                errmsg("invalid output redirection mode:", sstatus.outmode);
                exit(EXIT_FAILURE);
            }
            if (fout == NULL) {
                errmsg("could not open file", sstatus.outfile);
                exit(EXIT_FAILURE);
            }
            freopen(sstatus.outfile, "a", stdout);
        }

        // set PARENT = environment variable, malloc and put in nenvironment
        // TODO
        char **nenv = NULL;
        int nenv_size = 0;
        int i = 0;
        while (environ[i] != NULL) {
            nenv = realloc(nenv, sizeof(char*) * (nenv_size + 1));
            if (nenv == NULL) {
                syserrmsg("realloc", NULL);
                exit(EXIT_FAILURE);
            }
            nenv[nenv_size++] = strdup(environ[i++]);
        }
        char cwd[MAX_BUFFER];
        if (getcwdstr(cwd, MAX_BUFFER) != NULL) {
            snprintf(tempbuf, MAX_BUFFER, "PARENT=%s", cwd);
            nenv = realloc(nenv, sizeof(char*) * (nenv_size + 1));
            if (nenv == NULL) {
                syserrmsg("realloc", NULL);
                exit(EXIT_FAILURE);
            }
            nenv[nenv_size++] = strdup(tempbuf);
        }

        // final exec of program
        execvp(args[0], args);
        syserrmsg("exec failed -", args[0]);
        exit(127);
    }

    // continued execution in parent process
    waitpid(child_pid, &status, 0);

    // TODO
    if (sstatus.outfile) {
        fclose(stdout);
    }
}


/***********************************************************************

 char * getcwdstr(char * buffer, int size);

return start of buffer containing current working directory pathname

***********************************************************************/

char *getcwdstr(char *buffer, int size)
{
    if (getcwd(buffer, size) != NULL) {
        return buffer;
    } else {
        perror("getcwd() error");
        return NULL;
    }
}
/***********************************************************************

FILE * redirected_op(shellstatus ststus)

  return o/p stream (redirected if necessary)

***********************************************************************/

FILE *redirected_op(shellstatus status)
{
    FILE *ostream = stdout;

    if (status.outfile != NULL) {
        ostream = freopen(status.outfile, status.outmode, stdout);
        if (ostream == NULL) {
            perror("freopen() error");
            return NULL;
        }
    }

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
