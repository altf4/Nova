#!/bin/bash

QUERY="
CREATE TABLE firstrun(
	run TIMESTAMP PRIMARY KEY
);

CREATE TABLE credentials(
	user VARCHAR(100),
	pass VARCHAR(100)
);

CREATE TABLE suspect_alerts (
	id INTEGER PRIMARY KEY AUTOINCREMENT,
	suspect VARCHAR(40),
	timestamp TIMESTAMP,
	statistics INT,
	classification DOUBLE
);

CREATE TABLE statistics (
		id INTEGER PRIMARY KEY AUTOINCREMENT,

		ip_traffic_distribution DOUBLE,
		port_traffic_distribution DOUBLE,
		packet_size_mean DOUBLE,
		packet_size_deviation DOUBLE,
		distinct_ips DOUBLE,
		distinct_ports DOUBLE,
		packet_interval_mean DOUBLE,
		packet_interval_deviation DOUBLE,
		tcp_percent_syn DOUBLE,
		tcp_percent_fin DOUBLE,
		tcp_percent_rst DOUBLE,
		tcp_percent_synack DOUBLE,
		haystack_percent_contacted DOUBLE
);"

dbFilePath="$DESTDIR/usr/share/nova/database.db"

if [[ $1 == "reset" ]]; then
	echo "You chose the reset option. This will clear all database data!"
	rm -fr $dbFilePath
fi



sqlite3 $dbFilePath <<< $QUERY
#sqlite3 "/usr/share/nova/database.db" <<< "INSERT INTO credentials VALUES('nova', '\$4\$nova\$h36yyW3noGPSWnx5JCalQCPoo74\$');"

chgrp nova $dbFilePath
chmod g+rw $dbFilePath

echo "SQL schema has been set up for nova."
