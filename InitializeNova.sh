#!/bin/bash

# Make sure we have root privs
if [[ $EUID -ne 0 ]]; then
   echo "You must be root to run this script. Aborting...";
   exit 1;
fi

# Add the user to the nova group
usermod -a -G nova $SUDO_USER

# Add /etc/sudoers.d to the sudoers file if needed
if grep -q "#includedir /etc/sudoers.d" /etc/sudoers; then
    echo "Note: sudoers.d is arleady included in /etc/sudoers"
else
    echo "No sudoers.d included in /etc/sudoers; attempting to add it";

    cp -f /etc/sudoers /tmp/sudoers.nova
    echo "#includedir /etc/sudoers.d" >> /tmp/sudoers.nova;

    if visudo -c -q -f /tmp/sudoers.nova; then
	echo "Saving changes to /etc/sudoers";
	cp -f /tmp/sudoers.nova /etc/sudoers;
    else
	echo "There was a problem with the sudoers file, aborting";
    fi

    rm -f /tmp/sudoers.nova
fi

