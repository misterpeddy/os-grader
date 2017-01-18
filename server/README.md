# Server Code

## Setup
Setup should hopefully be fairly straightforward. Simply clone the repo, and run make which creates all the binaries in the bin directory.

## Usage
The coordinator binary is the main executable for the server. It can be run in three modes: 
-module: To retrieve information about a module
-user: To retrieve information about a user
-runserver: To run the server that does the actual job of accepting submissions from users

## Architecture
The overall architecture of the server is quite simple. When the coordinator is started in runserver mode, it reads in information for all modules registered in settings.h. It then connects to the sqlite database in db, (creates it if it doesn't exist) starts up the server and begins listening for incoming connections. As a request is received, it moves it to a different port and continues listening. For that request, it first validates it to make sure it is for a legitimate module. Then, it spawns up a judge process, which does the actual compilation, running and judging of the submission. The judge process updates coordinator of events (compilation result, run result, diff result, etc.) by sending acknowledgement messages over a shared pipe. The coordinator then relays these message over the open socket to the client. The coordinator also uses a signaling mechanism to detect judges who have run for too long. In the case of a compilation failure, the coordinator sends the error file to the client.



