
#include "FirebaseESP8266.h"
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <SoftwareSerial.h>
#include <ESP8266HTTPClient.h>

#include "config.h"

SoftwareSerial __uart(D5, D6); // (Rx, Tx)
FirebaseData firebaseData;
String line_break = "**************************************************************";
bool wifiConnected = false;
bool newCodeAdded = false;

void connectToWifi(void)
{
    Serial.println(line_break);
    Serial.println("CONNECTING TO WI-FI");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    for (int counter = 0; counter < WIFI_TIMEOUT_S; counter++)
    {
        Serial.print(".");
        wifiConnected = (WiFi.status() == WL_CONNECTED);
        if (wifiConnected)
        {
            Serial.println();
            Serial.print("CONNECTED TO WI-FI WITH IP: ");
            Serial.println(WiFi.localIP());
            Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
            Firebase.reconnectWiFi(true);
            Serial.println("CONNECTED TO FIREBASE");
            break;
        }
        delay(1000);
    }

    if (!wifiConnected)
    {
        Serial.println();
        Serial.println("COULD NOT CONNECT TO WI-FI");
    }
    Serial.println(line_break);
}

void check_website_try_connecting(void)
{
    String result;
    Firebase.getString(firebaseData, "/connection/status", result);
    if (result == "NOT CONNECTED")
    {
        Firebase.setString(firebaseData, "/connection/status", "CONNECTED");
    }
}

String getLine(int i)
{
    String line;
    while (Firebase.getString(firebaseData, "line/" + String(i), line) == false)
    {
        delay(1);
        Serial.println("GETTING LINE: INTERNET SLOW");
    }
    return line;
}

void sendCode(void)
{
    int lines_count;
    char receivedChar;
    String line;

    Serial.println("NEW CODE ADDED");

    /* Getting number of lines */
    Serial.println("GETTING NUMBER OF LINES");
    while (Firebase.getInt(firebaseData, "/code/lines", lines_count) == false)
    {
        Serial.println("GETTING NUMBER OF LINES: INTERNET SLOW");
        delay(1);
    }
    Serial.print("NUMBER OF LINES: ");
    Serial.println(lines_count);

    /* Sending start char and wait for confirm */
    Serial.println("SENDEING START CHAR");
    while (__uart.read() != OTA_READ_CONFIRM_CHAR)
    {
        __uart.write(OTA_DATA_START_CHAR);
        delay(1000);
    }

    Serial.println("SENDEING CODE");

    /* Sending each line */
    for (int i = 0; i < lines_count; i++)
    {
        /* Getting the line text */
        line = getLine(i);

        /* Convert string into array */
        int length = line.length() + 1;
        char line_array[length];
        line.toCharArray(line_array, length);

        /* Sending line */
        __uart.write(line_array);

        /* Sending line break char after line has ended */
        __uart.flush();
        __uart.write(OTA_LINE_BREAK_CHAR);
        while (__uart.read() != OTA_READ_CONFIRM_CHAR)
        {
            delay(1);
        }

        /* Line is now sent */
        Serial.println("LINE " + String(i + 1) + " SENT: " + line);
    }

    /* Lines has ended */
    __uart.write(OTA_DATA_END_CHAR);
    while (__uart.read() != OTA_READ_CONFIRM_CHAR)
    {
        delay(1);
    }

    Serial.println("LINES ENDED");

    /* Setting the code not new */
    while (Firebase.setBool(firebaseData, "/code/new", false) == false)
    {
        delay(1);
        Serial.println("INTERNET SLOW");
    }
    Serial.println("ALL CODE IS FLASHED");
    Serial.println(line_break);
}

void setup()
{

    Serial.begin(9600);
    __uart.begin(UART_BAUDRATE);
    connectToWifi();
}

void loop()
{
    if (wifiConnected)
    {
        Firebase.getBool(firebaseData, "/code/new", newCodeAdded);
        if (newCodeAdded)
        {
            sendCode();
            newCodeAdded = false;
        }

        check_website_try_connecting();
    }
    delay(500);
}
