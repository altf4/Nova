#!/bin/bash

# Make sure we have root privs
if [[ $EUID -ne 0 ]]; then
   echo "You must be root to run this script. Aborting...";
   exit 1;
fi

# Add the user to the nova group
usermod -a -G nova $USER

