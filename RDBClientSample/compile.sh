#!/bin/bash

# compile the RDB client example

g++ -o sampleClientRDB ../Common/RDBHandler.cc ExampleConsoleDriverCtrl.cpp -I../Common/
