/*******************************************************************************************************************
** MB85_FRAM class method definitions. See the header file for program details and version information            **
**                                                                                                                **
** This program is free software: you can redistribute it and/or modify it under the terms of the GNU General     **
** Public License as published by the Free Software Foundation, either version 3 of the License, or (at your      **
** option) any later version. This program is distributed in the hope that it will be useful, but WITHOUT ANY     **
** WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the   **
** GNU General Public License for more details. You should have received a copy of the GNU General Public License **
** along with this program.  If not, see <http://www.gnu.org/licenses/>.                                          **
**                                                                                                                **
*******************************************************************************************************************/
#include "MB85_FRAM.h"                                                        // Include the header definition    //
                                                                              //----------------------------------//
MB85_FRAM_Class::MB85_FRAM_Class()  {}                                        // Empty & unused class constructor //
MB85_FRAM_Class::~MB85_FRAM_Class() {}                                        // Empty & unused class destructor  //
/*******************************************************************************************************************
** Method begin starts communications with the device. There are 4 possible memory sizes, 8kB, 16kB, 32kB, and    **
** 64kB and this function will automatically determine the size of each memory found. The memories will wrap      **
** around from the highest-address back to 0 on reads/writes and the detection process makes use of this feature. **
** The contents of memory address 0 is saved and overwritten with 0x00. Then, starting with the smallest memory,  **
** the value of the highest value for that memory plus 1 is stored and written to 0xFF. If address 0 has changed  **
** to 0xFF then we know we've had a wrap-around and have identified the chip, otherwise we repeat the procedure   **
** for the next possible memory size address and so on.                                                           **
*******************************************************************************************************************/
uint8_t MB85_FRAM_Class::begin() {                                            // Find I2C device                  //
  Wire.begin();                                                               // Start I2C as master device       //
  for(uint8_t i=MB85_MIN_ADDRESS;i<MB85_MIN_ADDRESS+8;i++) {                  // loop all possible addresses      //
    Wire.beginTransmission(i);                                                // Check current address for device //
    if (Wire.endTransmission()==0) {                                          // If no error we have a device     //
      for(uint16_t memSize=8192;memSize!=0;memSize=memSize*2) {               // Check each memory size           //
                                                                              //----------------------------------//
        Wire.beginTransmission(i);                                            // Start transmission               //
        Wire.write((uint8_t)0);Wire.write((uint8_t)0);                        // Start at address 0               //
        _TransmissionStatus = Wire.endTransmission();                         // Close transmission               //
        Wire.requestFrom(i,(uint8_t)1);                                       // Request 1 byte of data           //
        uint8_t minimumByte = Wire.read();                                    // Store value of byte 0            //
                                                                              //----------------------------------//
        Wire.beginTransmission(i);                                            // Start transmission               //
        Wire.write((uint8_t)0);Wire.write((uint8_t)0);                        // Start at address 0               //
        Wire.write(0xFF);                                                     // write high value to address 0    //
        _TransmissionStatus = Wire.endTransmission();                         // Close transmission               //
                                                                              //----------------------------------//
        Wire.beginTransmission(i);                                            // Start transmission               //
        Wire.write((uint8_t)memSize>>8);                                      // Write MSB of address             //
        Wire.write((uint8_t)memSize);                                         // Write LsB of address             //
        _TransmissionStatus = Wire.endTransmission();                         // Close transmission               //
        Wire.requestFrom(i,(uint8_t)1);                                       // Request 1 byte of data           //
        uint8_t maximumByte = Wire.read();                                    // Store value of high byte for chip//
                                                                              //----------------------------------//
        Wire.beginTransmission(i);                                            // Start transmission               //
        Wire.write((uint8_t)memSize>>8);                                      // Write MSB of address             //
        Wire.write((uint8_t)memSize);                                         // Write LsB of address             //
        Wire.write(0x00);                                                     // write low value to max address   //
        _TransmissionStatus = Wire.endTransmission();                         // Close transmission               //
                                                                              //----------------------------------//
        Wire.beginTransmission(i);                                            // Start transmission               //
        Wire.write((uint8_t)0);Wire.write((uint8_t)0);                        // Start at address 0               //
        _TransmissionStatus = Wire.endTransmission();                         // Close transmission               //
        Wire.requestFrom(i,(uint8_t)1);                                       // Request 1 byte of data           //
        uint8_t newMinimumByte = Wire.read();                                 // Store value of byte 0            //
        if(newMinimumByte!=0x00) {                                            // If the value has changed         //
          _I2C[i-MB85_MIN_ADDRESS] = memSize/1024;                            // Store memory size in kB          //
          Wire.beginTransmission(i);                                          // Start transmission               //
          Wire.write((uint8_t)0);Wire.write((uint8_t)0);                      // Position to address 0            //
          Wire.write(minimumByte);                                            // restore original value           //
          _TransmissionStatus = Wire.endTransmission();                       // Close transmission               //
         break;                                                               // Exit the loop                    //
        } else {                                                              //                                  //
          Wire.beginTransmission(i);                                          // Start transmission               //
          Wire.write((uint8_t)memSize>>8);                                    // Write MSB of address             //
          Wire.write((uint8_t)memSize);                                       // Write LsB of address             //
          Wire.write(maximumByte);                                            // restore original value           //
          _TransmissionStatus = Wire.endTransmission();                       // Close transmission               //
        } // of if-then-else we've got a wraparound                           //                                  //
        if (!_I2C[i-MB85_MIN_ADDRESS]) _I2C[i-MB85_MIN_ADDRESS] = 32;         // If none of the above, then 32kB  //
      } // of for-next loop for each memory size                              //                                  //
      _DeviceCount++;                                                         // Increment the found count        //
    } // of if-then we have found a device                                    //                                  //
  } // of for-next each I2C address loop                                      //                                  //
  return _DeviceCount;                                                        // return number of memories found  //
} // of method begin()                                                        //----------------------------------//

/*******************************************************************************************************************
** Method totalBytes() returns the sum of all the memories found                                                  **
*******************************************************************************************************************/
uint32_t MB85_FRAM_Class::totalBytes() {                                      // Return total memory found        //
  uint32_t bytesFound = 0;                                                    // Initialize return variable       //
  for(uint8_t i=0;i<MB85_MAX_DEVICES;i++) bytesFound += _I2C[i];              // Sum up the the kilobytes         //
  return(bytesFound*1024);                                                    // return total bytes               //
} // of method totalBytes()                                                   //----------------------------------//

/*******************************************************************************************************************
** Method getDevice() is an internal call used in the read() and write() templates to compute which memory to     **
** use for a given memory address                                                                                 **
*******************************************************************************************************************/
uint8_t MB85_FRAM_Class::getDevice(uint32_t &memAddress,uint32_t &endAddress){// Return device offset address     //
  uint8_t  device       = 0;                                                  //                                  //
  uint32_t startAddress = 0;                                                  //                                  //
           endAddress   = UINT32_MAX;                                         // Start at max so adding 1 = 0     //
  for(device=0;device<MB85_MAX_DEVICES;device++) {                            // Loop through all devices found   //
    if(_I2C[device]) {                                                        // If there's a memory at address   //
      startAddress = endAddress+1;                                            // Chip starts after previous end   //
      endAddress  += (uint32_t)_I2C[device] * 1024;                           // Compute end of memory chip       //
     if (endAddress>=memAddress) break;                                       // Exit if we are in range          //
     memAddress  -= (uint32_t)_I2C[device] * 1024;                            // adjust memory address            //
    } // of if we have a device at address                                    //                                  //
  } // of for-next all possible devices                                       //                                  //
  return device;                                                              //                                  //
} // of internal method getDevice()                                           //----------------------------------//

/*******************************************************************************************************************
** Method memSize() returns the device's size in Kb                                                               **
*******************************************************************************************************************/
uint16_t MB85_FRAM_Class::memSize(const uint8_t memNumber) {                  // Return memory size in bytes      //
   if(memNumber<=_DeviceCount) return(_I2C[memNumber]*1024); else return 0;   // Return appropriate value         //
} // of method memSize()                                                      //----------------------------------//   
