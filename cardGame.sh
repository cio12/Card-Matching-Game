#!/bin/bash

g++ -std=c++11 main.cpp -lpthread

echo Starting Game 1
./a.out 1 log_1.txt
echo Game 1 Over

echo Starting Game 2
./a.out 2 log_2.txt
echo Game 2 Over

echo Starting Game 3
./a.out 3 log_3.txt
echo Game 3 Over

echo Starting Game 4
./a.out 4 log_4.txt
echo Game 4 Over

echo Starting Game 5
./a.out 5 log_5.txt
echo Game 5 Over
