#bash

mkdir -p ~/Code

apt-get install git build-essential libann-dev libpcap0.8-dev libsparsehash-dev libboost-program-options-dev libboost-serialization-dev libnotify-dev sqlite3 libsqlite3-dev libcurl3 libcurl4-gnutls-dev iptables libevent-dev libdumbnet-dev libpcap-dev libpcre3-dev libedit-dev bison flex libtool automake

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
git checkout -f integration
make debug
make novagui-debug

cd ~/Code/Nova/NovaWeb
bash getDependencies.sh
npm install -g forever

cd ~/Code/Nova
make web
make install

bash ~/Code/Nova/Installer/nova_init

cd ~/Code
wget http://nmap.org/dist/nmap-6.01.tar.bz2
tar -xf nmap-6.01.tar.bz2
cd nmap-6.01
./configure
make
make install



