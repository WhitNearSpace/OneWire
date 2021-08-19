#include "OneWire.h" 
#include "DS2781.h"

OneWire::OneWire(PinName data_pin, PinName power_pin){
    _data_pin = new DigitalInOut(data_pin);
    _power_pin = new DigitalInOut(power_pin);
    _data_pin->mode(PullUp);
}

bool OneWire::reset(){
    bool result = false; 
    _data_pin->output();
    _data_pin->write(0);
    wait_us(480); // Manually set to 480 us delay 
    _data_pin->input(); // let the data line float high
    delay('I', 1);
    if(_data_pin->read() == 0){
        result = true; 
    }
    delay('J', 1);
    return result; 
}

void OneWire::writeBit(bool bit_data){
    if(bit_data){
        // If the bit data is high, write '1' bit
        _data_pin->output(); // Write to pin as output 
        _data_pin->write(0); // Drive data line low 
        delay('A',1); //  6us LOW 
        _data_pin->write(1); // Release the bus
        delay('B',1); //  64us HIGH 
    }else{
        // Write '0' bit
        _data_pin->output();
        _data_pin->write(0); // Drive data line low
        delay('C',1); // 60 us LOW
        _data_pin->write(1); // Release the bus
        delay('D',1); // 10 us HIGH 
    }
}


bool OneWire::readBit(){
    bool answer;
    _data_pin->output(); // Write to pin as output 
    _data_pin->write(0); // Drive bus low 
    /**To read a bit, the bus is meant to be held low for 6us (standard operation)
     * To acheive this wait_ns is used for a duration of 4000 ns (4us)
     * This is done because 
     * 1) wait_us has significant overhead time of AT LEAST 10us
     *    which causes all sorts of problems 
     * 2) allowing the bus to float high via the input method has 
     *    overhead of ~2us 
     * Thus, a 4000ns wait + 2us of time from input() = the suggested 
     * 6us wait that the bus should be held low 
    **/
    wait_ns(4000); // Call 4us delay using wait_ns
    _data_pin->input(); // Change pin mode to input
    wait_us(3); // Call 3us delay 
    answer = _data_pin->read(); // Sample bit value from the slave 
    wait_us(55); // Call delay of ~55us 
    return answer; 
}

void OneWire::writeByte(char byte_data){
    for(int i = 0; i < 8; i++){
        writeBit(byte_data & 0x01);
        byte_data >>= 1; 
    }
}

char OneWire::readByte(){
    char answer = 0x00; // Empty char to hold response 
    for(int i = 0; i < 8; i++){ // Construct the response bit by bit
        answer = answer >> 1; // Right shift to make room for the next bit 
        if(readBit()){
            answer = answer | 0x80; // If the data port is high, set the bit to 1
        }
    }
    return answer; 
}

// Global search variables
#define FALSE 0
#define TRUE 1
char search_ROM[8];
int last_descrepancy;
bool done_flag;

int OneWire::find_first_device(){
    // Reset the search state
    last_descrepancy = 0; 
    done_flag = false; 
    return ROM_search();
}

// Function form based on search functions at  https://www.maximintegrated.com/en/design/technical-documents/app-notes/1/187.html 
int OneWire::find_next_device()
{
   // leave the search state alone
   return ROM_search();
}

// Function form based on search functions at https://github.com/JohnMLarkin/Mbed_DS1820/blob/master/DS1820.cpp
// and https://www.maximintegrated.com/en/design/technical-documents/app-notes/1/187.html 
int OneWire::ROM_search() {
    int descrepancy_marker;
    int ROM_bit_index;
    bool return_value; 
    bool Bit_A = 0; // First AND bit to be read from the slaves 
    bool Bit_B = 0; // Complement AND to be read from the slaves 
    int byte_counter;
    char bit_mask;
    int search_direction; 
    printf("DONE FLAG %i\n", done_flag);
    return_value = false;
    // If the last device was no the last
    if(!done_flag){
        if (!reset()) {
            // If a reset fail is encountered, reset the search 
            last_descrepancy = 0; 
            done_flag = false; 
            printf("Reset fail\n");
            return false;
        } else {
            ROM_bit_index = 1;
            descrepancy_marker = 0;
            char command_shift = SEARCH_ROM;
            // Send the search command 
            for (int n=0; n<8; n++) {           
                writeBit(command_shift & 0x01);
                command_shift = command_shift >> 1; // now the next bit is in the least sig bit position.
            } 
            byte_counter = 0;
            bit_mask = 0x01;
            // Loop to do the search 
            while (ROM_bit_index<=64) {
                Bit_A = readBit();
                Bit_B = readBit();
                printf("Current bit index: %i\n", ROM_bit_index);
                printf("Bit A: %i\tBit B: %i\n", Bit_A, Bit_B);
                if (Bit_A & Bit_B) {
                    break; 
                } else {
                    if (Bit_A | Bit_B) { // If A and B are NOT equal 
                        // Then the search direction is equal to Bit_A, so we write this value to the search ROM array 
                        search_direction = Bit_A; // bit write value for seach 
                    } else {
                        // both bits A and B are low, so there are two or more devices present
                        if (ROM_bit_index == last_descrepancy) { // If bit position is equal to the last discrepancy, take the '1' path 
                            search_direction = search_ROM[byte_counter] | bit_mask; // Set ROM bit to one
                        } else {
                            if (ROM_bit_index > last_descrepancy) { // If bit position is greater than the last discrepancy, take the '0' path 
                                search_direction = (ROM_bit_index == last_descrepancy);
                            } else {
                                search_direction = ((search_ROM[byte_counter] & bit_mask) == 0x00);
                            }
                        }
                    }

                    // If 0 was picked then record its position in the descrepancy marker
                    if(!search_direction){
                        descrepancy_marker = ROM_bit_index;
                    }

                    // Set or clear the bit in the ROM byte rom_byte_number 
                    // using the bit mask
                    if(search_direction == 1){
                        printf("Writting %0x to index %0x of the ROM buffer\n", (search_ROM[byte_counter] | bit_mask), byte_counter);
                        search_ROM[byte_counter] = search_ROM[byte_counter] | bit_mask;
                    }else{
                        printf("Writting %0x to index %0x of the ROM buffer\n", (search_ROM[byte_counter] & ~bit_mask), byte_counter);
                        search_ROM[byte_counter] = search_ROM[byte_counter] & ~bit_mask;
                    }

                    // Write the search direction 
                    writeBit(search_direction);

                    ROM_bit_index++;
                    if (bit_mask & 0x80){
                        byte_counter++;
                        bit_mask = 0x01;
                    } else {
                        bit_mask = bit_mask << 1;
                    }
                }
            }
            
        }
        
        last_descrepancy = descrepancy_marker;
        if(last_descrepancy == 0){
            done_flag = true;
        }

        return_value = TRUE;
    }

    // Now that the ROM ID has been found, store it in the _ROM of the current device 
    // Note that this code flips the ROM order from LSB first to MSB first
    // _ROM contains the id in this form: [CRC] [48-BIT SERIAL #] [FAMILY CODE]
    int j = 7; 
    for (int i = 0; i<8; i++) {
        _ROM[i] = search_ROM[j];
        j--;
    }
    return return_value;
}


bool OneWire::ROM_search_by_family(char family_code){
    bool result = true;
    return result; 
}

void OneWire::delay(char letter, bool standard){
    // Recommended delays from Maxim at standard speed 
    // https://www.maximintegrated.com/en/app-notes/index.mvp/id/126
    
    
    //Letter  Speed       Standard Recommended (µs)    Overdrive Recommended (µs) 
    //A       Standard    6                            1.0
    //B       Standard    64                           7.5
    //C       Standard    60                           7.5
    //D       Standard    10                           2.5
    //E       Standard    9                            1.0
    //F       Standard    55                           7
    //G       Standard    0                            2.5
    //H       Standard    480                          70
    //I       Standard    70                           8.5
    //J       Standard    410                          40
    switch(letter){
        case 'A': 
            if(standard) wait_us(6); 
            else wait_us(1);
            break;
        case 'B': 
            if(standard) wait_us(64);
            else wait_us(7.5); 
            break;
        case 'C': 
            if(standard) wait_us(60); 
            else wait_us(7.5);
            break;
        case 'D': 
            if(standard) wait_us(10); 
            else wait_us(2.5);
            break;
        case 'E': 
            if(standard) wait_us(9); 
            else wait_us(1);
            break;
        case 'F': 
            if(standard) wait_us(55); 
            else wait_us(7);
            break;
        case 'G': 
            if(standard) wait_us(0); 
            else wait_us(2.5);
            break;
        case 'H': 
            if(standard) wait_us(480); 
            else wait_us(70);
            break;
        case 'I': 
            if(standard) wait_us(70); 
            else wait_us(8.5);
            break;
        case 'J': 
            if(standard) wait_us(410);
            else wait_us(40); 
            break;
        default: break;
    }
}

// void OneWire::set_family_code(char family_code){
//     _family_code = family_code; 
// }


unsigned long long OneWire::whoAmI() {
    unsigned long long myName = 0;
    for (int i = 0; i<8; i++) {
        myName = (myName<<8) | _ROM[i];
    }
    return myName;
}
