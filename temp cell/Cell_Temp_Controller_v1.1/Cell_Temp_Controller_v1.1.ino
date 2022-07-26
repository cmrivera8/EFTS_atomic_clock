#include <SPI.h>
#include <PID_v1.h>

const int CS_Cell_Temp_ADC = 10;
const int CS_Cell_Temp_DAC = 6;

double Input, Output;
double Kp=1, Ki=1, Kd=0, Setpoint= 28750;// Initial parameters

PID myPID(&Input, &Output, &Setpoint, Kp, Ki, Kd, DIRECT);

void setup() {
  pinMode(CS_Cell_Temp_ADC, OUTPUT);
  pinMode(CS_Cell_Temp_DAC, OUTPUT);
  digitalWrite(CS_Cell_Temp_ADC, HIGH);
  digitalWrite(CS_Cell_Temp_DAC, HIGH);
  Serial.begin(9600);
  SPI.begin();
  myPID.SetMode(AUTOMATIC);
  myPID.SetOutputLimits(0x0, 0x1f40);

  //SPI.setClockDivider(SPI_CLOCK_DIV128); // Lower the SPI transmision speed

}

void loop() {
  Input = Cell_Temp_Read();
  myPID.Compute();
  Serial.println(Input);
 // Serial.println((int) Output);
  Cell_Temp_Write((int) Output);
  delay(100);
}

double Cell_Temp_Read(){
  digitalWrite(CS_Cell_Temp_ADC, LOW);
  unsigned int a = SPI.transfer(0x0);
  unsigned int b = SPI.transfer(0x0);
  digitalWrite(CS_Cell_Temp_ADC, HIGH);
  
  unsigned int adc_value = ((a << 8) | b);
  double Weatstone_V = (((double) adc_value / 32767) - 1) * 1.25;
  
  return (double)(((0.5-(Weatstone_V/1.25))/((0.5+Weatstone_V/1.25)))*10000);
}

void Cell_Temp_Write(int correction){
  byte a = (unsigned int) correction >> 8;
  byte b = (unsigned int) correction & 0xff;
  digitalWrite(CS_Cell_Temp_DAC, LOW);
  SPI.transfer(a);
  SPI.transfer(b);
  digitalWrite(CS_Cell_Temp_DAC, HIGH);
}  

