#!/bin/bash
./FirstStep "$1"
if [ $? -eq 1 ]
then 
	exit 1
fi
python3 -W ignore src/SecondStep.py
./ThirdStep "$1"