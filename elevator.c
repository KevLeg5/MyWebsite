#include <xc.h>
#include <inttypes.h>
#include <stdbool.h>
#include <ctype.h>


// *************************************************************************************
//                               LCD Defines 
// *************************************************************************************
#define DELAY_LED (1 << 15) //PA15, physical pin 16. For testing sanity purposes

#define DISP_RS (1 << 24) //PA25, physical pin 24
//#define DISP_RW --> GROUNDED PERMANENTLY! No need to attach. We want to stay in write mode.
#define DISP_E (1 << 23) //PA23, physical pin 22
#define DISP_D7 (1 << 22) //PA22, physical pin 21
#define DISP_D6 (1 << 19) //PA19, physical pin 20
#define DISP_D5 (1 << 18)//PA18, physical pin 19
#define DISP_D4 (1 << 17) //PA17, physical pin 18

#define HIGH 1
#define LOW 0

// *************************************************************************************
//                               Keypad Defines 
// *************************************************************************************
//Rows are Outputs while we read the Cols as EXTINTn
#define keypad_row0 (1 << 06)
#define keypad_row1 (1 << 07)
#define keypad_row2 (1 << 04)
#define keypad_row3 (1 << 05)

// See the IO Multiplexing chapter as they are used as both EXTINTn Pins and PA Input Pins
#define keypad_col0 (1 << 0) // 0
#define keypad_col1 (1 << 1) // 1
#define keypad_col2 (1 << 2) // 2
#define keypad_col3 (1 << 3) // 3

char keypadMapping[4][4] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};
// initial debug values. Meaningless.
int row = -1;
int col = -1;
char key = 'z'; 

// *************************************************************************************
//                               Elevator Defines
// *************************************************************************************
#define buzzer (1 << 11) // PA11 physical pin 14
#define redLED (1 << 10) // PA10 physical pin 13 
#define yellowLED (1 << 9) // PA9 physical pin 12 
#define greenLED (1 << 8) // PA8 physical pin 11 

int elevatorFloor = 1; // The floor the user is currently on 
int targetFloor = -1; // Where the user wants to ride the elevator to
int userFloor = -1; // Where the elevator gets called to (the floor the summoning user is on)
uint32_t startTime = -1; // for holding door open detection

#define moving_up  1
#define moving_down 0
int dir = -1;

#define door_open 1 
#define door_closed 0 
int doorState = door_closed;

#define elevator_ready 0
#define elevator_boarded 1
#define elevator_moving 2
int elevatorState = elevator_ready;


// *************************************************************************************
//                               Helper Functions
// *************************************************************************************

// Write high or low to PA output pin.
void digitalWrite(int pin, int val) {
    if (val == 0) {
        PORT_REGS->GROUP[0].PORT_OUTCLR = pin;
    } else {
        PORT_REGS->GROUP[0].PORT_OUTSET = pin;
    }
}

// Each clock cycle on 8Mhz is 125ns long. 125ns is the fastest rate that count can be updated.
// A single "wait" is 125ns. 2 waits is 250ns... so on.
void delay_125nanos(uint32_t numWaits) {
    // count32 is set to the 8MHz clock. Thus every count increment is 125ns
    uint32_t endTime = TC4_REGS->COUNT32.TC_COUNT + (numWaits);
    while (TC4_REGS->COUNT32.TC_COUNT < endTime); // wait until specified period
}

// This is true micro time using TC4 counter.
void delay_micros(uint32_t ms) {
    // 8 count increments (1 count per 125ns) = 1 microsecond
    uint32_t endTime = TC4_REGS->COUNT32.TC_COUNT + (ms * 8);
    while (TC4_REGS->COUNT32.TC_COUNT < endTime);
}

// This is true ms time using TC4 counter.
void delay_millis(uint32_t ms) {
    // 8000 count increments (1 count per 125ns) = 1 millisecond
    uint32_t endTime = TC4_REGS->COUNT32.TC_COUNT + (ms * 8000);
    while (TC4_REGS->COUNT32.TC_COUNT < endTime);
}

// *************************************************************************************
//                               Setup Functions
// *************************************************************************************

void configurePorts() {
    PM_REGS->PM_APBBMASK |= (1 << 3); // Enable clock for PORT, which is on APB-B bus. Default.

    // LED outputs
    PORT_REGS->GROUP[0].PORT_DIRSET = DELAY_LED;
    PORT_REGS->GROUP[0].PORT_DIRSET = redLED | yellowLED | greenLED | buzzer;

    // LCD Display outs
    PORT_REGS->GROUP[0].PORT_DIRSET = DISP_RS | DISP_E | DISP_D4 | DISP_D5 | DISP_D6 | DISP_D7;

    // Keypad Ports
    PORT_REGS->GROUP[0].PORT_DIRCLR = keypad_col0 | keypad_col1 | keypad_col2 | keypad_col3; // Set cols as inputs
    PORT_REGS->GROUP[0].PORT_DIRSET = keypad_row0 | keypad_row1 | keypad_row2 | keypad_row3; // Set all the rows to be output pins

    // Hardware Interrupt input
    PORT_REGS->GROUP[0].PORT_PINCFG[0] |= (1 << 1) | (1); // enable input sampling on PA0 (col 0). Enable multiplexed interrupt.
    PORT_REGS->GROUP[0].PORT_PINCFG[1] |= (1 << 1) | (1); // enable input sampling on PA1 (col 1). Enable multiplexed interrupt.
    PORT_REGS->GROUP[0].PORT_PINCFG[2] |= (1 << 1) | (1); // enable input sampling on PA2 (col 2). Enable multiplexed interrupt.
    PORT_REGS->GROUP[0].PORT_PINCFG[3] |= (1 << 1) | (1); // enable input sampling on PA3 (col 3). Enable multiplexed interrupt.
}

// Enabled clock following instructions: Enabling Peripheral Setup [pg 108] section 14.4
// Extra info: SYSCTRL Clock Sources [158] [160-17.5.3] || Power Manager[130] || GCLK Initialization[113-15.6.2] )
void configureClock() {
    // Initializing Clock Source: OSC8m Oscillator @1MHz. Below line is unnecessary. They are defaults. Leaving for verbosity
    SYSCTRL_REGS->SYSCTRL_OSC8M |= (0b1 << 1) | (0b11 << 8); // Enable the Clock Source. Set OSC 8Mhz pre-scaler to 2^(3).
    SYSCTRL_REGS->SYSCTRL_OSC8M &= ~(11 << 8); // Set Clock source pre-scaler to 0 so we get 8MHz & 125ns clock periods.
    // Binding the OSC8m Oscillator Clock Source as input to a Generic Clock Generator for output to Peripherals TC4,TC5.
    GCLK_REGS->GCLK_GENDIV |= (0b1) | (0b0 << 8); // Select Generic Clock Generator 1. Set Clock Generator division from oscillator to 0 (as default). 
    GCLK_REGS->GCLK_GENCTRL |= (0b1) | (0x06 << 8) | (1 << 16); // Select Generic Clock Generator 1. Setting OSC8M Clock Source Oscillator as input to selected GCLK.
    GCLK_REGS->GCLK_CLKCTRL |= (0x1 << 8) | (1 << 14) | (0x1C); // Select Generic Clock Generator 1. Enable Clock. Set Selected GCLK to drive TC4 & TC5
}

// Configures TC4 for clocked 32 bit operation.
// To properly setup clocked peripherals, a clock must already be associated with them.
// Therefore, do not call this method before the configureClock method, above, which sets GCLK 1 to drive TC4 & TC5.
void configureTC4() {
    // Enable ARM system interrupts from TC4. [567-30.6.4.2] [47-11.2]
    NVIC_EnableIRQ(TC4_IRQn); // Hello Reader. I am just realizing I left this in before submission. It was not necessary
    // and it therefore should not be necessary. But I'm not deleting it at this point because I cannot re-test the circuit.

    // Configuring TC4 in 32 bit mode.
    // We're not using interrupts this time for TC4. Just continuous counting for timing.
    PM_REGS->PM_APBCMASK |= (1 << 12); // "unmasking" the TC4 "peripheral," which is connected to the APB bus C bridge. Enables clock input and peripheral register writes
    TC4_REGS->COUNT32.TC_CTRLA |= (0x2 << 2) | (0x0 << 5); //Set mode for 32 bit counter. Set 32-bit counter "waveform" max value to 2^32.
    TC4_REGS->COUNT32.TC_CTRLA |= (1 << 1); // Enable peripheral TC4. 
}

void configureEIC() {
    // ***REMEMBER*** to enable BOTH input sampling (different than just setting to input!), 
    // and enable the multiplexing EXTINT function in configurePorts().
    
    NVIC_EnableIRQ(EIC_IRQn);
    PM_REGS->PM_APBAMASK |= (1 << 6); // Enable clock for external interrupt controller EIC. Default. 
    // EIC config[0] --> interrupts [0,7]. EIC config[1] --> interrupts [8,15]
    EIC_REGS->EIC_CONFIG[0] |= (0x4 << 0); // pin PA00 is on the EXTINT[0] interrupt request line (shared with PA17)
    EIC_REGS->EIC_CONFIG[0] |= (0x4 << 4); // pin PA01 is on the EXTINT[1] interrupt request line (shared with PA18)
    EIC_REGS->EIC_CONFIG[0] |= (0x4 << 8); // pin PA02 is on the EXTINT[2] interrupt request line (shared with PA19)
    EIC_REGS->EIC_CONFIG[0] |= (0x4 << 12); //pin PA03 is on the EXTINT[3] interrupt request line (shared with PA20)
    EIC_REGS->EIC_INTENSET = keypad_col0 | keypad_col1 | keypad_col2 | keypad_col3; // Enable interrupts from pins.
    EIC_REGS->EIC_CTRL |= (1 << 1); // Enable the EIC. MUST BE LAST!
}

// *************************************************************************************
//                               LCD Functions
// *************************************************************************************

// Some of these timings are faster than suggested, while others are slower.
// Wire lengths, temperature, ATSAMD soldering contact integrity could 
// necessitate the need for different delays. Increase delays if something breaks.

// Data should already be present on bus pins D4-D7
// this merely toggles the enable capture sequence for the display to register values
void captureNybble() {
    // Manual pg 7 says only need 1200ns enable cycle time, but that's totally incorrect. 
    // Might be a unit typo in the manual. < 1400 microseconds is unreliable. 
    digitalWrite(DISP_E, HIGH);
    delay_micros(700); 
    digitalWrite(DISP_E, LOW);
    delay_micros(700);
}

// Sends an 8 bit command to the LCD
// we operate in 4 bit mode. The most significant bits are sent first. Then least significant.
void sendCommand(char hex_cmd) {
    digitalWrite(DISP_RS, LOW); // Set for Command. Manual sets after first write, but timing sheet shows this can go wherever...
    // Place upper 4 bits on port lines
    digitalWrite(DISP_D4, hex_cmd & (1 << 4));
    digitalWrite(DISP_D5, hex_cmd & (1 << 5));
    digitalWrite(DISP_D6, hex_cmd & (1 << 6));
    digitalWrite(DISP_D7, hex_cmd & (1 << 7));
    captureNybble();
    // Place lower 4 bits on port lines
    digitalWrite(DISP_D4, hex_cmd & (1 << 0));
    digitalWrite(DISP_D5, hex_cmd & (1 << 1));
    digitalWrite(DISP_D6, hex_cmd & (1 << 2));
    digitalWrite(DISP_D7, hex_cmd & (1 << 3));
    captureNybble();
}

// Sends an 8 bit data write to the LCD (char is 8 bits)
// we operate in 4 bit mode. The most significant bits are sent first. Then least significant.
void hexWrite(char hex_data) {
    digitalWrite(DISP_RS, HIGH); // Set for Data. Manual sets after first write, but timing sheet shows this can go wherever...
    // Place most significant 4 bits hex value on port lines
    digitalWrite(DISP_D4, hex_data & (1 << 4));
    digitalWrite(DISP_D5, hex_data & (1 << 5));
    digitalWrite(DISP_D6, hex_data & (1 << 6));
    digitalWrite(DISP_D7, hex_data & (1 << 7));
    captureNybble();
    // Place least significant 4 bits hex value on port lines
    digitalWrite(DISP_D4, hex_data & (1 << 0));
    digitalWrite(DISP_D5, hex_data & (1 << 1));
    digitalWrite(DISP_D6, hex_data & (1 << 2));
    digitalWrite(DISP_D7, hex_data & (1 << 3));
    captureNybble();
}

// Initializes the LCD in 4-bit mode. Call once on startup. Default LCD mode is 8-bit
void initLCD_4Bit() {
    delay_millis(40); // Wait > 40ms after power is applied. Seems we can go lower but it is pretty explicit.

    //Write 0x30 == 0011 0000 on LCD data lines 0-7, and allow for percolation delay
    digitalWrite(DISP_D4, HIGH);
    digitalWrite(DISP_D5, HIGH);
    // 3 wake commands (above) must be read in succession, with minimum 160us delay between.
    captureNybble(); // send wake command 1st time
    captureNybble(); // send wake command 2nd time
    captureNybble(); // send wake command 3rd time

    // Write 0x20 == 0010 0000 on LCD data lines 0-7
    digitalWrite(DISP_D4, LOW); // we just need to turn off the 4th pin from previous capture...
    captureNybble();

    sendCommand(0x28); // set 4-bit mode on TWO lines ("2"x16 LCD)
    sendCommand(0x10); // Set Cursor
    sendCommand(0x0C); // Display ON, no cursor nor cursor blink. (0x0F) cursor on and blink
    sendCommand(0x06); // Entry mode
    sendCommand(0x01); // Clear display!
}

void clearDisplay() {
    sendCommand(0x01);
}

// "Built-in Font Table," with 2^8 entries, **MOSTLY** follows ASCII.
// This will work for all English letter characters and some punctuation.
void writeString(const char* enteredString){
    const char* letter = enteredString;
    while(*letter != '\0'){
        hexWrite(*letter);
        letter++;
    }
}

// DDRAM hex addresses for 2x16 LCD:
// 0x00 --> 0F first row
// 0x40 --> 4F second row
void setCursor(int row, int cursorPos){
    char hexDDRAM = 0b10000000; // [6-0] 2^7 bits are used for cursorPos
    hexDDRAM += cursorPos;
    if (row == 1){
        hexDDRAM += 0x40;
    }
    sendCommand(hexDDRAM);
}

// *************************************************************************************
//                               Elevator Logic
// *************************************************************************************

// Called whenever the elevator needs to move to a different floor (end floor)
void moveElevator(int endFloor) {

    // redLED on
    digitalWrite(redLED, HIGH);
    elevatorState = elevator_moving;
    clearDisplay();
    writeString("Floor: ");
    hexWrite(0x30 + elevatorFloor); //0x30 is floor 0
    setCursor(1, 0);
    writeString("Moving...");
    //delay_millis(500);
    if (endFloor == elevatorFloor) {
        delay_millis(2000);
        clearDisplay();
        writeString("Floor: ");
        hexWrite(0x30 + elevatorFloor); //0x30 is floor 0
        digitalWrite(redLED, LOW);
        return;
    }
    while (elevatorFloor > endFloor) {
        clearDisplay();
        writeString("Floor: ");
        hexWrite(0x30 + elevatorFloor); //0x30 is floor 0
        //Moving Flashes for 2 secs per floor
        setCursor(1, 0);
        writeString("Moving...");
        delay_millis(500);
        setCursor(1, 0);
        writeString("         ");
        delay_millis(500);
        setCursor(1, 0);
        writeString("Moving...");
        delay_millis(500);
        setCursor(1, 0);
        writeString("         ");
        delay_millis(500);
        
        elevatorFloor--;

    }
    while (elevatorFloor < endFloor) {
        clearDisplay();
        writeString("Floor: ");
        hexWrite(0x30 + elevatorFloor); //0x30 is floor 0
        //Moving Flashes for 2 secs per floor
        setCursor(1, 0);
        writeString("Moving...");
        delay_millis(500);
        setCursor(1, 0);
        writeString("         ");
        delay_millis(500);
        setCursor(1, 0);
        writeString("Moving...");
        delay_millis(500);
        setCursor(1, 0);
        writeString("         ");
        delay_millis(500);
        
        elevatorFloor++;
    }

    clearDisplay();
    writeString("Floor: ");
    hexWrite(0x30 + elevatorFloor); //0x30 is floor 0
    digitalWrite(redLED, LOW);
    //redLED off
    return;
}

bool isDigit(char c) {
    return (c >= '0' && c <= '9');
}

void EIC_Handler() {
    int cols = (PORT_REGS->GROUP[0].PORT_IN & (0b1111)); // Read in the last four digits of the input (the 4 inputs we are using)
    // turn the binary back into an int we can use to index the 2d array
    if (cols == 0b1000) {
        col = 3;
    } else if (cols == 0b0100) {
        col = 2;
    } else if (cols == 0b0010) {
        col = 1;
    } else if (cols == 0b0001) {
        col = 0;
    }
    // Get the key from the 2d array
    key = keypadMapping[row][col];
    
    // Get elevator state to see what to do with user input    
    if (elevatorState == elevator_ready) {
        // if key is int
        if (isDigit(key)) {
            // if elevator floor == user floor 
            if (elevatorFloor == userFloor) {
                doorState = door_open;

                // Turn the green LED on for 2 sec
                digitalWrite(greenLED, HIGH);
                delay_millis(2000);
                digitalWrite(greenLED, LOW);

                // change elevator state to 1 (sending state)
                elevatorState = elevator_boarded;
            } else if (elevatorFloor != userFloor) {
                moveElevator(((int) key) - 48);
                doorState = door_open;

                // Turn the green LED on for 2 sec
                setCursor(1,0);
                writeString("Summoned!");
                digitalWrite(greenLED, HIGH);
                delay_millis(2000);
                digitalWrite(greenLED, LOW);
                setCursor(1,0);
                writeString("Select Direction");

                elevatorState = elevator_boarded;
            }
        }
    }
    else if (elevatorState == elevator_boarded) {
        setCursor(0,12);
        writeString("                ");
        if (key == 'A') {
            dir = moving_up;
            setCursor(0,13);
            writeString("UP");
            setCursor(1,0);
            writeString("                ");
            setCursor(1,0);
            writeString("Pick a Floor");
        } else if (key == 'B') {
            dir = moving_down;
            setCursor(0,13);
            writeString("DN");
            setCursor(1,0);
            writeString("                ");
            setCursor(1,0);
            writeString("Pick a Floor");
        } else if (key == 'C') {
            doorState = door_closed;
            moveElevator(targetFloor);
            doorState = door_open;
            elevatorState = elevator_ready;
            setCursor(1, 0);
            writeString("Arrived!");
            for (int i = 0; i < 4; i++) {
                digitalWrite(greenLED, HIGH);
                delay_millis(250);
                digitalWrite(greenLED, LOW);
                delay_millis(250);
            }
            setCursor(1, 0);
            writeString("Ready to Summon!");
        } else if (key == 'D') {
            doorState = door_open;
            if (startTime == -1) {
                startTime = TC4_REGS->COUNT32.TC_COUNT;
            }
            int currTime = TC4_REGS->COUNT32.TC_COUNT;
            // if elapsed time since hold is greater than 10 seconds (in 125 nanoseconds)
            if ((currTime - startTime) > 80000000) { 
                setCursor(1,0);
                writeString("                ");
                setCursor(1,0);
                writeString("Door Closing...");
                digitalWrite(buzzer, HIGH);
                digitalWrite(yellowLED, HIGH);
                delay_millis(2000);
                digitalWrite(buzzer, LOW);
                digitalWrite(yellowLED, LOW);

                doorState = door_closed;
                if (targetFloor == -1) {
                    moveElevator(elevatorFloor);
                    elevatorState = elevator_boarded;
                    startTime = -1;
                }
                else {
                    moveElevator(targetFloor);
                    elevatorState = elevator_ready;
                    startTime = -1;
                    targetFloor = -1;
                }
                setCursor(1, 0);
                writeString("Arrived!");
                for (int i = 0; i < 4; i++) {
                    digitalWrite(greenLED, HIGH);
                    delay_millis(250);
                    digitalWrite(greenLED, LOW);
                    delay_millis(250);
                }
                setCursor(1, 0);
                writeString("Ready to Summon!");
            }
        } else if (isDigit(key)) {
            setCursor(1,0);
            writeString("                ");
            setCursor(1,0);
            writeString("--> Close Door");
                targetFloor = ((int) key) - 48;
        }
    }
    // if elevatorState == elevator_moving --> do nothing
    
    // reset handler flags PA 0-3
    EIC_REGS->EIC_INTFLAG |= 0b1111;
}

int main(void) {
    configurePorts();
    configureClock();
    configureTC4();
    initLCD_4Bit();
    configureEIC();

    // BELOW is just LCD Testing.
    // Letter codes on page 9 of display reference
    // Send A --> 0100 0001 --> HEX: 0x46
    // Print A through G
//    hexWrite(0x41);
//    hexWrite(0x42);
//    hexWrite(0x43);
//    hexWrite(0x44);
//    hexWrite(0x45);
//    hexWrite(0x46);
//    hexWrite(0x47);
//
//    delay_millis(1000);
//    clearDisplay();
//    delay_millis(1000);
    
//    writeString("Hidy HO!");
//    setCursor(1, 0);
//    writeString("Don't ya know?");
//    delay_millis(1000);
    // Print 0 - 6
//    hexWrite(0x30);
//    hexWrite(0x31);
//    hexWrite(0x32);
//    hexWrite(0x33);
//    hexWrite(0x34);
//    hexWrite(0x35);
//    hexWrite(0x36);
//    delay_millis(1000);
    // END LCD testing.
    
    writeString("Virtual Elevator");
    delay_millis(1500);

    PORT_REGS->GROUP[0].PORT_OUTCLR = keypad_row0 | keypad_row1 | keypad_row2 | keypad_row3; // Drives all rows to Low
    clearDisplay();
    writeString("Floor: ");
    hexWrite(0x30 + elevatorFloor); //0x30 is floor 0
    setCursor(1, 0);
    writeString("Ready to Summon!");
    while (1) {
        
        startTime = -1;
        delay_millis(100); 
        PORT_REGS->GROUP[0].PORT_OUTTGL = DELAY_LED; // Periodic flash when outside handler to ensure sanity

        // Cycle through the rows allowing for only 1 to be high at one time.
        PORT_REGS->GROUP[0].PORT_OUTCLR = keypad_row3; // Drives Row 3 to Low
        PORT_REGS->GROUP[0].PORT_OUTSET = keypad_row0; // Turn on row 1
        row = 0;
                delay_millis(1); //Short delay to allow for propagation

        PORT_REGS->GROUP[0].PORT_OUTCLR = keypad_row0; // Drives Row 0 to Low
        PORT_REGS->GROUP[0].PORT_OUTSET = keypad_row1; // Turn on row 1
        row = 1;
                delay_millis(1); //Short delay to allow for propagation

        PORT_REGS->GROUP[0].PORT_OUTCLR = keypad_row1; // Drives Row 1 to Low
        PORT_REGS->GROUP[0].PORT_OUTSET = keypad_row2; // Turn on row 2
        row = 2;
                delay_millis(1); //Short delay to allow for propagation

        PORT_REGS->GROUP[0].PORT_OUTCLR = keypad_row2; // Drives Row 2 to Low
        PORT_REGS->GROUP[0].PORT_OUTSET = keypad_row3; // Turn on row 3
        row = 3;
                delay_millis(1); //Short delay to allow for propagation
    }
    return 0;
}