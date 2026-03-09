#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoOTA.h>
#include <EEPROM.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

const char* ssid = "YOUR SSID";
const char* password = "PASSWORD";

ESP8266WebServer server(80);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", -10800, 60000); // UTC-3

unsigned long previousMillis = 0;
const long interval = 5000; // Intervalo de 5 segundos

const int relePin = D1;

// Variables de horarios y modo
int hi1 = 0, mi1 = 0;
int hf1 = 0, mf1 = 0;
bool modoAutomatico = false;
bool estadoManual = false; // true si el relé está en modo manual

struct Configuracion {
  int hi1;
  int mi1;
  int hf1;
  int mf1;
  bool modoAutomatico;
  bool configValida; // Nuevo campo para verificar la validez de la configuración
};

Configuracion config;

void guardarConfiguracion() {
  config.configValida = true; // Marcar la configuración como válida
  EEPROM.put(0, config);
  EEPROM.commit();
}

void cargarConfiguracion() {
  EEPROM.get(0, config);
  if (!config.configValida) {
    // Inicializar con valores por defecto
    config.hi1 = 0;
    config.mi1 = 0;
    config.hf1 = 0;
    config.mf1 = 0;
    config.modoAutomatico = false;
    guardarConfiguracion();
  }
}

void controlarReleAutomaticamente() {
  if (!config.modoAutomatico) return;

  timeClient.update();
  int horaActual = timeClient.getHours();
  int minutoActual = timeClient.getMinutes();

  int actualMin = horaActual * 60 + minutoActual;
  int onMin = config.hi1 * 60 + config.mi1;
  int offMin = config.hf1 * 60 + config.mf1;

  bool encender = false;

  if (onMin < offMin) {
    encender = (actualMin >= onMin && actualMin < offMin);
  } else {
    encender = (actualMin >= onMin || actualMin < offMin);
  }

  digitalWrite(relePin, encender ? HIGH : LOW);
}

void handleSetHorario() {
  if (server.hasArg("hi1")) hi1 = server.arg("hi1").toInt();
  if (server.hasArg("mi1")) mi1 = server.arg("mi1").toInt();
  if (server.hasArg("hf1")) hf1 = server.arg("hf1").toInt();
  if (server.hasArg("mf1")) mf1 = server.arg("mf1").toInt();

  guardarConfiguracion(); // No guardar el modo automático

  config.hi1 = hi1;
  config.mi1 = mi1;
  config.hf1 = hf1;
  config.mf1 = mf1;

  Serial.printf("Recibidos - ON: %02d:%02d | OFF: %02d:%02d\n", hi1, mi1, hf1, mf1);
  server.send(200, "text/plain", "Horarios guardados");
}

void handleModoAutomatico() {
  if (server.hasArg("auto")) {
    config.modoAutomatico = server.arg("auto") == "1";
    estadoManual = false;
    guardarConfiguracion();
    controlarReleAutomaticamente(); // Actualiza de inmediato
    server.send(200, "text/plain", config.modoAutomatico ? "Modo automático activado" : "Modo automático desactivado");
  } else {
    server.send(400, "text/plain", "Falta argumento 'auto'");
  }
}

void handleEstado() {
  bool estado = digitalRead(relePin);
  String mensaje = modoAutomatico ? "MODO AUTOMATICO" : (estado ? "ON" : "OFF");
  server.send(200, "text/plain", mensaje);
}

void setupOTA() {
  ArduinoOTA.begin();
  Serial.println("OTA Ready");
}

void setup() {
  Serial.begin(115200);
  pinMode(relePin, OUTPUT);
  digitalWrite(relePin, LOW); // Apagado inicial

  EEPROM.begin(512);
  cargarConfiguracion();

  timeClient.begin();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConectado a WiFi. IP: " + WiFi.localIP().toString());

  server.on("/on", []() {
  config.modoAutomatico = false;
  estadoManual = true;
  guardarConfiguracion();
  digitalWrite(relePin, HIGH);
  server.send(200, "text/plain", "ON");
});

server.on("/off", []() {
  config.modoAutomatico = false;
  estadoManual = true;
  guardarConfiguracion();
  digitalWrite(relePin, LOW);
  server.send(200, "text/plain", "OFF");
});

  server.on("/estado", handleEstado);
  server.on("/setHorario", HTTP_POST, handleSetHorario);
  server.on("/modo", HTTP_GET, handleModoAutomatico);

  server.begin();
  setupOTA();

  //controlarReleAutomaticamente();
}

void loop() {
  ArduinoOTA.handle();
  server.handleClient();
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    controlarReleAutomaticamente();
  }
}