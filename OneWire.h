#ifndef ONEWIRE_H
#define ONEWIRE_H

#include <mbed.h>

#define SKIP_ROM 0xCC // Allows the host to access device functions without providing the 64-bit ROM ID
#define READ_ROM 0x33 // Allows the host to read the 8-bit family code, the 48-bit serial number, and the eight-bit CRC
#define MATCH_ROM 0x55 // Allows the host to specify a particular device, identified by its ROM ID, to respond to the next command 
// Send MATCH_ROM, then the ROM ID of the device you wish to communicate with, and the next device command will be executed by that device
#define SEARCH_ROM 0xF0 // Allows the host to discover the ROM ID values for all devices in the system
#define READ_DATA 0x69
#define WRITE_DATA 0x6C
#define COPY_DATA 0x48 
#define RECALL_DATA 0xB8
#define LOCK 0x6A 

class OneWire{
    public:
        /** Create an OneWire interface, connected to the specified pins
         *
         *  @param data_pin primary PIO
         *  @param power_pin optional pin that is by default NC
         */
        OneWire(PinName data_pin, PinName power_pin = NC);

        /*** Send reset command on the 1-Wire bus 
         * @returns
         *  true if the reset command had been communicated 
         *  flase if the reset command fails
         * **/
        bool reset();

        /*** Write a bit
         * @param bit_data the bit to be written 
         * **/
        void writeBit(bool bit_data);

        /*** 
         * Read a bit
         * **/
        bool readBit();

        /*** Write a byte
         * @param byte_data the byte to be written 
         * **/
        void writeByte(char byte_data);

        /*** 
         * Read a byte
         * **/
        char readByte();

        /*** 
         * Collect the ROM IDs of all 1-Wire devices on the bus 
         * **/
        int ROM_search();

        int find_first_device();

        int find_next_device();


        /*** 
         * Collect the ROM IDs of all 1-Wire devices on the bus 
         * belonging to a particular family specified by 
         * @param family_code 
         * **/
        bool ROM_search_by_family(char family_code);

        /*** Create a delay in standard or overdrive mode based on input char
         * 
         * @param letter correlates to a delay time specified by Maxim documentation
         * Different letters are used to trigger delays to effectively communicate 
         * between master and slave
         * @param standard toggles delay times between standard (1) and 
         * overdrive (0)  
         * **/
        void delay(char letter, bool standard);
        

        /**
         * Contributed by John M. Larkin (jlarkin@whitworth.edu)
         * This will return the 64 bit address of the connected device
        ***/ 
        unsigned long long whoAmI();


    protected: 
        DigitalInOut* _data_pin;
        DigitalInOut* _power_pin; 
        unsigned char _ROM[8] = {0,0,0,0,0,0,0,0}; // Unique and unalterable ROM ID number 
};

#endif