#include <WiFi.h>
#include <esp_wifi.h>
#include <stdio.h>
#include <sys/time.h>
#include <HardwareSerial.h>

// Define the limit of the clientAddresses matrix
#define MAX_ADDRESSES 1024


#include "Arduino.h"

char clientAddresses[MAX_ADDRESSES][18]; // Setting the time the ESP will sniff a channel before switching
const unsigned long channelTime = 1000; // The number of MAC addresses currently in the matrix
int ClientCount = 0; // Device ID to specify which device captured the MAC address
char deviceID[9] = "D0000003";

//void promiscuousCallback(void *buf, wifi_promiscuous_pkt_type_t type);
//void sendList();
//void emptyList();

uint32_t AutoBaud()
{
    static uint32_t rates[] = {115200, 9600, 57600,  38400, 19200,  74400, 74880,
                               230400, 460800, 2400,  4800,  14400, 28800
                              };
    for (uint8_t i = 0; i < sizeof(rates) / sizeof(rates[0]); i++) {
        uint32_t rate = rates[i];
        Serial.printf("Trying baud rate %u\n", rate);
        Serial1.updateBaudRate(rate);
        delay(10);
        for (int j = 0; j < 10; j++) {
            Serial1.print("AT\r\n");
            String input = Serial1.readString();
            if (input.indexOf("OK") >= 0) {
                Serial.printf("Modem responded at rate:%u\n", rate);
                return rate;
            }
        }
    }
    Serial1.updateBaudRate(115200);
    return 0;
}

void setup()
{
    Serial.begin(115200); // Set console baud rate

    Serial.println("Start Sketch");

    Serial1.begin(115200, SERIAL_8N1, 16, 17);


    if (AutoBaud()) {
        Serial.println(F("***********************************************************"));
        Serial.println(F(" You can now send AT commands"));
        Serial.println(F(" Enter \"AT\" (without quotes), and you should see \"OK\""));
        Serial.println(F(" If it doesn't work, select \"Both NL & CR\" in Serial Monitor"));
        Serial.println(F(" DISCLAIMER: Entering AT commands without knowing what they do"));
        Serial.println(F(" can have undesired consiquinces..."));
        Serial.println(F("***********************************************************\n"));
    } else {
        Serial.println(F("***********************************************************"));
        Serial.println(F(" Failed to connect to the modem! Check the baud and try again."));
        Serial.println(F("***********************************************************\n"));
    }
    
    // ESP mode = WiFi Station
    WiFi.mode(WIFI_MODE_STA);
    
    // Initialize WiFi in promiscuous mode
    esp_wifi_set_promiscuous(false);
    esp_wifi_set_promiscuous_rx_cb(&promiscuousCallback);
    esp_wifi_set_promiscuous(true);
    delay(1000);
    Serial.println("Configuring A7670E Modem...");

    Serial1.println("AT");
    delay(1000);

    Serial1.println("AT+CREG?");
    delay(2000);

    Serial1.println("AT+CGATT=1");
    delay(2000);

    Serial1.println("AT+CGACT=1,1");
    delay(2000);

    Serial1.println("AT+CGPADDR=1");
    delay(2000);

    Serial1.println("AT+NETOPEN");
    delay(5000);

    Serial1.println("AT+CIPOPEN=0,\"UDP\",,,12345");
    delay(2000);

    Serial1.println("AT+CGDCONT=1,\"IP\",\"data.lycamobile.com\",\"10.43.192.62\"");
    delay(2000);
}

void loop() {
  // Timer setup for debugging
  struct timeval start, end;
  double elapsed;
  gettimeofday(&start, NULL);

  // Loop through WiFi channels
  for (int i = 0; i < 2; i++) {
    for (int channel = 1; channel <= 13; channel++) {
      esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
      delay(100);

      unsigned long startTime = millis();
      while (millis() - startTime < channelTime) {
        // Just wait
      }
    }
  }

  // Calculate elapsed time
  gettimeofday(&end, NULL);
  elapsed = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;

  sendList();
  emptyList();
}

// Callback for receiving WiFi packets
void promiscuousCallback(void *buf, wifi_promiscuous_pkt_type_t type) {
  wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buf;

  if (ClientCount < MAX_ADDRESSES) {
    const uint8_t *payload = pkt->payload;
    uint16_t len = pkt->rx_ctrl.sig_len;

    for (int i = 0; i < len - 9; i++) {
      if (memcmp(payload + i, "\x40\x00\x00\x00\xff\xff\xff\xff\xff\xff", 10) == 0) {
        char macAddress[18];
        sprintf(macAddress, "%02X:%02X:%02X:%02X:%02X:%02X",
                payload[i + 10], payload[i + 11], payload[i + 12],
                payload[i + 13], payload[i + 14], payload[i + 15]);

        bool found = false;
        for (int j = 0; j < ClientCount; j++) {
          if (strcmp(macAddress, clientAddresses[j]) == 0) {
            found = true;
            break;
          }
        }

        if (!found) {
          strcpy(clientAddresses[ClientCount], macAddress);
          ClientCount++;
        }
      }
    }
  }
}

float getTemp() {
  // Placeholder for actual temperature reading
  return 20.00;
}

// Send the list to Serial

void sendList() {
  String json = "[\"" + String(deviceID) + "\",\"" + String(getTemp()) + "\",";

  for (int i = 0; i < ClientCount; i++) {
    json += "\"" + String(clientAddresses[i]) + "\"";
    if (i < ClientCount - 1) {
      json += ", ";
    }
  }
  json += "]";

  Serial.println("Sending JSON:");
  Serial.println(json);

  int length = json.length();

  Serial.println(length);

  // Format and send AT+CIPSEND command for A7670E

  String cmd = "AT+CIPSEND=0," + String(length) + ",\"138.91.62.132\",12345\r\n";
  Serial.println(cmd);

  Serial1.print("AT+CIPSEND=0,");
  Serial1.print(length);
  Serial1.print(",\"138.91.62.132\",12345\r\n");  // End with CRLF
  delay(100);  // Wait for '>' prompt

  // Send the actual data
  Serial1.print(json);  // Use .print() to avoid extra newline
}

// Empty the list
void emptyList() {
  memset(clientAddresses, 0, sizeof(clientAddresses));
  ClientCount = 0;
}