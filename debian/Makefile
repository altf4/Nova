
#The default build should be a release build
all: release
	
#Release Target
release:
	$(MAKE) novalib-release
	$(MAKE) ui_core-release
	$(MAKE) release-helper

release-helper: novad-release novacli-release

#Debug target
debug:
	$(MAKE) novalib-debug
	$(MAKE) ui_core-debug
	$(MAKE) debug-helper

debug-helper: novad-debug novacli-debug

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

#Web UI
web:
	cd NovaWeb;npm --unsafe-perm install
	cd NovaWeb/NodeNovaConfig;npm --unsafe-perm install

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

# Make debug + test
all-test: test

#Cleans both Release and Debug
clean: clean-debug clean-release clean-test
	rm -f Installer/Read/manpages/*.gz
	#remove binaries from staging area
	rm -f NovaCLI/novacli
	rm -f Novad/novad
	rm -f Nova_UI_Core/libNova_UI_Core.so
	rm -f NovaLibrary/libNovaLibrary.a

clean-debug:
	$(MAKE) -C NovaLibrary/Debug clean
	$(MAKE) -C Nova_UI_Core/Debug clean
	$(MAKE) -C Novad/Debug clean
	$(MAKE) -C NovaCLI/Debug clean
	rm -f Nova_UI_Core/Debug/Nova_UI_Core

clean-release:
	$(MAKE) -C NovaLibrary/Release clean
	$(MAKE) -C Nova_UI_Core/Release clean
	$(MAKE) -C Novad/Release clean
	$(MAKE) -C NovaCLI/Release clean
	rm -f Nova_UI_Core/Release/Nova_UI_Core

clean-test:
	rm -fr NovaTest/NovadSource/*
	rm -f NovaTest/Debug/NovadSource/*.d
	rm -f NovaTest/Debug/NovadSource/*.o
	rm -f NovaTest/Debug/src/NovadSource/*.d
	rm -f NovaTest/Debug/src/NovadSource/*.o
	$(MAKE) -C NovaTest/Debug clean

clean-web:
	cd NovaWeb/NodeNovaConfig; node-waf clean
	rm -rf NovaWeb/node_modules/forever/node_modules/utile/node_modules/.bin/
	rm -rf NovaWeb/node_modules/forever/node_modules/.bin
	rm -rf NovaWeb/node_modules/socket.io/node_modules/socket.io-client/node_modules/.bin/
	rm -rf NovaWeb/node_modules/.bin/


#Installation (requires root)
install: install-data install-docs
	mkdir -p $(DESTDIR)/usr/bin
	mkdir -p $(DESTDIR)/usr/lib
	install NovaCLI/novacli $(DESTDIR)/usr/bin
	install Novad/novad $(DESTDIR)/usr/bin
	install Nova_UI_Core/libNova_UI_Core.so $(DESTDIR)/usr/lib
	#sh debian/postinst

install-data:
	#make folder in etc with path locations to nova files
	mkdir -p $(DESTDIR)/etc/nova
	install Installer/Read/paths $(DESTDIR)/etc/nova
	install Installer/Read/nmap-os-db $(DESTDIR)/etc/nova
	install Installer/Read/nmap-mac-prefixes $(DESTDIR)/etc/nova
	mkdir -p $(DESTDIR)/usr/share/applications
	#Copy the hidden directories and files
	cp -frup Installer/Write/nova/nova $(DESTDIR)/etc/nova
	#Copy the scripts and logs
	mkdir -p $(DESTDIR)/usr/share/nova
	mkdir -p $(DESTDIR)/usr/bin
	cp -frup Installer/Write/nova $(DESTDIR)/usr/share/
	cp -frup Installer/Read/icons $(DESTDIR)/usr/share/nova
	cp -fup  Installer/Write/nova_mailer $(DESTDIR)/usr/bin
	install Installer/nova_init $(DESTDIR)/usr/bin
	mkdir -p $(DESTDIR)/var/log/honeyd
	mkdir -p $(DESTDIR)/etc/rsyslog.d/
	mkdir -p $(DESTDIR)/etc/sysctl.d/
	mkdir -p $(DESTDIR)/etc/bash_completion.d/
	#Install permissions
	mkdir -p $(DESTDIR)/etc/sudoers.d/
	install Installer/Read/sudoers_nova $(DESTDIR)/etc/sudoers.d/ --mode=0440
	install Installer/Read/40-nova.conf $(DESTDIR)/etc/rsyslog.d/ --mode=664
	install Installer/Read/30-novactl.conf $(DESTDIR)/etc/sysctl.d/ --mode=0440
	mkdir -p $(DESTDIR)/usr/share/man/man1
	# Copy the bash completion files
	install Installer/Read/novacli $(DESTDIR)/etc/bash_completion.d/ --mode=755

install-pcap-debug:
	#debug sudoers file that allows sudo gdb to pcap without password prompt
	mkdir -p $(DESTDIR)/etc/sudoers.d/
	install Installer/Read/sudoers_nova_debug $(DESTDIR)/etc/sudoers.d/ --mode=0440

install-docs:
	# TODO: Combine man pages
	gzip -c Installer/Read/manpages/novad.1 > Installer/Read/manpages/novad.1.gz
	gzip -c Installer/Read/manpages/novacli.1 > Installer/Read/manpages/novacli.1.gz
	install Installer/Read/manpages/*.1.gz $(DESTDIR)/usr/share/man/man1

install-web:
	cp -frup NovaWeb $(DESTDIR)/usr/share/nova
	mkdir -p $(DESTDIR)/usr/bin
	install NovaWeb/novaweb $(DESTDIR)/usr/bin/novaweb

#Uninstall
uninstall: uninstall-files uninstall-permissions

uninstall-files:
	rm -rf $(DESTDIR)/etc/nova
	rm -rf $(DESTDIR)/usr/share/nova
	rm -f $(DESTDIR)/usr/bin/novacli
	rm -f $(DESTDIR)/usr/bin/novad
	rm -f $(DESTDIR)/usr/bin/nova_mailer
	rm -f $(DESTDIR)/usr/lib/libNova_UI_Core.so
	rm -f $(DESTDIR)/etc/sudoers.d/sudoers_nova
	rm -f $(DESTDIR)/etc/sudoers.d/sudoers_nova_debug
	rm -f $(DESTDIR)/etc/rsyslog.d/40-nova.conf
	rm -f $(DESTDIR)/etc/sysctl.d/30-novactl.conf

uninstall-permissions:
	#sh Installer/postrm

# Reinstall nova without messing up the permissions
reinstall: uninstall-files
	$(MAKE) install

reinstall-debug: uninstall-files
	$(MAKE) install

# Does a fresh uninstall, clean, build, and install
reset: uninstall-files
	$(MAKE) clean
	$(MAKE) release
	$(MAKE) test 
	$(MAKE) install

reset-debug: uninstall-files
	$(MAKE) clean
	$(MAKE) debug
	$(MAKE) test
	$(MAKE) install

