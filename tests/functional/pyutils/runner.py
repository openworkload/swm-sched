#!/usr/bin/env python3

import json
import logging
import os

from subprocess import Popen, PIPE, STDOUT
from pyutils.jsoner import JsonManipulator


log = logging.getLogger("scheduler")


class SchedulerRunner:

    def __init__(self):
        self._my_dir = os.path.dirname(os.path.realpath(__file__))

    def run(self, json_map):
        json_manipulator = JsonManipulator()
        bin_in = json_manipulator.convert_to_bin(json_map)
        return self._run_scheduler(bin_in)

    def _run_scheduler(self, bin_in):
        log.debug("Run scheduler")
        bin_dir = os.path.join(self._my_dir, "..", "..", "..", "swm-sched", "bin")
        scheduler = os.path.join(bin_dir, "swm-sched")
        args = "%s -p %s -d" % (scheduler, bin_dir)
        cwd = os.path.join(self._my_dir, "..")
        p = Popen(args, cwd=cwd, shell=True, stdout=PIPE, stdin=PIPE)
        out = p.communicate(input=bin_in)[0]
        log.debug("Scheduler result: %s" % out)
        return out
