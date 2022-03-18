#!/bin/bash

# compile the RDB client example

g++ -o get_image ../Common/RDBHandler.cc get_image.cpp -I../Common
