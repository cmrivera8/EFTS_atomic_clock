// Added integral term to the CPT lock 
// Added d-part and serial interface for pid parameter input to the CPT lock part
// Adc 1V
// lock step
// Added D part to CPT lock
// new Laser Lock ONE wrzucone do ErrorDisplay()
// Wersja 2.0 dodane przedrostki DQI do interfejsu
// Wersja po uporzadkowaniu i dodaniu tlumika RF
// FSR ADC 256 mV
// Test sygnalu bledu przy modulacji TEC lasera
// FM dev 1.67kHz

#include <SPI.h>
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
const int Lock_trigger = 61;

const int lock_step = 1;                                  // Laser lock step
const int averages = 1;                                   // Number of averages before each laser lock step
int init_VCSEL_current = 50798;                     // Laser current word after initial startup
const int quartz_init = 20200;                            // Initial quartz dac word
float kp = 1;
float ki = 1;
float kd = 1;
float kpl = 0.1;
unsigned int scanStart = 45000; 
int lock_step_cpt = 1;
unsigned int attenuator_word = 0;
unsigned int TEC_WORD = 8829;
unsigned int SweepModulationWidth = 1;
int RF_step = 30;

void setup() {
  pinMode(CS_ADF4158, OUTPUT);
  pinMode(61, OUTPUT);
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
 // SPI.setClockDivider(SPI_CLOCK_DIV128);                // Lower the SPI transmision speed in case of long signal wires
  Serial.begin(115200);  
  print_title();
  print_menu();
  analogWriteResolution(12);
  analogWrite(DAC1, attenuator_word);
}

void loop(){                                              // Main loop
  while (!Serial.available());
  parseMenu(Serial.read());
}


/* -------------------------------------------------------- FUNCTION DECLARATIONS ----------------------------------------------- */
void parseMenu(char c){                                  // Switches between different menu options
   switch (c){
     case '0':
       //QuartzScan();
       //LaserScan(scanStart);
       ErrorDisplay();
       //LockLaserCPT();
       //VCSEL_startup();
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
       QuartzScan();
       break;

     case 'c':
       set_synth_CPT();
       break;
       
     case 'i':
       setLockStart();
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
       LaserScan(scanStart);
       break;

     case 't':
       LaserScanSetStart();
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
  printDebug("******************************************************************");
  printDebug("* Diagnostic mode program Rev. 1.4 MAC demonstrator             *");
  printDebug("* FEMTO-ST 2016                                                 *");
  printDebug("*                                                               *");
  printDebug("* Software ver. 1.5  24/10/2016                                 *");
  printDebug("* Author: J. Rutkowski                                          *");
  printDebug("*****************************************************************");
  printDebug("");
}


void print_menu() {                                      // Prints main menu.  
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


 void DAC_load(int CS_pin) {                             // Loads data to LTC1655L DAC through SPI, takes the CS pin as input
   unsigned int value = Serial.parseInt();               // Parses an int from the serial input
   byte byte1 = (value >> 8);
   byte byte2 = (value & 0xff);
   digitalWrite(CS_pin, LOW);
   SPI.transfer(byte1);                                  
   SPI.transfer(byte2);
   digitalWrite(CS_pin, HIGH);
   Serial.print("D DAC word: ");
   Serial.print(byte1, BIN);
   Serial.println(byte2, BIN);
}


void DAC_load(int CS_pin, unsigned int value) {          // Loads data to LTC1655L DAC through SPI, takes the CS pin and value to load (uint) as input
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


void ADF4158_Set_CPT() {                                 // Sets PLL values to show CPT signal
  ADF4158_Write(0x7);
  ADF4158_Write(0x800006);
  ADF4158_Write(0x3E86);
  ADF4158_Write(0x800005);
  ADF4158_Write(0x1002A5);
  ADF4158_Write(0x783204);
  ADF4158_Write(0x43);
  ADF4158_Write(0x40800A);
  ADF4158_Write(0x1280001);
  ADF4158_Write(0xF8E5D0A8);
}


void ADF4158_Set_CPT_lock(){
  ADF4158_Write(0x7);
  ADF4158_Write(0x800006);
  ADF4158_Write(0x6);
  ADF4158_Write(0x800005);
  ADF4158_Write(0x200695);
  ADF4158_Write(0x180104);
  ADF4158_Write(0x143);
  ADF4158_Write(0x40800A);
  ADF4158_Write(0xDDA8001);
  ADF4158_Write(0xE5D128);
}

void set_synth_CPT(){
  DAC_load(CS_DAC_quartz, quartz_init);
  ADF4158_Set_CPT_lock();
  digitalWrite(Lock_trigger, LOW);
  digitalWrite(TXDATA, LOW);
}

void TEC_on() {                                          // Turns LTC1923 on by pulling ~SDCYNC (pin 3) high
  digitalWrite(TEC_on_off,HIGH);
  printDebug("TEC ON");
}


void TEC_off() {                                         // Turns LTC1923 off by pulling ~SDCYNC (pin 3) low
  digitalWrite(TEC_on_off,LOW);
  printDebug("TEC OFF");  
}


void toggle_TXDATA(){                                    // Toggles ADF4158 TXDATA pin
  digitalWrite(TXDATA,(!digitalRead(TXDATA)));
}


void toggle_Trigger(){
  digitalWrite(Lock_trigger,(!digitalRead(Lock_trigger)));
}


void VCSEL_startup(){                                    // VCSEL slow-start, slowly ramps up the VCSEL current
  for(unsigned int i = 0; i < (init_VCSEL_current + 100); i += 100){
      DAC_load(CS_VCSEL, i);
      delay(2);
  }
  printDebug("Startup terminated, VCSEL at I ~ 1,7 mA");
}


void Show_CPT(){
  DAC_load(CS_DAC_quartz, quartz_init);
  VCSEL_startup();                                       // Ramp-up VCSEL current
  TEC_DAC_load(TEC_WORD);                                    // Set VCSEL TEC Word
  TEC_on();
  ADF4158_Set_CPT();
  DAC_load(CS_mag, 10000);                               // Sets magnetic field word to 10000
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
  SPI.transfer(B00001000);                               // ADC FSR
  SPI.transfer(B11100011);
  digitalWrite(CS_ADC, HIGH);
  SPI.setDataMode(SPI_MODE0);
}

void setPidParams(){
  //scanStart = Serial.parseFloat();
  //printDebug(scanStart);
  kp = Serial.parseFloat();
  ki = Serial.parseFloat();
  kd = Serial.parseFloat();
  kpl = Serial.parseFloat();
  printDebug(kp);
  printDebug(ki);
  printDebug(kd);
  printDebug(kpl);
}

void setLockStep(){
  RF_step = Serial.parseInt();
  printDebug(RF_step);

}

void setAttenuatorWord(){
  attenuator_word = Serial.parseInt();
  printDebug((int)(attenuator_word));
  analogWrite(DAC1, attenuator_word);
}

void setLockStart(){
  //scanStart = Serial.parseFloat();
  //printDebug(scanStart);
  init_VCSEL_current = Serial.parseFloat();
  printDebug(init_VCSEL_current);
}

void LaserScanSetStart(){
  scanStart = Serial.parseInt();
  printDebug((int)(scanStart));
}

void SetTecWord(){
  TEC_WORD = ((unsigned int)(Serial.parseInt()));
  printDebug((int)(TEC_WORD));
}

void SetSweepModulationWidth(){
  SweepModulationWidth = ((unsigned int)(Serial.parseInt()));
  printDebug((int)(SweepModulationWidth));
}

void NewLaserLock(){
  ADC_startup();
  int ButtonState = 0;
  ButtonState = digitalRead(ButtonPin);
  printDebug("Lock ON");
  int value1, value2;
  int previous_DAC_val = init_VCSEL_current;
  int Error, New_DAC_val;
  while(ButtonState == LOW){
    digitalWrite(Lock_trigger,(!digitalRead(61)));
    DAC_load(CS_VCSEL, (previous_DAC_val + lock_step));
    value1 = ADC_read();
    DAC_load(CS_VCSEL, (previous_DAC_val - lock_step));
    value2 = ADC_read();
    Error = value1-value2;
    New_DAC_val = (previous_DAC_val - kpl*Error);
    DAC_load(CS_VCSEL, New_DAC_val);
    previous_DAC_val = New_DAC_val;
    ButtonState = digitalRead(ButtonPin);
    Serial.println("I "+String(New_DAC_val));
  }
  printDebug("Lock OFF");
}

int NewLaserLockONE(int previous_DAC_val){
  int value1, value2;
  int ErrorLas, New_DAC_val;
  
  DAC_load(CS_VCSEL, (previous_DAC_val + lock_step_cpt));
  value1 = ADC_read();
  DAC_load(CS_VCSEL, (previous_DAC_val - lock_step_cpt));
  value2 = ADC_read();
  ErrorLas = value1-value2;
  New_DAC_val = (previous_DAC_val - kpl*ErrorLas);
  DAC_load(CS_VCSEL, (unsigned int)(New_DAC_val));
  Serial.println("I "+String(New_DAC_val));
  return New_DAC_val;
}


void LaserLock(){
  ADC_startup();
  int ButtonState = 0;
  ButtonState = digitalRead(ButtonPin);
  printDebug("Lock ON");
  int previous_ADC_val = ADC_read();
  int previous_DAC_val = init_VCSEL_current;
  int current_ADC_val;
  int flag = 1;
  unsigned int new_DAC_val;
  while(ButtonState == LOW){
    digitalWrite(Lock_trigger,(!digitalRead(61)));
    current_ADC_val = 0;
    for(int i =0; i < averages; i++){
      current_ADC_val += ADC_read();
    }
    current_ADC_val =  current_ADC_val/averages;
    
    if(current_ADC_val < previous_ADC_val){flag = 1;}
    else if(current_ADC_val > previous_ADC_val){flag = -1;}
    new_DAC_val = (unsigned int)(previous_DAC_val + lock_step*flag);
    DAC_load(CS_VCSEL, new_DAC_val);
    previous_DAC_val = new_DAC_val;
    previous_ADC_val = current_ADC_val;
    ButtonState = digitalRead(ButtonPin);
    Serial.println("I "+String(new_DAC_val));
  }
  printDebug("Lock OFF");
}


void ErrorDisplay(){ 
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
  
  while(ButtonState == LOW){
    previous_laser_DAC_val = NewLaserLockONE(previous_laser_DAC_val);
    toggle_Trigger();
    toggle_TXDATA();
    int val1 = ADC_read();
    toggle_Trigger();
    toggle_TXDATA();
    int val2 = ADC_read();
    int Error = val1-val2;
    ErrorSum += Error;
    ErrorDiff = Error - previousError;
    int NewVal = (previous_quartz_val - (kp*Error + ki*ErrorSum + kd*ErrorDiff));
    DAC_load(CS_DAC_quartz, NewVal);
    previous_quartz_val = NewVal;
    previousError = Error;
    Serial.println("Q "+String(NewVal));
    ButtonState = digitalRead(ButtonPin);
  }
  
}

void LaserScan(unsigned int startVal){
  digitalWrite(Lock_trigger, LOW);
  int ButtonState = 0;
  ButtonState = digitalRead(ButtonPin);
  while(ButtonState == LOW){
    toggle_Trigger();
    for(unsigned int i = startVal; i < (startVal + 5001); i += 1){
       DAC_load(CS_VCSEL, i);
       delayMicroseconds(10);
    }
    ButtonState = digitalRead(ButtonPin); 
  }
}


void QuartzScan(){
  DAC_load(CS_DAC_quartz, 0);
  ADF4158_Set_CPT_lock();
  digitalWrite(Lock_trigger, LOW);
  digitalWrite(TXDATA, LOW);
  for(unsigned int i = 0; i < 41530; i += 10){
       DAC_load(CS_DAC_quartz, i);
       delayMicroseconds(200);
       toggle_Trigger();
       toggle_TXDATA();
       int val1 = ADC_read();
       toggle_Trigger();
       toggle_TXDATA();
       int val2 = ADC_read();
       int Error = val1-val2;
       Serial.println("E "+String(i)+" "+String(Error));
  }
  
}

void printDebug(String message){
  Serial.println("D "+message);
}

void printDebug(int value){
  Serial.println("D "+String(value));
}

void printDebug(float value){
  Serial.println("D "+String(value));
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

