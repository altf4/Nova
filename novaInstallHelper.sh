#bash

mkdir -p ~/Code

apt-get install libann-dev libpcap0.8-dev build-essential g++ git libqt4-dev libsparsehash-dev libghc6-zlib-dev libapr1-dev libaprutil1-dev libcap2-bin libboost-serialization-dev libnotify-dev mysql-server libcurl3 libcurl4-gnutls-dev unzip iptables libevent-dev libdumbnet-dev libpcap-dev libpcre3-dev libedit-dev bison flex libtool automake 

cd ~/Code
git clone git://github.com/DataSoft/Honeyd.git
git clone git://github.com/DataSoft/Nova.git

cd ~/Code/Honeyd
./autogen.sh
automake
./configure
make
make install

cd ~/Code/Nova
make
make install

cd ~/Code/Nova/NovaWeb
bash getDependencies.sh
npm install -g forever

cd ~/Code/Nova
make web
make install-web

bash ~/Code/Nova/Installer/nova_init

cd ~/Code
wget http://nmap.org/dist/nmap-6.01.tar.bz2
tar -xf nmap-6.01.tar.bz2
cd nmap-6.01
./configure
make
make install



