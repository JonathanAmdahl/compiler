#!/bin/bash

# file error handling 
if [ ! -d "./tests" ]; then
    echo "Error: tests directory not found!"
    exit 1
fi

if [ ! -e "./a.out" ]; then
    echo "Error: a.out not found!"
    exit 1
fi

if [ ! -x "./a.out" ]; then
    echo "Error: a.out not executable!"
    exit 1
fi

let count=0; 
# getting the names of all files in the for loop
for f in $(ls ./tests/*.txt); do
  ./a.out <$f> ./tests/`basename $f ./txt`.output; 
done;

# getting all expected output files
for f in $(ls ./tests/*.output); do 
  diff -Bw $f ./tests/`basename $f .output`.expected > ./tests/`basename $f .output`.diff;  
done

for f in $(ls ./tests/*.diff); do 
  echo "=================================================================";
  echo "FILE:" `basename $f .output`;
  echo "=================================================================";
  if [ -s $f ]; then
    echo "Lines that are different between the Expected file and Your Output: "; 
    cat ./tests/`basename $f .diff`;
    echo -e "\n---------------------------------------------------------";
    cat $f
  else 
    count=$((count+1)); 
    echo "YAY NO ERRORS :)!";
    echo "LEEEEEEEEEEEEEEEEEEEETTTTTTTTTTTTTTSSSSSSSSSS GOOOOOOOOOOOOOOOOOOOOOO"; 
  fi
done

echo "Test Cases Passed: " $count

#rm tests/*.output
rm tests/*.diff
