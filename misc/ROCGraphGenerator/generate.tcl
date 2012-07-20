set knownHostilesFile "hostiles.txt"
set novaProcessName "novad"
set classificationThreshold 0.01

# Make a list of hostile suspects
set knownHostilesTemp [split [read [set fh [open $knownHostilesFile]]] "\n"]; close $fh
set knownHostiles [list]
foreach line $knownHostilesTemp {if {$line != ""} {lappend knownHostiles $line}}


set novaProcess [open "|$novaProcessName |& cat" r+]
fconfigure $novaProcess -blocking 0 -buffering line
fileevent $novaProcess readable [list readNovadOutput $novaProcess]

proc readNovadOutput {chan} {
	if [eof $chan] {
		puts "Nova process died"
		exit
	}

	gets $chan line
	if {$line != ""} {

		if {[string match "*Done processing PCAP file*" $line]} {
			set suspectList [exec -ignorestderr novacli get all csv]
			set closeResult [exec -ignorestderr novacli stop nova]


			set falsePositives 0
			set truePositives 0

			set suspects [split $suspectList "\n"]
			foreach suspectRow $suspects {
				set suspectData [split $suspectRow ","]

				set suspect [lindex $suspectData 0]
				set classification [lindex $suspectData end]

				if {$classification > $::classificationThreshold} {
					if {[lsearch $::knownHostiles $suspect] == -1} {
						incr falsePositives
					} else {
						incr truePositives
					}
				}
			}

			puts "True positives: $truePositives"
			puts "False positives: $falsePositives"

			puts "True positive rate: [expr {1.0*$truePositives/[llength $::knownHostiles]}]"
			puts "False positive rate: [expr {1.0*$falsePositives/([llength $suspects] - [llength $::knownHostiles])}]"


			exit
		}
		puts $line
	}
}

vwait forever
