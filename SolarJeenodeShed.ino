// Solar Jeenode -- Shed (node 5) June 27, 2016
// Burn to device as Arduino Mini w/ATMEGA328
// Replaced DHT with SHT21 module for better performance

#include <JeeLib.h>
#include <Ports.h>
#include <avr/sleep.h>
#include <movingAvg.h>

movingAvg avgSolarPanel(4);
movingAvg avgBattery(4);

long bvAvg, spAvg;

#include <Wire.h>
#include <Sodaq_SHT2x.h>

//  DHTxx dht (3);          // DHT connected to pin D3
#define SEND_MODE 2     // set to 3 if fuses are e=06/h=DE/l=CE, else set to 2

int batteryPin = 1;     // Battery to A1
int spPin = 0;          // Solar Panel to A0

struct {
  int temp;	    // temperature
  int hum;	    // humidity
  long bv;      // battery voltage
  long spv;     // solar panel voltage
}
payload;

// this must be defined since we're using the watchdog for low-power waiting
ISR(WDT_vect) {
  Sleepy::watchdogEvent();
}

void setup() {
  //Serial.begin(9600);
  Wire.begin();

  avgSolarPanel.begin();
  avgBattery.begin();

  rf12_initialize(5, RF12_433MHZ, 212); // 433 Mhz, net group 212, node 11
  rf12_control(0xC040); 		// set low-battery level to 2.2V i.s.o. 3.1V
  rf12_sleep(RF12_SLEEP);
}


static byte sendPayload () {
  rf12_sleep(RF12_WAKEUP);
  while (!rf12_canSend())
    rf12_recvDone();
  rf12_sendStart(0, &payload, sizeof payload);
  rf12_sendWait(SEND_MODE);
  rf12_sleep(RF12_SLEEP);
}

void loop() {

  float t, h;

  //if (dht.reading(t, h, true)) {
  if (SHT2x.GetHumidity(), true) {
    h = SHT2x.GetHumidity() * 10;
    t = SHT2x.GetTemperature() * 10;
  }
  //  Serial.print("\ntemperature = ");
  //  Serial.println(t);
  //  Serial.print("humidity = ");
  //  Serial.println(h);
  //}

  long b_v = analogRead(batteryPin);
  bvAvg = avgBattery.reading(b_v);
  long sp_v = analogRead(spPin);
  spAvg = avgSolarPanel.reading(sp_v);

  //  Serial.print("Battery Voltage (raw): ");
  //  Serial.println(b_v);
  //  Serial.print("Battery Voltage (Volts): ");
  //  Serial.println(b_v * 3.3 / 1024);
  //  Serial.flush();

  payload.bv = bvAvg;
  payload.spv = spAvg;

  payload.temp = t;
  payload.hum = h;
  sendPayload();

  for (byte i = 0; i < 3; ++i) {
    Sleepy::loseSomeTime(30000); //wake up after 90 seconds
  }
}
