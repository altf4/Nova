#!/bin/bash
# This is a quick install script for nodejs and cvv8 headers

sudo apt-get install python-software-properties
sudo add-apt-repository ppa:chris-lea/node.js
sudo apt-get update
sudo apt-get install nodejs nodejs-dev npm

wget http://v8-juice.googlecode.com/files/libv8-convert-20120219.tar.gz
tar -xf libv8-convert-20120219.tar.gz
sudo cp -fr libv8-convert-20120219/include/cvv8 /usr/include
