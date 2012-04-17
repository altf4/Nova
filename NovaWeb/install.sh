#!/bin/bash
#######################################################
# Install helper script
#######################################################


#######################################################
# Dependencies you must manually get first
#######################################################
#   Nodejs v0.6.12
#	Currently at: https://github.com/joyent/node/zipball/v0.6.12
#	Install with the standard ./configure && make && sudo make install
#
#######################################################
#   cvv8 headers in /usr/include
#	Currently at: http://v8-juice.googlecode.com/files/libv8-convert-20120219.tar.gz
#	Install by copying the include/cvv8 folder to /usr/include/
#
#######################################################

# Note: If you have another web server using /var/www,
# you may need to change the path in tst.js or delete
# what's there.
echo "========================================"
echo "Making symlink /var/www -> ~/Code/Nova/NovaWeb/www"
echo "========================================"
ln -f -T -s ~/Code/Nova/NovaWeb/www /var/www

echo "========================================"
echo "Making a symlink to our current Dojo version"
echo "========================================"
ln -f -T -s dojo-release-1.7.1 www/dojo

echo "========================================"
echo "Installing Nowjs"
echo "========================================"
npm install now -g 

echo "========================================"
echo "Configuring NOVA node"
echo "========================================"
node-waf configure

# Compile the nova node
# (This needs to be done every time the source changes)
echo "========================================"
echo "Building NOVA node"
echo "========================================"
node-waf

# To run the Nova Web interface,
# bash$ NODE_PATH=build/Release node tst.js

echo "========================================"
echo "Installation finished"
echo "To run the web interface, run "NODE_PATH=build/Release node tst.js" from the NovaWeb folder"
echo "========================================"
