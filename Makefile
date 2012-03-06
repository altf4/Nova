
#The default build should be a release build
all: release
	

#Release Target
release: 
	$(MAKE) -C NovaLibrary/Release
	$(MAKE) -C Nova_UI_Core/Release
	$(MAKE) -C Novad/Release
	cd NovaGUI; qmake -recursive CONFIG+=debug_and_release novagui.pro
	$(MAKE) -C NovaGUI release
	
#Debug target
debug:
	$(MAKE) -C NovaLibrary/Debug
	$(MAKE) -C Nova_UI_Core/Debug
	$(MAKE) -C Novad/Debug
	cd NovaGUI; qmake -recursive CONFIG+=debug_and_release novagui.pro
	$(MAKE) -C NovaGUI debug
	
#Cleans both Release and Debug
clean: clean-debug clean-release
	rm -f Installer/Read/manpages/*.gz

clean-debug:
	$(MAKE) -C NovaLibrary/Debug clean
	$(MAKE) -C Nova_UI_Core/Debug clean
	$(MAKE) -C Novad/Debug clean
	$(MAKE) -C NovaGUI debug-clean

clean-release:
	$(MAKE) -C NovaLibrary/Release clean
	$(MAKE) -C Nova_UI_Core/Release clean
	$(MAKE) -C Novad/Release clean
	$(MAKE) -C NovaGUI debug-clean

install: install-release

#Requires root
install-release:
	#The binaries themselves
	mkdir -p $(DESTDIR)/usr/bin
	mkdir -p $(DESTDIR)/usr/lib
	install NovaGUI/NovaGUI $(DESTDIR)/usr/bin
	install Novad/Release/Novad $(DESTDIR)/usr/bin
	install Nova_UI_Core/Release/libNova_UI_Core.so $(DESTDIR)/usr/lib
	#make folder in etc with path locations to nova files
	mkdir -p $(DESTDIR)/etc/nova
	install Installer/Read/nmap-mac-prefixes $(DESTDIR)/etc/nova
	install Installer/Read/paths $(DESTDIR)/etc/nova
	install Installer/Read/nmap-os-db $(DESTDIR)/etc/nova
	mkdir -p $(DESTDIR)/usr/share/applications
	install Installer/Read/Nova.desktop  $(DESTDIR)/usr/share/applications
	#Copy the hidden directories and files
	cp -rp Installer/.nova $(DESTDIR)/etc/nova
	#Copy the scripts and logs
	mkdir -p $(DESTDIR)/usr/share/nova
	mkdir -p $(DESTDIR)/usr/share/nova/icons
	cp -rp Installer/Write/nova $(DESTDIR)/usr/share/
	cp Installer/Read/icons/* $(DESTDIR)/usr/share/nova/icons
	mkdir -p $(DESTDIR)/var/log/honeyd
	mkdir -p $(DESTDIR)/etc/rsyslog.d/
	install Installer/Read/40-nova.conf $(DESTDIR)/etc/rsyslog.d/ --mode=664
	mkdir -p $(DESTDIR)/etc/sudoers.d/
	install Installer/Read/sudoers_nova $(DESTDIR)/etc/sudoers.d/ --mode=0440
	mkdir -p $(DESTDIR)/usr/share/man/man1
	# TODO: Combine man pages
	gzip -c Installer/Read/manpages/Novad.1 > Installer/Read/manpages/Novad.1.gz
	gzip -c Installer/Read/manpages/NovaGUI.1 > Installer/Read/manpages/NovaGUI.1.gz
	install Installer/Read/manpages/*.1.gz $(DESTDIR)/usr/share/man/man1
	sh Installer/postinst

#requires root
install-debug:
	#The binaries themselves
	mkdir -p $(DESTDIR)/usr/bin
	mkdir -p $(DESTDIR)/usr/lib
	install NovaGUI/NovaGUI $(DESTDIR)/usr/bin
	install Novad/Debug/Novad $(DESTDIR)/usr/bin
	install Nova_UI_Core/Debug/libNova_UI_Core.so $(DESTDIR)/usr/lib
	#make folder in etc with path locations to nova files
	mkdir -p $(DESTDIR)/etc/nova
	install Installer/Read/paths $(DESTDIR)/etc/nova
	install Installer/Read/nmap-os-db $(DESTDIR)/etc/nova
	install Installer/Read/nmap-mac-prefixes $(DESTDIR)/etc/nova
	mkdir -p $(DESTDIR)/usr/share/applications
	install Installer/Read/Nova.desktop  $(DESTDIR)/usr/share/applications
	#Copy the hidden directories and files
	cp -rp Installer/.nova $(DESTDIR)/etc/nova
	#Copy the scripts and logs
	mkdir -p $(DESTDIR)/usr/share/nova
	mkdir -p $(DESTDIR)/usr/share/nova/icons
	cp -rp Installer/Write/nova $(DESTDIR)/usr/share/
	cp Installer/Read/icons/* $(DESTDIR)/usr/share/nova/icons
	mkdir -p $(DESTDIR)/var/log/honeyd
	mkdir -p $(DESTDIR)/etc/rsyslog.d/
	install Installer/Read/40-nova.conf $(DESTDIR)/etc/rsyslog.d/ --mode=664
	mkdir -p $(DESTDIR)/etc/sudoers.d/
	install Installer/Read/sudoers_nova $(DESTDIR)/etc/sudoers.d/ --mode=0440
	mkdir -p $(DESTDIR)/usr/share/man/man1
	# TODO: Combine man pages
	gzip -c Installer/Read/manpages/Novad.1 > Installer/Read/manpages/Novad.1.gz
	gzip -c Installer/Read/manpages/NovaGUI.1 > Installer/Read/manpages/NovaGUI.1.gz
	install Installer/Read/manpages/*.1.gz $(DESTDIR)/usr/share/man/man1
	sh Installer/postinst

#Requires root
uninstall:
	rm -rf $(DESTDIR)/etc/nova
	rm -rf $(DESTDIR)/usr/share/nova
	rm -rf $(DESTDIR)/$(HOME)/.nova
	rm -f $(DESTDIR)/usr/bin/NovaGUI
	rm -f $(DESTDIR)/usr/bin/Novad
	rm -f $(DESTDIR)/libNova_UI_Core.so
	rm -f $(DESTDIR)/etc/sudoers.d/sudoers_nova
	rm -f $(DESTDIR)/usr/share/applications/Nova.desktop
	rm -f $(DESTDIR)/etc/rsyslog.d/40-nova.conf
	#rm -f $(DESTDIR)/usr/share/man/man1/LocalTrafficMonitor.1.gz
	sh Installer/postrm

