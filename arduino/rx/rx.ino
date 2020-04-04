#include <RH_ASK.h>
#include <SPI.h> // Not actualy used but needed to compile

RH_ASK driver;

void setup()
{  
  pinMode(LED_BUILTIN, OUTPUT);
  
    Serial.begin(9600);  // Debugging only
    if (!driver.init())
         Serial.println("init failed");
}

void loop()
{
    byte buf[2];
    uint8_t buflen = sizeof(buf);
    if (driver.recv(buf, &buflen)) // Non-blocking
    {
      int i;
      i = (buf[1] << 8) | buf[0];
      // Message with a good checksum received, dump it.
      Serial.write(buf,2);         
    }
}
