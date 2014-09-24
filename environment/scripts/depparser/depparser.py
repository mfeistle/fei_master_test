#!/usr/local/bin/python3
# encoding: utf-8
'''
depparser -- Dependency Parser for projects dependency files

depparser is a parser to use with dependency files

@author:     Beat Morf
        
@copyright:  2014 Regent Beleuchtungskörper AG
        
@contact:    b.morf@regent.ch
'''

import sys
import os
import re
import subprocess
import string

from argparse import ArgumentParser
from argparse import RawDescriptionHelpFormatter

__all__ = []
__version__ = 1.1
__date__ = '2013-02-19'
__updated__ = '2013-03-21'

DEBUG = 0
TESTRUN = 0
PROFILE = 0

class CLIError(Exception):
    '''Generic exception to raise and log different fatal errors.'''
    def __init__(self, msg):
        super(CLIError).__init__(type(self))
        self.msg = "E: %s" % msg
    def __str__(self):
        return self.msg
    def __unicode__(self):
        return self.msg

def main(argv=None): # IGNORE:C0111
    '''Command line options.'''
    
    if argv is None:
        argv = sys.argv
    else:
        sys.argv.extend(argv)

    program_name = os.path.basename(sys.argv[0])
    program_version = "v%s" % __version__
    program_build_date = str(__updated__)
    program_version_message = '%%(prog)s %s (%s)' % (program_version, program_build_date)
    program_shortdesc = __import__('__main__').__doc__.split("\n")[1]
    program_license = '''%s

  Created by Beat Morf on %s.
  Copyright 2014 Regent Beleuchtungskörper AG. All rights reserved.
  
  Distributed on an "AS IS" basis without warranties
  or conditions of any kind, either express or implied.

USAGE
''' % (program_shortdesc, str(__date__))

    try:
        # Setup argument parser
        parser = ArgumentParser(description=program_license, formatter_class=RawDescriptionHelpFormatter)
        parser.add_argument("-v", "--verbose", dest="verbose", action="count", help="set verbosity level [default: %(default)s]", default=0)
        parser.add_argument('-V', '--version', action='version', version=program_version_message)
        parser.add_argument(dest="dependency_file", help="uVision4 dependency file (absolute path)")
        parser.add_argument(dest="root", help="Root directory of sources (absolute path)")
        
        # Process arguments from command line
        args = parser.parse_args()
        settings = {}
        settings['verbose'] = args.verbose
        settings['root'] = args.root
        settings['dependency-file'] = args.dependency_file

        # Open the dep file
        depFile = open(settings['dependency-file'], 'r')
        
        # Verify type of dep file
        uv4DepFile = 0
        stvdDepFile = 0
        line = depFile.readline()
        if 0 <= line.find("Dependencies for Project '") and 0 <= line.find("(DO NOT MODIFY !)"):
            uv4DepFile = 1
        elif 0 <= line.find("STMicroelectronics dependencies file"):
            stvdDepFile = 1
        else:
            print("Not a valid dependency file")
        
        # Directory manipulations
        workingDirectory = os.path.abspath(os.curdir)
        if not (os.path.isdir('output')):
            os.mkdir('output')
        outputDirectory = "%s\\output" %(workingDirectory);
        print(outputDirectory)
      
        # Parse it
        if 1 == uv4DepFile:
            for aLine in depFile:
                os.chdir(os.path.dirname(settings['dependency-file']))
                res = re.search(r'[FI][ ][(](?P<file>[.][A-Za-z0-9 .\\_-]*)', aLine)
                if (res and res.group('file')):
                    absFile = os.path.abspath(res.group('file'))
                    os.chdir(outputDirectory)
                    newFile = absFile.replace(settings['root'], outputDirectory)
                    newPath = os.path.dirname(newFile)
                    print("Copy %s to %s" %(absFile, newPath))
                    returnCode = subprocess.call(["xcopy.exe", absFile, "%s\\" %(newPath), "/Y"])
                    if (0 != returnCode):
                        exit(returnCode)
        elif 1 == stvdDepFile:
            for aLine in depFile:
                if 0 == aLine.find("ExternDep="):
                    fileList = aLine.split(" ")
                    for aFile in fileList:
                        os.chdir(os.path.dirname(settings['dependency-file']))
                        if 0 == aFile.find("."):
                            absFile = os.path.abspath(aFile)
                            if os.path.isfile(absFile):
                                os.chdir(outputDirectory)
                                newFile = absFile.replace(settings['root'], outputDirectory)
                                newPath = os.path.dirname(newFile)
                                print("Copy %s to %s" %(absFile, newPath))
                                returnCode = subprocess.call(["xcopy.exe", absFile, "%s\\" %(newPath), "/Y"])
                                if (0 != returnCode):
                                    exit(returnCode)
            
        depFile.close()

        return 0

    except KeyboardInterrupt:
        ### handle keyboard interrupt ###
        return 0

    except Exception as e:
        if DEBUG or TESTRUN:
            raise(e)
        indent = len(program_name) * " "
        sys.stderr.write(program_name + ": " + repr(e) + "\n")
        sys.stderr.write(indent + "  for help use --help")
        return 2


if __name__ == "__main__":
    if DEBUG:
        sys.argv.append("-v")
        sys.argv.append("D:\\sources\\EZR_Funk\\application\\rbg\\roth_ir_combo.dep")
        sys.argv.append("D:\\sources\\EZR_Funk")
    if TESTRUN:
        import doctest
        doctest.testmod()
    if PROFILE:
        import cProfile
        import pstats
        profile_filename = 'remotetracker_profile.txt'
        cProfile.run('main()', profile_filename)
        statsfile = open("profile_stats.txt", "wb")
        p = pstats.Stats(profile_filename, stream=statsfile)
        stats = p.strip_dirs().sort_stats('cumulative')
        stats.print_stats()
        statsfile.close()
        sys.exit(0)
    sys.exit(main())