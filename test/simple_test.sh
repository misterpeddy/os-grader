#!/bin/bash 
ZERO=0
COUNTER=0
MAX=10
while [ $COUNTER -lt $MAX ]; do
  printf "****** Run #%d *********\n" "$COUNTER" 

  # Make 2 concurrent requests
  ../client/client tester 0 ../client/examples/solution_0.c && ../client/client tester 1 ../client/examples/solution_1.c

  # Take diff with expected solutions files
  diff master/instructor_0_sol.c instructor_0_sol.c > diff0.txt
  diff master/instructor_1_sol.c instructor_1_sol.c > diff1.txt

  # Make sure module 0 has empty diff
  FILESIZE=$(stat -c%s "diff0.txt")
  if [ "$FILESIZE" -ne "$ZERO" ] 
  then
    echo This run failed
    exit 0
  fi

  # Make sure module 1 has empty diff
  FILESIZE=$(stat -c%s "diff1.txt")
  if [ "$FILESIZE" -ne "$ZERO" ] 
  then
    echo This run failed
    exit 0
  fi

  # Clean up for next run
  rm -f diff*.txt instructor_*_sol.c

  printf "****** Run #%d Done *********\n" "$COUNTER" 
  let COUNTER=COUNTER+1 
done
