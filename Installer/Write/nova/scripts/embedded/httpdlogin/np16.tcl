#!/usr/bin/tclsh

set scriptsFolder "/usr/share/nova/scripts"
set responseFolder [file join $scriptsFolder embedded httpdlogin responses]

set sourceIp [lindex $argv 0]
set sourcePort [lindex $argv 1]
set destinationIp [lindex $argv 2]
set destinationPort [lindex $argv 3]

set secureFiles [list sysStatus.shtml synOpStatus.shtml sysCfg.shtml synNwCfg.shtml acctSetup.shtml serPortCfg.shtml pwrPortCfg.shtml pwrAutoPing.shtml log.shtml]

# Stuff we might want to change into arguments
set versionString "Synaccess"
set date [exec date]

proc readFile {fileName} {
	set fh [open $fileName "r"]
	set data [read $fh]
	close $fh

	return $data
}

proc processOutputString {outputString} {
	regsub {%DATE%} $outputString $::date outputString
	regsub {%SERVER%} $outputString $::destinationIp outputString
	regsub {%PORT%} $outputString $::destinationPort outputString
	regsub {%METHOD%} $outputString $::requestMethod outputString
	regsub {%URI%} $outputString $::requestURI outputString
	regsub {%SERVER_VERSION%} $outputString $::versionString outputString

	puts -nonewline $outputString
}

gets stdin request

if {![regexp {^([^ ]+) ([^ ]+) ?([^ ]*)} $request -> requestMethod requestURI requestVersion]} {
	set outputString [readFile [file join $responseFolder 400]]
	processOutputString $outputString
	exit
}

if {$requestVersion == ""} {
	if {$requestMethod != "GET" && $requestMethod != "POST"} {
		set outputString [readFile [file join $responseFolder 400]]
		processOutputString $outputString
		exit
	}
} else {
	set line "bogusLine"
	while {$line != ""} {
		gets stdin line
	}
}


switch -- $requestMethod {
	GET {
		if {[regexp {^/$|^/index.html(\?.*)?$} $requestURI]} {
			set outputString [readFile [file join $responseFolder 200_index.header]]
			append outputString [readFile [file join $responseFolder index.html]]
        } elseif {[regexp {^/(.*)} $requestURI]} {
            foreach fileName $secureFiles {
                if {[string match "/$fileName*" $requestURI]} {
                    set outputString [readFile [file join $responseFolder 401.header]]
                } else {
                    set outputString [readFile [file join $responseFolder 404.header]]
                    append outputString [readFile [file join $responseFolder 404.html]]
                }
            }
		} else {
			set outputString [readFile [file join $responseFolder 404.header]]
			append outputString [readFile [file join $responseFolder 404.html]]
		}
	}

	HEAD {
		if {[regexp {^/$|^/index.html(\?.*)?$} $requestURI]} {
			set outputString [readFile [file join $responseFolder 200_index.header]]
		} else {
			set outputString [readFile [file join $responseFolder 404.header]]
		}

	}

	POST {
		if {[regexp {^/$|^/index.html(\?.*)?$} $requestURI]} {
			set outputString [readFile [file join $responseFolder 200_index.header]]
			append outputString [readFile [file join $responseFolder index.html]]
		} else {
			set outputString [readFile [file join $responseFolder 404.header]]
			append outputString [readFile [file join $responseFolder 404.html]]
		}
	}

	OPTIONS {
		if {[regexp {^\*$|^/.*} $requestURI]} {
			set outputString [readFile [file join $responseFolder 200_options.header]]
		} else {
			set outputString [readFile [file join $responseFolder 400]]
		}
	}

	default {
		set outputString [readFile [file join $responseFolder 501]]
	}
}

processOutputString $outputString
exit

