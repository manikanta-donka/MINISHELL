// Name: Manikanta Donka
// Date: 25/10/2025
// Description: mini shell


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string.h>
#include "main.h"

char prompt[25] = "minishell$";
char *external_commands[155]; 
static char command[20];
char full_cmd[20];
int status = 0;
int pid = 0;
Slist *head = NULL;

void scan_input(char *prompt, char *input_string);
char *get_command(char *input_string);
int check_command_type(char *command);
void extract_external_commands(char **external_commands);
void execute_internal_commands(char *input_string);
void execute_external_commands(char *input_string);
void n_pipe(char *input_string);
void signal_handler(int signum);
int insert_at_first(Slist **head, int pid, char *command);
int delete_first(Slist **head);
void print_list(Slist *head);


int main()
{
    char input_string[25];  // Character variable to store command

    system("clear");    // Clear the terminal

    scan_input(prompt, input_string);   // Read command from user and check command type

    return 0;
}

void scan_input(char *prompt, char *input_string)
{
    char buffer[20];

    signal(SIGINT, signal_handler); // Terminate the process signal -> ctrl + c
    signal(SIGTSTP, signal_handler);    // Stop the process -> ctrl + z

    extract_external_commands(external_commands);   // Extract external commands
    while(1)
    {
        printf("%s ", prompt);  // Print the default prompt in terminal

        input_string[0] = '\0';
        __fpurge(stdin);
		scanf("%[^\n]", input_string);  // read command from user

        memset(buffer, '\0', sizeof(buffer));   // Add default NULL at every end

        strcpy(full_cmd, input_string);
		
        if(strlen(input_string) == 0)   // If input string length is zero print again prompt
            continue;

        // Check PS1 command or not
        if(strncmp(input_string, "PS1=", 4) == 0)
        {
            int space_found = 0;
            for(int i = 0; input_string[i] != '\0'; i++)
            {
                if (input_string[i] == ' ')
                {
                    space_found = 1;
                    break;
                }
            }

            if(space_found)
            {
                printf("%s: Command not found\n", input_string);
            }
            else
            {
                strcpy(prompt, input_string + 4);   // copy and print the user entered prompt
            }
        }

        // Remaining commands
        else
        {
            char *command = get_command(input_string);  // Store the command and options

            int cmd_type = check_command_type(command); // Check command type

            if(cmd_type == BUILTIN)
                execute_internal_commands(input_string);

            else if(cmd_type == EXTERNAL)
            {
                // Create a process
                pid = fork();

                // Child process
                if (pid == 0)
                {
                    // Changing the properties of the signal -> Default signal
                    signal(SIGINT, SIG_DFL);
                    signal(SIGTSTP, SIG_DFL);

                    // Execute the command
                    execute_external_commands(input_string);
                    exit(0);
                }

                // Parent process
                else if (pid > 0)
                {
                    waitpid(pid, &status, WUNTRACED);   // waiting for child process termination
                    pid = 0;    // making pid value as zero after successful termination 
                }
            }

            // Condition for printing the pending processes work in background
            else if (strcmp(input_string, "jobs") == 0)
            {
                print_list(head);
            }

            // Foreground process
            else if (strcmp(input_string, "fg") == 0)
            {
                kill(head->pid, SIGCONT);
                waitpid(head->pid, &status, WUNTRACED);
                delete_first(&head);
            }

            // Background process
            else if (strcmp(input_string, "bg") == 0)
            {
                kill(head->pid, SIGCONT);
                signal(SIGCHLD, signal_handler);
                delete_first(&head);
            }

            // Condition for invalid command
            else if(cmd_type == NO_COMMAND)
                printf("%s: command is not found\n", command);
        }
    }
}

char *get_command(char *input_string)
{
    int i = 0;

    while (input_string[i] != ' ' && input_string[i] != '\0' && input_string[i] != '\n')
    {
        command[i] = input_string[i];   // Storing command without space, NULL and newline
        i++;
    }

    command[i] = '\0';
    return command;
}

int check_command_type(char *command)
{
    char *builtins[] = {"echo", "printf", "read", "cd", "pwd", "pushd", "popd", "dirs", "let", "eval",
						"set", "unset", "export", "declare", "typeset", "readonly", "getopts", "source",
						"exit", "exec", "shopt", "caller", "true", "type", "hash", "bind", "help", NULL};

    // Condition for checking command is builtin command or not
    for(int i = 0; builtins[i] != NULL; i++)
    {
        if(strcmp(command, builtins[i]) == 0)
            return BUILTIN; // If yse return BUILTIN macro
    }    
    
    // Condition for checking command is external command or not
    for(int i = 0; external_commands[i] != NULL; i++)
    {
        if(strcmp(command, external_commands[i]) == 0)
            return EXTERNAL;
    }

    return NO_COMMAND;  // Return invalid command

}

void extract_external_commands(char **external_commands)
{
    int fd = open("external_commands.txt", O_RDONLY);   // Open external command text file in read mode
    // Validate the file
    if(fd == -1)
    {
        printf("INFO: Error opening external_commands.txt file");
        return;
    }

    char buffer[50];
    char ch;
    int row = 0, col = 0;

    // Read character by character
    while(read(fd, &ch, 1) > 0)
    {
        if(ch == '\r')
            continue;

        // Store commands
        if(ch == '\n')
        {
            external_commands[row] = malloc((col + 1) * sizeof(char));
            if(external_commands[row] == NULL)
            {
                printf("Memory allocation faild\n");
                close(fd);
                return;
            }

            for(int i = 0; i < col; i++)
            {
                external_commands[row][i] = buffer[i];
            }
            external_commands[row][col] = '\0';

            row++;
            col = 0;
            memset(buffer, '\0', sizeof(buffer));
        }
        else
        {
            buffer[col++] = ch;
        }
    }

    if (col > 0)
    {
        external_commands[row] = malloc((col + 1) * sizeof(char));
        if (external_commands[row] != NULL)
        {
            for (int i = 0; i < col; i++)
                external_commands[row][i] = buffer[i];
            external_commands[row][col] = '\0';
            row++;
        }
    }

    external_commands[row] = NULL;

    close(fd);
}

void execute_internal_commands(char *input_string)
{
    // Exit from terminal
    if (strcmp(input_string, "exit") == 0)
    {
        exit(0);
    }

    // Print present working directory
    else if (strcmp(input_string, "pwd") == 0)
    {
        char path[256];
        if (getcwd(path, sizeof(path)) != NULL)
            printf("%s\n", path);
        else
            perror("pwd");
    }

    // Change directory
    else if (strncmp(input_string, "cd", 2) == 0)
    {
        char *dir = input_string + 3;
        if (strlen(dir) == 0)
        {
            return;
        }
        if (chdir(dir) != 0)
        {
            perror("cd");
        }
    }

    // echo command
    else if(strncmp(input_string, "echo", 4) == 0)
    {
        char *token = strtok(input_string, " \t\n");
        token = strtok(NULL, " \t\n");

        if(token == NULL)
        {
            return;
        }

        // echo $$ for printing parent id of bash
        else if (strcmp(token, "$$") == 0)
        {
            printf("%d\n", getpid());
        }

        // echo $? for exit status of previous process 
        else if (strcmp(token, "$?") == 0)
        {
            printf("%d\n", WEXITSTATUS(status));
        }

        // echo $SHELL for printing executable path
        else if (strcmp(token, "$SHELL") == 0)
        {
            printf("%s\n", getenv("SHELL"));
        }

        // print the enterd string 
        else
        {
            while (token != NULL)
            {
                printf("%s ", token);
                token = strtok(NULL, " \t\n");
            }
            printf("\n");
        }
    }

}

void execute_external_commands(char *input_string)
{
    // Condition for checking pipe
    if(strchr(input_string, '|') != NULL)
    {
        n_pipe(input_string);   // If pipe found execute n pipes
    }
    else
    {
        char *args[10];
        char *token = strtok(input_string, " ");
        int i = 0;

        // Store command and options 
        while (token != NULL)
        {
            args[i++] = token;
            token = strtok(NULL, " ");
        }
        args[i] = NULL;

        execvp(args[0], args);  // Execute upto NULL
        perror("execvp");
        exit(1);
    }
}

void n_pipe(char *input_string)
{
    char *cmds[10];
    int cmd_count = 0;
    int stdin_fd = 0;

    // Seperate the commands using strtok fuction
    char *token = strtok(input_string, "|");
    while(token != NULL)
    {
        cmds[cmd_count++] = token;
        token = strtok(NULL, "|");
    }
    
    int pipe_fd[2]; // variable for Pipe file descripters

    // Run loop upto commands
    for(int i = 0; i < cmd_count; i++)
    {
        // Dont create pipe for last command
        if(i != cmd_count - 1)  // Check condition for last command
        {
            // Create a pipe
            if(pipe(pipe_fd) == -1)
            {
                perror("pipe"); // Print error if pipe creation fails
                return;
            }
        }

        // Creating a process
        pid = fork();

        // Child process
        if(pid == 0)
        {
            // Taking backup for stdin fd
            if(stdin_fd != 0)
            {
                dup2(stdin_fd, 0);  // Replacing previous stdin fd in 0th index of file table
                close(stdin_fd);
            }
            if(i != cmd_count - 1)  // Upto last command
            {
                // close(pipe_fd[0]);
                dup2(pipe_fd[1], 1);    // Duplcate the pipe writing end in 1st of file table
                close(pipe_fd[1]);
            }

            close(pipe_fd[0]);

            // Execute the command
            char *args[10];
            char *arg = strtok(cmds[i], " ");
            int j = 0;
            while (arg != NULL)
            {
                args[j++] = arg;
                arg = strtok(NULL, " ");
            }
            args[j] = NULL;

            execvp(args[0], args);
            perror("execvp");
            exit(1);
        }

        // Parent process
        else if (pid > 0)
        {
            // waitpid(pid, &status, WUNTRACED);   // wait for child termination
            wait(NULL);

            if (stdin_fd != 0)
                close(stdin_fd);

            // Storing previous command output in backup stdin fd
            if(i != cmd_count - 1)
            {
                close(pipe_fd[1]);
                stdin_fd = pipe_fd[0];
            }
        }
    }
}

void signal_handler(int signum)
{
    // Signal for treminate process
    if (signum == SIGINT)
    {
        if (pid == 0)
        {
            printf("\n%s ", prompt);    // print the prompt
            fflush(stdout);
        }
    }

    // Signal for stop the process
    else if (signum == SIGTSTP)
    {
        if (pid == 0)
        {
            printf("\n%s ", prompt);
            fflush(stdout);
        }
        else
        {
            insert_at_first(&head, pid, full_cmd);
        }
    }

    // Signal for terminating orphan child process
    else if (signum == SIGCHLD)
    {
        waitpid(-1, &status, WUNTRACED);
       // delete_first(&head);
    }
}

int insert_at_first(Slist **head, int pid, char *command)
{
    Slist *new = malloc(sizeof(Slist)); // Creating new node

    if (new == NULL)
        return -1;  // Return if node creation fails

    new -> pid = pid;   // Store pid
    strcpy(new -> command, command);    // store command
    new -> link = *head;    // link with previous node

    *head = new;    // Store new node in head

    return 0;
}

int delete_first(Slist **head)
{
    if (*head == NULL)
        return -1;

    Slist *temp = *head;

    *head = (*head)-> link;
    free(temp); // Deleting node from list

    return 0;
}

void print_list(Slist *head)
{
    if (head == NULL)
    {
        printf("INFO : Resources are cleared\n");
    }

    // Print the background running process
    else
    {
        while (head)
        {
            printf("pid: %d\n", head->pid);
            printf("command: %s\n", head->command);
            printf("\n");

            head = head->link;
        }
        
    }
    
}
