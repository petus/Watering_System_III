/*
 * Watering System version 3
 * Made by Petus (2018)
 *
 * https://time4ee.com (EN)
 * https://chiptron.cz (CZ)
 *
 * Libraries:
 * https://github.com/adafruit/Adafruit_ADS1X15
 * https://github.com/adafruit/Adafruit_SSD1306
 * https://github.com/adafruit/Adafruit-GFX-Library
 * https://github.com/LowPowerLab/SI7021
 * https://github.com/PaulStoffregen/Time
 *
 */

#include "setting.h"
#include "Wire.h"

/* Define */
/* -------- Can be changed by user -------- */
#define USE_WIFI	// Uncomment if you want to use WiFi
#define USE_OLED	// Uncomment for using of OLED
#define USE_SI7021	// Uncomment for using of SI7021 sensor
#define USE_ADS1X15 // Uncomment for using of 4-channel ADC
#define USE_TMEP    // Uncomment for sending data to tmep server

#define TIME_FROM 20	// The time of watering
#define TIME_TO		23	// The time of watering

#define SOIL_MOISTURE_THRES	2600	// Threshold for measurement of soil moisture

#define SLEEP_TIME_IN_SECONDS	30

#define DISPLAY_TIME  5  // Time in seconds for switch of page on display

#define WATERING_TIME 10000 // miliseconds

const char* host = "HOSTNAME.tmep.cz";

#define PUMP  OUT1 
#define VALVE_1 OUT2 
#define VALVE_2 OUT3  
//#define VALVE_3 OUT4  
//#define VALVE_4 OUT5  

#define DBG_MSG		// Enable debug messages through UART
/* ---------------------------------------- */

/* Include */
#ifdef USE_SI7021
    #include "SI7021.h"
#endif // USE_SI7021

#ifdef USE_OLED
    #include "Adafruit_GFX.h"
    #include "Adafruit_SSD1306.h"
#endif // USE_OLED

#ifdef USE_WIFI
    #include "ESP8266WiFi.h"
    #include "time.h"
    #include "TimeLib.h"
#endif

#ifdef USE_ADS1X15
    #include "Adafruit_ADS1015.h"
#endif // USE_ADS1X15

/* Define */
/* Do not change */
#define SDA 4 // GPIO4 on D2
#define SCL 5 // GPIO5 on D1
#define OLED_RESET 19 //choose unused pin, connect the RESET pin to VCC (3.3V)

#define OUT1  2 //GPIO2 on D4
#define OUT2 14 //GPIO14 on D5
#define OUT3 12  //GPIO12 on D6
#define OUT4 13  //GPIO13 on D7
#define OUT5 15  //GPIO15 on D8

#ifdef USE_OLED
    Adafruit_SSD1306 display(OLED_RESET);
#endif // USE_OLED

#ifdef USE_ADS1X15
    Adafruit_ADS1115 ads;  // 16-bit version
#endif // USE_ADS1X15

#ifdef USE_SI7021
    SI7021 sensor;
#endif // USE_SI7021

#ifdef USE_WIFI
    int timezone = 1;
    int dst = 0;
#endif // USE_WIFI



/* Global variables */

static volatile uint8_t displayPage = 1;
volatile int16_t adc0, adc1, adc2, adc3;
volatile int temperature = 0;
volatile unsigned int humidity = 0;
volatile int Hour = 0;
volatile int Minutes = 0;
volatile int Seconds = 0;

void timer0_ISR(void)
{
#ifdef USE_OLED
    if (displayPage == 1) // first side
    {
        display.clearDisplay();
        // text display tests
        display.setTextSize(1);
        // text color
        display.setTextColor(WHITE);
        // set position
        display.setCursor(0,0);
        display.print("Temp: ");
        display.print(String(temperature/100.0, 2));
        display.println(" 'C");
        display.setCursor(0,10);
        display.print("Hum: ");
        display.print(humidity);
        display.println(" %");
        display.setCursor(0,20);
        display.print("Mois0: ");
        display.print(adc0);
        display.println(" ?");
        display.setCursor(0,30);
        display.print("Mois1: ");
        display.print(adc1);
        display.println(" ?");
        display.setCursor(0,40);
        display.print("Mois2: ");
        display.print(adc2);
        display.println(" ?");
        display.setCursor(0,50);
        display.print("Mois3: ");
        display.print(adc3);
        display.println(" ?");
        display.display();
        displayPage++;
    }
    else if (displayPage == 2)
    {
        display.clearDisplay();
        display.setTextSize(1);
        // text color
        display.setTextColor(WHITE);
        // set position
        display.setCursor(0,0);
        display.print("Druha strana");
        display.display();
        displayPage++;
    }
#ifdef USE_WIFI
    else if (displayPage == 3) // NTP time
    {
        display.clearDisplay();
        // text display tests
        display.setTextSize(1);
        // text color
        display.setTextColor(WHITE);
        // set position
        display.setCursor(0,0);
        display.println("Watering between: ");
        display.setCursor(0,20);
        display.print("from ");
        display.print(TIME_FROM);
        display.print(" to ");
        display.println(TIME_TO);
        display.setCursor(0,40);
        time_t now = time(nullptr);
        /*Hour = hour(now);
        Minutes = minute(now);
        Seconds = second(now);
        display.print(Hour);
        display.print(":");
        display.print(Minutes);
        display.print(":");
        display.println(Seconds);*/
        display.println(ctime(&now));
        display.display();
        displayPage++;
    }
#endif // USE_WIFI
    else
    {
        displayPage = 1;
    }
    
#endif // USE_OLED

    timer0_write(ESP.getCycleCount() + DISPLAY_TIME*80000000L); // 80MHz == 1sec
}


//The setup function is called once at startup of the sketch
void setup()
{
// Add your initialization code here
#ifdef DBG_MSG
	  Serial.begin(115200);
#endif // DBG_MSG

#ifdef USE_SI7021
	  sensor.begin(SDA,SCL);
#endif // USE_SI7021

#ifdef USE_ADS1X15
    ads.begin();
#endif // USE_ADS1X15

#ifdef USE_OLED
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x64)
    display.clearDisplay();
#endif // USE_OLED

#ifdef USE_WIFI
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    Serial.println("\nConnecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) 
    {
        Serial.print(".");
        delay(1000);
    }

    configTime(3 * 3600, 0, "pool.ntp.org", "time.nist.gov");
    Serial.println("\nWaiting for time");
    while (!time(nullptr)) 
    {
        Serial.print(".");
        delay(1000);
    }
    Serial.println("");
#endif // USE_WIFI

    noInterrupts(); 
    timer0_isr_init(); 
    timer0_attachInterrupt(timer0_ISR); 
    timer0_write(ESP.getCycleCount() + DISPLAY_TIME*80000000L); // 80MHz == 1sec 
    interrupts();

    pinMode(PUMP, OUTPUT);

#ifdef VALVE_1
    pinMode(VALVE_1, OUTPUT);
#endif // VALVE_1

#ifdef VALVE_2
    pinMode(VALVE_2, OUTPUT);
#endif // VALVE_2

#ifdef VALVE_3
    pinMode(VALVE_3, OUTPUT);
#endif // VALVE_3
    
#ifdef VALVE_4
    pinMode(VALVE_4, OUTPUT);
#endif // VALVE_4

}

// The loop function is called in an endless loop
void loop()
{
//Add your repeated code here
#ifdef USE_SI7021
	  temperature = sensor.getCelsiusHundredths();
	  humidity = sensor.getHumidityPercent();

#ifdef DBG_MSG
	  Serial.println("Temperature: ");
	  Serial.print(String(temperature/100.0, 2));
	  Serial.println(" °C");
	  Serial.println("Humidity: ");
	  Serial.print(humidity);
	  Serial.println(" %");
#endif // DBG_MSG
#endif // USE_SI7021

#ifdef USE_ADS1X15
    adc0 = ads.readADC_SingleEnded(0);
    adc0 = 0.1875*adc0;
    adc1 = ads.readADC_SingleEnded(1);
    adc1 = 0.1875*adc1;
    adc2 = ads.readADC_SingleEnded(2);
    adc2 = 0.1875*adc2;
    adc3 = ads.readADC_SingleEnded(3);
    adc3 = 0.1875*adc3;
#ifdef DBG_MSG
    Serial.print("AIN0: "); Serial.println(adc0);
    Serial.print("AIN1: "); Serial.println(adc1);
    Serial.print("AIN2: "); Serial.println(adc2);
    Serial.print("AIN3: "); Serial.println(adc3);
    Serial.println(" ");
#endif // DBG_MSG
#endif // USE_ADS1X15

#ifdef USE_WIFI
    time_t now = time(nullptr);
    Hour = hour(now);
    Minutes = minute(now);
    Seconds = second(now);
    Serial.print(Hour);
    Serial.print(":");
    Serial.print(Minutes);
    Serial.print(":");
    Serial.println(Seconds);
    Serial.println(ctime(&now));
#endif // USE_WIFI

#if defined(USE_WIFI) && defined(USE_TMEP)
  /*----------petus1.tmep.cz--------------*/
  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect(host, httpPort))
  {
    Serial.println("connection failed");
    return;
  }

  // We now create a URI for the request
  String url = "/index.php?";

  Serial.print("Requesting URL: ");
  Serial.println(url);

  // This will send the request to the server
  client.print(String("GET ") + url + "tempC=" + (String(temperature/100.0, 2)) + "&humV=" + adc0 + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n\r\n");

  delay(10);

  // Read all the lines of the reply from server and print them to Serial
  Serial.println("Respond:");
  while (client.available())
  {
    String line = client.readStringUntil('\r');
    Serial.print(line);
  }

#endif // defined(USE_WIFI) && defined(USE_TMEP)

    /* --- Watering sequence --- */
    digitalWrite(PUMP, HIGH);

#ifdef VALVE_1
    digitalWrite(VALVE_1, HIGH);
#endif // VALVE_2

#ifdef VALVE_2
    digitalWrite(VALVE_2, HIGH);
#endif // VALVE_2

#ifdef VALVE_3
    digitalWrite(VALVE_3, HIGH);
#endif // VALVE_3

#ifdef VALVE_4
    digitalWrite(VALVE_4, HIGH);
#endif // VALVE_4

    if((Hour > TIME_FROM) && (Hour < TIME_TO))
    {
#ifdef VALVE_1
        if(adc0 > SOIL_MOISTURE_THRES)
        {
            // Start
#ifdef DBG_MSG
            Serial.println("1th valve");
#endif // DBG_MSG
            digitalWrite(PUMP, LOW);
            digitalWrite(VALVE_1, LOW);
            delay(WATERING_TIME);

            // Stop
            digitalWrite(PUMP, HIGH);
            digitalWrite(VALVE_1, HIGH);
        }  
#endif // VALVE_1

#ifdef VALVE_2
        if(adc0 > SOIL_MOISTURE_THRES)
        {
            // Start
#ifdef DBG_MSG
            Serial.println("2th valve");
#endif // DBG_MSG
            digitalWrite(PUMP, LOW);
            digitalWrite(VALVE_2, LOW);
            delay(WATERING_TIME);

            // Stop
            digitalWrite(PUMP, HIGH);
            digitalWrite(VALVE_2, HIGH);
        }  
#endif // VALVE_2

#ifdef VALVE_3
        if(adc0 > SOIL_MOISTURE_THRES)
        {
            // Start
#ifdef DBG_MSG
            Serial.println("3th valve");
#endif // DBG_MSG
            digitalWrite(PUMP, LOW);
            digitalWrite(VALVE_3, LOW);
            delay(WATERING_TIME);

            // Stop
            digitalWrite(PUMP, HIGH);
            digitalWrite(VALVE_3, HIGH);
        }  
#endif // VALVE_3

#ifdef VALVE_4
        if(adc0 > SOIL_MOISTURE_THRES)
        {
            // Start
#ifdef DBG_MSG
            Serial.println("4th valve");
#endif // DBG_MSG
            digitalWrite(PUMP, LOW);
            digitalWrite(VALVE_4, LOW);
            delay(WATERING_TIME);

            // Stop
            digitalWrite(PUMP, HIGH);
            digitalWrite(VALVE_4, HIGH);
        }
#endif // VALVE_4
      
    }


    delay(60000);
    
}
