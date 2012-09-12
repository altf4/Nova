#!/bin/bash

PWD=`pwd`

if [ -z $BUILDDIR ]; then
	BUILDDIR=$PWD/nova-build
fi

echo "Build Dir is $BUILDDIR"

check_err() {
	ERR=$?
	if [ "$ERR" -ne "0" ] ; then
		echo "Error occurred during build process; terminating script!"
		exit $ERR
	fi
}

mkdir -p ${BUILDDIR}

echo "##############################################################################"
echo "#                          NOVA DEPENDENCY CHECK                             #"
echo "##############################################################################"
apt-get -y install git build-essential libann-dev libpcap0.8-dev libsparsehash-dev libboost-program-options-dev libboost-serialization-dev libnotify-dev sqlite3 libsqlite3-dev libcurl3 libcurl4-gnutls-dev iptables libevent-dev libdumbnet-dev libpcap-dev libpcre3-dev libedit-dev bison flex libtool automake libcap2-bin
check_err

echo "##############################################################################"
echo "#                         DOWNLOADING NOVA FROM GIT                          #"
echo "##############################################################################"
cd ${BUILDDIR}
git clone git://github.com/DataSoft/Honeyd.git
check_err
git clone git://github.com/DataSoft/Nova.git
check_err

echo "##############################################################################"
echo "#                              BUILDING HONEYD                               #"
echo "##############################################################################"
cd ${BUILDDIR}/Honeyd
./autogen.sh
check_err
automake
check_err
./configure
check_err
make -j2
check_err
make install
check_err

cd ${BUILDDIR}/Nova
git checkout -f integration
check_err

echo "##############################################################################"
echo "#                             BUILDING NOVA                                  #"
echo "##############################################################################"
cd ${BUILDDIR}/Nova/Quasar
bash getDependencies.sh
npm install -g forever
check_err

cd ${BUILDDIR}/Nova
make debug
check_err
make install
check_err

bash ${BUILDDIR}/Nova/Installer/nova_init

echo "##############################################################################"
echo "#                             FETCHING NMAP 6                                #"
echo "##############################################################################"
cd ${BUILDDIR}
wget http://nmap.org/dist/nmap-6.01.tar.bz2
check_err
tar -xf nmap-6.01.tar.bz2
check_err
cd nmap-6.01
./configure
check_err
make -j2
check_err
make install
check_err

echo "##############################################################################"
echo "#                                    DONE                                    #"
echo "##############################################################################"
