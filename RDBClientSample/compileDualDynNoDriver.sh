#!/bin/bash

# compile the RDB client example

g++ -o dualDynNoDriver ../Common/RDBHandler.cc DualDynNoDriver.cpp -I../Common
