//Versione 3.0 con Interrupt

#include <EEPROM.h>
#include <mcp_can.h>
#include <SPI.h>


unsigned char calibrateValue[16];
const unsigned char SPI_CS_PIN = 13;
unsigned long time_now = 0;
unsigned int periodo = 100;
MCP_CAN CAN(SPI_CS_PIN);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  while (CAN_OK != CAN.begin(CAN_1000KBPS))              // init can bus : baudrate = 500k
  {
    Serial.println("CAN BUS Shield init fail");
    Serial.println("Init CAN BUS Shield again");
    delay(100);
  }
  Serial.println("CAN BUS Shield init ok!");

    pinMode(A1, INPUT);
  pinMode(A2, INPUT);
  pinMode(A3, INPUT);
  pinMode(A4, INPUT);
  pinMode(A8, INPUT);
  pinMode(A9, INPUT);
  pinMode(A10, INPUT);
  pinMode(A11, INPUT);

  attachInterrupt(0, MCP2515_ISR, FALLING); // start interrupt


  /*
     set mask, set both the mask to 0x3ff
  */
  CAN.init_Mask(0, 0, 0x3ff);                         // there are 2 mask in mcp2515, you need to set both of them
  CAN.init_Mask(1, 0, 0x3ff);


  /*
     set filter, we can receive id from 0x04 ~ 0x09
  */
  CAN.init_Filt(0, 0, 0x100);
  CAN.init_Filt(1, 0, 0x110);
  CAN.init_Filt(2, 0, 0x120);
  CAN.init_Filt(3, 0, 0x130);

  for (unsigned char addr = 0; addr < 16; addr++)
    calibrateValue[addr] = EEPROM.read(addr);

}

void MCP2515_ISR()
{
  readCanMsg();
}

void printCanMSG(unsigned long *id, unsigned char *canMsg) {
  Serial.println("-----------------------------");
  Serial.print("ID: ");
  Serial.print(*id, HEX);
  Serial.print(", ");
  Serial.println("Data: ");
  for (unsigned char count = 0; count < 8; count++)  {
    Serial.print(*canMsg, HEX);
    Serial.print(" ");
    canMsg++;
  }
  Serial.println();
  Serial.println("-----------------------------");
  Serial.println("");
}

void sendMessage(unsigned long *id, unsigned char canMsg[]) {
  printCanMSG(id, canMsg);
  CAN.sendMsgBuf(id, 0, 8, canMsg);
  delay(100);
}

void canBusValue(unsigned int *sensorValue, unsigned char canMsg[], unsigned char *count) {
  if (*sensorValue < 0)
    sensorValue = 0;
  long int temp = *sensorValue;
  long int temp2 = temp*500;
  *sensorValue = temp2 / 1023;

  if (*sensorValue > 255) {
    *sensorValue -= 255;
    canMsg[*count] = 0xFF;
    *count = *count + 1;
    canMsg[*count] = *sensorValue;
    *count = *count + 1;
  }
  else {
    canMsg[*count] = *sensorValue;
    *count = *count + 2;
  }
}


void loop() {

  int sensorPin = A1;
  unsigned char canMsg[8];

  memset(canMsg, 0x00, 8);
  sensorPin = A0;
  unsigned int sensorValue = 0;
  unsigned long id;

  
  //dumper+SteamAngle : 3 sensori
   id = 0x601;

  for (unsigned char count = 0; count < 6;) {
    sensorValue = analogRead(sensorPin++) - calibrateValue[count] - calibrateValue[count + 1];
    canBusValue(&sensorValue, canMsg, &count);
  }

  if(millis() >= time_now + period) {
    sendMessage(&id, canMsg);
    time_now = millis();
  }
  

  memset(canMsg, 0x00, 8);
  //FRENI - 2 sensori
  id = 0x602;
  count = 0;
  sensorPin = A4;
  sensorValue = analogRead(sensorPin) - calibrateValue[count + 7] - calibrateValue[count + 8];
  canBusValue(&sensorValue, canMsg, &count);

  sensorPin = A8;
  sensorValue = analogRead(sensorPin) - calibrateValue[count + 7] - calibrateValue[count + 8];
  canBusValue(&sensorValue, canMsg, &count);
  
  if(millis() >= time_now + period) {
    sendMessage(&id, canMsg);
    time_now = millis();
  }

  memset(canMsg, 0x00, 8);
  //Accel. - 3 sensori
  id = 0x603;
  for (unsigned char count = 0; count < 6;) {
    sensorValue = analogRead(sensorPin++) - calibrateValue[count + 11] - calibrateValue[count + 12];
    canBusValue(&sensorValue, canMsg, &count);
  }
  sendMessage(&id, canMsg);
  memset(canMsg, 0x00, 8);

  readCanMsg();

}

