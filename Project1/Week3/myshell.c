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
        break;
    }

    case 2:
    {
        // possible batch/script
        // TODO
        instream=fopen(argv[1],"r");
        if (!instream){
            syserrmsg("Batchfile",argv[1]);
            instream=stdin;//Change back to stdin
        }
        else {
            setbuf(instream,NULL);
        }
        break;
    }
    default: // too many arguments
        fprintf(stderr, "%s command line error; max args exceeded\n"
                        "usage: %s [<scriptfile>]",
                stripath(argv[0]), stripath(argv[0]));
        exit(1);
    }

    // get starting cwd to add to readme pathname
    // TODO
    char readme_cwd[MAX_BUFFER];
    sprintf(readme_cwd,"%s/readme",getcwd(NULL,0));
    readmepath=readme_cwd;
    // get starting cwd to add to shell pathname
    // TODO
    char shell_cwd[MAX_BUFFER];
    char shellname[MAX_BUFFER];
    sprintf(shell_cwd,"SHELL=%s/myshell",getcwd(NULL,0));
    sprintf(shellname,"%s/myshell",getcwd(NULL,0));
    status.shellpath=shellname;
    // set SHELL= environment variable, malloc and store in environment
    // TODO
    putenv(shell_cwd);

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
        if (instream==stdin){
            printf("%s",getenv("PWD"));
            printf(prompt);
            fflush(stdout);
        }

        // get command line from input
        if (fgets(linebuf, MAX_BUFFER, instream))
        {
            // read a line
            // tokenize the input into args array
            arg = args;
            *arg++ = strtok(linebuf, SEPARATORS); // tokenize input
            while ((*arg++ = strtok(NULL, SEPARATORS)));

            // last entry will be NULL
            if (args[0])
            {
                // check for i/o redirection
                check4redirection(args, &status);

                // check for internal/external commands
                // "cd" cdcommand
                // TODO
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
                        sprintf(env_var,"PWD=%s",getcwd(NULL,0));
                        putenv(env_var);
                    }
                }
                // "clr" command
                // TODO
                else if (!strcmp(args[0], "clr"))
                {
                    args[0]="clear";
                    args[1]=NULL;
                    execute(args,status);
                }
                // "dir" command
                // TODO
                else if (!strcmp(args[0], "dir"))
                {
                    char *dirpath=args[1] ? args[1] : ".";//execute in the specific directory or in the PWD
                    args[0]="ls";
                    args[1]="-al";
                    args[2]=dirpath;
                    args[3]=NULL;
                    execute(args,status);
                }
                // "echo" command
                // TODO
                else if (!strcmp(args[0], "echo"))
                {
                    arg=&args[1];//read the comment
                    ostream=redirected_op(status);//check for redirection
                    while (*arg){
                        //prepare stdin for echo
                        fprintf(ostream,"%s ",*arg++);
                    }
                    fprintf(ostream,"\n");
                    if (ostream!=stdout){
                        fclose(ostream);
                    }
                }
                // "environ" command
                // TODO
                else if (!strcmp(args[0], "environ"))
                {
                    ostream=redirected_op(status);
                    char **env=environ;
                    while (*env){
                        fprintf(ostream,"%s\n",*env++);
                    }
                    if (ostream!=stdout){
                        fclose(ostream);
                    }
                }
                // "help" command
                else if (!strcmp(args[0], "help"))
                {
                    args[0] = "more";
                    args[1] = readmepath;
                    args[2] = NULL;
                    execute(args,status);
                }
                // "pause" command - note use of getpass - this is a made to measure way of turning off
                //  keyboard echo and returning when enter/return is pressed
                // TODO
                else if (!strcmp(args[0], "pause"))
                {
                    printf("Press 'Enter' to continue");//Stop, until the user press "Enter"
                    fflush(stdout);
                    char *input=getpass("");//Ignore the user's input
                    printf("\n");
                }
                // "quit" command
                // TODO
                else if (!strcmp(args[0], "quit"))
                {
                    break;
                }
                // else pass command on to OS shell
                // TODO
                else {
                    execute(args,status);
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
            *args=NULL;//any argument after the < will not be executed as a command argument
            if(*(args+1)){
                sstatus->infile=*(args+1);//Check weather there is a input file for Myshell
                args++;
            }
        }
        // output direction
        else if (!strcmp(*args, ">") || !strcmp(*args, ">>"))
        {
            // TODO
            sstatus->outmode=(!strcmp(*args,">")) ? "w" : "a";//>, replace and create a new file. >>, add to the specified file
            *args=NULL;//any argument after the > or >> will not be executed as a command argument
            if (*(args+1)){
                sstatus->outfile=*(args+1);//Check the redorection output file
                args++;
            }
        }
        else if (!strcmp(*args, "&"))
        {
            // TODOs
            //Background
            *args=NULL;
            sstatus->foreground=FALSE;
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
        if (sstatus.infile){
            FILE *fin=freopen(sstatus.infile,"r",stdin);
            if (fin==NULL){//check whether the inputfile exists
                errmsg("could not open file",sstatus.infile);
                exit(1);
            }
        }
        if (sstatus.outfile){
            FILE *fout;
            if (sstatus.outmode==">" || sstatus.outmode==">>"){
                fout=freopen(sstatus.outfile,sstatus.outmode,stdout);
            }
            else {
                errmsg("redirection failed with mode: ",sstatus.outmode);
                exit(1);
            }
        }

        // set PARENT = environment variable, malloc and put in nenvironment
        // TODO
        char parent_path[MAX_BUFFER];
        sprintf(parent_path,"PARENT=%s/myshell",sstatus.shellpath);
        putenv(parent_path);
        //final exec of program
        execvp(args[0], args);
        syserrmsg("exec failed -", args[0]);
        exit(127);
    }
    // continued execution in parent process
    // TODO
    if (sstatus.foreground==1){
        waitpid(child_pid,NULL,WUNTRACED);
    }
    else if (sstatus.foreground==0){
        waitpid(child_pid,NULL,WNOHANG);
    }
}

/***********************************************************************

 char * getcwdstr(char * buffer, int size);

return start of buffer containing current working directory pathname

***********************************************************************/

char *getcwdstr(char *buffer, int size)
{
    // TODO
    if (getcwd(buffer, size) != NULL)
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
    if (status.outfile != NULL) {
        ostream = fopen(status.outfile, status.outmode);
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
