#include <WiFi.h>
#include <esp_wifi.h>
#include <stdio.h>
#include <sys/time.h>
#include <HardwareSerial.h>

// Define the limit of the clientAddresses matrix
#define MAX_ADDRESSES 1024

char clientAddresses[MAX_ADDRESSES][18]; // Setting the time the ESP will sniff a channel before switching
const unsigned long channelTime = 1000; // The number of MAC addresses currently in the matrix
int ClientCount = 0; // Device ID to specify which device captured the MAC address
char deviceID[9] = "D0000001";

void setup() {
  delay(5000);
  Serial.begin(115200);  // Serial monitor
  Serial1.begin(115200, SERIAL_8N1, 20, 21); // UART to Air780EU

  // ESP mode = WiFi Station
  WiFi.mode(WIFI_MODE_STA);
  
  // Initialize WiFi in promiscuous mode
  esp_wifi_set_promiscuous(false);
  esp_wifi_set_promiscuous_rx_cb(&promiscuousCallback);
  esp_wifi_set_promiscuous(true);
  delay(1000);
  Serial.println("Configuring Air780E Modem...");

  Serial1.println("AT");
  delay(500);

  Serial1.println("AT+CREG?");
  delay(1000);

  Serial1.println("AT+CGATT?");
  delay(1000);

  Serial1.println("AT+CIPMUX=0");
  delay(500);

  Serial1.println("AT+CIPQSEND=1");
  delay(500);

  Serial1.println("AT+CSTT");  // Use default APN; if needed, set it like AT+CSTT="your.apn"
  delay(1000);

  Serial1.println("AT+CIICR");
  delay(3000);  // Takes a bit longer

  Serial1.println("AT+CIFSR");
  delay(1000);

  Serial1.println("AT+CIPSTATUS");
  delay(1000);

  Serial1.println("AT+CIPSTART=\"UDP\",\"138.91.62.132\",12345");
  delay(3000);  // Allow time to establish connection

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

  // Step 1: Tell the module how many bytes you're going to send
  Serial1.print("AT+CIPSEND=");
  Serial1.println(length);
  delay(100);  // Give the module time to return '>'

  // Step 2: Send the actual data
  Serial1.print(json);  // Use .print(), not .println(), to avoid sending a newline
}


// Empty the list
void emptyList() {
  memset(clientAddresses, 0, sizeof(clientAddresses));
  ClientCount = 0;
}







