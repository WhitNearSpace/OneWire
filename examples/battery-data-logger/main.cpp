#include <mbed.h>
#include "DS2781.h"
#include "OneWire.h"
#include "DS18B20.h"
#include <FATFileSystem.h> // File system library 
#include <SDBlockDevice.h> // SD library 
 
 // ROM ID of the battery monitor
 // Note this ID is bit-sequential with the LSB coming first 
char battROM[8] = {0x3D, 0xD0, 0x1B, 0xD7, 0x00, 0x00, 0x00, 0x63};

OneWire onewire(p9);
DS2781 ds27(&onewire, battROM);
DigitalOut mountLight(LED3); // Signals SD card in use
DigitalOut errorLight(LED4); // Error signal 
SDBlockDevice sd(p5, p6, p7, p8);
FATFileSystem fs("fs");


int main() {
    float voltage, current, temp; // Initialize data variables  
    mountLight = 0; // SD card not in use
    errorLight = 0;  // no errors yet so turn off
    // try to mount the SD card
    int errors;  // holds error codes (if any)
    errors = fs.mount(&sd);  // connect fs to sd
    if (errors) {  // true (not zero) if errors occurred
        errorLight = 1;  // turn on error indicator LED
        printf("The SD card failed to mount. Error code: %i", errors);
        return -1;   // exit the program indicating an error
    }
    mountLight = 1; // SD card in use - don't disturb
    // Open the file on the SD and get its file descriptor (link)
    FILE* logFile = fopen("/fs/log.txt", "a");  // a = append (add to file if exists, create if not)
    if (logFile == NULL) {  // file pointer will be NULL if not opened successfully
    errorLight = 1;
    return -1;
    }
    while(1){
        // Collect data
        ThisThread::sleep_for(59700ms); // 59.7 second delay 
        voltage = ds27.readVoltage();
        ThisThread::sleep_for(100ms); // Time buffer
        current = ds27.readCurrent();
        ThisThread::sleep_for(100ms); // Time buffer
        temp = ds27.readTemp();
        ThisThread::sleep_for(100ms); // Time buffer

        // Write data to the log.txt file 
        // fprintf has the same format string options as printf 
        fprintf("Temp: %.3f C\n", temp);
        fprintf("Voltage: %.3f V\n", voltage);
        fprintf("Current: %.6f A\n", current);
        fprintf("\n");
    }
}