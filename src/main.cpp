#include <WiFi.h>
#include <SPIFFS.h>
#include <DNSServer.h>
#include <wifi_net_blocks.h>
#include <wifi_net_blocks.cpp>
#include <Preferences.h>



Preferences pref;
http_parse http_parse;
uart_parse _uart;

const char *ssid_access_ap = "ESP32_test_server";
const char *password_access_ap = "ESP32_test_server";

bool login_saved = false;
bool passw_saved = false;
char buffer[128];
uint8_t buffer2[512];
int connections = 0;
int reboot_count = 0;

WiFiServer server(80);
WifiNetBlocks wifi_net_blocks;

String responseHTML;
String html_gen;
String MAC          = WiFi.macAddress();

int scan_block;
int test_scan;
File file;
float time_t_scan = 0;



void showConnectionsCount()
{
    char data[32];
    sprintf(data, "Connections: %d", connections);
}

void WiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info)
{
    connections += 1;
    showConnectionsCount();
}

void setup() {
    Serial.begin(115200);
    pref.begin("ESP32FLASH", false);

    if (!SPIFFS.begin(true)) {
        return;
    }
    
    file = SPIFFS.open("/index_esp.html");

    if (!file) {
        return;
    }
    else {
        Serial.println("File is found");
        Serial.println(file.name());
    }

    if (pref.isKey("Reboot_count")) {} else { pref.putInt("Reboot_count", reboot_count);}
    if (pref.isKey("accesss_point")) {} else {pref.putBool("accesss_point", false);}

    if (pref.isKey("SSID") && pref.isKey("PASS") && pref.getBool("accesss_point") == false) {
        // Проверка на наличие ключа в flash-памяти
        WiFi.mode(WIFI_STA);

        String ssid_saved = pref.getString("SSID");
        String pass_saved = pref.getString("PASS");
        test_scan = WiFi.scanNetworks();

        for (int i = 0; i < test_scan; ++i){
                           
            String ssid   = WiFi.SSID(i); 
            int rssi_ref  = WiFi.RSSI(i);
            char* ssid_rm_spaces = wifi_net_blocks.removeLineBreak((char*)ssid_saved.c_str());

            if (String(ssid_saved) == ssid && rssi_ref > -80) {
                Serial.print("Connect to net ");
                Serial.println(ssid_saved);
                WiFi.begin(ssid_saved, pass_saved);
            }
        }
        
        while (WiFi.status()!= WL_CONNECTED) {
            Serial.print(".");
            delay(500);
            connections++;
            if (connections > 10) {
                break;
            }
        }

        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("\nConnected to the WiFi network");
            Serial.print("Local ESP32 IP: ");
            Serial.println(WiFi.localIP());
        } else {
            Serial.println("Failed to connect to the WiFi network");
            Serial.println("Retrying...");
            int saved_reboot_count = pref.getInt("Reboot_count");

            if (saved_reboot_count > 2) {
                pref.putBool("accesss_point", true);
                WiFi.mode(WIFI_AP);
                Serial.println("Access point enabled");
                saved_reboot_count = 0;
                pref.putInt("Reboot_count", saved_reboot_count);
            }
            saved_reboot_count++;
            pref.putInt("Reboot_count", saved_reboot_count);

            delay(5000);
            ESP.restart();
        }
    } else {
        pref.putBool("accesss_point", true);
        WiFi.mode(WIFI_AP);
    }
    if (pref.getBool("accesss_point") == true) {
        WiFi.softAP(ssid_access_ap, password_access_ap);
        WiFi.onEvent(WiFiStationConnected);
    }
    IPAddress ip_address = WiFi.softAPIP();
    server.begin();
    Serial1.begin(115200, SERIAL_8N1, 6,7);
}

void loop() {   
    

    WiFiClient client = server.available();
    if ((millis() - time_t_scan) > 10000) {
        scan_block = WiFi.scanNetworks();
        time_t_scan  =  millis();
    }

    if (client) {
        String currentLine = "";
        uint8_t counter = 0x00;
        while (client.connected())
        {
            if (client.available())
            {
                char c = client.read();
                String request = http_parse.parse(c, buffer);
                if (request != String(0) && request.startsWith("PSK")) {
                    String parse_space_passw  = request.substring(request.indexOf(" ")+1, request.length()-1);
                    pref.putString("PASS", parse_space_passw);
                    passw_saved = true;
                }

                if (request != String(0) && request.startsWith("SSID")) {
                    String parse_space_login  = request.substring(request.indexOf(" ")+1, request.length()-1);
                    pref.putString("SSID", parse_space_login);
                    login_saved = true;
                }


                if (passw_saved == true && login_saved == true) {
                    Serial.println("Login0 and Pass saved. Reboot...");
                    pref.putBool("accesss_point", false);
                    ESP.restart();
                }

                if (request != String(0) && request.startsWith("GET")) {
                    String parse_space_request = request.substring(request.indexOf(" ") + 1, request.length() - 1);
                    if (parse_space_request.startsWith("/get_data")) {
                        
                        uint8_t _buff[5] = {0x40, 0x40, 0x00, 0x00, 0x00};

                        for (uint16_t counter = 0; counter < 0xFF; counter++) {
                            if (client.available()) {
                                Serial1.write(_buff, sizeof(_buff));
                                while (!Serial1.available()) {};
                                
                                uint16_t len_message = Serial1.available();
                                Serial1.readBytes(buffer2, len_message);
                                
                                uint16_t crc = _uart.crc16(buffer2, len_message-2);
                                Serial.println(crc);
                                Serial.println((buffer2[len_message-2] << 8 | buffer2[len_message-1]));
                                if (crc == (buffer2[len_message-2] << 8 | buffer2[len_message-1])) {
                                    Serial.println("CRC OK");
                                    _buff[4] = buffer2[4];
                                    for (int i = 0; i < len_message; i++) {
                                        client.print(buffer2[i], HEX);
                                    }
                                    client.println();
                                    
                            } else {

                                Serial.println("CRC error");
                                Serial.println("Попытка дочитать");

                                while (!Serial1.available()) {}
                                Serial.println("Есть данные в буффере");
                                
                                uint16_t len_message_additional = Serial1.available();
                                Serial1.readBytes(buffer2 + len_message, len_message_additional);
                                uint16_t full_len = len_message + len_message_additional;
                                
                                uint16_t crc = _uart.crc16(buffer2, len_message-2);
                                Serial.println((buffer2[len_message-2] << 8 | buffer2[len_message-1]));

                                if (crc == (buffer2[len_message-2] << 8 | buffer2[len_message-1])) {
                                    Serial.println("Успешно дочитано!");
                                    Serial.println("CRC OK");
                                    _buff[4] = buffer2[4];
                                    for (int i = 0; i < len_message; i++) {
                                        client.print(buffer2[i], HEX);
                                    }
                                    client.println();
                                }


                            }
                        } else {
                            counter = 0xFF;
                        }
                            
                        }
                        client.flush();
                        client.stop();
                    }
                }
                if (request!= String(0) && request.startsWith("GET")) {
                    String parse_space_request = request.substring(request.indexOf(" ")+1, request.length()-1);
                    if (parse_space_request.startsWith("/main")) {
                        if (c == '\n')
                            {
                    if (currentLine.length() == 0) {
                        client.println("HTTP/1.1 200 OK");
                        client.println("Content-type:text/html"); 
                        client.println("Connection: close");                       
                        client.println();
                        
                        while (file.available()) {
                            responseHTML+=(char)file.read();
                        }

                        file.close();
                        client.print(responseHTML);
                        client.println();

                        String MAC_data = wifi_net_blocks.genJS_script_data(MAC, "MAC_addr");
                        client.println(MAC_data);
                        client.println();


                        for (int i = 0; i < scan_block; ++i){
                           
                            String ssid   = WiFi.SSID(i); 
                            int rssi_ref  = WiFi.RSSI(i);
                            String rssi   = String(rssi_ref);
                            String encryption;   

                            switch (WiFi.encryptionType(i))
                            {
                            case WIFI_AUTH_OPEN:
                                encryption  = "Open";
                                break;
                            case WIFI_AUTH_WEP:
                                encryption  = "WEP";
                                break;
                            case WIFI_AUTH_WPA_PSK:
                                encryption  = "WPA";
                                break;
                            case WIFI_AUTH_WPA2_PSK:
                                encryption  = "WPA2";
                                break;
                            case WIFI_AUTH_WPA_WPA2_PSK:
                                encryption  = "WPA WPA2";
                                break;
                            case WIFI_AUTH_ENTERPRISE:
                                encryption  = "Enterprise";
                                break;
                            case WIFI_AUTH_WPA3_PSK:
                                encryption  = "WPA3 PSK";
                                break;
                            case WIFI_AUTH_WPA2_WPA3_PSK:
                                encryption  =  "WPA2 WPA3";
                                break;
                            case WIFI_AUTH_WAPI_PSK:
                                encryption  =  "WAPI";
                                break;
                            case WIFI_AUTH_WPA3_ENT_192:
                                encryption  =   "WPA3 ENTERPRICE 192";
                                break;
                            case WIFI_AUTH_MAX:
                                encryption  =  "Unknown";
                                break;
                            }
                            html_gen = wifi_net_blocks.genHTML_button(ssid, rssi, encryption);
                            client.println(html_gen);
                            client.println();
                        }
                        break;
                    }
                    else {
                        currentLine = "";
                    }
                } else if (c != '\r') {
                    currentLine += c;
                }
                    }
                }

                
            }
        }
        client.stop();
        WiFi.scanDelete();
        client.stop();
        
    }
}