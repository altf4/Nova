
#The default build should be a release build
all: release
	

#Release Target
release: 
	cd NovaLibrary/Release; $(MAKE)
	cd ClassificationEngine/Release; $(MAKE)
	cd DoppelgangerModule/Release; $(MAKE)
	cd Haystack/Release; $(MAKE)
	cd LocalTrafficMonitor/Release; $(MAKE)
	cd NovaGUI; qmake -recursive CONFIG+=debug_and_release novagui.pro
	cd NovaGUI; $(MAKE) release
	mkdir -p Installer/.nova/Config
	cp NovaLibrary/Config/NOVAConfig.txt Installer/.nova/Config/NOVAConfig.txt
	cp Haystack/Config/haystack.config Installer/.nova/Config/haystack.config
	cp DoppelgangerModule/Config/doppelganger.config Installer/.nova/Config/doppelganger.config
	mkdir -p Installer/.nova/Data
	cp ClassificationEngine/Config/data.txt Installer/.nova/Data/data.txt
	cp ClassificationEngine/Config/training.db Installer/.nova/Data/training.db

#Debug target
debug:
	cd NovaLibrary/Debug; $(MAKE)
	cd ClassificationEngine/Debug; $(MAKE)
	cd DoppelgangerModule/Debug; $(MAKE)
	cd Haystack/Debug; $(MAKE)
	cd LocalTrafficMonitor/Debug; $(MAKE)
	cd NovaGUI; qmake -recursive CONFIG+=debug_and_release novagui.pro
	cd NovaGUI; $(MAKE) debug
	mkdir -p Installer/.nova/Config
	cp NovaLibrary/Config/NOVAConfig.txt Installer/.nova/Config/NOVAConfig.txt
	cp Haystack/Config/haystack.config Installer/.nova/Config/haystack.config
	cp DoppelgangerModule/Config/doppelganger.config Installer/.nova/Config/doppelganger.config
	mkdir -p Installer/.nova/Data
	cp ClassificationEngine/Config/data.txt Installer/.nova/Data/data.txt
	cp ClassificationEngine/Config/training.db Installer/.nova/Data/training.db

#Cleans both Release and Debug
clean: clean-debug clean-release
	rm -f Installer/Read/manpages/*.gz

clean-debug:
	cd NovaLibrary/Debug; $(MAKE) clean
	cd Novad/Debug; $(MAKE) clean
	cd NovaGUI; $(MAKE) debug-clean

clean-release:
	cd NovaLibrary/Release; $(MAKE) clean
	cd Novad/Release; $(MAKE) clean
	cd NovaGUI; $(MAKE) release-clean

install: install-release

#Requires root
install-release:
	#The binaries themselves
	mkdir -p $(DESTDIR)/usr/bin
	install NovaGUI/NovaGUI $(DESTDIR)/usr/bin
	install Novad/Release/Novad $(DESTDIR)/usr/bin
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
	#gzip -c Installer/Read/manpages/LocalTrafficMonitor.1 > Installer/Read/manpages/LocalTrafficMonitor.1.gz
	gzip -c Installer/Read/manpages/NovaGUI.1 > Installer/Read/manpages/NovaGUI.1.gz
	install Installer/Read/manpages/*.1.gz $(DESTDIR)/usr/share/man/man1
	sh Installer/postinst

#requires root
install-debug:
	#The binaries themselves
	mkdir -p $(DESTDIR)/usr/bin
	install NovaGUI/NovaGUI $(DESTDIR)/usr/bin
	install Novad/Debug/Novad $(DESTDIR)/usr/bin
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
	#gzip -c Installer/Read/manpages/LocalTrafficMonitor.1 > Installer/Read/manpages/LocalTrafficMonitor.1.gz
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
	rm -f $(DESTDIR)/etc/sudoers.d/sudoers_nova
	rm -f $(DESTDIR)/usr/share/applications/Nova.desktop
	rm -f $(DESTDIR)/etc/rsyslog.d/40-nova.conf
	#rm -f $(DESTDIR)/usr/share/man/man1/LocalTrafficMonitor.1.gz
	sh Installer/postrm

