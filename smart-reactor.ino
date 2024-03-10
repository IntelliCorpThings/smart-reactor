#include <CO2Sensor.h>
#include <SPI.h>
#include <Ethernet.h>      // Biblioteca Ethernet
#include <PubSubClient.h>  // Biblioteca MQTT
#include "max6675.h"
#include "HX711.h"
#include <DHT.h>

#define DHTPIN 7
#define DHTTYPE DHT22

// Temperatura INTERNA
int ktcSO = 4;   //PINO DIGITAL (SO)
int ktcCS = 5;   //PINO DIGITAL (CS)
int ktcCLK = 6;  //PINO DIGITAL (CLK / SCK)
MAX6675 ktc(ktcCLK, ktcCS, ktcSO);

// Pressão
uint8_t clockPinPressureSensorTop = A4;
uint8_t dataPinPressureSensorTop = A5;
HX711 scale_top;

// Brix
uint8_t clockPinPressureSensorMiddle = A0;
uint8_t dataPinPressureSensorMiddle = A1;
HX711 scale_middle;

uint8_t clockPinPressureSensorBottom = A2;
uint8_t dataPinPressureSensorBottom = A3;
HX711 scale_bottom;

//CO2
CO2Sensor co2Sensor(A7, 0.99, 100);

// Temperatura EXTERNA e UMIDADE
DHT dht(DHTPIN, DHTTYPE);

char tempex[33];  //Stores temperature value
char tempin[33];  //Stores temperature internal value
char pressure_top[33];
char pressure_bottom[33];
char pressure_middle[33];
char carbon[33];

// MQTT / FIWARE - ETHERNET
void callback(char* topic, byte* payload, unsigned int length);

//Ethernet
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };  // Endereço MAC da placa Ethernet
IPAddress server(34, 95, 217, 0);                     // Endereço IP do servidor MQTT (substitua pelo seu servidor)
EthernetClient ethClient;
PubSubClient client(server, 1883, callback, ethClient);

//Callback para lidar com mensagens recebidas
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Tópico: ");
  Serial.println(topic);
  Serial.print("Mensagem recebida: ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void setup() {
  Serial.begin(9600);
  Ethernet.begin(mac);

  dht.begin();

  co2Sensor.calibrate();

  scale_top.begin(dataPinPressureSensorTop, clockPinPressureSensorTop);
  scale_middle.begin(dataPinPressureSensorMiddle, clockPinPressureSensorMiddle);
  scale_bottom.begin(dataPinPressureSensorBottom, clockPinPressureSensorBottom);

  scale_top.set_scale(2280.f);  // this value is obtained by calibrating the scale with known weights; see the README for details
  scale_middle.set_scale(2280.f); 
  scale_bottom.set_scale(2280.f);

  scale_top.tare();
  scale_middle.tare();
  scale_bottom.tare();

  // Conecte-se ao servidor MQTT
  while (!client.connected()) {
    if (client.connect("ArduinoClient")) {
      Serial.println("Conectado ao servidor MQTT");
      client.subscribe("/TEF/winetest2/attrs/ti");
      client.subscribe("/TEF/winetest2/attrs/te");
      client.subscribe("/TEF/winetest2/attrs/p1");
      client.subscribe("/TEF/winetest2/attrs/p2");
      client.subscribe("/TEF/winetest2/attrs/pt");
      client.subscribe("/TEF/winetest2/attrs/co");
      Serial.println("Inscrito");
    } else {
      Serial.println("Falha na conexão com o servidor MQTT. Tentando novamente em 5 segundos...");
      delay(5000);
    }
  }
}

void loop() {
  // Verifique a conexão com o servidor MQTT
  if (!client.connected()) {
    Serial.println("Conexão perdida. Tentando reconectar...");
    if (client.connect("ArduinoClient")) {
      Serial.println("Reconectado ao servidor MQTT");
      client.subscribe("/TEF/winetest2/attrs/ti");
      client.subscribe("/TEF/winetest2/attrs/te");
      client.subscribe("/TEF/winetest2/attrs/p1");
      client.subscribe("/TEF/winetest2/attrs/p2");
      client.subscribe("/TEF/winetest2/attrs/pt");
      client.subscribe("/TEF/winetest2/attrs/co");
    }
  }

  delay(2000);
  dtostrf(ktc.readCelsius(), 5, 2, tempin);
  dtostrf(dht.readTemperature(), 5, 2, tempex);
  dtostrf(co2Sensor.read(), 5, 2, carbon);

  Serial.print("Temperatura Interna: ");
  Serial.println(tempin);
  Serial.print("Temperatura Externa: ");
  Serial.println(tempex);
  Serial.print("Pressão: ");
  Serial.println(pressure_top);
  Serial.print("CO2: ");
  Serial.println(carbon);

  //Envia mensagem MQTT
  client.publish("/TEF/wine001/attrs/ti", tempin);
  delay(1000);
  client.publish("/TEF/wine001/attrs/te", tempex);
  delay(1000);
  client.publish("/TEF/wine001/attrs/pt", pressure_top);
  delay(1000);
  client.publish("/TEF/wine001/attrs/pt", pressure_bottom);
  delay(10);
  client.publish("/TEF/wine001/attrs/pt", pressure_middle);
  delay(10);
  client.publish("/TEF/wine001/attrs/co", carbon);
  delay(1000);
  Serial.println("---");

  scale_top.power_down();  // put the ADC in sleep mode
  scale_middle.power_down();  // put the ADC in sleep mode
  scale_bottom.power_down();  // put the ADC in sleep mode

  delay(5000);

  scale_top.power_up();
  scale_middle.power_up();
  scale_bottom.power_up();

  // Verifique se há mensagens recebidas
  client.loop();
  delay(5000);
}