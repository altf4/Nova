#!/bin/bash
# 
# FTP (WU-FTPD) Honeypot-Script intended for use with 
# Honeyd from Niels Provos 
# -> http://www.citi.umich.edu/u/provos/honeyd/
# 
# Author: Maik Ellinger
# Last modified: 13/06/2002
# Version: 0.0.8
# 
# Changelog: 
# 0.0.6; some ftp comamnds implemented (MKD)
# 
# 0.0.4; some ftp comamnds implemented (CWD)
# 
# 0.0.3: some bugfixes/new commands implemented
#
# 0.0.1: initial release
# 

#set -x -v
DATE=`date`
host=`hostname`
domain=`dnsdomainname`
log=/tmp/honeyd/ftp-$1.log
AUTH="no"
PASS="no"
echo "$DATE: FTP started from $1 Port $2" >> $log
echo -e "220 $host.$domain FTP server (Version wu-2.6.0(5) $DATE) ready.\r"
while read incmd parm1 parm2 parm3 parm4 parm5
do
	# remove control-characters
	incmd=`echo $incmd | sed s/[[:cntrl:]]//g`
	parm1=`echo $parm1 | sed s/[[:cntrl:]]//g`
	parm2=`echo $parm2 | sed s/[[:cntrl:]]//g`
	parm3=`echo $parm3 | sed s/[[:cntrl:]]//g`
	parm4=`echo $parm4 | sed s/[[:cntrl:]]//g`
	parm5=`echo $parm5 | sed s/[[:cntrl:]]//g`

	# convert to upper-case
	incmd_nocase=`echo $incmd | gawk '{print toupper($0);}'`
	#echo $incmd_nocase

	if [ "$AUTH" == "no" ]
        then
	    if [ "$incmd_nocase" != "USER" ]
            then 
		if [ "$incmd_nocase" != "QUIT" ]
    		then 
        	    echo -e "530 Please login with USER and PASS.\r"
		    continue
		fi
	    fi
	fi

	case $incmd_nocase in

	    QUIT* )	
		echo -e "221 Goodbye.\r"
                exit 0;;
	    SYST* )	
		echo -e "215 UNIX Type: L8\r"
                ;;
	    HELP* )
		echo -e "214-The following commands are recognized (* =>'s unimplemented).\r"
		echo -e "   USER    PORT    STOR    MSAM*   RNTO    NLST    MKD     CDUP\r"
		echo -e "   PASS    PASV    APPE    MRSQ*   ABOR    SITE    XMKD    XCUP\r"
		echo -e "   ACCT*   TYPE    MLFL*   MRCP*   DELE    SYST    RMD     STOU\r"
		echo -e "   SMNT*   STRU    MAIL*   ALLO    CWD     STAT    XRMD    SIZE\r"
		echo -e "   REIN*   MODE    MSND*   REST    XCWD    HELP    PWD     MDTM\r"
		echo -e "   QUIT    RETR    MSOM*   RNFR    LIST    NOOP    XPWD\r"
		echo -e "214 Direct comments to ftp@$domain.\r"
		;;
	    USER* )
		parm1_nocase=`echo $parm1 | gawk '{print toupper($0);}'`
		if [ "$parm1_nocase" == "ANONYMOUS" ]
		then
		  echo -e "331 Guest login ok, send your complete e-mail address as a password.\r"
                  AUTH="ANONYMOUS"
		else
		  echo -e "331 Password required for $parm1\r"
                  AUTH=$parm1
		fi
		;;
	    PASS* )
                PASS=$parm1
		if [ "$AUTH" == "ANONYMOUS" ]
		then
		    echo -e "230-Hello User at $1,\r"
		    echo -e "230-we have 911 users (max 1800) logged in in your class at the moment.\r"
		    echo -e "230-Local time is: $DATE\r"
		    echo -e "230-All transfers are logged. If you don't like this, disconnect now.\r"
		    echo -e "230-\r"
		    echo -e "230-tar-on-the-fly and gzip-on-the-fly are implemented; to get a whole\r"
		    echo -e "230-directory \"foo\", \"get foo.tar\" or \"get foo.tar.gz\" may be used.\r"
		    echo -e "230-Please use gzip-on-the-fly only if you need it; most files already\r"
		    echo -e "230-are compressed, and I will kill your processes if you waste my\r"
		    echo -e "230-ressources.\r"
		    echo -e "230-\r"
		    echo -e "230-The command \"site exec locate pattern\" will create a list of all\r"
		    echo -e "230-path names containing \"pattern\".\r"
		    echo -e "230-\r"
		    echo -e "230 Guest login ok, access restrictions apply.\r"
		else
		  echo -e "530 Login incorrect.\r"
		fi
		;;
	    MKD* )
		# choose :
		echo -e "257 \"$parm1\" new directory created.\r"
		#echo -e "550 $parm1: Permission denied.\r"
		;;
	    CWD* )
		# choose :
		echo -e "250 CWD command successful.\r"
		# echo -e "550 $parm1: No such file or directory.\r"
		;;
	    NOOP* )
		echo -e "200 NOOP command successful.\r"
		;;
	    PASV* )
		echo -e "227 Entering Passive Mode (134,76,11,100,165,53)\r"
		;;
	    ACCT* )
		echo -e "502 $incmd command not implemented.\r"
		;;
	    SMNT* )
		echo -e "502 $incmd command not implemented.\r"
		;;
	    REIN* )
		echo -e "502 $incmd command not implemented.\r"
		;;
	    MLFL* )
		echo -e "502 $incmd command not implemented.\r"
		;;
	    MAIL* )
		echo -e "502 $incmd command not implemented.\r"
		;;
	    MSND* )
		echo -e "502 $incmd command not implemented.\r"
		;;
	    MSON* )
		echo -e "502 $incmd command not implemented.\r"
		;;
	    MSAM* )
		echo -e "502 $incmd command not implemented.\r"
		;;
	    MRSQ* )
		echo -e "502 $incmd command not implemented.\r"
		;;
	    MRCP* )
		echo -e "502 $incmd command not implemented.\r"
		;;
	    MLFL* )
		echo -e "502 $incmd command not implemented.\r"
		;;
	    * )
		echo -e "500 '$incmd': command not understood.\r"
		;;
	esac
	echo -e "$incmd $parm1 $parm2 $parm3 $parm4 $parm5" >> $log
done

