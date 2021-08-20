#include <mbed.h>
#include "DS2781.h"
#include "OneWire.h"
#include <FATFileSystem.h> // File system library 
#include <SDBlockDevice.h> // SD library 
 
 // ROM ID of the battery monitor
 // Note this ID is bit-sequential with the LSB coming first 
char battROM[8] = {0x3D, 0xD0, 0x1B, 0xD7, 0x00, 0x00, 0x00, 0x63};

OneWire onewire(p9);
DS2781 ds27(&onewire, battROM);
DigitalOut mountLight(LED3); // Signals SD card in use
DigitalOut errorLight(LED4); // Error signal 
SDBlockDevice sd(p5, p6, p7, p8); // mosi, miso, sclk, cs
FATFileSystem fs("fs");
Timer timelog; // Timer to track the time since data collection started. These timestamps will be stored in the file log 
Timer data_collect_interval; // Timer to track the data collection interval


int main() {
    ThisThread::sleep_for(100ms);
    printf("Started the program\n");
    float voltage, current, temp; // Initialize data variables  
    bool notDone; // Boolean controlling the data collection loop 
    mountLight = 0; // SD card not in use
    errorLight = 0;  // no errors yet so turn off
    // try to mount the SD card
    int errors;  // holds error codes (if any)
    errors = fs.mount(&sd);  // connect fs to sd
    if (errors) {  // true (not zero) if errors occurred
        errorLight = 1;  // turn on error indicator LED
        printf("The SD card failed to mount. Error code: %i\n", errors);
        return -1;   // exit the program indicating an error
    }
    mountLight = 1; // SD card in use - don't disturb
    // Open the file on the SD and get its file descriptor (link)
    FILE* logFile = fopen("/fs/battery_log.txt", "a");  // a = append (add to file if exists, create if not)
    if (logFile == NULL) {  // file pointer will be NULL if not opened successfully
        errorLight = 1;
        return -1;
    }
    fprintf(logFile, "Time (s)\tVoltage (V)\tCurrent (A)\tTemperature (C)\n"); // Header 
    printf("Starting timer\n");
    timelog.start();
    data_collect_interval.start();
    while(notDone){
        if(data_collect_interval.elapsed_time() > 1s){
            // Collect data
            voltage = ds27.readVoltage();
            ThisThread::sleep_for(1ms); // Time buffer
            current = ds27.readCurrent();
            ThisThread::sleep_for(1ms); // Time buffer
            temp = ds27.readTemp();
            ThisThread::sleep_for(1ms); // Time buffer

            // If temp, current, and voltage are all returning error boolean (0),
            // Set notDone to 0 as the battery has died 

            // Write data to the log.txt file 
            // fprintf has the same format string options as printf
            fprintf(logFile, "%.4f\t", (timelog.elapsed_time().count())*0.000001); // Log time in seconds (converted from us)
            fprintf(logFile, "%.4f\t", temp);
            fprintf(logFile, "%.4f\t", voltage);
            fprintf(logFile, "%.4f\n", current);
            data_collect_interval.reset();
        }
    }
    data_collect_interval.stop();
    timelog.stop();
    // close the file when done
    fclose(logFile);
    mountLight = 0; // It is ok to eject the SD card now
    printf("Program complete\n");
    return 0; // exit indicating success
}