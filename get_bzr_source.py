#!/usr/bin/env python3

import os
import sys
import shutil
import subprocess
from optparse import OptionParser

parser = OptionParser(usage="usage: %prog [options] lp:branch")
parser.add_option(
    "-d", "--directory", dest="source_dir")
options, args = parser.parse_args()

if len(args) != 1:
    parser.error("No branch URL given")

if options.source_dir is None:
    parser.error("Source directory not specified")

if os.path.exists(options.source_dir):
    print("Source directory exists.... removing it")
    shutil.rmtree(options.source_dir)

print("Pulling branch "+args[0]+" into "+os.path.abspath(options.source_dir)+"\n")
sys.exit(subprocess.call(["bzr", "export", options.source_dir, args[0]]))
