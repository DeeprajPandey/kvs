#!/bin/bash
typeset -i i j;

for i in 1 4 8 16 32;
do
  # if a file exists, delete
  if [ -e "set$i.dat" ]; then
    rm -f "set$i.dat";
  fi;

  let j=1;
  while ((j <= i*16384));
  do
    echo "SET $j 7" >> "set$i.dat";
    let j++;
  done;
done;
