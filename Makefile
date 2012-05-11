
#The default build should be a release build
all: release
	

#Release Target
novad-release:
	$(MAKE) -C NovaLibrary/Release
	$(MAKE) -C Nova_UI_Core/Release
	$(MAKE) -C Novad/Release
	$(MAKE) -C NovaCLI/Release

release: novad-release
	cd NovaGUI; qmake -recursive CONFIG+=debug_and_release novagui.pro
	$(MAKE) -C NovaGUI release
	
novad-debug:
	$(MAKE) -C NovaLibrary/Debug
	$(MAKE) -C Nova_UI_Core/Debug
	$(MAKE) -C Novad/Debug
	$(MAKE) -C NovaCLI/Debug

#Debug target
debug: novad-debug
	cd NovaGUI; qmake -recursive CONFIG+=debug_and_release novagui.pro
	$(MAKE) -C NovaGUI debug
	
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

test: test-prepare
	$(MAKE) -C NovaTest/Debug

web:
	cd NovaWeb;npm --unsafe-perm install
	cd NovaWeb/NodeNovaConfig;npm --unsafe-perm install

# Make debug + test
all-test: debug test



#Cleans both Release and Debug
clean: clean-debug clean-release
	rm -f Installer/Read/manpages/*.gz

clean-debug:
	$(MAKE) -C NovaLibrary/Debug clean
	$(MAKE) -C Nova_UI_Core/Debug clean
	$(MAKE) -C Novad/Debug clean
	$(MAKE) -C NovaCLI/Debug clean
	cd NovaGUI; qmake -nodepend CONFIG+=debug_and_release novagui.pro
	$(MAKE) -C NovaGUI debug-clean

clean-release:
	$(MAKE) -C NovaLibrary/Release clean
	$(MAKE) -C Nova_UI_Core/Release clean
	$(MAKE) -C Novad/Release clean
	$(MAKE) -C NovaCLI/Release clean
	cd NovaGUI; qmake -nodepend CONFIG+=debug_and_release novagui.pro
	$(MAKE) -C NovaGUI release-clean

clean-test:
	rm -fr NovaTest/NovadSource/*
	$(MAKE) -C NovaTest/Debug clean

clean-web:
	cd NovaWeb/NodeNovaConfig; node-waf clean

install: install-release

#Requires root
install-release: install-data install-docs
	#release sudoers, does not allow pcap permissions in eclipse
	mkdir -p $(DESTDIR)/etc/sudoers.d/
	install Installer/Read/sudoers_nova $(DESTDIR)/etc/sudoers.d/ --mode=0440
	#The binaries themselves
	mkdir -p $(DESTDIR)/usr/bin
	mkdir -p $(DESTDIR)/usr/lib
	install NovaGUI/novagui $(DESTDIR)/usr/bin
	install NovaCLI/Release/novacli $(DESTDIR)/usr/bin
	install Novad/Release/novad $(DESTDIR)/usr/bin
	install Nova_UI_Core/Release/libNova_UI_Core.so $(DESTDIR)/usr/lib
	sh Installer/postinst

#requires root
install-debug: install-data install-docs
	#debug sudoers file that allows sudo gdb to pcap without password prompt
	mkdir -p $(DESTDIR)/etc/sudoers.d/
	install Installer/Read/sudoers_nova_debug $(DESTDIR)/etc/sudoers.d/ --mode=0440
	#The binaries themselves
	mkdir -p $(DESTDIR)/usr/bin
	mkdir -p $(DESTDIR)/usr/lib
	install NovaGUI/novagui $(DESTDIR)/usr/bin
	install NovaCLI/Debug/novacli $(DESTDIR)/usr/bin
	install Novad/Debug/novad $(DESTDIR)/usr/bin
	install Nova_UI_Core/Debug/libNova_UI_Core.so $(DESTDIR)/usr/lib
	sh Installer/postinst
	
install-data:
	#make folder in etc with path locations to nova files
	mkdir -p $(DESTDIR)/etc/nova
	install Installer/Read/paths $(DESTDIR)/etc/nova
	install Installer/Read/nmap-os-db $(DESTDIR)/etc/nova
	install Installer/Read/nmap-mac-prefixes $(DESTDIR)/etc/nova
	mkdir -p $(DESTDIR)/usr/share/applications
	install Installer/Read/Nova.desktop  $(DESTDIR)/usr/share/applications
	#Copy the hidden directories and files
	cp -frup Installer/Write/nova/.nova $(DESTDIR)/etc/nova
	#Copy the scripts and logs
	mkdir -p $(DESTDIR)/usr/share/nova
	cp -frup Installer/Write/nova $(DESTDIR)/usr/share/
	cp -frup Installer/Read/icons $(DESTDIR)/usr/share/nova
	cp -fup  Installer/Write/nova_mailer $(DESTDIR)/usr/bin
	mkdir -p $(DESTDIR)/var/log/honeyd
	mkdir -p $(DESTDIR)/etc/rsyslog.d/
	install Installer/Read/40-nova.conf $(DESTDIR)/etc/rsyslog.d/ --mode=664
	install Installer/Read/30-novactl.conf $(DESTDIR)/etc/sysctl.d/ --mode=0440
	mkdir -p $(DESTDIR)/usr/share/man/man1
	# Copy the bash completion files
	install Installer/Read/novacli $(DESTDIR)/etc/bash_completion.d/ --mode=755

install-docs:
	# TODO: Combine man pages
	gzip -c Installer/Read/manpages/novad.1 > Installer/Read/manpages/novad.1.gz
	gzip -c Installer/Read/manpages/novagui.1 > Installer/Read/manpages/novagui.1.gz
	gzip -c Installer/Read/manpages/novacli.1 > Installer/Read/manpages/novacli.1.gz
	install Installer/Read/manpages/*.1.gz $(DESTDIR)/usr/share/man/man1

install-web:
	cp -frup NovaWeb $(DESTDIR)/usr/share/nova
	install NovaWeb/novaweb $(DESTDIR)/usr/bin/novaweb
	cd NovaWeb; bash install.sh


uninstall-files:
	rm -rf $(DESTDIR)/etc/nova
	rm -rf $(DESTDIR)/usr/share/nova
	rm -f $(DESTDIR)/usr/bin/novagui
	rm -f $(DESTDIR)/usr/bin/novacli
	rm -f $(DESTDIR)/usr/bin/novad
	rm -f $(DESTDIR)/usr/bin/nova_mailer
	rm -f $(DESTDIR)/usr/lib/libNova_UI_Core.so
	rm -f $(DESTDIR)/etc/sudoers.d/sudoers_nov*
	rm -f $(DESTDIR)/usr/share/applications/Nova.desktop
	rm -f $(DESTDIR)/etc/rsyslog.d/40-nova.conf
	rm -f $(DESTDIR)/etc/sysctl.d/30-novactl.conf

uninstall-permissions:
	sh Installer/postrm

uninstall: uninstall-files uninstall-permissions

# Reinstall nova without messing up the permissions
reinstall: uninstall-files install
reinstall-debug: uninstall-files install-debug

# Does a frest uninstall, clean, build, and install
reset-debug: uninstall-files clean clean-test debug test install-debug
reset: uninstall-files clean clean-test release test install-release



