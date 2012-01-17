
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
	cp NovaLibrary/Config/Log4cxxConfig.xml Installer/.nova/Config/Log4cxxConfig.xml
	cp NovaLibrary/Config/Log4cxxConfig_Console.xml Installer/.nova/Config/Log4cxxConfig_Console.xml
	cp NovaLibrary/Config/NOVAConfig.txt Installer/.nova/Config/NOVAConfig.txt
	cp Haystack/Config/haystack.config Installer/.nova/Config/haystack.config
	cp DoppelgangerModule/Config/doppelganger.config Installer/.nova/Config/doppelganger.config
	mkdir -p Installer/.nova/Data
	cp ClassificationEngine/Config/data.txt Installer/.nova/Data/data.txt

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
	cp NovaLibrary/Config/Log4cxxConfig.xml Installer/.nova/Config/Log4cxxConfig.xml
	cp NovaLibrary/Config/Log4cxxConfig_Console.xml Installer/.nova/Config/Log4cxxConfig_Console.xml
	cp NovaLibrary/Config/NOVAConfig.txt Installer/.nova/Config/NOVAConfig.txt
	cp Haystack/Config/haystack.config Installer/.nova/Config/haystack.config
	cp DoppelgangerModule/Config/doppelganger.config Installer/.nova/Config/doppelganger.config
	mkdir -p Installer/.nova/Data
	cp ClassificationEngine/Config/data.txt Installer/.nova/Data/data.txt

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
	#make nova group
	groupadd -f nova
	usermod -a -G nova $(SUDO_USER)
	#make folder in etc with path locations to nova files
	mkdir -p /etc/nova
	cp Installer/Read/paths /etc/nova
	cp Installer/Read/nmap-os-db /etc/nova
	cp Installer/Read/sudoers_nova	/etc/sudoers.d
	chmod 0440 /etc/sudoers.d/sudoers_nova 
	#Copy the hidden directories and files
	mkdir -p $(HOME)/.nova
	cp -R -f Installer/.nova $(HOME)
	#Copy the scripts and logs
	mkdir -p /usr/share/nova
	cp -R Installer/Write/nova /usr/share/
	#Give permissions
	chmod -R a+rw $(HOME)/.nova
	chmod -R a+rw /usr/share/nova
	chmod a-w /usr/share/nova
	chmod u+w /usr/share/nova
	#The binaries themselves
	cp NovaGUI/NovaGUI /usr/local/bin
	cp ClassificationEngine/Release/ClassificationEngine /usr/local/bin
	cp DoppelgangerModule/Release/DoppelgangerModule /usr/local/bin
	cp Haystack/Release/Haystack /usr/local/bin
	cp LocalTrafficMonitor/Release/LocalTrafficMonitor /usr/local/bin
	#Set pcap permissions
	setcap 'CAP_NET_RAW+eip CAP_NET_ADMIN+eip' /usr/local/bin/LocalTrafficMonitor
	setcap 'CAP_NET_RAW+eip CAP_NET_ADMIN+eip' /usr/local/bin/Haystack
	setcap 'CAP_NET_RAW+eip CAP_NET_ADMIN+eip' /usr/bin/honeyd

#requires root
install-debug:
	#make nova group
	groupadd -f nova
	usermod -a -G nova $(SUDO_USER)
	#make folder in etc with path locations to nova files
	mkdir -p /etc/nova
	cp Installer/Read/paths /etc/nova
	cp Installer/Read/nmap-os-db /etc/nova
	cp Installer/Read/sudoers_nova	/etc/sudoers.d
	chmod 0440 /etc/sudoers.d/sudoers_nova 
	#Copy the hidden directories and files
	mkdir -p $(HOME)/.nova
	cp -R -f Installer/.nova $(HOME)
	#Copy the scripts and logs
	mkdir -p /usr/share/nova
	cp -R Installer/Write/nova /usr/share/
	#Give permissions
	chmod -R a+rw $(HOME)/.nova
	chmod -R a+rw /usr/share/nova
	chmod a-w /usr/share/nova
	chmod u+w /usr/share/nova
	#The binaries themselves
	cp NovaGUI/NovaGUI /usr/local/bin
	cp ClassificationEngine/Debug/ClassificationEngine /usr/local/bin
	cp DoppelgangerModule/Debug/DoppelgangerModule /usr/local/bin
	cp Haystack/Debug/Haystack /usr/local/bin
	cp LocalTrafficMonitor/Debug/LocalTrafficMonitor /usr/local/bin
	#Set pcap permissions
	setcap 'CAP_NET_RAW+eip CAP_NET_ADMIN+eip' /usr/local/bin/LocalTrafficMonitor
	setcap 'CAP_NET_RAW+eip CAP_NET_ADMIN+eip' /usr/local/bin/Haystack
	setcap 'CAP_NET_RAW+eip CAP_NET_ADMIN+eip' /usr/bin/honeyd

#Requires root
uninstall:
	rm -rf /etc/nova
	rm -rf /usr/share/nova
	rm -rf $(HOME)/.nova
	rm -f /usr/local/bin/NovaGUI
	rm -f /usr/local/bin/ClassificationEngine
	rm -f /usr/local/bin/DoppelgangerModule
	rm -f /usr/local/bin/Haystack
	rm -f /usr/local/bin/LocalTrafficMonitor
	rm -f /etc/sudoers.d/sudoers_nova
	rm -rf .nova/Config
	rm -rf .nova/Data
	rm -rf bin
	-groupdel nova
