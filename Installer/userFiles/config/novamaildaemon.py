#!/bin/python

import os, sys, signal
import pyinotify

def handler(signum, frame):
  notifier.stop()
  sys.exit(0)

signal.signal(signal.SIGINT, handler)

wm = pyinotify.WatchManager()
mask = pyinotify.IN_MODIFY

class EventHandler(pyinotify.ProcessEvent):
  def process_IN_MODIFY(self, event):
    # Here is where we get the first line of the log and append it to the 
    # file that will be the email attachment
    first_line = ''
    with open('/var/log/nova/Nova.log', 'r') as f:
      first_line = f.readline()
      f.close()
    with open(os.environ['HOME'] + '/.config/nova/config/attachment.txt', 'a+') as f:
      f.write(first_line)
      f.close()
    print event.pathname + " has been modified, first line is " + first_line

notifier = pyinotify.ThreadedNotifier(wm, EventHandler())
notifier.daemon = True
notifier.start()
wdd = wm.add_watch('/var/log/nova/Nova.log', mask)

while True:
  pass
