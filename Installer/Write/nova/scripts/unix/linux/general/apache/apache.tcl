#!/usr/bin/tclsh

# TODO
#

set scriptsFolder "/usr/share/nova/scripts"
set responseFolder [file join $scriptsFolder unix linux general apache responses]

set sourceIp [lindex $argv 0]
set sourcePort [lindex $argv 1]
set destinationIp [lindex $argv 2]
set destinationPort [lindex $argv 3]

# Stuff we might want to change into arguments
set versionString "Apache/2.2.20 (Ubuntu)"

set date [exec date]

set getRequest ""
set method ""
set uri ""

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
	regsub {%REQUEST%} $outputString $::getRequest outputString
	regsub {%METHOD%} $outputString $::method outputString
	regsub {%URI%} $outputString $::uri outputString
	regsub {%SERVER_VERSION%} $outputString $::versionString outputString

	puts -nonewline $outputString
}

gets stdin request

if {![regexp {^([^ ]+) ([^ ]+) ([^ ]+)} $request -> requestMethod requestURI requestVersion]} {
	set outputString [readFile [file join $responseFolder 400]]
	processOutputString $outputString
	exit
}

set line "bogusLine"
while {$line != ""} {
	gets stdin line
}


# GET on the index
if {[regexp {GET (/|/ .*|/index.html(\?.*)?|/index.html .*)$} $request]} {
	set outputString [readFile [file join $responseFolder 200_index.header]]
	append outputString [readFile [file join $responseFolder index.html]]
# GET on a different file
} elseif {[regexp {GET (/.*).*} $request -> getRequest]} {
	set outputString [readFile [file join $responseFolder 404.header]]
	append outputString [readFile [file join $responseFolder 404.html]]
# HEAD on the index
} elseif {[regexp {HEAD (/ .*|/index.html(\?.*)?|/index.html .*)$} $request]} {
	set outputString [readFile [file join $responseFolder 200_index.header]]
# HEAD on a different file
} elseif {[regexp {HEAD (/.*).*} $request]} {
	set outputString [readFile [file join $responseFolder 404.header]]
# POST on index or some version of it
} elseif {[regexp {POST (/|/ .*|/index.html(\?.*)?|/index.html .*)$} $request]} {
	set outputString [readFile [file join $responseFolder 200_index.header]]
# POST on any other file
} elseif {[regexp {POST (/.*).*} $request]} {
	set outputString [readFile [file join $responseFolder 404.header]]
# OPTIONS command
} elseif {[regexp {OPTIONS /[^ ]* [^ ]+} $request]} {
	set outputString [readFile [file join $responseFolder 200_options.header]]
# Undefined method
} elseif {[regexp {([^ ]+) (/[^ ]*).*} $request -> method uri]} {
	set outputString [readFile [file join $responseFolder 501]]
# Who knows. Throw a 400 Invalid Request and hope it's a reasonble response to whatever they sent
} else {
	set outputString [readFile [file join $responseFolder 400]]
}

processOutputString $outputString
exit

