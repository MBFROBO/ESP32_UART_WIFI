#pragma once
#include "wifi_net_blocks.h"

String WifiNetBlocks::genHTML_button(String SSID, String RSSI, String Ecrypt_type) {
    String html = "<button id="+ SSID +" class=WiFi_data onClick=callback(this)><table><tr><td>" + RSSI + "</td><td id=SSID>" + SSID + "</td><td>" + Ecrypt_type + "</td></tr></table></button>";
    return html;
}

String WifiNetBlocks::genJS_script_data(String data, String variable_name)  {
    String js = "<script> var " + variable_name + " = \""  + data + "\"</script>";
    return js;
}

String http_parse::parse_login(String data) {
    return data;
}
String http_parse::parse_passw(String data) {
    return data;
}

String http_parse::parse(char data,char buffer[]) {
    int size = strlen(buffer);
    if (data == '\n') {
        String buff = String(buffer);
        memset(buffer, 0, 128);
        return buff;
    } else {
        buffer[size] = data;
    }
    return String(0);
}

char * WifiNetBlocks::removeLineBreak(char *str) 
{ 
    int i = 0, j = 0; 
    
    while (str[i]) 
    { 
        if (str[i] != '\n') 
        str[j++] = str[i]; 
        i++; 
    } 
    str[j] = '\0'; 
    return str; 
} 



uint16_t uart_parse::crc16(uint8_t *buf, int len) {
    if(!buf || !len) {return 0;}
   
    unsigned short crc = 0;
    while (len--) {
        crc = (crc>> 8) ^ crc16_table[(unsigned char)crc ^ *buf++];
    
    }
    return crc;
}

unsigned int uart_parse::pocket_number(char *buf) {
    /*
        Вчисляем номер поулченного пакета и возвращаем его в счётчик  
    */
};

unsigned int uart_parse::black_box_count_pockets(char *buf) {
    /*
        Вчисляем размер чёрного ящикая
    */
};

unsigned short uart_parse::black_box_package(int counter, char *buffer2){
    unsigned char array[255];
    byte _buff[] = {0x40, 0x40, 0x00, 0x00, (byte)counter};
    Serial1.write(_buff, sizeof(_buff));

    while (counter < 255) {
        if (Serial1.available()) {
        Serial1.readBytes(buffer2, 512);
        int i = 511;
        while (buffer2[i] == 0x00) {
            i--;
        }
        unsigned char local_buffer[i+1];
        if (sizeof(local_buffer) > 0) {
            for (int j = 0; j < sizeof(local_buffer); j++) {
                local_buffer[j] = buffer2[j];
            }
            for (int j = 0; j < sizeof(local_buffer); j++) {
                char hexString[3];
                sprintf(hexString, "%02X", local_buffer[j]);
                Serial.print(hexString);
                Serial.print(" ");
            }
            array[counter] = *local_buffer;
        }

        Serial.println();
        memset(buffer2, 0, 512);
        
        return *array;
    }
    }
    
 
};