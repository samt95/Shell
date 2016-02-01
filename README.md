# Shell
A simple shell program implemented in C.


###Features:
 - external program execution (will search all directories in PATH environment variable)
  - For example, the 'ls' command will work as expected, calling the program in the /bin folder
 - background process execution
  - use 'bg [programName]' command to execute [programName] in the background
 - background process management (list, kill, resume, stop)
  - 'bglist' will list running background processes
  - 'bgkill [processNum]' will terminate the process with the specified process number
  - 'stop [processNum]' will pause execution of the process with the specified process number
  - 'start [processNum]' will resume execution of the currently stopped process witht the specified process number


###Use:
For use on Unix systems, navigate to the directory where main.c and Makefile are and type the following commands in the terminal:
- make
- ./shell

Yes, this is as simple as a makefile can get, but any practice is good practice.
