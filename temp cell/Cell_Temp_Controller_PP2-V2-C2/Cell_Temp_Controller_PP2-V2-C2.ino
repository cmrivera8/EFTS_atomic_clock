#include <SPI.h>
#include <PID_v1.h>

const int CS_Cell_Temp_ADC = 10;
const int CS_Cell_Temp_DAC = 6;

double Input, Output;
// 25, 1, 0.3 PP2-V2-C2 and 25, 3, 0.8 PP1-V1-C1
// double Kp=25, Ki=1, Kd=0.3, Setpoint= 1480;// 75 degree Celsius/R = 1480 OHM SONDE NTC 10K (Jaune)
double Kp=50, Ki=3, Kd=1, Setpoint= 1480; //testing
PID myPID(&Input, &Output, &Setpoint, Kp, Ki, Kd, REVERSE);

void setup() {
  pinMode(CS_Cell_Temp_ADC, OUTPUT);
  pinMode(CS_Cell_Temp_DAC, OUTPUT);
  digitalWrite(CS_Cell_Temp_ADC, HIGH);
  digitalWrite(CS_Cell_Temp_DAC, HIGH);
  Serial.begin(9600);
  SPI.begin();
  myPID.SetMode(AUTOMATIC);
  myPID.SetOutputLimits(0, 32767);

  //SPI.setClockDivider(SPI_CLOCK_DIV128); // Lower the SPI transmision speed

}

void loop() {
  // Change setpoint in real time
  if(Serial.available()){
    double newSetpoint = Serial.parseFloat();
    if(newSetpoint>0){
    Setpoint=newSetpoint;
    Serial.print("New setpoint: ");
    Serial.println(Setpoint);
    }
  }
  Input = Cell_Temp_Read();
  myPID.Compute();
  //Serial.print("Setpoint: ");
  Serial.print(Setpoint);
  //Serial.print(" Current: ");
  Serial.print(" ");
  Serial.println(Input);
  //Serial.print("--OUT--->");
  //Serial.println((int)Output);
  Cell_Temp_Write((int) Output);
  delay(50);
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
