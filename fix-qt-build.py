#!/usr/bin/env python3
import os
import shutil
from optparse import OptionParser

def backup (workDir):
    for root, dirs, files in os.walk(workDir):
        for file in files:
            if file.endswith(".gitmodules"):
                shutil.copyfile(os.path.join(root, file), os.path.join(root, "ubuntu_save_gitmodules"))

    if os.path.isdir(os.path.join(workDir,".git")):
        print("Removing "+os.path.join(workDir,".git"))
        shutil.rmtree(os.path.join(workDir,".git"))

    for entry in os.listdir(workDir):
         if os.path.isdir(os.path.join(workDir,entry)):
             gitDir = os.path.join(workDir, entry, ".git")
             if os.path.isdir(gitDir):
                 print("Removing "+gitDir)
                 shutil.rmtree(gitDir)

def fix (workDir):
    for root, dirs, files in os.walk(workDir):
        for file in files:
            if file.endswith("ubuntu_save_gitmodules"):
                shutil.copy2(os.path.join(root, file), os.path.join(root, ".gitmodules"))

    #all subdirectores need a .git dir
    for entry in os.listdir(workDir):
         if os.path.isdir(os.path.join(workDir,entry)):
             gitDir = os.path.join(workDir, entry, ".git")
             if not os.path.isdir(gitDir):
                 print("Creating "+gitDir)
                 os.mkdir(gitDir)
             else:
                print(gitDir+" Already exists")
                
parser = OptionParser(usage="usage: %prog [options] <directory>")
parser.add_option(
    "-b", "--backup", dest="do_backup", action="store_true")
parser.add_option(
    "-f", "--fix", dest="do_fix", action="store_true")
options, args = parser.parse_args()

if options.do_backup and options.do_fix:
    parser.error("options -a and -f are mutually exclusive")

if len(args) != 1:
    parser.error("No Qt directory given")

if options.do_backup:
    backup(args[0])
else:
    fix(args[0])
