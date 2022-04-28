#!/usr/bin/env python3

import os
import sys
import shutil
import subprocess
from optparse import OptionParser

parser = OptionParser(usage="usage: %prog [options] <repository>")
parser.add_option(
    "-d", "--directory", dest="source_dir")
parser.add_option(
    "-b", "--branch", dest="source_branch")
parser.add_option(
    "-k", "--keepgit", dest="delete_git_dir", action="store_false")
options, args = parser.parse_args()

if len(args) != 1:
    parser.error("No branch URL given")

if options.source_dir is None:
    parser.error("Source directory not specified")

if os.path.exists(options.source_dir):
    print("Source directory exists.... removing it")
    shutil.rmtree(options.source_dir)

if options.source_branch is None:
    print("Pulling repository "+args[0]+" into "+os.path.abspath(options.source_dir)+"\n")
    ret = subprocess.call(["git", "clone", "--depth", "1", args[0], options.source_dir])
else:
    print("Pulling branch "+options.source_branch+" of repo "+args[0]+" into "+os.path.abspath(options.source_dir)+"\n")
    ret = subprocess.call(["git", "clone", "--depth", "1", "-b", options.source_branch, args[0], options.source_dir])
if ret != 0:
    sys.exit(ret)

#destroy the git history
if options.delete_git_dir:
    shutil.rmtree(options.source_dir+"/.git")

sys.exit(0)
