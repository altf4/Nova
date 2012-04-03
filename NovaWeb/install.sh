#!/bin/bash

# Install helper script
# Dependencies,
#   Nodejs v0.6.12
#   cvv8 headers in /usr/include

ln -f -T -s www /var/www
ln -f -T -s dojo-release-1.7.1 www/dojo

node-waf configure
node-waf

# To run,
# bash$ NODE_PATH=build/Release node tst.js
