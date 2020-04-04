#include <RH_ASK.h>
#include <SPI.h> // Not actually used but needed to compile

RH_ASK driver;

byte TXBuffer[2] = {0};

int potPin = 0;
int val = 0;

void setup()
{
    Serial.begin(9600);    // Debugging only
    if (!driver.init())
         Serial.println("init failed");
}

void loop()
{
    val = analogRead(potPin);
    
    
    memcpy(TXBuffer, &val, 2);
  
    driver.send(TXBuffer, 2);
    driver.waitPacketSent();
    delay(50);
}
