#!/bin/bash

QUERY1="
PRAGMA journal_mode = WAL;
PRAGMA foreign_keys = ON;

CREATE TABLE packet_count_types(
	type TEXT PRIMARY KEY NOT NULL
);
INSERT INTO packet_count_types VALUES('tcp');
INSERT INTO packet_count_types VALUES('udp');
INSERT INTO packet_count_types VALUES('icmp');
INSERT INTO packet_count_types VALUES('other');
INSERT INTO packet_count_types VALUES('total');
INSERT INTO packet_count_types VALUES('tcpRst');
INSERT INTO packet_count_types VALUES('tcpAck');
INSERT INTO packet_count_types VALUES('tcpSyn');
INSERT INTO packet_count_types VALUES('tcpFin');
INSERT INTO packet_count_types VALUES('tcpSynAck');
INSERT INTO packet_count_types VALUES('bytes');
	
CREATE TABLE packet_counts(
	ip TEXT REFERENCES suspect(ip),
	interface TEXT REFERENCES suspect(interface),

	count INTEGER,
	type TEXT NOT NULL REFERENCES packet_count_types(type)
);


CREATE TABLE features(
	id INTEGER PRIMARY KEY,
	name TEXT NOT NULL
);
INSERT INTO features VALUES(0, 'ip_traffic_distribution');
INSERT INTO features VALUES(1, 'port_traffic_distribution');
INSERT INTO features VALUES(2, 'packet_size_mean');
INSERT INTO features VALUES(3, 'packet_size_deviation');
INSERT INTO features VALUES(4, 'distinct_ips');
INSERT INTO features VALUES(5, 'distinct_tcp_ports');
INSERT INTO features VALUES(6, 'distinct_udp_ports');
INSERT INTO features VALUES(7, 'avg_tcp_ports_per_host');
INSERT INTO features VALUES(8, 'avg_udp_ports_per_host');
INSERT INTO features VALUES(9, 'tcp_percent_syn');
INSERT INTO features VALUES(10, 'tcp_percent_fin');
INSERT INTO features VALUES(11, 'tcp_percent_rst');
INSERT INTO features VALUES(12, 'tcp_percent_synack');
INSERT INTO features VALUES(13, 'haystack_percent_contacted');

CREATE TABLE suspect_features(
	ip TEXT REFERENCES suspect(ip),
	interface TEXT REFERENCES suspect(interface),

	value DOUBLE,
	name TEXT NOT NULL REFERENCES features(name)
);

CREATE TABLE suspects (
	ip TEXT,
	interface TEXT,

	startTime INTEGER,
	endTime INTEGER,
	lastTime INTEGER,

	classification DOUBLE,
	hostileNeighbors INTEGER,
	isHostile INTEGER,

	classificationNotes TEXT,
	
	PRIMARY KEY(ip, interface)
);

CREATE TABLE packetSizeTable (
	ip TEXT REFERENCES suspect(ip),
	interface TEXT REFERENCES suspect(interface),

	packetSize INTEGER,
	count INTEGER
);

CREATE TABLE ipPortTable (
	ip TEXT REFERENCES suspect(ip),
	interface TEXT REFERENCES suspect(interface),

	dstip TEXT,
	port INTEGER,
	count INTEGER
);

"

novadDbFilePath="$DESTDIR/usr/share/nova/userFiles/data/novadDatabase.db"
echo "Writing database $novadDbFilePath"
rm -fr "$novadDbFilePath"
sqlite3 "$novadDbFilePath" <<< $QUERY1
chgrp nova "$novadDbFilePath"
chmod g+rw "$novadDbFilePath"



QUERY2="
PRAGMA journal_mode = WAL;

CREATE TABLE firstrun(
	run TIMESTAMP PRIMARY KEY
);

CREATE TABLE suspectsSeen(
	ip VARCHAR(16),
	interface VARCHAR(16),
	seenSuspect INTEGER,
	seenAllData INTEGER,

	PRIMARY KEY(ip, interface)
);

CREATE TABLE novalogSeen(
	linenum INTEGER PRIMARY KEY,
	line VARCHAR(2048),
	seen INTEGER
);

CREATE TABLE honeydlogSeen(
	linenum INTEGER PRIMARY KEY,
	line VARCHAR(2048),
	seen INTEGER
);

CREATE TABLE credentials(
	user VARCHAR(100) PRIMARY KEY,
	pass VARCHAR(100),
	salt VARCHAR(100)
);

CREATE TABLE lastHoneydNodeIPs(
	mac VARCHAR(100) PRIMARY KEY,
	ip varchar(100)
);

CREATE TABLE lastTrainingDataSelection(
	uid INTEGER PRIMARY KEY,
	included INTEGER
);

"


quasarDbFilePath="$DESTDIR/usr/share/nova/userFiles/data/quasarDatabase.db"
rm -fr "$quasarDbFilePath"
sqlite3 "$quasarDbFilePath" <<< $QUERY2
chgrp nova "$quasarDbFilePath"
chmod g+rw "$quasarDbFilePath"

pulsarDbFilePath="$DESTDIR/usr/share/nova/userFiles/data/pulsarDatabase.db"
rm -fr "$pulsarDbFilePath"
sqlite3 "$pulsarDbFilePath" <<< $QUERY2
chgrp nova "$pulsarDbFilePath"
chmod g+rw "$pulsarDbFilePath"

echo "SQL schema has been set up for nova."
