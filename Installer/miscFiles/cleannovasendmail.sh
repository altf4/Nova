#!/bin/bash

cd /etc/cron.hourly
sudo rm -f novasendmail

cd /etc/cron.daily
sudo rm -f novasendmail

cd /etc/cron.weekly
sudo rm -f novasendmail

cd /etc/cron.monthly
sudo rm -f novasendmail
