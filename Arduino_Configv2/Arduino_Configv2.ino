//Versione Interrupt + Software Serial
#include <mcp_can.h>
#include <SPI.h>

const unsigned char SPI_CS_PIN = 27;
MCP_CAN CAN(SPI_CS_PIN);
char arduino = 0;
int value = 0;
int sensorPin = 0;


void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);

  while (CAN_OK != CAN.begin(CAN_500KBPS))              // init can bus : baudrate = 500k
  {
    Serial.println("CAN BUS Shield init fail");
    Serial.println("Init CAN BUS Shield again");
    delay(100);
  }
  Serial.println("CAN BUS Shield init ok!");

  attachInterrupt(0, MCP2515_ISR, FALLING); // start interrupt

  /*
     set mask, set both the mask to 0x3ff
  */
  CAN.init_Mask(0, 0, 0x3ff);                         // there are 2 mask in mcp2515, you need to set both of them
  CAN.init_Mask(1, 0, 0x3ff);


  /*
     set filter, we can receive id from 0x04 ~ 0x09
  */
  CAN.init_Filt(0, 0, 0x160);                          // there are 6 filter in mcp2515
  CAN.init_Filt(1, 0, 0x170);

}

void MCP2515_ISR()
{
  readCanMsg();
}

void sendMessage(unsigned long *id, unsigned char canMsg[]) {
  CAN.sendMsgBuf(id, 0, 8, canMsg);
  delay(100);
}

void canBusValue(unsigned int *sensorValue, unsigned char canMsg[], unsigned char *count) {
  if (*sensorValue < 0)
    sensorValue = 0;
  *sensorValue = (*sensorValue * 500) / 1023;

  if (*sensorValue > 255) {
    *sensorValue -= 255;
    canMsg[*count] = 0xFF;
    *count++;
    canMsg[*count] = *sensorValue;
    *count++;
  }
  else {
    canMsg[*count] = *sensorValue;
    *count += 2;
  }
}

void readCanMsg() {
  unsigned char buf[8] = {0};
  while (CAN_MSGAVAIL == CAN.checkReceive()) {        // check if data coming
    unsigned char len;
    unsigned long canID;
    CAN.readMsgBufID(&canID, &len, buf);
    //riceve da arduino1
    if (canID == 0x160) {
      //1 = A1
      //2 = A2
      //3 = A3
      //4 = A4
      //8 = A8
      //9 = A9
      //10 = A10
      //11 = A11
      arduino = 0;
      value = buf[1] + buf[2];
      Serial.println("Y" + value);
    }
    //riceve da arduino2
    else if (canID == 0x170) {
      //1 = A1
      //2 = A2
      //3 = A3
      //4 = A4
      //8 = A8
      //9 = A9
      //10 = A10
      //11 = A11
      arduino = 1;
      value = buf[1] + buf[2];
      Serial.println("Y" + value);
    }
  }
}


void loop() {
  unsigned char canMsg[8] = {0};
  memset(canMsg, 0x00, 8);
  unsigned char buf[8] = {0};
  unsigned long id;
  memset(buf, 0x00, 8);
  if (Serial.available()) {
    String cmd = Serial.readString();
    delay(100);
    if (cmd == "CONF") {
      id = 0x100;
      sendMessage(&id, canMsg);
    }
    else if (cmd == "CLOSECONF") {
      id = 0x110;
      sendMessage(&id, canMsg);
    }
    else if (cmd == "QUIT") {
      sensorPin = 0;
      arduino = 0;
      value = 0;
    }
    else if (cmd.startsWith("A")) {
      // ASKxy where x is Arduino number and y is sensorNumber
      arduino = cmd.substring(3, 4).toInt();
      int sensorNumber = cmd.substring(4).toInt();
      switch (sensorNumber) {
        case 0:
          sensorPin = 1;
          break;
        case 1:
          sensorPin = 2;
          break;
        case 2:
          sensorPin = 3;
          break;
        case 3:
          sensorPin = 4;
          break;
        case 4:
          sensorPin = 8;
          break;
        case 5:
          sensorPin = 9;
          break;
        case 6:
          sensorPin = 10;
          break;
        case 7:
          sensorPin = 11;
          break;
      }
      if (arduino == 0) {
        canMsg[0] = sensorPin;
        id = 0x160;
        sendMessage(&id, canMsg);
      }
      if (arduino == 1) {
        canMsg[0] = sensorPin;
        id = 0x170;
        sendMessage(&id, canMsg);
      }
    }
    else if (cmd.startsWith("S")) {
      //SETxxx where xxx is the new Value
      value = cmd.substring(3, 6).toInt();

      canMsg[0] = sensorPin;
      canBusValue(&value, canMsg, 1);
      if (arduino == 0) {
        canMsg[0] = sensorPin;
        id = 0x120;
      }
      if (arduino == 1) {
        canMsg[0] = sensorPin;
        id = 0x140;
      }
      sendMessage(&id, canMsg);
      value = 0;
    }
  }
  delay(1000);
}


