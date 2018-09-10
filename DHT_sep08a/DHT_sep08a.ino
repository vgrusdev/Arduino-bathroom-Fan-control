/*
#include "DHT.h" 
*/

#include <DHT.h>

#define DHTPIN 3
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);
 
void setup() 
{
  Serial.begin(115200);
  Serial.println("DHT22 Tests:");
  dht.begin();
}
 
void loop() 
{
  delay(2000);
  /* Serial.print("Starting read humidity... ");
   */
  float h = dht.readHumidity();
/*  Serial.println("done.");
  Serial.print("Starting read temperature... ");
*/
  float t = dht.readTemperature();

  /*Serial.println("done.");
 */
  if (isnan(h) || isnan(t))
    {
      Serial.println("Error reading DHT22");
      return;                                  
    }
  
  float hic = dht.computeHeatIndex(t, h, false);
  Serial.print("Humidity: ");
  Serial.print(h);
  Serial.print(" %\t");
  Serial.print("Temperature: ");
  Serial.print(t);
  Serial.println(" *C ");
}
