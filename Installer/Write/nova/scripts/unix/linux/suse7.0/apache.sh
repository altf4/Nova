#!/bin/sh
#
# by Fabian Bieker <fabian.bieker@web.de>
#

. scripts/misc/base.sh

SRCIP=$1
SRCPORT=$2
DSTIP=$3
DSTPORT=$4

SERVICE="apache/HTTP"
HOST="serv"

REQUEST=""
LOG="/var/log/honeyd/web.log"

rand1=`head -c 10 /dev/urandom | hexdump | sed -e 's/[0 ]//g' | head -c 5 | head -n 1`
rand2=`head -c 20 /dev/urandom | hexdump | sed -e 's/[0 ]//g' | head -c 9 | head -n 1`

my_start

read req1

# remove control-characters
name=`echo $req1 | sed s/[[:cntrl:]]//g`

echo "$req1" >> $LOG

NEWREQUEST=`echo "$req1" | grep -E "GET .* HTTP/1.(0|1)"`
if [ -n "$NEWREQUEST" ] ; then
	REQUEST="GET"
fi

NEWREQUEST=`echo "$req1" | grep -E "GET (/|/?index.html?|/?index.php3?|/?index.cgi) HTTP/1.(0|1)"`
if [ -n "$NEWREQUEST" ] ; then
	REQUEST="GET_/"
fi

NEWREQUEST=`echo "$req1" | grep -E "HEAD .* HTTP/1.(0|1)"`
if [ -n "$NEWREQUEST" ] ; then
	REQUEST="HEAD"
fi

while read name; do
	
	# remove control-characters
	name=`echo $name | sed s/[[:cntrl:]]//g`

	LINE=`echo "$name" | egrep -i "[a-z:]"`
	if [ -z "$LINE" ]
	then
		break
	fi
	echo "$name" >> $LOG

done

case $REQUEST in
  HEAD)
	cat << _eof_
HTTP/1.1 200 OK
Date: $DATE 
Server: Apache/1.3.12 (Unix)  (SuSE/Linux) ApacheJServ/1.1.2 mod_fastcgi/2.2.2 mod_perl/1.24 PHP/4.0.0 mod_ssl/2.6.5 OpenSSL/0.9.5a
Last-Modified: $DATE 
ETag: "$rand1-0-$rand2"
Accept-Ranges: bytes
Content-Length: 0
Connection: close
Content-Type: text/html; charset=iso-8859-1

_eof_
  ;;
  GET)
	cat << _eof_
HTTP/1.1 400 Bad Request
Date: $DATE 
Server: Apache/1.3.12 (Unix)  (SuSE/Linux) ApacheJServ/1.1.2 mod_fastcgi/2.2.2 mod_perl/1.24 PHP/4.0.0 mod_ssl/2.6.5 OpenSSL/0.9.5a
Last-Modified: $DATE 
Accept-Ranges: bytes
Connection: close
Content-Type: text/html; charset=iso-8859-1

<!DOCTYPE HTML PUBLIC "-//IETF//DTD HTML 2.0//EN">
<HTML><HEAD>
<TITLE>400 Bad Request</TITLE>
</HEAD><BODY>
<H1>Bad Request</H1>
The requested URL $req1 was not found on this server.<P>
<HR>
<ADDRESS>Apache/1.3.12 (Unix) (SuSE/Linux) ApacheJServ/1.1.2 at $HOST.$DOMAIN Port $2</ADDRESS>
</BODY></HTML>
_eof_
	;;
	GET_/)
cat << _eof_
HTTP/1.1 200 OK
Date: $DATE 
Server: Apache/1.3.12 (Unix)  (SuSE/Linux) ApacheJServ/1.1.2 mod_fastcgi/2.2.2 mod_perl/1.24 PHP/4.0.0 mod_ssl/2.6.5 OpenSSL/0.9.5a
Last-Modified: Fri, 13 Dec 2001 12:23:08 GMT
ETag: "$rand1-0-$rand2"
Accept-Ranges: bytes
Connection: close
Content-Type: text/html; charset=iso-8859-1

<HTML>
<HEAD>
         <TITLE>Apache HTTP Server - start page</TITLE>
</HEAD>
<BODY BGCOLOR="#ffffff" LINK="#008000" ALINK="#008000" VLINK="#fb8000">
<P>&nbsp;</P>
<CENTER>
<TABLE WIDTH=95% BORDER=1 BGCOLOR="#009900" CELLSPACING=0 CELLPADDING=0>
<TR ALIGN=CENTER VALIGN=CENTER>
         <TD BGCOLOR="#ffffff">
                 <A HREF="http://www.suse.com/"><IMG BORDER=0 ALT="SuSE GmbH" SRC="gif/suse_150.gif"></A>
         </TD>
         <TD VALIGN=CENTER>
                 <FONT COLOR="#ffffff"><H1>Willkommen bei SuSE Linux<BR>Welcome to SuSE Linux</H1></FONT>
         </TD>
         <TD BGCOLOR="#ffffff">
                 <A HREF="http://www.linux.org/"><IMG BORDER=0 ALT="Hit ESCAPE to stop the animation" SRC="gif/penguin.gif"></A>
         </TD>
</TR>
</TABLE>
</CENTER>
<P><HR NOSHADE></P>
<CENTER>
<TABLE WIDTH=95% BORDER=1 BGCOLOR="#ffffff" CELLSPACING=0 CELLPADDING=0>
<TR><TD>
<TABLE WIDTH=100% BORDER=0 BGCOLOR="#e9e9e9" BORDERCOLOR="#000000" 
CELLPADDING=10 CELLSPACING=5>
         <!-------------------------------------------------------------- -->
         <TR BGCOLOR="#88dd66" VALIGN=TOP>
                 <TD ALIGN=CENTER WIDTH=50%>
                         <FONT SIZE=+1>
                         SuSE Linux 7.0 (i386) <BR>
                         </FONT>
                         <FONT SIZE=-1>
                         Rechner: susi.rkh-kassel, Kernel: 2.2.16 (i586)

                         </FONT>
                 </TD>
                 <TD ALIGN=CENTER WIDTH=50%>
                         <FONT SIZE=+1>
                         SuSE Linux 7.0 (i386) <BR>
                         </FONT>
                         <FONT SIZE=-1>
                         Host: susi.rkh-kassel, Kernel: 2.2.16 (i586)

                         </FONT>
                 </TD>
         </TR>
         <TR VALIGN=TOP>
                 <TD ALIGN=LEFT WIDTH=50%>
                         <B>Lokal installierte SuSE Dokumentation</B><BR>
                         <UL>
                         <LI><A HREF="/hilfe/">SuSE Hilfe-Seiten</A>
                         <LI><A HREF="/sdb/de/html/index.html">SuSE Support-Datenbank</A>

                                 <LI><A HREF="/doc/">Verzeichnis /usr/share/doc/</A>
                         </UL>
                         <B>Die SuSE Website</B><BR>
                         <UL>
                                 <LI><A HREF="http://www.suse.de/">SuSE WWW-Server (Deutschland)</A>
                                 <LI><A HREF="http://www.suse.com/">SuSE WWW-Server (USA)</A>
                         </UL>
                 </TD>
                 <TD ALIGN=LEFT WIDTH=50%>
                         <B>Locally installed SuSE Documentation</B><BR>
                         <UL>
                         <LI><A HREF="/hilfe/index_e.html">SuSE Help Pages</A>
                         <LI>English SuSE Support Database is not installed

                                 <LI><A HREF="/doc/">Directory /usr/share/doc/</A><BR>
                         </UL>
                         <B>The SuSE Website</B><BR>
                         <UL>
                                 <LI><A HREF="http://www.suse.de/">SuSE Webserver (Germany)</A>
                                 <LI><A HREF="http://www.suse.com/">SuSE Webserver (USA)</A>
                         </UL>
                 </TD>
         </TR>
         <!-- 
------------------------------------------------------------ -->
         <TR BGCOLOR="#88dd66" VALIGN=TOP>
                 <TD ALIGN=CENTER WIDTH=50%>
                         <FONT SIZE=+1>Der Apache WWW Server</FONT><BR>
                         <FONT SIZE=-1>Apache/1.3.12 (Unix) 
(SuSE/Linux)</FONT>
                 </TH>
                 <TD ALIGN=CENTER WIDTH=50%>
                         <FONT SIZE=+1>The Apache WWW Server</FONT><BR>
                         <FONT SIZE=-1>Apache/1.3.12 (Unix) 
(SuSE/Linux)</FONT>
                 </TD>
         </TR>
         <TR VALIGN=TOP>
                 <TD ALIGN=LEFT WIDTH=50%>
                         <B>Dokumentation</B><BR>
                         <UL>
                         <LI><A HREF="./manual/">Dokumentation zum Apache-Server</A> (Englisch)
                         <LI>PHP ist installiert

                         </UL>
                         <B>Test- und Beispiel-Seiten</B><BR>
                         <UL>
                 <LI>Apache perl Module (mod_perl) ist installiert

           <LI>Apache PHP3 Module (mod_php) ist installiert

                         </UL>
                 </TD>
                 <TD ALIGN=LEFT WIDTH=50%>
                         <B>Documentation</B><BR>
                         <UL>
                         <LI><A HREF="./manual/">Apache webserver documentation</A>
                         <LI>PHP is not installed

                         </UL>
                         <B>Test and Example Pages</B><BR>
                         <UL>
           <LI>Apache perl module (mod_perl) is installed

           <LI>Apache PHP3 Module (mod_php) is installed

                         </UL>
                 </TD>
         </TR>
</TABLE>
</TD></TR>
</TABLE>
<!-- /opt/www/htdig/db/db.docdb does not exist, install dochost + htdig and run /usr/sbin/suserundig -->
<br>


</CENTER>
<P>&nbsp;</P>
<CENTER>
<TABLE WIDTH=95% BORDER=0 BGCOLOR="#ffffff" CELLSPACING=0 CELLPADDING=0>
<TR ALIGN=CENTER VALIGN=CENTER>
         <TD>
         <A HREF="http://www.suse.de/"><IMG ALIGN=LEFT BORDER=0 ALT="Powered by SuSE Linux" SRC="/gif/suse_button.gif"></A>
         <A HREF="http://www.apache.org/"><IMG ALIGN=RIGHT BORDER=0 ALT="Powered by Apache" SRC="gif/apache_pb.gif"></A>
         </TD>
</TR>
</TABLE>
</CENTER>
<HR NOSHADE>
<ADDRESS>
SuSE GmbH N&uuml;rnberg, 2000; <A HREF="mailto:feedback@suse.de">&lt;feedback@suse.de&gt;</A>
</ADDRESS>
</BODY>
</HTML>
_eof_
	;;
	*)
cat << _eof_
<!DOCTYPE HTML PUBLIC "-//IETF//DTD HTML 2.0//EN">
<HTML><HEAD>
<TITLE>501 Method Not Implemented</TITLE>
</HEAD><BODY>
<H1>Method Not Implemented</H1>
$req1 to /index.php not supported.<P>
Invalid method in request $req1<P>
<HR>
<ADDRESS>Apache/1.3.12 Server at $HOST.$DOMAIN Port $DSTPORT</ADDRESS>
</BODY></HTML>
_eof_
	;;
esac

my_stop
