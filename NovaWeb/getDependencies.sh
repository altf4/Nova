#!/bin/bash
# This is a quick install script for nodejs and cvv8 headers

# Get nodejs
wget https://github.com/joyent/node/zipball/v0.6.12
unzip v0.6.12
cd joyent-node-346a3a8
./configure
make
make install

# Get cvv8
wget http://v8-juice.googlecode.com/files/libv8-convert-20120219.tar.gz
tar -xf libv8-convert-20120219.tar.gz
sudo cp -fr libv8-convert-20120219/include/cvv8 /usr/include
