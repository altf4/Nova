
#The default build should be a release build
all: release

all-the-things: release 
	$(MAKE) web

#Release Target
release:
	$(MAKE) novalib-release
	$(MAKE) ui_core-release
	$(MAKE) release-helper

release-helper: novad-release novacli-release novatrainer-release hhconfig-release

#Debug target
debug:
	$(MAKE) novalib-debug
	$(MAKE) ui_core-debug
	$(MAKE) debug-helper

debug-helper: novad-debug novacli-debug novatrainer-debug hhconfig-debug

#Nova Library
novalib-release:
	$(MAKE) -C NovaLibrary/Release
	cp NovaLibrary/Release/libNovaLibrary.a NovaLibrary/

novalib-debug:
	$(MAKE) -C NovaLibrary/Debug
	cp NovaLibrary/Debug/libNovaLibrary.a NovaLibrary/

#novad
novad-release:
	$(MAKE) -C Novad/Release
	cp Novad/Release/novad Novad/

novad-debug:
	$(MAKE) -C Novad/Debug
	cp Novad/Debug/novad Novad/

#UI_Core
ui_core-release:
	$(MAKE) -C Nova_UI_Core/Release
	cp Nova_UI_Core/Release/libNova_UI_Core.so Nova_UI_Core/

ui_core-debug:
	$(MAKE) -C Nova_UI_Core/Debug
	cp Nova_UI_Core/Debug/libNova_UI_Core.so Nova_UI_Core/

#nova CLI
novacli-release:
	$(MAKE) -C NovaCLI/Release
	cp NovaCLI/Release/novacli NovaCLI/

novacli-debug:
	$(MAKE) -C NovaCLI/Debug
	cp NovaCLI/Debug/novacli NovaCLI/

# Nova trainer
novatrainer-debug:
	$(MAKE) -C NovaTrainer/Debug
	cp NovaTrainer/Debug/novatrainer NovaTrainer/novatrainer

novatrainer-release:
	$(MAKE) -C NovaTrainer/Release
	cp NovaTrainer/Release/novatrainer NovaTrainer/novatrainer

#Web UI
web:
	cd NovaWeb;npm --unsafe-perm install
	cd NovaWeb/NodeNovaConfig;npm --unsafe-perm install

#Honeyd HostConfig
hhconfig-release:
	$(MAKE) -C HoneydHostConfig/Release
	cp HoneydHostConfig/Release/honeydhostconfig HoneydHostConfig/

hhconfig-debug:
	$(MAKE) -C HoneydHostConfig/Debug
	cp HoneydHostConfig/Debug/honeydhostconfig HoneydHostConfig/

coverageTests: test-prepare
	$(MAKE) -C NovaLibrary/Coverage
	$(MAKE) -C Nova_UI_Core/Coverage
	$(MAKE) -C NovaTest/Coverage

#Unit tests
test: debug test-prepare
	$(MAKE) -C NovaTest/Debug

test-prepare:
	# Make the folder if it doesn't exist
	mkdir -p NovaTest/NovadSource
	# Make new links to the cpp files
	rm -fr NovaTest/NovadSource/*.cpp
	ln Novad/src/*.cpp NovaTest/NovadSource/
	# Make new links to the h files
	rm -fr NovaTest/NovadSource/*.h
	ln Novad/src/*.h NovaTest/NovadSource/
	# Delete the link to Main so we don't have multiple def of main()
	rm -f NovaTest/NovadSource/Main.cpp

clean: clean-lib clean-ui-core clean-novad clean-test clean-hhconfig clean-web clean-novatrainer clean-staging
	

#remove binaries from staging area
clean-staging:
	rm -f NovaCLI/novacli
	rm -f Novad/novad
	rm -f Nova_UI_Core/libNova_UI_Core.so
	rm -f NovaLibrary/libNovaLibrary.a

#Removes created man pages
clean-man:
	rm -f Installer/Read/manpages/*.gz

clean-novad: clean-novad-debug clean-novad-release

clean-novad-debug:
	$(MAKE) -C Novad/Debug clean

clean-novad-release:
	$(MAKE) -C Novad/Release clean

clean-ui-core: clean-ui-core-debug clean-ui-core-release clean-ui-core-coverage

clean-ui-core-debug:
	$(MAKE) -C Nova_UI_Core/Debug clean
	rm -f Nova_UI_Core/Debug/Nova_UI_Core

clean-ui-core-release:
	$(MAKE) -C Nova_UI_Core/Release clean
	rm -f Nova_UI_Core/Release/Nova_UI_Core

clean-ui-core-coverage:
	$(MAKE) -C Nova_UI_Core/Coverage clean
	rm -f Nova_UI_Core/Coverage/Nova_UI_Core

clean-lib: clean-lib-debug clean-lib-release clean-lib-coverage
	
clean-lib-debug:
	$(MAKE) -C NovaLibrary/Debug clean

clean-lib-release:
	$(MAKE) -C NovaLibrary/Release clean

clean-lib-coverage:
	$(MAKE) -C NovaLibrary/Coverage clean

clean-cli: clean-cli-debug clean-cli-release

clean-cli-debug:
	$(MAKE) -C NovaCLI/Debug clean

clean-cli-release:
	$(MAKE) -C NovaCLI/Release clean

clean-test:
	rm -fr NovaTest/NovadSource/*
	rm -f NovaTest/Debug/NovadSource/*.d
	rm -f NovaTest/Debug/NovadSource/*.o
	rm -f NovaTest/Debug/src/NovadSource/*.d
	rm -f NovaTest/Debug/src/NovadSource/*.o
	$(MAKE) -C NovaTest/Debug clean

clean-web:
	-cd NovaWeb/NodeNovaConfig; node-waf clean
	rm -rf NovaWeb/node_modules/forever/node_modules/utile/node_modules/.bin/
	rm -rf NovaWeb/node_modules/forever/node_modules/.bin
	rm -rf NovaWeb/node_modules/socket.io/node_modules/socket.io-client/node_modules/.bin/
	rm -rf NovaWeb/node_modules/.bin/

clean-hhconfig: clean-hhconfig-debug clean-hhconfig-release
	
clean-hhconfig-debug:
	$(MAKE) -C HoneydHostConfig/Debug clean

clean-hhconfig-release:
	$(MAKE) -C HoneydHostConfig/Release clean


clean-novatrainer: clean-novatrainer-debug clean-novatrainer-release

clean-novatrainer-debug:
	$(MAKE) -C NovaTrainer/Debug clean

clean-novatrainer-release:
	$(MAKE) -C NovaTrainer/Release clean

#Installation (requires root)
install: install-data
	$(MAKE) install-helper

install-helper: install-docs install-cli install-novad install-ui-core install-hhconfig install-novatrainer install-web
	sh debian/postinst
	-bash Installer/createDatabase.sh

install-data:
	#make folder in etc with path locations to nova files
	mkdir -p $(DESTDIR)/usr/bin
	mkdir -p $(DESTDIR)/usr/lib
	mkdir -p $(DESTDIR)/usr/share/applications
	mkdir -p $(DESTDIR)/usr/share/nova
	mkdir -p $(DESTDIR)/usr/share/man/man1
	mkdir -p $(DESTDIR)/var/log/honeyd
	mkdir -p $(DESTDIR)/etc/nova
	mkdir -p $(DESTDIR)/etc/rsyslog.d/
	mkdir -p $(DESTDIR)/etc/sysctl.d/
	mkdir -p $(DESTDIR)/etc/bash_completion.d/
	mkdir -p $(DESTDIR)/etc/sudoers.d/
	
	install Installer/Read/paths $(DESTDIR)/etc/nova
	install Installer/Read/nmap-os-db $(DESTDIR)/etc/nova
	install Installer/Read/nmap-mac-prefixes $(DESTDIR)/etc/nova
	#Copy the hidden directories and files
	cp -frup Installer/Write/nova/nova $(DESTDIR)/etc/nova
	#Copy the scripts and logs
	cp -frup Installer/Write/nova $(DESTDIR)/usr/share/
	cp -frup Installer/Read/icons $(DESTDIR)/usr/share/nova
	install Installer/nova_init $(DESTDIR)/usr/bin
	#Install permissions
	install Installer/Read/sudoers_nova $(DESTDIR)/etc/sudoers.d/ --mode=0440
	install Installer/Read/40-nova.conf $(DESTDIR)/etc/rsyslog.d/ --mode=664
	install Installer/Read/30-novactl.conf $(DESTDIR)/etc/sysctl.d/ --mode=0440
	# Copy the bash completion files
	install Installer/Read/novacli $(DESTDIR)/etc/bash_completion.d/ --mode=755

install-pcap-debug:
	#debug sudoers file that allows sudo gdb to pcap without password prompt
	install Installer/Read/sudoers_nova_debug $(DESTDIR)/etc/sudoers.d/ --mode=0440

install-docs:
	# TODO: Combine man pages
	gzip -c Installer/Read/manpages/novad.1 > Installer/Read/manpages/novad.1.gz
	gzip -c Installer/Read/manpages/novacli.1 > Installer/Read/manpages/novacli.1.gz
	gzip -c Installer/Read/manpages/novaweb.1 > Installer/Read/manpages/novaweb.1.gz
	install Installer/Read/manpages/*.1.gz $(DESTDIR)/usr/share/man/man1

install-web:
	cp -frup NovaWeb $(DESTDIR)/usr/share/nova
	tar -C $(DESTDIR)/usr/share/nova/NovaWeb/www -xf NovaWeb/dojo-release-1.7.0.tar.gz
	#mv $(DESTDIR)/usr/share/nova/NovaWeb/www/dojo-release-1.7.3 $(DESTDIR)/usr/share/nova/NovaWeb/www/dojo
	-install NovaWeb/novaweb $(DESTDIR)/usr/bin/novaweb

install-hhconfig:
	-install HoneydHostConfig/honeydhostconfig $(DESTDIR)/usr/bin/honeydhostconfig
	-install Installer/Read/sudoers_HHConfig $(DESTDIR)/etc/sudoers.d/ --mode=0440

install-novad:
	install Novad/novad $(DESTDIR)/usr/bin

install-ui-core:
	install Nova_UI_Core/libNova_UI_Core.so $(DESTDIR)/usr/lib

install-cli:
	install NovaCLI/novacli $(DESTDIR)/usr/bin

install-novatrainer:
	install NovaTrainer/novatrainer $(DESTDIR)/usr/bin


#Uninstall
uninstall: uninstall-files uninstall-permissions

uninstall-files:
	rm -rf $(DESTDIR)/etc/nova
	rm -rf $(DESTDIR)/usr/share/nova
	rm -f $(DESTDIR)/usr/bin/novacli
	rm -f $(DESTDIR)/usr/bin/novad
	rm -f $(DESTDIR)/usr/bin/honeydhostconfig
	rm -f $(DESTDIR)/usr/bin/nova_mailer
	rm -f $(DESTDIR)/usr/bin/nova_init
	rm -f $(DESTDIR)/usr/bin/novaweb
	rm -f $(DESTDIR)/usr/bin/novatrainer
	rm -f $(DESTDIR)/usr/lib/libNova_UI_Core.so
	rm -f $(DESTDIR)/etc/sudoers.d/sudoers_nova
	rm -f $(DESTDIR)/etc/sudoers.d/sudoers_nova_debug
	rm -f $(DESTDIR)/etc/sudoers.d/sudoers_HHConfig
	rm -f $(DESTDIR)/etc/rsyslog.d/40-nova.conf
	rm -f $(DESTDIR)/etc/sysctl.d/30-novactl.conf

uninstall-permissions:
	sh debian/postrm

# Reinstall nova without messing up the permissions
reinstall: uninstall-files
	$(MAKE) install

reinstall-debug: uninstall-files
	$(MAKE) install
	novacli writesetting SERVICE_PREFERENCES 0:0+\;1:5+\;2:6+\;

# Does a fresh uninstall, clean, build, and install
reset: uninstall-files
	$(MAKE) clean
	$(MAKE) release
	$(MAKE) test 
	$(MAKE) install

reset-debug: 
	$(MAKE) clean
	$(MAKE) debug
	$(MAKE) web
	$(MAKE) test
	$(MAKE) reinstall-debug

