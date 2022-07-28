#include <SPI.h>

// Added integral term to the CPT lock 
// Added d-part and serial interface for pid parameter input to the CPT lock part
// Adc 1V
// lock step
// Added D part to CPT lock
// new Laser Lock ONE put into ErrorDisplay ()
// Version 2.0 added DQI prefixes to interface
// Version after tidying up and adding the RF suppressor
// FSR ADC 256 mV
// Test of error signal with laser TEC modulation
// FM dev 1.67kHz

/* Updated by Carlos RIVERA, july 2022:
- Added visual controls to operate the demonstrator without console commands.
- Converted all machine units in the GUI (graph and controls) to physical units.
- Completely redesigned the "Control panel".
- Added the option to change the laser modulation width.
- Added the option to change the Quartz scan parameters (initial and final values).
- Added double function buttons to start/processes without relying on the physical button.
- Added Absorption Signal graph to monitor the output of the photodiode and not use an oscilloscope.
- Added titles and axis labels to all the graphs.
- Added "Maximum Absorption" graphic tool.
- Added servo loops lock state markers for Laser and Quartz graphs
- Added CPT signal graph
*/

const int CS_DAC_quartz = 2;                              // Set CS pins
const int CS_ADF4158 = 3;                                 
const int TXDATA = 4;
const int CS_VCSEL = 5;
const int TEC_on_off = 6;
const int CS_DAC_TEC = 7;
const int CS_mag = 8;
const int ButtonPin = 9;
const int ADC_DRDY = 11;
const int CS_ADC = 12;
const int Lock_trigger = 61;    //  CR: analog pin A7, apparently not connected to anything, only oscilloscope trigger purpose.

int laser_lock_step = 1;         // CR: Laser lock step
int lock_step_cpt = 1;           // CR: Quartz lock step
const int averages = 1;          // Number of averages before each laser lock step
int init_VCSEL_current = 30000;  // Laser current word after initial startup ->44564 (1.7 mA)
int quartz_init = 10000;   // Initial quartz dac word -1500 avec synthese j
int quartz_offset = 0;
float kp = 0.4;
float ki = 0.01;
float kd = 0;
float kpl = 0.7;
unsigned int scanStart = 24000;  //---------------------
unsigned int scanEnd;
unsigned int scanSamples = 300;

unsigned int scanQuartzStart=0;
unsigned int scanQuartzEnd = 41530;

unsigned int attenuator_word = 0;
unsigned int TEC_WORD = 8829;
unsigned int SweepModulationWidth = 1;
int RF_step = 30;

void setup() {
  pinMode(CS_ADF4158, OUTPUT);
  pinMode(Lock_trigger, OUTPUT);
  pinMode(TXDATA, OUTPUT);
  pinMode(CS_DAC_quartz, OUTPUT);
  pinMode(CS_DAC_TEC, OUTPUT);
  pinMode(TEC_on_off, OUTPUT);
  pinMode(CS_mag, OUTPUT);
  pinMode(CS_VCSEL, OUTPUT);
  pinMode(CS_ADC, OUTPUT);
  pinMode(ButtonPin, INPUT);
  pinMode(ADC_DRDY, INPUT);
  digitalWrite(TEC_on_off, LOW);                          // Sets CS pins high and others low at startup
  digitalWrite(CS_ADF4158, HIGH);
  digitalWrite(CS_DAC_quartz, HIGH);
  digitalWrite(CS_DAC_TEC, HIGH);
  digitalWrite(CS_mag, HIGH);
  digitalWrite(CS_VCSEL, HIGH);
  digitalWrite(CS_ADC, HIGH);
  digitalWrite(Lock_trigger, LOW);
  digitalWrite(TXDATA, LOW);
  
  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV128);                // Lower the SPI transmision speed in case of long signal wires
  Serial.begin(115200);  
  print_title();
  print_menu();
  //analogWriteResolution(12);   // ???????? fonction pas défini
  //analogWrite(DAC1, attenuator_word);  // ???????? fonction pas défini

  // Set the magnetic field current and Quartz VCO at 0 by default:
  Show_CPT();
  return;
    //-------------initial value vcsel--------------erase this block if problem
   byte byte1 = (init_VCSEL_current >> 8);
   byte byte2 = (init_VCSEL_current & 0xff);
   digitalWrite(CS_VCSEL, LOW);
   SPI.transfer(byte1);                                 
   SPI.transfer(byte2);
   digitalWrite(CS_VCSEL, HIGH);
  //-------------------------------------------------
}

void loop(){                                              // Main loop
  while (!Serial.available());
  parseMenu(Serial.read());
}


/* -------------------------------------------------------- FUNCTION DECLARATIONS ----------------------------------------------- */
void parseMenu(char c){                                  // Switches between different menu options
  switch (c){
    case '0':
      ErrorDisplay();
      break;
    
    case '1':
      DAC_load(CS_DAC_quartz);
      break;
    
    case '2':
      Show_CPT();
      break;
    
    case '3':
      LaserLock();
      break;
      
    case '4':
      ADF4158_Reg_Write();
      break;
    
    case '5':
      TEC_DAC_load();
      break;
      
    case '6':
      TEC_on();
      break;
    
    case '7':
      TEC_off();
      break;

    case '8':
      DAC_load(CS_mag);
      break;

    case '9':
      DAC_load(CS_VCSEL);
      break;

    case 'a':
      setPidParams();
      break;

    case 'b':
      QuartzScan(scanQuartzStart,scanQuartzEnd);
      break;

    case 'c':
      set_synth_CPT();
      break;
      
    case 'i':
      setLaserLockParameters();
      break;

    case 'j':
      setQuartzLockParameters();
      break;

    case 'd':
      setAttenuatorWord();
      break;

    case 'l':
      NewLaserLock();
      break;

    case 'u':
      setLockStep();
      break;

    case 's':
      LaserScan(scanStart,scanEnd,scanSamples);
      break;

    case 't':
      LaserScanSetStart();
      break;

    case 'v':
      QuartzScanSetStart();
      break;
    
    case 'r':
      RFPowerScan();
      break;

    case 'z':
      SetTecWord();
      break;

    case 'w':
      SetSweepModulationWidth();
      break;
  }
}


void print_title() {                                     // Prints title        
  printDebug("***************************************************");
  printDebug("* Program Rev. 1.7 MAC demonstrator\t\t\t*");
  printDebug("* FEMTO-ST 2016-2022\t\t\t\t*");
  printDebug("*\t\t\t\t\t*");
  printDebug("* Software ver. 1.7  // July 2022\t\t\t*");
  printDebug("* Author: J. Rutkowski (v1.6 Benoit Dubois) (v1.7 Carlos RIVERA)\t*");
  printDebug("***************************************************");
  // printDebug("");
}


void print_menu() {                                      // Prints main menu.  
  return; //CR: Console is no longer used, only GUI controls
  printDebug("Menu:");
  printDebug("  0 - VCSEL startup and set to 1,7 mA");
  printDebug("  1 - [space] [decimal value] - Set quartz bias"); //Set PLL to 4.6 GHz");
  printDebug("  2 - Set values to show CPT signal");
  printDebug("  3 - Laser Lock ON ");                     
  printDebug("  4 [space] [decimal value] - Set individual PLL register");
  printDebug("  5 [space] [decimal value] - Set laser TEC setpoint");
  printDebug("  6 - TEC ON");
  printDebug("  7 - TEC OFF");
  printDebug("  8 - [space] [decimal value] - Set Mag field");
  printDebug("  9 - [space] [decimal value] - Set VCSEL current");
  printDebug("");
}

//case '1':
void DAC_load(int CS_pin) {                             // Loads data to LTC1655L 16-bit DAC through SPI, takes the CS pin as input
  unsigned int value = Serial.parseInt();               // Parses an int from the serial input
  byte byte1 = (value >> 8);   // byte1 (MSb) = 0x xxxx xxxx 0000 0000
  byte byte2 = (value & 0xff); // byte2 (LSb) = 0x 0000 0000 xxxx xxxx
  digitalWrite(CS_pin, LOW);
  SPI.transfer(byte1);                                  
  SPI.transfer(byte2);
  digitalWrite(CS_pin, HIGH);  // The DAC register loads the data from the shift register when CS/LD is pulled high.
  Serial.print("D DAC word: ");
  Serial.print(byte1, BIN);
  Serial.println(byte2, BIN);
}

void DAC_load(int CS_pin, unsigned int value) {          // Loads data to LTC1655L 16-bit DAC through SPI, takes the CS pin and value to load (uint) as input
   byte byte1 = (value >> 8);   
   byte byte2 = (value & 0xff); 
   digitalWrite(CS_pin, LOW);
   SPI.transfer(byte1);                                 
   SPI.transfer(byte2);
   digitalWrite(CS_pin, HIGH);
}

void TEC_DAC_load() {                                    // Loads data to LTC1658 14-bit DAC (laser TEC) through SPI
  unsigned int value = Serial.parseInt();                // Parses an int from the serial input
  value = value << 2;                                    // Converts a 32-bit int to 14 bit
  byte byte1 = (value >> 8);
  byte byte2 = (value & 0xff);
  digitalWrite(CS_DAC_TEC, LOW);
  SPI.transfer(byte1);
  SPI.transfer(byte2);
  digitalWrite(CS_DAC_TEC, HIGH);
  Serial.print("D DAC word: ");
  Serial.print(byte1, BIN);
  Serial.println(byte2, BIN);
}

void TEC_DAC_load(unsigned int value) {                  // Loads data to LTC1658 14-bit DAC (laser TEC) through SPI, takes Unsigned int value to write
  value = value << 2;  
  byte byte1 = (value >> 8);
  byte byte2 = (value & 0xff);
  digitalWrite(CS_DAC_TEC, LOW);
  SPI.transfer(byte1);
  SPI.transfer(byte2);
  digitalWrite(CS_DAC_TEC, HIGH);
  Serial.print("D DAC word: ");
  Serial.print(byte1, BIN);
  Serial.println(byte2, BIN);
}


void ADF4158_Reg_Write() {                               // Writes an arbitrary PLL register
  unsigned int value1 = Serial.parseInt();               // Parses an int from the serial input
  digitalWrite(CS_ADF4158, LOW);
  delayMicroseconds(1);
  SPI.transfer(((value1 >> 24)& 0xff), SPI_CONTINUE);    // Splits the 32-bit int into 4 bytes
  SPI.transfer(((value1 >> 16)& 0xff), SPI_CONTINUE);
  SPI.transfer(((value1 >> 8)& 0xff), SPI_CONTINUE);
  SPI.transfer((value1& 0xff), SPI_CONTINUE);
  digitalWrite(CS_ADF4158, HIGH);
}


void ADF4158_Write(int value1) {                         // Writes one PLL register, takes int value as input
  digitalWrite(CS_ADF4158, LOW);
  delayMicroseconds(1);
  SPI.transfer(((value1 >> 24)& 0xff), SPI_CONTINUE);    // Splits the 32-bit int into 4 bytes
  SPI.transfer(((value1 >> 16)& 0xff), SPI_CONTINUE);
  SPI.transfer(((value1 >> 8)& 0xff), SPI_CONTINUE);
  SPI.transfer((value1& 0xff), SPI_CONTINUE);
  digitalWrite(CS_ADF4158, HIGH);
}

// CR: I found this function sets the RF synthesizer to 4.596301446 GHz exactly with a 10 MHz reference, with ramp mode activated.
void ADF4158_Set_CPT() {        // Sets PLL values to show CPT signal
  ADF4158_Write(0x7);           // R7: Delay disabled
  ADF4158_Write(0x800006);      // R6 - Ramp 2: Setting the number of steps to 0.
  ADF4158_Write(0x3E86);        // R6 - Ramp 1: Setting the number of steps to 2 000.
  ADF4158_Write(0x800005);      // R5 - Ramp 2: Disable the deviation.
  ADF4158_Write(0x1002A5);      // R5 - Ramp 1: Setting the deviation to "84" and offset to "2".
  ADF4158_Write(0x783204);      // R4: Set the clock divider mode to "ramp divider" and the divider value to 100.
  ADF4158_Write(0x43);          // R3: Set the ramp to continuous sawtooth.
  ADF4158_Write(0x40800A);      // R2: i charge pump (cp) set to 0.35mA /cyrus (CR: Set current setting to 0.31, r-counter divide ratio to 1 and clock divider to 1.)
  ADF4158_Write(0x1280001);     // R1: PRESCEDENT FFF8001 (CR: This register sets the LSb of the FRAC value)
  ADF4158_Write(0xF8E5D0A8);    // R0: pll setting= Muxout ramp Status on 0xF8E5D0A8 (CR: Set the MSb of the FRAC value and INT value)
}
// From the comments the "precedent" values were:
// R1: 0xFFF8001
// R0: 0xF8E5D0A8 (no change)
// This would set the frequency to 4.5963037106 GHz

// CR: I found this function sets the RF synthesizer to 4.5963733 GHz exactly with a 10 MHz reference, without ramp mode activated.
void ADF4158_Set_CPT_lock(){
  ADF4158_Write(0x7);         // R7: Delay disabled
  ADF4158_Write(0x800006);    // R6 - Ramp 2: Setting the number of steps to 0.
  ADF4158_Write(0x6);         // R6 - Ramp 1: Setting the number of steps to 0.
  ADF4158_Write(0x800005);    // R5 - Ramp 2: Disable the deviation.
  ADF4158_Write(0x200695);    // R5 - Ramp 1: Setting the deviation to "210" and offset to "4".
  ADF4158_Write(0x180104);    // R4: Set the clock divider mode to "ramp divider" and the divider value to 2.
  ADF4158_Write(0x143);       // R3: Set the ramp to continuous sawtooth (FSK enabled).
  ADF4158_Write(0x40800A);    // R2: i charge pump (cp) set to 0.35mA /cyrus (CR: Set current setting to 0.31, r-counter divide ratio to 1 and clock divider to 1.)
  ADF4158_Write(0x80F0001);   //OxDDA8001 ces 2 registres**  fonctionne bien avec la synthese J (CR: )
  ADF4158_Write(0xE5D190);    //0xE5D128 ne pas oublier ce registre** MSB FRAC (CR: Set MUXOUT CONTROL to "THREE-STATE OUTPUT" and the MSb of the FRAC value and INT value)
}
// From the comments the another option is found:
// R1: 0xDDA8001
// R0: 0xE5D128
// This would set the frequency to 4.5963424459 GHz


// case 'c':
void set_synth_CPT(){
  DAC_load(CS_DAC_quartz, quartz_init);
  ADF4158_Set_CPT_lock();
  digitalWrite(Lock_trigger, LOW);
  digitalWrite(TXDATA, LOW);
}

void TEC_on() {                                          // Turns LTC1923 on by pulling ~SDCYNC (pin 3) high
  digitalWrite(TEC_on_off,HIGH);
  // printDebug("TEC ON");
}


void TEC_off() {                                         // Turns LTC1923 off by pulling ~SDCYNC (pin 3) low
  digitalWrite(TEC_on_off,LOW);
  // printDebug("TEC OFF");  
}


void toggle_TXDATA(){                                    // Toggles ADF4158 TXDATA pin
  digitalWrite(TXDATA,(!digitalRead(TXDATA))); //pin 4 of Arduino
  // This pin is connected to "txdata" of the synthesizer 
  // which makes it operate Frequency Shift Keying (FSK) mode:
  // Logic 0 is represented by a wave at a specific frequency, 
  // and logic 1 is represented by a wave at a different frequency. 
  // The distance between logic 0 and logic 1 is known as the deviation or shift point.
  // The central frequency is the current Quartz frequency defined by "CS_DAC_quartz"
  // and the modulation to calculate the error signal is 1 kHz around it.
  // Page: 26
}


void toggle_Trigger(){
  digitalWrite(Lock_trigger,(!digitalRead(Lock_trigger))); //pin 61 = A7 (Analog)
}


void VCSEL_startup(){                                    // VCSEL slow-start, slowly ramps up the VCSEL current
  for(unsigned int i = 0; i < 44564; i += 100){
      DAC_load(CS_VCSEL, i);
      delay(2);
  }
  // printDebug("Startup terminated, VCSEL at I ~ 1,7 mA");
}

// case '2':
void Show_CPT(){
  DAC_load(CS_DAC_quartz, 0); //quartz_init
  VCSEL_startup();                                      // Ramp-up VCSEL current
  TEC_DAC_load(TEC_WORD);                               // Set VCSEL TEC Word
  TEC_on();
  ADF4158_Set_CPT_lock();
  DAC_load(CS_mag, 0);                              // Sets magnetic field word to 0
}

int ADC_read(){                                          // Reads the ADC value, returns a 16-bit int (two's complement!)
  SPI.setDataMode(SPI_MODE1);
  digitalWrite(CS_ADC, LOW);
  while(digitalRead(ADC_DRDY) == HIGH){}
  byte inbyte1 = SPI.transfer(0x00);
  byte inbyte2 = SPI.transfer(0x00);
  digitalWrite(CS_ADC, HIGH);
  int val = ((inbyte1<<8) | inbyte2);
  SPI.setDataMode(SPI_MODE0);
  return val;
}


void ADC_startup() {                                     // Starts ADS1118
  SPI.setDataMode(SPI_MODE1);
  digitalWrite(CS_ADC, LOW);
  SPI.transfer(B00000110);                               // ADC FSR
  SPI.transfer(B11100011);
  digitalWrite(CS_ADC, HIGH);
  SPI.setDataMode(SPI_MODE0);
}
// case 'a':
void setPidParams(){
  kp = Serial.parseFloat();
  ki = Serial.parseFloat();
  kd = Serial.parseFloat();
  kpl = Serial.parseFloat();
  printDebug("Quartz PID parameters:");
  printDebug(kp);
  printDebug(ki);
  printDebug(kd);
  printDebug("Laser PID parameters:");
  printDebug(kpl);
}
//case 'u':
void setLockStep(){
  RF_step = Serial.parseInt();
  printDebug(RF_step);

}
// case 'd':
void setAttenuatorWord(){
  attenuator_word = Serial.parseInt();
  printDebug((int)(attenuator_word));
  analogWrite(DAC1, attenuator_word);
}
//case 'i':
void setLaserLockParameters(){
  init_VCSEL_current = Serial.parseFloat();
  laser_lock_step = Serial.parseFloat();
  printDebug(init_VCSEL_current);
  printDebug(laser_lock_step);
}
//case 'j':
void setQuartzLockParameters(){
  quartz_init = Serial.parseFloat();
  quartz_offset = Serial.parseFloat();
}
//case 't':
void LaserScanSetStart(){
  scanStart=Serial.parseInt();
  scanEnd=Serial.parseInt();
  scanSamples=Serial.parseInt();
}
//case 'v':
void QuartzScanSetStart(){
  scanQuartzStart=Serial.parseInt();
  scanQuartzEnd=Serial.parseInt();  
}
void SetTecWord(){
  TEC_WORD = ((unsigned int)(Serial.parseInt()));
  printDebug((int)(TEC_WORD));
}

void SetSweepModulationWidth(){
  SweepModulationWidth = ((unsigned int)(Serial.parseInt()));
  printDebug((int)(SweepModulationWidth));
}

int limit_value(int new_DAC, int limit){ // Current limit: 44564 or 1.7 mA
  if(new_DAC>limit){
    return limit;
  }else{
    return new_DAC;
  }
}
//case 'l':
void NewLaserLock(){
  ADC_startup();
  int ButtonState = 0;
  ButtonState = digitalRead(ButtonPin);
  printDebug("Lock ON");
  int value1, value2;
  int previous_DAC_val = init_VCSEL_current;
  int Error, New_DAC_val,right_measurement, left_measurement;
  int ADC_limit=44564;
  while(ButtonState == LOW){
    if(Serial.available()){
      if((char)Serial.read()=='x'){
        break;
      }
    }
    digitalWrite(Lock_trigger,(!digitalRead(Lock_trigger)));
    right_measurement=previous_DAC_val + laser_lock_step;
    left_measurement=previous_DAC_val - laser_lock_step;

    DAC_load(CS_VCSEL, limit_value(right_measurement,ADC_limit));
    value1 = ADC_read();
    
    DAC_load(CS_VCSEL, limit_value(left_measurement,ADC_limit));
    value2 = ADC_read();
    
    Error = value1-value2;
    New_DAC_val = (previous_DAC_val - kpl*Error);
    DAC_load(CS_VCSEL, limit_value(New_DAC_val,ADC_limit));
    previous_DAC_val = New_DAC_val;
    ButtonState = digitalRead(ButtonPin);
    Serial.println("I "+String(New_DAC_val));
    if(countCycles(1,50)){
      Serial.println("L "+String(right_measurement));
      Serial.println("M "+String(left_measurement));
    }
  }
  printDebug("Lock OFF");
}

int NewLaserLockONE(int previous_DAC_val){
  int value1, value2;
  int ErrorLas, New_DAC_val;
  int right_measurement, left_measurement;
  int ADC_limit=44564; // 44564 = 1.7 mA (For protection function)

  right_measurement=previous_DAC_val + laser_lock_step;
  left_measurement=previous_DAC_val - laser_lock_step;
  
  DAC_load(CS_VCSEL, limit_value(right_measurement,ADC_limit));
  value1 = ADC_read();
  DAC_load(CS_VCSEL, limit_value(left_measurement,ADC_limit));
  value2 = ADC_read();
  ErrorLas = value1-value2;
  New_DAC_val = (previous_DAC_val - kpl*ErrorLas);
  DAC_load(CS_VCSEL, limit_value(New_DAC_val,ADC_limit));
  Serial.println("I "+String(New_DAC_val));
  if(countCycles(1,50)){
      Serial.println("L "+String(right_measurement));
      Serial.println("M "+String(left_measurement));
    }
  return New_DAC_val;
}

// case '3':
void LaserLock(){
  ADC_startup();
  int ButtonState = 0;
  ButtonState = digitalRead(ButtonPin);       // CR: This line doesn't do anything here
  printDebug("Lock ON");
  int previous_ADC_val = ADC_read();
  int previous_DAC_val = init_VCSEL_current;
  int current_ADC_val;
  int flag = 1;
  unsigned int new_DAC_val;
  int ADC_limit=44564; // 44564 = 1.7 mA (For protection function)
  while(ButtonState == LOW){
    digitalWrite(Lock_trigger,(!digitalRead(Lock_trigger)));
    current_ADC_val = 0;
    for(int i =0; i < averages; i++){
      current_ADC_val += ADC_read();
    }
    current_ADC_val =  current_ADC_val/averages;
    
    if(current_ADC_val < previous_ADC_val){flag = 1;}
    else if(current_ADC_val > previous_ADC_val){flag = -1;}
    new_DAC_val = (unsigned int)(previous_DAC_val + laser_lock_step*flag);
    DAC_load(CS_VCSEL, limit_value(new_DAC_val,ADC_limit));
    previous_DAC_val = new_DAC_val;
    previous_ADC_val = current_ADC_val;
    ButtonState = digitalRead(ButtonPin);
    Serial.println("I "+String(new_DAC_val));
  }
  printDebug("Lock OFF");
}

// case '0':
void ErrorDisplay(){
  ADC_startup();
  DAC_load(CS_DAC_quartz, quartz_init);
  ADF4158_Set_CPT_lock();
  digitalWrite(Lock_trigger, LOW);
  digitalWrite(TXDATA, LOW);

  int ButtonState = 0;
  int ErrorSum = 0;
  ButtonState = digitalRead(ButtonPin);
  int previous_quartz_val = quartz_init;
  int previousError = 0;
  int ErrorDiff = 0;
  int previous_laser_DAC_val = init_VCSEL_current;
  int val1,val2,Error,NewVal;
  printDebug("Lock ON");
  while(ButtonState == LOW){
    if(Serial.available()){
      if((char)Serial.read()=='x'){
        break;
      }
    }
    previous_laser_DAC_val = NewLaserLockONE(previous_laser_DAC_val);
    toggle_Trigger();
    toggle_TXDATA();
    val1 = ADC_read();
    toggle_Trigger();
    toggle_TXDATA();
    val2 = ADC_read();
    Error = val1-val2-quartz_offset;
    ErrorSum += Error;
    ErrorDiff = Error - previousError;
    NewVal = (previous_quartz_val - (kp*Error + ki*ErrorSum + kd*ErrorDiff));
    DAC_load(CS_DAC_quartz, NewVal);
    previous_quartz_val = NewVal;
    previousError = Error;
    Serial.println("Q "+String(NewVal));
    if(countCycles(2,50)){
      Serial.println("N "+String(NewVal));
    }
    ButtonState = digitalRead(ButtonPin);
  }
  printDebug("Lock OFF");
}
// Function to count cycles and avoid excessive serial communication/plotting, returns 1 once the max_cycles threshold is met
int cycle[]={0,0};
int countCycles(int i,int max_cycles){
  if (cycle[i]<max_cycles){
      cycle[i]++;
      return 0;
    }else{
      cycle[i]=0;
      return 1;
  }
}
//
void LaserScan(unsigned int startVal,unsigned int endVal,unsigned int nSamples){
  ADC_startup();
  digitalWrite(Lock_trigger, LOW);
  int ButtonState = 0;
  ButtonState = digitalRead(ButtonPin);
  int sample_range=endVal-startVal;
  int cycles_per_sample = 1; // aprox: 1 second refresh rate
  int enable_measurement=0;
  int ADC_limit=44564; // 44564 = 1.7 mA (For protection function)

  printDebug("Scan ON");
  while(ButtonState == LOW){
    if(Serial.available()){
      char received=Serial.read();
      if(received=='x'){
        break;
      }
    }
    int old_i = 0;
    //Set initial value and wait for the signal to stabilize, provides better plot
    DAC_load(CS_VCSEL, limit_value(startVal,ADC_limit));
    delayMicroseconds(3000);  
    for(unsigned int i = startVal; i < endVal; i += 1){ 
      DAC_load(CS_VCSEL, limit_value(i,ADC_limit)); // This takes 13.58 us
      if (i-old_i>sample_range/nSamples && enable_measurement){
        Serial.println("A "+String(i)+" "+String(ADC_read())); // This line takes 95 us
        old_i = i;        
      }else{
        delayMicroseconds(95);  
      }
    }
    enable_measurement=countCycles(1,cycles_per_sample);
    ButtonState = digitalRead(ButtonPin); 
  }
  printDebug("Scan OFF");
}

// case 'b':
void QuartzScan(unsigned int startVal,unsigned int endVal){
  ADC_startup();
  DAC_load(CS_DAC_quartz, 0);
  digitalWrite(Lock_trigger, LOW);
  digitalWrite(TXDATA, LOW);
  printDebug("Quartz Scan ON");
  for(unsigned int i = startVal; i < endVal; i += 10){ // previous hardcoded values: 0 to 41530, step 10
    if(Serial.available()){
      if((char)Serial.read()=='x'){
        break;
      }
    }
    DAC_load(CS_DAC_quartz, i);
    toggle_TXDATA(); // 0 ms
    int val1 = ADC_read();
    toggle_TXDATA(); // 1.03 ms, complete cycle: 420 Hz
    int val2 = ADC_read();
    int Error = val1-val2;
    Serial.println("E "+String(i)+" "+String(Error)); // Stream error value
    Serial.println("C "+String(i)+" "+String(val1));  // Stream cpt signal value
  }
  Serial.println("x"); // To indicate GUI the error measurement is complete
  printDebug("Quartz Scan OFF");
  DAC_load(CS_DAC_quartz, 0); // Set quartz DAC back to 0
  // }
}

int average_ADC(int n_averages){
  int current_ADC_val=0;
  for(int i =0; i < n_averages; i++){
      current_ADC_val += ADC_read();
    }
    return  current_ADC_val/n_averages;
}
void printDebug(String message){
  Serial.println("D "+message);
}

void printDebug(int value){
  Serial.println("D "+String(value));
}

void printDebug(float value){
  Serial.println("D "+String(value,3));
}


void RFPowerScan(){ 
  ADF4158_Set_CPT_lock();
  digitalWrite(Lock_trigger, LOW);
  digitalWrite(TXDATA, LOW);
  DAC_load(CS_VCSEL, init_VCSEL_current);
  int previous_laser_DAC_val = init_VCSEL_current;
  int previous_quartz_val = 20400;
  int NewVal=0;
  int Corr = 0;
  
  for(int i = 500; i < 1801; i++){
    analogWrite(DAC1, i);

    TEC_DAC_load(TEC_WORD+SweepModulationWidth);
    for(int j =0; j < 10; j++){
      previous_laser_DAC_val = NewLaserLockONE(previous_laser_DAC_val);
      toggle_Trigger();
      toggle_TXDATA();
      int val1 = ADC_read();
      toggle_Trigger();
      toggle_TXDATA();
      int val2 = ADC_read();
      int Error = val1-val2;
      Corr = kp*Error;
      NewVal = (previous_quartz_val - (kp*Error));
      DAC_load(CS_DAC_quartz, NewVal);
      previous_quartz_val = NewVal;
      Serial.println("Q "+String(NewVal));
    }
    int Error1 = Corr;
    
    TEC_DAC_load(TEC_WORD-SweepModulationWidth);
    for(int j =0; j < 10; j++){
      previous_laser_DAC_val = NewLaserLockONE(previous_laser_DAC_val);
      toggle_Trigger();
      toggle_TXDATA();
      int val1 = ADC_read();
      toggle_Trigger();
      toggle_TXDATA();
      int val2 = ADC_read();
      int Error = val1-val2;
      Corr = kp*Error;
      NewVal = (previous_quartz_val - (kp*Error));
      DAC_load(CS_DAC_quartz, NewVal);
      previous_quartz_val = NewVal;
      Serial.println("Q "+String(NewVal));
    }
    int Error2 = Corr;

    int ErrorTot = Error1-Error2;
    Serial.println("E "+String(i)+" "+String(ErrorTot));
  }
}
