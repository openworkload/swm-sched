#!/usr/bin/env python3
# Usage example:
# benchmark.py -s ../setups/benchmark-simple.json -t job=ID1:2 -t job=ID2:2 -t node=ID1:4

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
    parser.add_argument("-t", "--test", help="Test statements", action="append", required=True)
    parser.add_argument("-d", "--debug", help="Enable debug mode", action="store_true")
    parser.add_argument("-o", "--only", help="Print only this thing")
    args = parser.parse_args()
    opts["SETUP_FILES"] = args.setup
    opts["TEST_STATEMENTS"] = args.test
    opts["DEBUG"] = args.debug
    opts["LOG_FILE"] = "/tmp/swm-sched-benchmark.log"
    opts["ONLY"] = args.only
    _validate_opts(opts)
    return opts


def _validate_opts(opts):
    for file_path in opts.get("SETUP_FILES", []):
        if not os.path.isfile(file_path):
            log.error("Setup file does not exist: %s" % file_path)
            sys.exit(1)


def _print_results(resp_bin, opts):
    log.debug("Parse response: %s" % resp_bin)
    my_dir = os.path.dirname(os.path.realpath(__file__))
    scripts_dir = os.path.join(my_dir, "..", "..", "..", "swm-sched", "scripts")
    convertor = os.path.join(scripts_dir, "result-to-str.escript")
    if opts.get("ONLY", None):
        convertor += " " + opts["ONLY"]
    p = Popen(convertor, cwd=scripts_dir, shell=True, stdout=PIPE, stdin=PIPE)
    out = p.communicate(input=resp_bin)[0]
    #print("%s\n" % str(out).replace("\\n", "\n"))
    sys.stdout.buffer.write(out)


def main():
    opts = _read_opts()
    _set_logger(opts)

    json_manipulator = JsonManipulator()
    json_map = json_manipulator.generate(opts)

    runner = SchedulerRunner()
    bin_out = runner.run(json_map)

    _print_results(bin_out, opts)


if __name__ == "__main__":
    main()
