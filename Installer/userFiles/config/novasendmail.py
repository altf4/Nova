#!/usr/bin/python

import os, sys, signal, argparse, time
import pyinotify, smtplib

from email.MIMEMultipart import MIMEMultipart
from email.MIMEBase import MIMEBase
from email.MIMEText import MIMEText
from email.Utils import COMMASPACE, formatdate
from email import Encoders

def getConfigValue(configName):
  for line in open(os.environ['HOME'] + '/.config/nova/config/NOVAConfig.txt').readlines():
    if line.startswith(configName):
      return line[len(configName) + 1:]
  return ''
  
def getSMTPPass():
  if getConfigValue('SMTP_USEAUTH').strip() == '0':
    return ''
  f = open(os.environ['HOME'] + '/.config/nova/config/smtp.txt')
  f.readline()
  return f.readline()

SERVER = getConfigValue('SMTP_DOMAIN').strip()
PORT = getConfigValue('SMTP_PORT').strip()
FROM = getConfigValue('SMTP_ADDR').strip()
TO = getConfigValue('RECIPIENTS').strip()
TO = ', '.join(TO.split(','))
PASS = getSMTPPass().strip()

def genMessageContents():
  DEBUG = 0
  INFO = 0
  NOTICE = 0
  WARNING = 0
  ERROR = 0
  CRITICAL = 0
  ALERT = 0
  EMERGENCY = 0
  for line in open(os.environ['HOME'] + '/.config/nova/config/attachment.txt').readlines():
    if 'DEBUG' in line:
      DEBUG += 1
    elif 'INFO' in line:
      INFO += 1
    elif 'NOTICE' in line:
      NOTICE += 1
    elif 'WARNING' in line:
      WARNING += 1
    elif 'ERROR' in line:
      ERROR += 1
    elif 'CRITICAL' in line:
      CRITICAL += 1
    elif 'ALERT' in line:
      ALERT += 1
    elif 'EMERGENCY' in line:
      EMERGENCY += 1
  ret = 'In this email alert, there are:\n'
  if DEBUG != 0:
    ret += str(DEBUG)  + ' DEBUG log messages\n'
  if INFO != 0:
    ret += str(INFO)  + ' INFO log messages\n'
  if NOTICE != 0:
    ret += str(NOTICE)  + ' NOTICE log messages\n'
  if WARNING != 0:
    ret += str(WARNING)  + ' WARNING log messages\n'
  if ERROR != 0:
    ret += str(ERROR)  + ' ERROR log messages\n'
  if CRITICAL != 0:
    ret += str(CRITICAL)  + ' CRITICAL log messages\n'
  if ALERT != 0:
    ret += str(ALERT)  + ' ALERT log messages\n'
  if EMERGENCY != 0:
    ret += str(EMERGENCY)  + ' EMERGENCY log messages\n'
  return ret

def sendEmailAlert():
  MSG = MIMEMultipart()
  MSG['Subject'] = 'Nova Email Alert'
  MSG['Date'] = formatdate(localtime=True)
  MSG['From'] = FROM
  MSG['To'] = TO
  MSG.preamble = 'Nova Email Alert'
  MSG.attach(MIMEText(genMessageContents()))  

  part = MIMEBase('application', 'octet-stream')
  dontsend = False
  try:
    part.set_payload(open(os.environ['HOME'] + '/.config/nova/config/attachment.txt', 'rb').read())
  except Exception as e:
    dontsend = True
  if dontsend:
    sys.exit(0)
  Encoders.encode_base64(part)
  part.add_header('Content-Disposition', 'attachment; filename="%s"' % os.path.basename('attachment.txt'))
  MSG.attach(part)

  server = smtplib.SMTP(SERVER + ':' + PORT)
  server.set_debuglevel(0)
  server.ehlo()

  if len(PASS) != 0:
    server.starttls()
    server.login(FROM, PASS);
  try:
    server.sendmail(FROM, TO.split(', '), MSG.as_string());
    server.quit();
  except Exception as e:
    print e

if SERVER == '' or PORT == '' or FROM == '' or TO == '':
  sys.exit(2)

sendEmailAlert()

