'''
Created on 09.04.2014

@author: Beat Morf
'''
from array import array
from crc16 import CRC16
import time

class RS485Layer(object):
  '''
  classdocs
  '''
  
  crc = CRC16(modbus_flag = True)

  def __init__(self, stream, verboseLevel=0):
      '''
      Constructor
      '''
      self.stream = stream
      self.verboseLevel = verboseLevel


  def clearRXBuffer(self):
      # Read all the data in the receiving buffer on the radio gateway
      data = bytearray.fromhex("00")
      result = 0
      while 0 == result and data:
          result, data = self.sendData()
      return result
    
        
  def sendData(self, _frameHandler, data=[]):
    '''
    Append CRC16 and send it back
    '''
    packet = bytearray()
    receivedData = bytearray()
    result = 0
    
    # Data
    for c in data:
        packet.append(c)
        
    # CRC16
    myCrc = self.crc.calculate(packet)
    packet.append(myCrc & 0xFF)
    packet.append((myCrc >> 8) & 0xFF)

    # dump        
    _frameHandler.printData(packet, direction='TX')

    # Send packet
    self.stream.write(packet)
    
    # Read back packet
    response = self.stream.read(3)
    if 3 != len(response):
      result = 1
    else:
      for c in response:
        receivedData.append(c)

    if 0 == result:
      # Read type of ModBus Response
      length = 3
      if 0x03 == receivedData[1]:
        length = receivedData[2]

      response = self.stream.read(length)

      if length != len(response):
        result = 2
      else:
        for c in response:
          receivedData.append(c)
          
    if 0 == result:
      response = self.stream.read(2)

      if (2 == len(response)):
        #Verify CRC
        myCrc = self.crc.calculate(receivedData)

        for c in response:
            receivedData.append(c)

        if (((myCrc & 0xFF) != response[0]) or ((myCrc >> 8) & 0xFF != response[1])):
          result = 3
            
      else:
        result = 4
    
    # dump        
    _frameHandler.printData(receivedData, direction='RX')
    
    if 0 == result:
        receivedData.pop(len(receivedData)-1)
        receivedData.pop(len(receivedData)-1)
      
    time.sleep(0.002)
                
    return result, receivedData
        


