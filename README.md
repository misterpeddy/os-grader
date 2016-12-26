# os-grader

### Rationale

This repo contains a work in progress (as of 12/26/16).
There are two main purposes behind this project:

- To use as a means of automating grading of c programs in any context (but especially for CS 4414 HW 0.
- To utilize what I have learned in UVa's Operating Systems class (CS 4414 taught by (Andrew Grimshaw)[http://www.cs.virginia.edu/~grimshaw/]). This project uses a ton of topics covered in OS: process creation and management, mutual exclusion and synchronization, threads, socket programming, inter-process communication, shared pipes and file redirects, interrupts and signals, security principles and privilege escalation to name a few. OS has been one of the most useful and interesting classes I've taken and I would love for everyone to get as much out of it as I did. Contact me at pedram@virginia.edu with any questions :)

### Design

#### Receiver: In charge of listening for incoming requests. Each request will have 3 components: a user id, an assignment number and a file containing source code. The receiver receives this request, sanitizes the input and creates a separate process for the Judge to run in.

#### Judge: In charge of  
