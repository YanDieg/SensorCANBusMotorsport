/* Versione 1.0 solo CHECK

*/

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

  for (unsigned char addr = 0; addr < 16; addr++)
    calibrateValue[addr] = EEPROM.read(addr);

  time_now = millis();

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
  CAN.sendMsgBuf(*id, 0, 8, canMsg);
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

void readCanMsg() {
  while (CAN_MSGAVAIL == CAN.checkReceive()) {
    // check if data coming
    int sensorPin = A1;
    unsigned char buf[8];
    unsigned char len;
    unsigned long canID;
    CAN.readMsgBufID(&canID, &len, buf);
    Serial.println("-----------------------------");
    Serial.println("Get data from ID: ");
    Serial.println(canID, HEX);
    if (canID == 0x120) {
      char pos = buf[0] * 2;
      calibrateValue[pos] = buf[1];
      calibrateValue[pos++] = buf[2];
      EEPROM.write(pos, calibrateValue[pos]);
      EEPROM.write(pos++, calibrateValue[pos++]);
      break;
    }
    else if (canID == 0x130) {
      sensorPin += (buf[0] - 1);
      int sensorValue = analogRead(sensorPin);
      unsigned char count = 1;
      canBusValue(&sensorValue, buf, &count);
      canID = 0x160;
      sendMessage(&canID, buf);
      break;

      Serial.println("-----------------------------");
      Serial.println();
    }
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
  readCanMsg();
   id = 0x601;

  for (unsigned char count = 0; count < 6;) {
    sensorValue = analogRead(sensorPin++) - calibrateValue[count] - calibrateValue[count + 1];
    canBusValue(&sensorValue, canMsg, &count);
  }

  if(millis() >= time_now + period) {
    sendMessage(&id, canMsg);
    time_now = millis();
  }
  

  readCanMsg();
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

  readCanMsg();
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
