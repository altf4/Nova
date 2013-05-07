#!/bin/bash

echo | openssl s_client -connect 192.168.11.221:8443 2>&1 | sed -ne '/-BEGIN CERTIFICATE-/,/-END CERTIFICATE-/p' > mycert.pem

export CLASSPATH=libs/bcprov-jdk15on-146.jar
CERTSTORE=res/raw/test.bks
if [ -a $CERTSTORE ]; then
	rm $CERTSTORE || exit 1
fi

keytool -import -v -trustcacerts -alias 0 -file <(openssl x509 -in mycert.pem) -keystore $CERTSTORE -storetype BKS -provider org.bouncycastle.jce.provider.BouncyCastleProvider -providerpath /usr/share/java/bcprov.jar -storepass toortoor

