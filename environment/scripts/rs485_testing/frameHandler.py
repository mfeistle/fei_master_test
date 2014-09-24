'''
Created on 09.04.2014

@author: Beat Morf
'''
import time
import struct
from datetime import datetime
import xml.etree.ElementTree as _ElementTree
from ctypes import *

class FrameHandler(object):
    '''
    classdocs
    '''
    VERBOSE_LEVEL_SETTINGS = 1
    VERBOSE_LEVEL_VARIABLES = 1
    VERBOSE_LEVEL_HANDLING = 1
    VERBOSE_LEVEL_DATA = 3
    VERBOSE_LEVEL_VERIFY = 2


    def __init__(self, verboseLevel=0, configFile=None, interface=None, logfile=None):
        '''
        Constructor
        '''
        self.verboseLevel = verboseLevel
        self.configFile = configFile
        self.interface = interface
        self.logfile = logfile
        self.variables = {}
        self.settings = {}
        self.tree = None
        self.root = None
        self.errorCounter = 0
        self.cyclic = 0


    def getTimestamp(self):
        '''
        Print out the timestamp including ms
        '''
        return str(datetime.now().strftime('%Y-%m-%d %H:%M:%S.')) + str(datetime.utcnow().strftime('%f')[:3])


    def printMsg(self, msg):
        '''
        Print out a message with its timestamp
        '''
        print('%s %s' %(self.getTimestamp(), msg))


    def printError(self, request, setting, msg):
        '''
        Print an error with its timestamp
        '''
        self.errorCounter += 1
        print("%s ERROR(%d, %s) - (Request=%s, Setting=%s) %s" %(self.getTimestamp(), self.errorCounter, str(datetime.now()), request, setting, msg))


    def printData(self, data, direction='', printAlways=0):
        '''
        Print received data
        '''
        line = self.getTimestamp() + ' ' + direction + ' '
        for c in data:
            line += str('%02x ' %c)
        if self.VERBOSE_LEVEL_DATA <= self.verboseLevel or 1==printAlways:
            print(line)
        if self.logfile:
            f = open(self.logfile, 'a')
            f.write(line + '\n')
            f.close()


    def printSendingData(self, data, direction, printAlways=0):
        '''
        Print sending data
        '''
        line = self.getTimestamp() + ' TX '
        for c in data:
            line += str('%02x ' %c)
        if self.VERBOSE_LEVEL_DATA <= self.verboseLevel or 1 == printAlways:
            print(line)
        if self.logfile:
            f = open(self.logfile, 'a')
            f.write(line + '\n')
            f.close()

        
    def start(self):
        '''
        Start the communication procedure
        ''' 
        result = -1

        if self.configFile:
            # Parse the XML
            self.tree = _ElementTree.parse(self.configFile)
            self.root = self.tree.getroot()
    
            # Read the settings from the XML
            self.readSettings()
    
            # Read the variables form the XML
            self.readVariables()
    
            # Parse the XML
            result = self.parseRequests()
        
        return result
        

    def parseRequests(self):
        '''
        Parse the requests
        '''
        result, endless = self.execRequests(firstRun=1)
        while 0 == result and 1 == endless:
            result, endless = self.execRequests()
        return result


    def execRequests(self, firstRun=0):
        '''
        Parse and execute the requests
        '''
        result = 0
        isEndless = 0
        for aRequest in self.root.iter('request'):

            if 0 == result:
                # Verify if this is an endless request
                endlessMode = 0
                if 'endless' in aRequest.attrib:
                    endlessMode = int(aRequest.attrib['endless'])
                    if 1==endlessMode:
                      isEndless = 1

            if 0 == result and (1==firstRun or 1==endlessMode):
                # Read the name of the request
                requestName = ""
                if 'name' in aRequest.attrib:
                    requestName = aRequest.attrib['name']
                
                # Read the delay time
                delay = 0
                if 'delay' in aRequest.attrib:
                    delay = int(aRequest.attrib['delay'])
                    
                # Read if the program must stop on error
                stopOnError = 1
                if 'stoponerror' in aRequest.attrib:
                    stopOnError = int(aRequest.attrib['stoponerror'])
                
                # Read the cyclic delay
                cyclic = 0
                if 'cyclic' in aRequest.attrib:
                    cyclic = int(aRequest.attrib['cyclic'])

                # Calculate next request execution
                nextExecTime = 0
                if 'nextExecTime' in aRequest.attrib:
                    nextExecTime = int(aRequest.attrib['nextExecTime'])

            if 0 == result and (1 == firstRun or 1 == endlessMode) and (nextExecTime <= (time.time()*1000)):

                if 1 == firstRun:
                    # Delay
                    time.sleep(delay/1000)

                if (0 == result) and (self.VERBOSE_LEVEL_HANDLING <= self.verboseLevel):
                    self.printMsg("HANDLE REQUEST '%s'" %(requestName))

                # Read the data to send
                result, requestData = self.readRequestData(requestName, aRequest)
    
                if 0 == result:
                    aRequest.attrib['nextExecTime'] = time.time()*1000 + cyclic;
                    if 0 < len(requestData):

                        # Start transferring
                        result, receivedData = self.interface.sendData(self, requestData)
                        if 0 != result:
                            self.printError(requestName, "", "RX/TX failed (%d)" %(result))

                        if 0 == result:
                            # Verify the received response data
                            result = self.verifyData(requestName, aRequest, receivedData)
                
                        if 0 == result:
                            # Save the data
                            result = self.saveData(requestName, aRequest, receivedData)
                        
                if 0 == stopOnError:
                    # Clear the error if we do not stop
                    result = 0

        return result, isEndless

                        
    def readSettings(self):
        '''
        Read in the settings from the XML
        '''
        if self.VERBOSE_LEVEL_SETTINGS <= self.verboseLevel:
            print("SETTINGS:")
        for aSetting in self.root.iter('setting'):
            if self.VERBOSE_LEVEL_SETTINGS <= self.verboseLevel:
                print("  %s = %s" %(aSetting.get('name'), aSetting.get('value')))
            self.settings[aSetting.get('name')] = aSetting.get('value')
            

    def readVariables(self):
        '''
        Read in the variables from the XML
        '''
        if self.VERBOSE_LEVEL_VARIABLES <= self.verboseLevel:
            print("VARIABLES:")
        for aVariable in self.root.iter('variable'):
            if self.VERBOSE_LEVEL_VARIABLES <= self.verboseLevel:
                print("  %s = %s" %(aVariable.get('name'), aVariable.get('value')))
            if 'format' in aVariable.attrib and 'hex' == aVariable.get('format'):
                self.variables[aVariable.get('name')] = bytearray.fromhex(aVariable.get('value'))
            else:
                self.variables[aVariable.get('name')] = int(aVariable.get('value'))

    def readRequestData(self, requestName, aRequest):
        '''
        Modify the request data according to the XML setting
        '''
        result = 0
        
        # Read in the bytearray
        requestData = bytearray.fromhex(aRequest.attrib['value'])

        # Change the request data bytes in the array
        for aChange in aRequest.iter('change'):
            
            if 0 == result:
                # Read out the name of the change
                changeName = ""
                if 'name' in aChange.attrib:
                    changeName = aChange.attrib['name']
                    
                changeOffset = int(aChange.attrib['offset'])
                changeSize = int(aChange.attrib['size'])
                
                if 'variable' in aChange.attrib:
                    if self.variables[aChange.attrib['variable']]:
                        variableData = self.variables[aChange.attrib['variable']]
                        if 2 == changeSize:
                          changeData = [(variableData>>(8*i))&0xff for i in range(1,-1,-1)]
                        elif 4 == changeSize:
                          changeData = [(variableData>>(8*i))&0xff for i in range(3,-1,-1)]
                    else:
                        self.printError(requestName, changeName, "Invalid variable name '%s'" %(aChange.attrib['variable']))
                        result = 1
                elif 'value' in aChange.attrib:
                    changeData = bytearray.fromhex(aChange.attrib['value'])
                else:
                    self.printError(requestName, changeName, "Invalid change settings '%s'" %(aChange.attrib['variable']))
                    result = 1
            
                if 0 == result:
                    # Modify the request data
                    i = 0
                    while (i < changeSize):
                        requestData[changeOffset + i] = changeData[i]
                        
                        i += 1
        
        return result, requestData
        

    def verifyData(self, requestName, aRequest, responseData):
        result = 0
        #<verify name="Verify Distance" variable="distance" value="600" tolerance="1"/>
        # Verify the response data according to the XML
        for aVerify in aRequest.iter('verify'):

            if 0 == result:
                # Read the name of the verify check
                verifyName = ""
                if 'name' in aVerify.attrib:
                    verifyName = aVerify.attrib['name']

                # Start the verification    
                if 'length' in aVerify.attrib:
                    if len(responseData) != int(aVerify.attrib['length']):
                        self.printError(requestName, verifyName, "Length verification failed (%d instead of %d)!" %(len(responseData), int(aVerify.attrib['length'])))
                        result = 1
                elif 'offset' in aVerify.attrib and 'size' in aVerify.attrib:
                    verifyOffset = int(aVerify.attrib['offset'])
                    verifySize = int(aVerify.attrib['size'])
      
                    if 'variable' in aVerify.attrib:
                      if 'tolerance' in aVerify.attrib:
                          verifyTolerance = int(aVerify.attrib['tolerance'])
                      else:
                          verifyTolerance = 0
                      
                      if 1 == verifySize:
                        verifyVariable = int(struct.unpack_from('>B', responseData, verifyOffset)[0])
                      elif 2 == verifySize:
                        verifyVariable = int(struct.unpack_from('>H', responseData, verifyOffset)[0])
                      else:
                        verifyVariable = int(struct.unpack('>I', responseData[verifyOffset])[0])
                      
                      reference=self.variables[aVerify.attrib['variable']]
                      
                      if (verifyVariable < (reference - (reference*verifyTolerance/100))) or (verifyVariable > (reference + (reference*verifyTolerance/100))):
                          self.printError(requestName, verifyName, "Verification failed: reference=%d tolerance=%d value=%d" %(reference, verifyTolerance, verifyVariable))
                          result = 1
                      elif self.VERBOSE_LEVEL_VERIFY <= self.verboseLevel:
                          self.printMsg("COMPARE: reference=%d tolerance=%d value=%d" %(reference, verifyTolerance, verifyVariable))

                    else:
                      # Read the compare value
                      if 'variable' in aVerify.attrib:
                          verifyData = self.variables[aVerify.attrib['variable']]
                      elif 'value' in aVerify.attrib:
                          verifyData = bytearray.fromhex(aVerify.attrib['value'])
      
                      i = 0
                      while (0 == result) and (i < verifySize):
                          if ((verifyOffset + i) >= len(responseData)):
                              self.printError(requestName, verifyName, "Received data too small for verification ")
                              result = 1
                          elif responseData[verifyOffset + i] != verifyData[i]:
                              self.printError(requestName, verifyName, "Verification failed")
                              result = 1
                          i += 1
                else:
                    self.printError(requestName, verifyName, "Invalid verify settings in '%s'!" %(verifyName))

        return result


    def saveData(self, requestName, aRequest, responseData):
        '''
        Save the data received by the controller
        '''
        result = 0
        for aSave in aRequest.iter('save'):
            if 0 == result:

                # Read the name of the save procedure
                saveName = ""
                if 'name' in aSave.attrib:
                    saveName = aSave.attrib['name']
                
                # Start the save procedure    
                if 'offset' in aSave.attrib and 'length' in aSave.attrib and 'variable' in aSave.attrib:
                    saveOffset = int(aSave.attrib['offset'])
                    saveLength = int(aSave.attrib['length'])
                    saveVariable = aSave.attrib['variable']
    
                    # Read the compare value
                    savedVariable = bytearray()
                    asciiVariable = bytearray()
                    
                    i=0
                    if len(responseData) >= (saveOffset + saveLength):
                        offset = 0
                        value = 0
                        increment = 0
                        decrement = 0
                        if 'increment' in aSave.attrib:
                            increment = int(aSave.attrib['increment'])
                        if 'decrement' in aSave.attrib:
                            decrement = int(aSave.attrib['decrement'])
                        while (0 == result) and (i < saveLength):
                            if 0 < (i % 2):
                              offset = i - 1
                            else:
                              offset = i + 1
                            savedVariable.append(responseData[saveOffset + i]) 
                            asciiVariable.append(responseData[saveOffset + offset]) 
                            value = value << 8
                            value += responseData[saveOffset + i]
                            i += 1
                        if 'print' in aSave.attrib:
                            if "sint8_t" == aSave.attrib['print']:
                                self.variables[saveVariable] = c_byte(value).value
                                if 'hex' in aSave.attrib:
                                  self.printMsg("%s: %d" %(saveName, bin(self.variables[saveVariable])))
                                else:
                                  self.printMsg("%s: %d" %(saveName, self.variables[saveVariable]))
                            elif "uint8_t" == aSave.attrib['print']:
                                self.variables[saveVariable] = c_ubyte(value).value
                                if 'hex' in aSave.attrib:
                                  self.printMsg("%s: %d" %(saveName, bin(c_byte(value).value)))
                                else:
                                  self.printMsg("%s: %d" %(saveName, self.variables[saveVariable]))
                            elif "sint16_t" == aSave.attrib['print']:
                                self.variables[saveVariable] = c_short(value).value
                                if 'hex' in aSave.attrib:
                                  self.printMsg("%s: %d" %(saveName, bin(self.variables[saveVariable])))
                                else:
                                  self.printMsg("%s: %d" %(saveName, self.variables[saveVariable]))
                            elif "uint16_t" == aSave.attrib['print']:
                                self.variables[saveVariable] = c_ushort(value).value
                                if 'hex' in aSave.attrib:
                                  self.printMsg("%s: %s" %(saveName, bin(c_short(value).value)))
                                else:
                                  self.printMsg("%s: %d" %(saveName, self.variables[saveVariable]))
                            elif "sint32_t" == aSave.attrib['print']:
                                self.variables[saveVariable] = c_int(value).value
                                self.printMsg("%s: %d" %(saveName, value, self.variables[saveVariable]))
                            elif "uint32_t" == aSave.attrib['print']:
                                self.variables[saveVariable] = c_uint(value).value
                                self.printMsg("%s: %d" %(saveName, value, self.variables[saveVariable]))
                            elif "ascii" == aSave.attrib['print']:
                                self.printMsg("%s: %s" %(saveName, asciiVariable.decode("utf-8")))
                        if "ascii" != aSave.attrib['print'] and 0 != increment:
                            self.variables[saveVariable] += increment
                            self.printMsg("%s: +%d=%d" %(saveName, increment, self.variables[saveVariable]))
                        if "ascii" != aSave.attrib['print'] and 0 != decrement:
                            self.variables[saveVariable] -= decrement
                            self.printMsg("%s: -%d=%d" %(saveName, decrement, self.variables[saveVariable]))
                    else:
                        self.printError(requestName, saveName, "Received data too small for saving")
                        result = 1
                else:
                    self.printError(requestName, saveName, "Invalid save settings in '%s'!" %(saveName))
                    result = 1
                    
        return result
