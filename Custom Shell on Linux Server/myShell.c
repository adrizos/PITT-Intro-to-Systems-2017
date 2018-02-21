// Alex Drizos
// myShell - custom written shell program for Linux server
// University of Pittsburgh 

#ifndef	_GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#define BUFFSIZE  500
#define ARGARRAYSIZE (BUFFSIZE/2)+1
#define DELIM " \t\n()|&;"
#define PUSHMAX 4


int main()
{

	//variables
	char buffer[BUFFSIZE];
	int pushCounter = 0;
	char* pushStack[PUSHMAX];

	//ignores sigint for ctrl c use in shell process
	signal(SIGINT, SIG_IGN);

	//*****loop to get input
	while (true)
	{
		//handle input
		printf("myShell> ");
		fgets(buffer, BUFFSIZE, stdin);

		//parse input
		char* tokFirstPtr = strtok(buffer, DELIM);

		//check if nothing entered
		if (tokFirstPtr == NULL)
		{
			continue;
		}

		//check if user entered "EXIT"
		if (strcmp(tokFirstPtr, "exit") == 0)
		{
			//check if theres an extra arg
			char* tok2Ptr = strtok(NULL, DELIM);
			//case 1,  no extra arg
			if (tok2Ptr == NULL)
			{
				exit(0);
			}
			//case 2, extra arg
			else
			{
				exit(atoi(tok2Ptr));
			}
		}

		//!!!!!!!!TODO this code is unchecked as now

		//check if user entered "cd"
		if (strcmp(tokFirstPtr, "cd") == 0)
		{
			//get data path
			char* dataPathPtr = strtok(NULL, DELIM);
			//pass datat path to chdir() function
			int ret = chdir(dataPathPtr);
			//check return value: 0=success -1=failure
			if (ret == -1)
			{
				printf("Could not complete request. \n");

			}
			continue;	//return to user entry loop
		}

		//pushd / popd
		// check if user entered pushd
		if (strcmp(tokFirstPtr, "pushd") == 0)
		{
			//case 1 - the stack is full
			if (pushCounter >= PUSHMAX)
			{
				printf("Sorry, the push stack is full. \n");

			}
			//case 2 - stack is not full
			else
			{
				//get user data path
				char* userDataPathPtr = strtok(NULL, DELIM);
				//backup current path
				char* backupPath = get_current_dir_name();
				//try to jump to next dir
				int ret = chdir(userDataPathPtr);
				//check return value: 0=success -1=failure
				if (ret == -1)
				{
					printf("Could not complete request. \n");
					//free heap allocation from get_current_dir_name
					free(backupPath);

				}
				else 	//cd worked
				{
					//push prev dir onto stack
					pushStack[pushCounter] = backupPath;

					//increment pushCounter
					pushCounter++;
				}
			}
			continue;	//return to user entry loop
		} //end of if pushd

		//check if user entered popd
		if (strcmp(tokFirstPtr, "popd") == 0)
		{
			//case 1 - stack is empty
			if (pushCounter == 0)
			{
				printf("Sorry, the stack is already empty. Could not complete request\n");
				continue;
			}

			//case 2 - stack is not empty
			//check if pathname is still valid
			//try to jump to prev dir
			int ret = chdir(pushStack[pushCounter-1]);
			//check return value: 0=success -1=failure
			if (ret == -1)
			{
				printf("Previous path is no longer valid. \n");
			}
			//pop the previous path name from the stack
			//free heap space
			free(pushStack[pushCounter-1]);
			//decrement pushCounter
			pushCounter--;
			continue;
		}	//end of if popd

		//check if user wants to run a program
		//treat all input that is not supported builtin commands as user wanting to run program
		pid_t pid = fork();
		//check fork() success
		if (pid == -1)
		{
			printf("Fork failed. \n");
			continue;
		}

		//code for child
		if (pid == 0)	//child responds
		{
			//resumes default sigint for ctrl c use in child process
			signal(SIGINT, SIG_DFL);


			//loop to fill an array with the parseable data entered by user
			//stop filling program arguments if a > or < flag is reached, signaling redirection info to be used elsewhere
			char* argumentsArray[ARGARRAYSIZE];
			bool inputFlag = false;
			bool outputFlag = false;
			int outputIndex;
			int inputIndex;
			argumentsArray[0] = tokFirstPtr;
			int i;
			for (i = 1; i < ARGARRAYSIZE; i++)
			{
				argumentsArray[i] = strtok(NULL, DELIM);
				if (argumentsArray[i] == NULL)
   				{
					break;	//nothing left to parse
				}
				else if (strcmp(argumentsArray[i], ">") == 0)
				{
					if (outputFlag == true)
					{
						printf("Second redirection of same type not supported.\n");
						exit(0);
					}
					outputFlag = true;
					argumentsArray[i] = NULL;
					outputIndex = i+1;	//mark the index of the filename text after the >
				}
				else if (strcmp(argumentsArray[i], "<") == 0)
				{
					if (inputFlag == true)
					{
						printf("Second redirection of same type not supported.\n");
						exit(0);
					}
					inputFlag = true;
					argumentsArray[i] = NULL;
					inputIndex = i+1;	//mark the index of the filename text after the <
				}


			}//end of loop to fill array with user data


			/*check for redirection file to open*/

			//handle input redirection
			if (inputFlag)
			{
				//open the file
				int descriptor = open(argumentsArray[inputIndex], O_RDONLY);
				if (descriptor == -1)	//couldn't open file
				{
					printf("Couldn't open file.\n");
					exit(0); //exit child
				}

				int dupReturn = dup2(descriptor, 0);
				if (dupReturn == -1)
				{
					printf("Couldn't dup. \n");
					exit(0); //exit child
				}
				int closeReturn = close(descriptor);
				if (closeReturn == -1)
				{
					printf("Couldn't close...\n");
					exit(0);
				}
			} // end of handle input redirection
			//handle output redirection
			if (outputFlag)
			{
				//open the file
				int descriptor = open(argumentsArray[outputIndex], O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
				if (descriptor == -1) //couldn't open file
				{
					printf("Couldn't open file.\n");
					exit(0);	//exit child
				}

				int dupReturn = dup2(descriptor, 1);
				if (dupReturn == -1)
				{
					printf("Couldn't dup.\n");
					exit(0); // exit child
				}
				int closeReturn = close(descriptor);
				if (closeReturn == -1)
				{
					printf("Couldn't close...\n");
					exit(0); //exit child
				}
			} //end of handle output redirection


			//call execvp with filename and possible arguments
			execvp(tokFirstPtr, argumentsArray);
			//in case of error
			const char str[] = "couldn't run the program. \n";
			perror(str);
			exit(0);

		}
		//code for parent
		else	//parent responds  - happens roughly at the same time as above if
		{
			int stat;
			int childPid = waitpid(pid, &stat, 0);
			//check childPid for waitpid errors
			if (childPid == -1)
			{
				printf("Waitpid failed. \n");
			}

			//check to see if child exited normally

			else if (!WIFEXITED(stat))
			{

				if (WIFSIGNALED(stat))
				{
					printf("Exited with signal: %d\n", WTERMSIG(stat));
				}
				else
				{
					//const int es = WEXITSTATUS(stat);
					//printf("\nexited with code %d, \n", es);
					printf("Exited abnormally...");
				}
			}
		}



	}//end of user input loop

}
