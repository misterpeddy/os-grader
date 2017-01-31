#!/bin/bash 
ZERO=0
LOG_FILE=logfile.txt


# Make a single request
../client/client tester 0 ../client/examples/solution_0.c

# Take diff with expected solutions files
diff master/instructor_0_sol.c instructor_0_sol.c > diff.txt

# Make sure module 0 has empty diff
FILESIZE=$(stat -c%s "diff.txt")
if [ "$FILESIZE" -ne "$ZERO" ] 
then
  echo This run failed
  exit 0
fi

# Clean up for next run
rm -f diff.txt instructor_0_sol.c
let COUNTER=COUNTER+1 
