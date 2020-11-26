//  LoRa-Sensor für Heizöltank oder Zisterne
//  Arduino MKR1310, Ultraschallsensor AJ-SR04M
//  Version 1.7 für Make:Magazin 6/2020
//  Arduino-Entwicklungs-Umgebung 1.8.12
//  Robert Kränzlein 24.10.2020

#include <MKRWAN.h>            // Version 1.0.12
#include <NewPing.h>           // Version 1.8 modifiziert
#include <ArduinoLowPower.h>   // Version 1.2.1

#define TRIGGER_PIN  8         // Arduino Pin für Trigger des Ultraschallsensore
#define ECHO_PIN     9         // Arduino Pin für Echo Ultraschallsensor
#define MAX_DISTANCE 220       // maximale Entfernung für den Ultraschallsensor
#define Tiefe 200              // Tiefe des zu messenden Behälters
#define Oberkante 25           // Abstand von maximalem Füllstand zur Sensorunterkante
#define Liter 5000             // Inhalt des Behälters bei maximaler Füllmenge
#define Schlaf 3600000         // Wartezeit zwischen zwei Messungen in Millisekunden 60 Min * 60 Sekunden * 1000 Millisekunden = 3600000, Testumgebung 2 Minuten = 120000
#define SPANNUNG A2            // Arduino Pin für Spannungsmessung der Batterie
#define TRANSJF1 10            // Verbunden mit dem JFET um den Strom des Ultraschallsensors abzuschalten
#define TRANSJF2 11            // Reserve zur Ansteuerung zweiter JFET

LoRaModem modem;                                    // Definition LoRaModem
NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE); // Definition des Ultraschallsensors in NewPing-Bibliothek

String appEui = "IhrAppEui";                 // siehe TTN-Console
String appKey = "IhrAppKey";                 // siehe TTN-Console
int luft = 0;           // Variable luft enthält die gemessene Entfernung von Sensor zur Flüssigkeitsoberfläche
int inhalt = 0;         // berechneter Inhalt des Behälters  
int volt100 = 370;      // Messwerte für Batteriespannung * 100 um zwei Nachkommastellen im Integer darstellen zu können
int volt101 = 370;
int volt102 = 370;
int i;                  // Zählervariable

void setup() {
  // Serial.begin(115200);        // alle Serial-Befehle werden im Produktivbetrieb als Kommentar ausser Betrieb genommen
  analogReadResolution(12);       // Sonderbefehl der MKR-Familie um die Auflösung der Analogeingänge zu definieren
  pinMode(LED_BUILTIN, OUTPUT);   // Pin-Definitionen als Ein/Ausgang, hier Onboard-LED
  pinMode(TRANSJF1, OUTPUT);      // Schalter für JFET
  pinMode(TRANSJF2, OUTPUT);      // Schalter für JFET2
  pinMode(SPANNUNG, INPUT);       // Eingang Spannungsmessung
  digitalWrite(TRANSJF1, HIGH);   // HIGH schalter JFET ein 
  digitalWrite(TRANSJF2, HIGH);   // LOW schalter aus
  // while (!Serial);
  if (!modem.begin(EU868)) {      // warten auf Bereitschaft des LoRa-Modems - ausserhalb Europas US915, AS923, etc verwenden
    // Serial.println("Failed to start module");
    while (1) {}
  };
  digitalWrite(LED_BUILTIN, LOW);    // interne LED auf MKR-Board ausschalten
  // Serial.print("Ihre LoRa modul version: ");
  // Serial.println(modem.version());
  // Serial.print("Ihre EUI lautet: ");  
  // die serielle Kommunikation muß zum Beginn des Aufbaus aktiviert werden, da die EUI zur Einrichtung im TTN benötigt wird
  // Serial.println(modem.deviceEUI());

  int connected = modem.joinOTAA(appEui, appKey);
  if (!connected) {                  // Fehlermeldung falls keine Verbindung zustande kommt
    // Serial.println("Keine Verbindung zum LoRa Gateway gefunden");
    for (int i=0; i <= 255; i++){    // schnelles Blinken als Fehlermeldung
      delay(300);  
      digitalWrite(LED_BUILTIN, HIGH);
      delay(300);        
      digitalWrite(LED_BUILTIN, LOW);
    }
  }
  for (int i=0; i <= 15; i++){       // beim ersten Start 15 Sekunden warten um neue Softwareversionen aufspielen zu können bevor DeepSleep einsetzt
    delay(1000);          
  }
}

void loop() {
  digitalWrite(LED_BUILTIN, HIGH);        // zweimal blinken der Onboard-LED um auf Start der Messung hinzuweisen
  delay(500);                             // kann im Produktivbetrieb auskommentiert werden
  digitalWrite(LED_BUILTIN, LOW);
  delay(500); 
  digitalWrite(LED_BUILTIN, HIGH);
  delay(500);    
  digitalWrite(LED_BUILTIN, LOW);
  delay(900); 
  
  digitalWrite(TRANSJF1, HIGH);           // Einschalten des JEFT-Transistors um den Ultraschallsensor mit Strom zu versorgen
  digitalWrite(TRANSJF2, HIGH); 
  delay(100);
  
  volt100 = analogRead(SPANNUNG);         // Spannungsmessung. Es wird der Mittelwert aus drei Messungen gebildet
  delay(2);                               // gemessen wir durch den Spannungsteiler nur die halbe Batteriespannung
  volt101 = analogRead(SPANNUNG);
  delay(2);
  volt102 = analogRead(SPANNUNG);
  // Serial.print("Analog "); 
  // Serial.println(volt100 + "  " + volt101 + "  " + volt102);
  volt100 = (volt100+volt101+volt102) * 220 / 4095;    // *220 entspricht *660 / 3 wegen 3 Meßwerten
  // Serial.print("Volt ");
  // Serial.println(volt100);  

  i = 0;                               // Zähler auf 0 setzen
  do {
    luft = sonar.ping_cm();            // Entfernungsmessung
    i++;                               // Zähler i um 1 erhöhen
    delay(150);                        // kurz warten
  } while ((luft < 20)&&(i < 6));      // die Messung wird maximal 5 mal wiederholt oder bis ein Meßwert größer 20 erreicht wird
  
  if (luft > 0) {
    // Serial.print("Ping: ");
    // Serial.print(luft); 
    // Serial.println("cm");
    inhalt = ( ( ( (Tiefe - Oberkante) - (luft - Oberkante)) * Liter ) / (Tiefe - Oberkante) );   // Berechnung des Inhalts in Liter aus der Entfernung
    // Serial.print("Inhalt: ");
    // Serial.print(inhalt); 
    // Serial.println(" Liter");
  }
  else {
    // Serial.println("Fehlmessung");
    inhalt = 1;                       // Wenn keine sinnvolle Messung zustand kam wird 1 als Meßwert übergeben
  }

   digitalWrite(TRANSJF1, LOW);       // die beiden JFETs sperren - Spannung Ultraschall abschalten und Spannungsteiler
   digitalWrite(TRANSJF2, LOW);
  

  modem.beginPacket();                // Zusammenstellen des LoRa_Datenpaketes
  modem.write(luft);
  modem.write(inhalt);
  modem.write(volt100);
  
  int err;
  err = modem.endPacket(true);       // LoRa-Datenpaket abschließen und senden
  if (err > 0) {                     // Verarbeitung des zurückgegebenden Fehlercodes des LoRa-Modems
    // Serial.println("Message sent correctly!");
 } else {
    // Serial.println("Error sending message :(");
    // Serial.println("(you may send a limited amount of messages per minute, depending on the signal strength");
    // Serial.println("it may vary from 1 message every couple of seconds to 1 message every minute)");
  }
   

   USBDevice.detach();           // Verbindung zum USB-Device trennen - Zeile auskommentieren solange Testbetrieb und Änderungen vorgenommen werden
   LowPower.deepSleep(Schlaf);   // Arduino in Tiefschlafmodus versetzen, der Delay-Befehl dahinter ist notwendig damit dies auch tatsächlich funktioniert
   delay(1000);

}
