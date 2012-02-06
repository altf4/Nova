
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

clean-debug:
	cd NovaLibrary/Debug; $(MAKE) clean
	cd ClassificationEngine/Debug; $(MAKE) clean
	cd DoppelgangerModule/Debug; $(MAKE) clean
	cd Haystack/Debug; $(MAKE) clean
	cd LocalTrafficMonitor/Debug; $(MAKE) clean
	cd NovaGUI; $(MAKE) debug-clean

clean-release:
	cd NovaLibrary/Release; $(MAKE) clean
	cd ClassificationEngine/Release; $(MAKE) clean
	cd DoppelgangerModule/Release; $(MAKE) clean
	cd Haystack/Release; $(MAKE) clean
	cd LocalTrafficMonitor/Release; $(MAKE) clean
	cd NovaGUI; $(MAKE) release-clean

install: install-release

#Requires root
install-release:
	#make folder in etc with path locations to nova files
	mkdir -p $(DESTDIR)/etc/nova
	install Installer/Read/nmap-mac-prefixes $(DESTDIR)/etc/nova
	install Installer/Read/paths $(DESTDIR)/etc/nova
	install Installer/Read/nmap-os-db $(DESTDIR)/etc/nova
	mkdir -p $(DESTDIR)/etc/sudoers.d/
	install Installer/Read/sudoers_nova $(DESTDIR)/etc/sudoers.d/
	#Copy the hidden directories and files
	cp -rp Installer/.nova $(DESTDIR)/etc/nova
	#Copy the scripts and logs
	mkdir -p $(DESTDIR)/usr/share/nova
	cp -rp Installer/Write/nova $(DESTDIR)/usr/share/
	#The binaries themselves
	mkdir -p $(DESTDIR)/usr/bin
	install NovaGUI/NovaGUI $(DESTDIR)/usr/bin
	install ClassificationEngine/Release/ClassificationEngine $(DESTDIR)/usr/bin
	install DoppelgangerModule/Release/DoppelgangerModule $(DESTDIR)/usr/bin
	install Haystack/Release/Haystack $(DESTDIR)/usr/bin
	install LocalTrafficMonitor/Release/LocalTrafficMonitor $(DESTDIR)/usr/bin
	sh Installer/postinst

#requires root
install-debug:
	#make folder in etc with path locations to nova files
	mkdir -p $(DESTDIR)/etc/nova
	install Installer/Read/paths $(DESTDIR)/etc/nova
	install Installer/Read/nmap-os-db $(DESTDIR)/etc/nova
	install Installer/Read/nmap-mac-prefixes $(DESTDIR)/etc/nova
	mkdir -p $(DESTDIR)/etc/sudoers.d/
	install Installer/Read/sudoers_nova $(DESTDIR)/etc/sudoers.d/
	#Copy the hidden directories and files
	cp -rp Installer/.nova $(DESTDIR)/etc/nova
	#Copy the scripts and logs
	mkdir -p $(DESTDIR)/usr/share/nova
	cp -rp Installer/Write/nova $(DESTDIR)/usr/share/
	#The binaries themselves
	mkdir -p $(DESTDIR)/usr/bin
	install NovaGUI/NovaGUI $(DESTDIR)/usr/bin
	install ClassificationEngine/Debug/ClassificationEngine $(DESTDIR)/usr/bin
	install DoppelgangerModule/Debug/DoppelgangerModule $(DESTDIR)/usr/bin
	install Haystack/Debug/Haystack $(DESTDIR)/usr/bin
	install LocalTrafficMonitor/Debug/LocalTrafficMonitor $(DESTDIR)/usr/bin
	sh Installer/postinst

#Requires root
uninstall:
	rm -rf $(DESTDIR)/etc/nova
	rm -rf $(DESTDIR)/usr/share/nova
	rm -rf $(DESTDIR)/$(HOME)/.nova
	rm -f $(DESTDIR)/usr/bin/NovaGUI
	rm -f $(DESTDIR)/usr/bin/ClassificationEngine
	rm -f $(DESTDIR)/usr/bin/DoppelgangerModule
	rm -f $(DESTDIR)/usr/bin/Haystack
	rm -f $(DESTDIR)/usr/bin/LocalTrafficMonitor
	rm -f $(DESTDIR)/etc/sudoers.d/sudoers_nova
	sh Installer/postrm

