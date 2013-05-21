#!/usr/bin/python

import os, sys, signal, argparse, time
import pyinotify, smtplib

from email.MIMEMultipart import MIMEMultipart
from email.MIMEBase import MIMEBase
from email.MIMEText import MIMEText
from email.MIMEImage import MIMEImage
from email.Utils import COMMASPACE, formatdate
from email import Encoders

try:
  with open(os.environ['HOME'] + '/.config/nova/config/maildaemon.lock'): pass
except IOError:
  sys.exit(0)

def getConfigValue(configName):
  for line in open(os.environ['HOME'] + '/.config/nova/config/NOVAConfig.txt').readlines():
    if line.startswith(configName):
      return line[len(configName) + 1:]
  return ''

SERVER = getConfigValue('SMTP_DOMAIN').strip()
PORT = getConfigValue('SMTP_PORT').strip()
FROM = getConfigValue('SMTP_ADDR').strip()
TO = getConfigValue('RECIPIENTS').strip()
TO = ', '.join(TO.split(','))
PASS = getConfigValue('SMTP_PASS').strip()

DEBUG = 0
INFO = 0
NOTICE = 0
WARNING = 0
ERROR = 0
CRITICAL = 0
ALERT = 0
EMERGENCY = 0

def calcLevelInstances():
  global DEBUG
  global INFO
  global NOTICE
  global WARNING
  global ERROR
  global CRITICAL
  global ALERT
  global EMERGENCY
  for line in open(os.environ['HOME'] + '/.config/nova/config/attachment.txt').readlines():
    if 'DEBUG' in line:
      DEBUG += 1
      continue
    elif 'INFO' in line:
      INFO += 1
      continue
    elif 'NOTICE' in line:
      NOTICE += 1
      continue
    elif 'WARNING' in line:
      WARNING += 1
      continue
    elif 'ERROR' in line:
      ERROR += 1
      continue
    elif 'CRITICAL' in line:
      CRITICAL += 1
      continue
    elif 'ALERT' in line:
      ALERT += 1
      continue
    elif 'EMERGENCY' in line:
      EMERGENCY += 1
      continue

def genMessageContentsText():
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

def genMessageContentsHTML():
  ret = ''
  for line in open('/usr/share/nova/sharedFiles/email_template.txt', 'r').readlines():
    ret += line
  replacewith = ''
  if DEBUG != 0:
    replacewith += '<li>' + str(DEBUG)  + ' DEBUG log messages</li>\n'
  if INFO != 0:
    replacewith += '<li>' + str(INFO)  + ' INFO log messages</li>\n'
  if NOTICE != 0:
    replacewith += '<li>' + str(NOTICE)  + ' NOTICE log messages</li>\n'
  if WARNING != 0:
    replacewith += '<li>' + str(WARNING)  + ' WARNING log messages</li>\n'
  if ERROR != 0:
    replacewith += '<li>' + str(ERROR)  + ' ERROR log messages</li>\n'
  if CRITICAL != 0:
    replacewith += '<li>' + str(CRITICAL)  + ' CRITICAL log messages</li>\n'
  if ALERT != 0:
    replacewith += '<li>' + str(ALERT)  + ' ALERT log messages</li>\n'
  if EMERGENCY != 0:
    replacewith += '<li>' + str(EMERGENCY)  + ' EMERGENCY log messages</li>\n'
  ret = ret.replace('LISTHERE', replacewith)
  return ret

def sendEmailAlert():
  MSG = MIMEMultipart('mixed')
  MSG['Subject'] = 'Nova Email Alert'
  MSG['Date'] = formatdate(localtime=True)
  MSG['From'] = FROM
  MSG['To'] = TO
  MSG.preamble = 'Nova Email Alert'
  calcLevelInstances()
  sumcheck = DEBUG + INFO + NOTICE + WARNING + ERROR + CRITICAL + ALERT + EMERGENCY
  if sumcheck == 0:
    sys.exit(0)
  
  sub = MIMEMultipart('alternative')
  sub.attach(MIMEText(genMessageContentsText(), 'plain'))
  sub.attach(MIMEText(genMessageContentsHTML(), 'html'))
  MSG.attach(sub)

  part = MIMEText(open(os.environ['HOME'] + '/.config/nova/config/attachment.txt', 'r').read(), 'plain')
  part.add_header('Content-Disposition', 'attachment', filename='attachment.txt')
  MSG.attach(part)

  msgImage = MIMEImage(open('/usr/share/nova/sharedFiles/icons/nova-icon.png', 'rb').read())
  msgImage.add_header('Content-ID', '<novaicon>')
  MSG.attach(msgImage)

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

