#!/usr/bin/env python

import sys
import os
import time
import subprocess

if len(sys.argv) == 2:
    pid = sys.argv[1]
else:
    print('No PID specified. Usage: %s <PID>' % os.path.basename(__file__))
    sys.exit(1)


def proct(pid):
    try:
        with open(os.path.join('/proc/', pid, 'stat'), 'r') as pidfile:
            proctimes = pidfile.readline()
            # get utime from /proc/<pid>/stat, 14 item
            utime = proctimes.split(' ')[13]
            # get stime from proc/<pid>/stat, 15 item
            stime = proctimes.split(' ')[14]
            # count total process used time
            proctotal = int(utime) + int(stime)
            return(float(proctotal))
    except IOError as e:
        print('ERROR: %s' % e)
        sys.exit(2)


def cput():
    try:
        with open('/proc/stat', 'r') as procfile:
            cputimes = procfile.readline()
            cputotal = 0
            # count from /proc/stat: user, nice, system, idle, iowait, irc, softirq, steal, guest
            for i in cputimes.split(' ')[2:]:
                i = int(i)
                cputotal = (cputotal + i)
            return(float(cputotal))
    except IOError as e:
        print('ERROR: %s' % e)
        sys.exit(3)

# assign start values before loop them
proctotal = proct(pid)
cputotal = cput()

try:
    while True:

        # for test, to compare results
        proc = subprocess.Popen("top -p %s -b -n 1 | grep -w mysql | awk '{print $9}'" % pid, shell=True, stdout=subprocess.PIPE)
        cpu_percentage = proc.communicate()
        print('With TOP: %s' % (cpu_percentage[0].rstrip('\n')))

        pr_proctotal = proctotal
        pr_cputotal = cputotal

        proctotal = proct(pid)
        cputotal = cput()

        try:
            res = ((proctotal - pr_proctotal) / (cputotal - pr_cputotal) * 100)
            print('With script: %s\n' % round(res, 1))
        except ZeroDivisionError:
            pass

        time.sleep(1)
except KeyboardInterrupt:
    sys.exit(0)