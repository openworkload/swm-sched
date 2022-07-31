#!/usr/bin/env python3
# Usage example:
# test.py -s simple.json -t job=ID1:2 -t job=ID2:2 -t node=ID1:4

import argparse
import logging
import os
import sys

from subprocess import Popen, PIPE, STDOUT

sys.path.insert(1, os.path.join(sys.path[0], '..'))
from pyutils.jsoner import JsonManipulator
from pyutils.runner import SchedulerRunner


log = logging.getLogger("scheduler")


def _set_logger(opts):
    global log
    for hdlr in log.handlers[:]:  # remove all old handlers
        log.removeHandler(hdlr)
    fh = logging.FileHandler(opts["LOG_FILE"])
    fmr = logging.Formatter('%(asctime)s [%(levelname)s] %(message)s')
    fh.setFormatter(fmr)
    log.addHandler(fh)
    if opts["DEBUG"]:
        log.setLevel(logging.DEBUG)
    else:
        log.setLevel(logging.INFO)
    ch = logging.StreamHandler(sys.stdout)
    ch.setFormatter(fmr)
    log.addHandler(ch)


def _read_opts():
    opts = {}
    parser = argparse.ArgumentParser(description='Run schedulers benchmark')
    parser.add_argument("-s", "--setup", help="Test setup file", action="append", required=True)
    parser.add_argument("-e", "--expected", help="File with expected result", required=True)
    parser.add_argument("-d", "--debug", help="Enable debug mode", action="store_true")
    args = parser.parse_args()
    opts["EXPECTED"] = args.expected
    opts["SETUP_FILES"] = args.setup
    opts["DEBUG"] = args.debug
    opts["LOG_FILE"] = "/tmp/swm-sched-test.log"
    _validate_opts(opts)
    return opts


def _validate_opts(opts):
    for file_path in opts.get("SETUP_FILES", []):
        if not os.path.isfile(file_path):
            log.error("Setup file does not exist: %s" % file_path)
            sys.exit(1)


def _validate(resp_bin, opts):
    log.debug("Validate scheduler response (size=%d)" % len(resp_bin))
    my_dir = os.path.dirname(os.path.realpath(__file__))
    scripts_dir = os.path.join(my_dir, "..", "..", "..", "swm-sched", "scripts")
    expected_full_path = os.path.abspath(opts.get("EXPECTED", ""))
    command = "validator.escript -e %s" % expected_full_path
    full_command = os.path.join(scripts_dir, command)
    process = Popen(full_command, shell=True, stdout=PIPE, stdin=PIPE)
    out = process.communicate(input=resp_bin)[0]
    sys.stdout.buffer.write(out)
    return process.returncode


def main():
    opts = _read_opts()
    _set_logger(opts)

    json_manipulator = JsonManipulator()
    json_map = json_manipulator.generate(opts)

    runner = SchedulerRunner()
    bin_out = runner.run(json_map)

    status = _validate(bin_out, opts)
    sys.exit(status)


if __name__ == "__main__":
    main()
