/*
 Name:		Slave_type_popper_01_v2.ino
 Created:	10/1/2026 12:39:25 PM
 Author:	tony
*/
//  Format of code to send to control panel:
//  target ID   01, 02, 03 etc. 
//  Data        eg. SCORE=5, COMMAND=TARGET_RESET

#include <WiFi.h>
#include <esp_now.h>

#define TARGET_1_PIN 33
#define TARGET_2_PIN 32
#define LED_PIN 15
#define BUTTON_PIN_BITMASK(GPIO) (1ULL << GPIO)  // 2 ^ GPIO_NUMBER in hex

String TARGET_1_ID = "01";
String TARGET_2_ID = "02";
String TARGET_1_SCORE = "05";
String TARGET_2_SCORE = "05";
String code;

void formatMacAddress(const wifi_tx_info_t* macAddr, char* buffer, int maxLength)
{
    snprintf(buffer, maxLength, "%02x:%02x:%02x:%02x:%02x:%02x", macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);
}

// Called when data is received
void receiveCallback(const wifi_tx_info_t* macAddr, const uint8_t* data, int dataLen)
{
    char buffer[ESP_NOW_MAX_DATA_LEN + 1];
    int msgLen = min(ESP_NOW_MAX_DATA_LEN, dataLen);
    strncpy(buffer, (const char*)data, msgLen);
    buffer[msgLen] = 0;
    char macStr[18];
    formatMacAddress(macAddr, macStr, 18);
    Serial.printf("Received message from: %s - %s\n", macStr, buffer);
    if (strcmp("r", buffer) == 0)     // Has a 'reset' been received?  
    {
       Serial.println("Reset received");
       digitalWrite(LED_PIN, LOW);
       delay(200);
       digitalWrite(LED_PIN, HIGH);
    }
}

// Called when data is sent
void sentCallback(const wifi_tx_info_t* macAddr, esp_now_send_status_t status)
{
    // char macStr[18];
    // formatMacAddress(macAddr, macStr, 18);
    // Serial.print("Last Packet Sent to: ");
    // Serial.println(macStr);
    // Serial.print("Last Packet Send Status: ");
    // Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

// Emulates a broadcast
void broadcast(const String& message)
{
    uint8_t broadcastAddress[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
    esp_now_peer_info_t peerInfo = {};
    memcpy(&peerInfo.peer_addr, broadcastAddress, 6);
    if (!esp_now_is_peer_exist(broadcastAddress))
    {
        esp_now_add_peer(&peerInfo);
    }
    esp_err_t result = esp_now_send(broadcastAddress, (const uint8_t*)message.c_str(), message.length());
    if (result == ESP_OK)
    {
        Serial.println("Broadcast message success");
    }
    else if (result == ESP_ERR_ESPNOW_NOT_INIT)
    {
        Serial.println("ESP-NOW not Init.");
    }
    else if (result == ESP_ERR_ESPNOW_ARG)
    {
        Serial.println("Invalid Argument");
    }
    else if (result == ESP_ERR_ESPNOW_INTERNAL)
    {
        Serial.println("Internal Error");
    }
    else if (result == ESP_ERR_ESPNOW_NO_MEM)
    {
        Serial.println("ESP_ERR_ESPNOW_NO_MEM");
    }
    else if (result == ESP_ERR_ESPNOW_NOT_FOUND)
    {
        Serial.println("Peer not found.");
    }
    else
    {
        Serial.println("Unknown error");
    }
}


void setup() {
  Serial.begin(115200);
  while (!Serial) { } // optional for native-USB boards

  WiFi.mode(WIFI_STA);
  Serial.println("ESP-NOW");
  Serial.print("MAC Address: ");
  Serial.println(WiFi.macAddress());
  WiFi.disconnect();

  if (esp_now_init() == ESP_OK)
  {
      Serial.println("ESP-NOW Init Success");
      esp_now_register_recv_cb((esp_now_recv_cb_t)receiveCallback);
      esp_now_register_send_cb((esp_now_send_cb_t)sentCallback);
  }
  else
  {
      Serial.println("ESP-NOW Init Failed");
      delay(3000);
      ESP.restart();
  }

  pinMode(LED_PIN, OUTPUT);               // Set the ledPin as an output pin
  pinMode(TARGET_1_PIN, INPUT_PULLDOWN);    // Set the target 1 pin as an input pin
  pinMode(TARGET_2_PIN, INPUT_PULLDOWN);    // SSet the target 2 pin as an input pin

	if (digitalRead(TARGET_1_PIN) == HIGH) {  // Target 1 has been hit
        Serial.println("Target 1 has been hit");    
        code = String(TARGET_1_ID + "|SCORE=" + TARGET_1_SCORE);
        broadcast(code);
        digitalWrite(LED_PIN, LOW);
        delay(200);
        digitalWrite(LED_PIN, HIGH);
    } 

    if (digitalRead(TARGET_2_PIN) == HIGH) {  // Target 2 has been hit
        Serial.println("Target 2 has been hit"); 
        code = String(TARGET_2_ID + "|SCORE=" + TARGET_2_SCORE);
        broadcast(code);
        digitalWrite(LED_PIN, LOW);
        delay(200);
        digitalWrite(LED_PIN, HIGH);
    }      

    int GPIO_reason = esp_sleep_get_ext1_wakeup_status();
    Serial.print("GPIO that triggered the wake up: GPIO ");
    Serial.println((log(GPIO_reason))/log(2), 0);
//   esp_sleep_enable_timer_wakeup((uint64_t)10 * 1000000ULL); 
    Serial.println("Entering Deep Sleep");
    esp_sleep_enable_gpio_wakeup(); 

    uint64_t bitmask = BUTTON_PIN_BITMASK((gpio_num_t)TARGET_1_PIN) | BUTTON_PIN_BITMASK((gpio_num_t)TARGET_2_PIN); 
    esp_sleep_enable_ext1_wakeup_io(bitmask, ESP_EXT1_WAKEUP_ANY_HIGH);   

 
    Serial.flush(); 
    esp_deep_sleep_start();       // Does not return
  
}

void loop()
{
    //  // Timer: wake after SLEEP_SECS seconds
    // esp_sleep_enable_timer_wakeup((uint64_t)SLEEP_SECS * 1000000ULL);
    // //esp_sleep_enable_gpio_wakeup(); 
    // //gpio_wakeup_enable((gpio_num_t)TARGET_1_PIN, GPIO_INTR_HIGH_LEVEL);    
    // Serial.println("Entering Light Sleep...");
    // Serial.flush();              
    // esp_light_sleep_start(); 
    // Serial.println("Resumed from Light Sleep!");
    
	// if (digitalRead(TARGET_1_PIN) == HIGH && target1_set == 1) {  // Target 1 has been hit
    //     Serial.println("Target 1 has been hit");    
    //     code = String(TARGET_1_ID + "|SCORE=" + TARGET_1_SCORE);
    //     broadcast(code);
    //     digitalWrite(LED_PIN, LOW);
    //     delay(200);
    //     digitalWrite(LED_PIN, HIGH);
    //     target1_set = 0;   
    // }

	// if (digitalRead(TARGET_1_PIN) == LOW && target1_set == 0) {  // Target 1 has been reset
    //     Serial.println("Target 1 has been reset"); 
    //     code = String(TARGET_1_ID + "|COMMAND=TARGETRESET");
    //     broadcast(code); 
    //     digitalWrite(LED_PIN, LOW);
    //     delay(200);
    //     digitalWrite(LED_PIN, HIGH); 
    //     target1_set = 1; 
    // }   

	// if (digitalRead(TARGET_2_PIN) == HIGH && target2_set == 1) {  // Target 2 has been hit
    //     Serial.println("Target 2 has been hit"); 
    //     code = String(TARGET_2_ID + "|SCORE=" + TARGET_2_SCORE);
    //     broadcast(code);
    //     digitalWrite(LED_PIN, LOW);
    //     delay(200);
    //     digitalWrite(LED_PIN, HIGH);
    //     target2_set = 0;    
    // }
	// if (digitalRead(TARGET_2_PIN) == LOW && target2_set == 0) {  // Target 2 has been reset
    //     Serial.println("Target 2 has been reset"); 
    //     code = String(TARGET_2_ID + "|COMMAND=TARGETRESET");
    //     broadcast(code);  
    //     target2_set = 1;    
    //     delay(200);
    // }      

    
}

