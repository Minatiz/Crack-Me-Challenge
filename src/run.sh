#!/bin/bash -l
varMpiRun=$(which mpirun)
if [ $# -lt 4 ]
  then
    echo "Usage: ./run.sh <nb_procs> <path_to_hostfile> <executable name> <sizeChar*>"
    exit
fi

$varMpiRun -np $1 -hostfile $2 $3 $4
