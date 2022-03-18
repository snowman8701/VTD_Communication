#!/bin/tcsh

# compile the RDB client example

g++ -o sampleClientProxyPkg ../Common/RDBHandler.cc ExampleConsoleProxyPkg.cpp -I../Common/
