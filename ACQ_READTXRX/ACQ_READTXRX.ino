/* Versione 4.0 readRxTxStatus


*/
#include <EEPROM.h>
#include <mcp_can.h>
#include <SPI.h>


unsigned char calibrateValue[16];
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
void readCanMsg() {
  int sensorValue = 0;
  while (CAN_MSGAVAIL == CAN.checkReceive()) {
    // check if data coming
    int sensorPin = A1;
    unsigned char buf[8];
    unsigned char len;
    unsigned long canID;
    CAN.readMsgBufID(&canID, &len, buf);
    Serial.println("-----------------------------");
    Serial.print("Get data from ID: ");
    Serial.println(canID, HEX);
    if (canID == 0x120) {
      calibrateValue[buf[0] - 1] = buf[1] + buf[2];
      EEPROM.write(buf[0] - 1, calibrateValue[buf[0] - 1]);
      // print
      break;
    }
    else if (canID == 0x130) {
      sensorPin += (buf[0] - 1);
      sensorValue = analogRead(sensorPin);
      unsigned char count = 1;

      canBusValue(&sensorValue, buf, &count);
      canID = 0x160;
      sendMessage(&canID, buf);
      break;
    }
    Serial.println("-----------------------------");
    Serial.println();
  }
}

void loop() {

  int sensorPin;
  unsigned char canMsg[8] = {0};
  unsigned long id;
  memset(canMsg, 0x00, 8);
  sensorPin = A0;
  unsigned int sensorValue = 0;
  //dumper+SteamAngle
  readCanMsg();

  //dumper+SteamAngle - 3 sensori
  id = 0x601;

  for (unsigned char count = 0; count < 6;) {
    sensorValue = analogRead(sensorPin++) - calibrateValue[count] - calibrateValue[count + 1];
    canBusValue(&sensorValue, canMsg, &count);
  }
  while (!CAN.readRxTxStatus()) {
    readCanMsg();
  }
  sendMessage(&id, canMsg);
  delay(100);

  readCanMsg();
  //PushRoad - 2 sensori
  id = 0x602;
  memset(canMsg, 0x00, 8);
  unsigned char countTemp = 0;
  sensorPin = A4;
  sensorValue = analogRead(sensorPin) - calibrateValue[countTemp + 7] - calibrateValue[countTemp + 8];
  sensorValue = 200;
  canBusValue(&sensorValue, canMsg, &countTemp);

  sensorPin = A8;
  sensorValue = analogRead(sensorPin) - calibrateValue[countTemp + 7] - calibrateValue[countTemp + 8];
  canBusValue(&sensorValue, canMsg, &countTemp);
  sendMessage(&id, canMsg);

  while (!CAN.readRxTxStatus()) {
    readCanMsg();
  }
  sendMessage(&id, canMsg);
  delay(100);

  readCanMsg();
  //Accel.
  id = 0x603;
  memset(canMsg, 0x00, 8);
  for (unsigned char count = 0; count < 6;) {
    sensorValue = analogRead(sensorPin++) - calibrateValue[count + 11] - calibrateValue[count + 12];
    canBusValue(&sensorValue, canMsg, &count);
  }
  while (!CAN.readRxTxStatus()) {
    readCanMsg();
  }
  sendMessage(&id, canMsg);
  delay(100);

}
