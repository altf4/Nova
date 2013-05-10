#!/bin/python

import os, sys, signal, getopt, time
import pyinotify, smtplib

from email.MIMEMultipart import MIMEMultipart
from email.MIMEBase import MIMEBase
from email.MIMEText import MIMEText
from email.Utils import COMMASPACE, formatdate
from email import Encoders

TIME_INCREMENT = 0
SERVER = ''
FROM = ''
TO = ''
PASS = ''

def usage():
  print 'doop'
  
# check for lock file and exit if found

try:
  opts, args = getopt.getopt(sys.argv[1:], "s:f:t:phHdw", ['server=', 'from=', 'to=','password=', 'help', 'hourly','daily','weekly'])
except:
  sys.exit(2)

for o, a in opts:
  if o in ['-h', '--help']:
    usage()
    sys.exit()
  elif o in ['-H', '--hourly'] and TIME_INCREMENT == 0:
    TIME_INCREMENT=3600
  elif o in ['-d', '--daily'] and TIME_INCREMENT == 0:
    TIME_INCREMENT=3600 * 24
  elif o == ['-w', '--weekly'] and TIME_INCREMENT == 0:
    TIME_INCREMENT=3600 * 24 * 7
  elif o in ['-s', '--server']:
    SERVER = a
  elif o in ['-f', '--from']:
    FROM = a
  elif o in ['-t', '--to']:
    TO = a
  elif o in ['-p', '--password']:
    PASS = a
  else
    assert False, 'invalid option'
    
def genMessageContents():
  # parse through attachment.txt for the number of occurrences 
  # of each LOG level and pretty-print them for the return
  return 'test'

def handler(signum, frame):
  notifier.stop()
  sys.exit(0)

def sendEmailAlert():
  MSG=MIMEMultipart()
  MSG['Subject'] = 'Nova Email Alert'
  MSG['Date'] = formatdate(localtime=True)
  MSG['From'] = 'a.wal.bear@gmail.com'
  # MSG['From'] = FROM
  MSG['To'] = 'a.wal.bear@gmail.com'
  # MSG['To'] = COMMASPACE.join(TO)
  MSG.preamble = 'Nova Email Alert'
  # the MIMEText() needs to be passed the content
  # of the message
  MSG.attach(MIMEText(genMessageContents()))

  part = MIMEBase('application', 'octet-stream')
  part.set_payload(open('attachment.txt', 'rb').read())
  Encoders.encode_base64(part)
  part.add_header('Content-Disposition', 'attachment; filename="%s"' % os.path.basename('attachment.txt'))
  MSG.attach(part)

  server = smtplib.SMTP(SERVER)
  
  server.ehlo()
  if PASS != '':
    server.starttls()
    server.ehlo()
    server.login(FROM, PASS);
  try:
    server.sendmail();
    server.quit();
  except Exception e:
    print e

signal.signal(signal.SIGINT, handler)

wm = pyinotify.WatchManager()
mask = pyinotify.IN_MODIFY

class EventHandler(pyinotify.ProcessEvent):
  def process_IN_MODIFY(self, event):
    # Here is where we get the first line of the log and append it to the 
    # file that will be the email attachment
    print event.pathname + " has been modified"

notifier = pyinotify.ThreadedNotifier(wm, EventHandler())
notifier.daemon = True
notifier.start()
wdd = wm.add_watch('/var/log/nova/Nova.log', mask)

while True:
  time.sleep(TIME_INCREMENT - (time.time() % TIME_INCREMENT))
  sendEmailAlert()

