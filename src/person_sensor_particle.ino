#include "Arduino.h"
// #include <Serial.h>
// #include <Wire.h>


#include "person_sensor.h"

// How long to wait between reading the sensor. The sensor can be read as
// frequently as you like, but the results only change at about 5FPS, so
// waiting for 200ms is reasonable.
const int32_t SAMPLE_DELAY_MS = 200;

const int outputArrayDimension = 32;
int outputArray[outputArrayDimension * outputArrayDimension];

Logger mainLog("app.main");

void printHex(uint8_t num) {
  char hexCar[3];

  sprintf(hexCar, "%02X", num);
  Serial.print(hexCar);
}


constexpr size_t I2C_BUFFER_SIZE = 40;

HAL_I2C_Config acquireWireBuffer() {
    HAL_I2C_Config config = {
        .size = sizeof(HAL_I2C_Config),
        .version = HAL_I2C_CONFIG_VERSION_1,
        .rx_buffer = new (std::nothrow) uint8_t[I2C_BUFFER_SIZE],
        .rx_buffer_size = I2C_BUFFER_SIZE,
        .tx_buffer = new (std::nothrow) uint8_t[I2C_BUFFER_SIZE],
        .tx_buffer_size = I2C_BUFFER_SIZE
    };
    return config;
}

/* ------------------------------ */
// function to move the terminal cursor back up to overwrite previous data printout
void moveTerminalCursorUp(int numlines) {
    String cursorUp = String("\033[") + String(numlines) + String("A");
    Serial.print(cursorUp);
    Serial.print("\r");
}

/* ------------------------------ */
// function to move the terminal cursor down to get past previous data printout - used on startup
void moveTerminalCursorDown(int numlines) {
    String cursorUp = String("\033[") + String(numlines) + String("B");
    Serial.print(cursorUp);
    Serial.print("\r");
}


/* ------------------------------ */
// function to pretty print data to serial port
//   retuns number of lines printed
int prettyPrintArray(int dataArray[], int numCols, int numRows) {
    //The ST library returns the data transposed from zone mapping shown in datasheet
    //Pretty-print data with increasing y, decreasing x to reflect reality 

    int lines = 0;
    int time = millis();
    Serial.printlnf("%ld                                   ", time );
    Serial.print("\t    ");
    for (int i = numCols-1; i >= 0; i--) {
        Serial.printf("%-2i",i);
    }
    Serial.println();
    lines++;
    for(int y = 0; y <= numRows * (numCols - 1) ; y += numCols)  {
        Serial.print("\t");
        Serial.printf("%-2i:  ", y/numCols);
        for (int x = numCols - 1 ; x >= 0 ; x--) {
            Serial.printf("%-2d", dataArray[x + y]);
        }
        Serial.println();
        lines++;
    } 
    return lines;
}

void setup() {
  // You need to make sure you call Wire.begin() in setup, or the I2C access
  // below will fail.

  HAL_I2C_Config config = acquireWireBuffer();
  hal_i2c_init(HAL_I2C_INTERFACE1, &config);
  Wire.begin();

  Serial.begin(9600);
  Serial.println("Commands: 1 - toggle streaming");

  moveTerminalCursorDown(40);

  mainLog("setup complete");
}

void loop() {
    static bool enableStreamingDisplay = true;

    if (Serial.available()) { // check if there's any data available on the serial port
        char command = Serial.read(); // read the incoming data and store it in a variable called "command"
        mainLog("serial available >0");
        // perform action based on the command received
        switch (command) {
        case '1':
            // toggle streaming display
            enableStreamingDisplay = !enableStreamingDisplay;
            if (enableStreamingDisplay) {
                Serial.println("Streaming Enabled");  
            } else {
                for (int i=0;i<15;i++) {
                    Serial.println("");
                }
                Serial.println("Streaming Disabled");
            }
            break;
        default:
            mainLog("serial command unknown");
            break;
        }
    }


    // get results
    static int i = 0; 
    person_sensor_results_t results = {};
    // Perform a read action on the I2C address of the sensor to get the
    // current face information detected.
    if (!person_sensor_read(&results)) {
        i++;
        Serial.println(i + ": No person sensor results found on the i2c bus");
        delay(SAMPLE_DELAY_MS);
        return;
    }

   
    
    
    if (results.num_faces <0){
        char* my_s_bytes = reinterpret_cast<char*>(&results);
        for(unsigned int j = 0; j <sizeof(results); j++){
            printHex(my_s_bytes[j]);
    }

    }

    if (enableStreamingDisplay) {

        // array to hold values we will display through serial port
        // set all values to 0
        for (int i=0; i<outputArrayDimension * outputArrayDimension; i++){
            outputArray[i] = 0;
        }

        for (int i = 0; i < results.num_faces; ++i) {
            const person_sensor_face_t* face = &results.faces[i];

            // bounding box scaled to size of output array
            int left = map(face->box_left, 0, 256, 0, outputArrayDimension);
            int right = map(face->box_right, 0, 256, 0, outputArrayDimension);
            int top = map(face->box_top, 0, 256, 0, outputArrayDimension);
            int bottom = map(face->box_bottom, 0, 256, 0, outputArrayDimension);

            for (int j=top; j<bottom; j++) {
                for (int k=left; k<right; k++) {
                    outputArray[j*outputArrayDimension + k] = 1;
                }
            }
        }

        int linesPrinted = 0;
        linesPrinted = prettyPrintArray(outputArray, outputArrayDimension, outputArrayDimension);

        Serial.println();
        Serial.println();
        Serial.println();
        linesPrinted += 3;

        // overwrite the previous display
        moveTerminalCursorUp(linesPrinted+1);

    } else {

        Serial.println();
        Serial.println("********");
        Serial.print(results.num_faces);
        Serial.println(" faces found");

        for (int i = 0; i < results.num_faces; ++i) {
            const person_sensor_face_t* face = &results.faces[i];
            Serial.print("Face #");
            Serial.print(i);
            Serial.print(": ");
            Serial.print(face->box_confidence);
            Serial.print(" confidence, (");
            Serial.print(face->box_left);
            Serial.print(", ");
            Serial.print(face->box_top);
            Serial.print("), (");
            Serial.print(face->box_right);
            Serial.print(", ");
            Serial.print(face->box_bottom);
            Serial.print("), ");
            if (face->is_facing) {
            Serial.println("facing");
            } else {
            Serial.println("not facing");
            }
        }
    }
    
    delay(SAMPLE_DELAY_MS);
}


