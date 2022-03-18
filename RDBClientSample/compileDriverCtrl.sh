#!/bin/tcsh

# compile the RDB client example

g++ -o sampleClientRDBDriverCtrl ../Common/RDBHandler.cc ExampleConsoleDriverCtrl.cpp -I../Common/
g++ -o sampleClientRDBDriverCtrlTorque ../Common/RDBHandler.cc ExampleConsoleDriverCtrlTorque.cpp -I../Common/
