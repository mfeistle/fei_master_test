#!/usr/local/bin/python3
# encoding: utf-8
'''
regbus -- Simulate the regent bus

regbus is a script to simulate and test the regent ModBus

It defines classes_and_methods

@author:     Beat Morf
        
@copyright:  -
        
@license:    -

@contact:    b.morf@regent.ch
@deffield    updated: Updated
'''

import sys
import os
import serial
from rs485 import RS485Layer
from frameHandler import FrameHandler

from argparse import ArgumentParser
from argparse import RawDescriptionHelpFormatter

__all__ = []
__version__ = 1.0
__date__ = '2014-04-09'
__updated__ = '2014-04-09'

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
        
    result = 0

    program_name = os.path.basename(sys.argv[0])
    program_version = "v%s" % __version__
    program_build_date = str(__updated__)
    program_version_message = '%%(prog)s %s (%s)' % (program_version, program_build_date)
    program_shortdesc = __import__('__main__').__doc__.split("\n")[1]
    program_license = '''%s

  Created by Beat Morf on %s.
  Copyright 2014 Regent Lighting. All rights reserved.
  
  Distributed on an "AS IS" basis without warranties
  or conditions of any kind, either express or implied.

USAGE
''' % (program_shortdesc, str(__date__))

    print('%s %s (%s)' % (program_name, program_version, program_build_date))

    try:
        # Setup argument parser
        parser = ArgumentParser(description=program_license, formatter_class=RawDescriptionHelpFormatter)
        
        # Optional parameters
        parser.add_argument("-v", "--verbose", dest="verbose", action="count", help="set verbosity level [default: %(default)s]", default=0)
        parser.add_argument('-V', '--version', action='version', version=program_version_message)
        parser.add_argument('-l', '--logfile', dest="logfile", help="Communication logfile", metavar="FILE")
        parser.add_argument('-t', '--timeout', dest="timeout", help="Read serial timeout in milliseconds", metavar="FILE")
        
        # positional parameters
        parser.add_argument(dest='com', action='store', type=int, help="COM port" )
        parser.add_argument(dest="configuration", help="configuration XML file")


        # Process arguments from command line
        args = parser.parse_args()
        settings = {}
        settings['verbose'] = args.verbose
        settings['com'] = args.com
        settings['config'] = args.configuration
        settings['logfile'] = args.logfile
        settings['timeout'] = args.timeout
        
        if not settings['timeout']:
            settings['timeout'] = 20
        else:
            settings['timeout'] = int(settings['timeout'])

        # Open the serial port
        serialPort = serial.Serial(port=int(settings['com'])-1, baudrate=115200, bytesize=serial.EIGHTBITS, parity=serial.PARITY_NONE, stopbits=serial.STOPBITS_TWO, timeout=(settings['timeout']/1000))
        
        if serialPort:
            # Initialize the RS485 layer
            rs485 = RS485Layer(serialPort, verboseLevel=settings['verbose'])
        else:
            result = 1
            
        if 0 == result:
            frameHandler = FrameHandler(verboseLevel=settings['verbose'], configFile=settings['config'], interface=rs485, logfile=settings['logfile'])
            
            result = frameHandler.start()

        print("\nTerminated")
        return result

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
        sys.argv.append("-vv")
        sys.argv.append("9")
        #sys.argv.append("bad2.xml")
        sys.argv.append("sniffer.xml")
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
