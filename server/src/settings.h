#ifndef SETTINGS_H
#define SETTINGS_H

#define APP_ROOT      "/home/pedram/repos/os-grader/server/"
#define BIN_ROOT      APP_ROOT"bin/"
#define MODULES_ROOT  APP_ROOT"modules/"

#define NUM_REGISTERED_MODULES 4

const char *REGISTERED_MODULES[] = {"0", "1", "2", "3"};

/*********************************************NOTES**********************************************/
/************************************************************************************************/
/*** For each registered module, the grading system assumes you have done the following *********/ 
/*** 1. Created a directory in APP_ROOT/modules with the name of the module *********************/
/*** 2. The solution file is in that directory with name "solution_<module name>.c" *************/
/*** 3. For every input file in that directort, there is an output file "out_<input filename> ***/
/*** 4. Input files may be named anything that doesn't have the prefix "out" or "solution" ******/
/************************************************************************************************/
/************************************************************************************************/

#endif
