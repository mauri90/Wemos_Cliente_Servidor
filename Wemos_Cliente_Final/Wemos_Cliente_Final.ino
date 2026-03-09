#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <NTPClient.h>
#include <EEPROM.h>

const char* serverIP = "192.168.18.48";  // Cambia esto por la IP del Wemos servidor

ESP8266WebServer server(80);  // Usamos ESP8266WebServer para la web

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", -10800, 60000);  // UTC-3 Argentina

String header;
String D1State = "off";
String D2State = "off";

#define RELAY1_PIN D1

#define EEPROM_SIZE 96
#define SSID_ADDR 10
#define PASS_ADDR 42

String storedSSID = "";
String storedPASS = "";

// Variables de horario automático
int horaInicio1, minutoInicio1, horaFin1, minutoFin1;
bool horarioGuardado = false;  // Variable para mostrar el mensaje

// Guardar y leer de EEPROM
void guardarHorarioEEPROM() {
  if (horaInicio1 != EEPROM.read(0) || minutoInicio1 != EEPROM.read(1) || horaFin1 != EEPROM.read(2) || minutoFin1 != EEPROM.read(3))
    EEPROM.write(0, horaInicio1);
    EEPROM.write(1, minutoInicio1);
    EEPROM.write(2, horaFin1);
    EEPROM.write(3, minutoFin1);

    EEPROM.commit();
}

void cargarHorarioEEPROM() {
  horaInicio1 = EEPROM.read(0);
  minutoInicio1 = EEPROM.read(1);
  horaFin1 = EEPROM.read(2);
  minutoFin1 = EEPROM.read(3);

}

void enviarHorariosAlRemoto(int hi1, int mi1, int hf1, int mf1) {
  WiFiClient client;
  if (client.connect("192.168.18.48", 80)) {  // IP del Wemos remoto
    String datos = "hi1=" + String(hi1) +
                   "&mi1=" + String(mi1) +
                   "&hf1=" + String(hf1) +
                   "&mf1=" + String(mf1);

    client.print(String("POST /setHorario HTTP/1.1\r\n") +
                 "Host: 192.168.18.48\r\n" +
                 "Content-Type: application/x-www-form-urlencoded\r\n" +
                 "Content-Length: " + datos.length() + "\r\n\r\n" +
                 datos);

    Serial.println("Horarios enviados al remoto");
  } else {
    Serial.println("Error al conectar con el remoto");
  }
}

void guardarCredencialesWiFi(const String& ssid, const String& pass) {
  for (int i = 0; i < 32; ++i) {
    EEPROM.write(SSID_ADDR + i, i < ssid.length() ? ssid[i] : 0);
    EEPROM.write(PASS_ADDR + i, i < pass.length() ? pass[i] : 0);
  }
  EEPROM.commit();
}

void cargarCredencialesWiFi() {
  char ssid[33], pass[33];
  for (int i = 0; i < 32; ++i) {
    ssid[i] = EEPROM.read(SSID_ADDR + i);
    pass[i] = EEPROM.read(PASS_ADDR + i);
  }
  ssid[32] = 0; pass[32] = 0;
  storedSSID = String(ssid);
  storedPASS = String(pass);
}

void setupWiFi() {
  cargarCredencialesWiFi();
  if (storedSSID.length() == 0 || storedSSID == String(0)) {
    iniciarModoConfiguracion();  // No hay datos guardados
  } else {
    WiFi.mode(WIFI_STA);
    WiFi.begin(storedSSID.c_str(), storedPASS.c_str());
    Serial.print("Conectando a WiFi: ");
    Serial.println(storedSSID);
    int intentos = 0;
    while (WiFi.status() != WL_CONNECTED && intentos < 20) {
      delay(500);
      Serial.print(".");
      intentos++;
    }

    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("Fallo de conexión. Entrando en modo configuración.");
      iniciarModoConfiguracion();
    } else {
      Serial.println("\nWiFi conectado: " + WiFi.localIP().toString());
    }
  }
}

void iniciarModoConfiguracion() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP("Config_ESP8266");

  server.on("/", HTTP_GET, mostrarFormularioWiFi);
  server.on("/save", HTTP_POST, guardarDatosWiFi);
  server.begin();

  Serial.println("Modo configuración iniciado. Conectarse a 'Config_ESP8266'.");

  while (true) {
    server.handleClient();
    delay(10);
  }
}

void mostrarFormularioWiFi() {
  int n = WiFi.scanNetworks();
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>Configuración WiFi</title>";
  html += "<style>body{font-family:Arial;background:black;color:white;text-align:center;padding:20px;}form{background:black;padding:20px;border-radius:10px;display:inline-block;}input,select{margin:10px;padding:10px;width:90%;max-width:300px;}input[type=submit]{background:#4CAF50;color:white;border:none;cursor:pointer;}input[type=submit]:hover{background:#45a049;}</style></head><body>";
  html += "<h2>Configurar WiFi</h2><form action='/save' method='post'>";
  html += "SSID: <select name='ssid'>";
  for (int i = 0; i < n; ++i) {
    html += "<option value='" + WiFi.SSID(i) + "'>" + WiFi.SSID(i) + " (" + WiFi.RSSI(i) + "dBm)</option>";
  }
  html += "</select><br>";
  html += "Contraseña: <input type='password' name='pass'><br>";
  html += "<input type='submit' value='Guardar y conectar'><br>";
  server.send(200, "text/html", html);
};

void guardarDatosWiFi() {
  String ssid = server.arg("ssid");
  String pass = server.arg("pass");

  if (ssid.length() > 0 && pass.length() > 0) {
    guardarCredencialesWiFi(ssid, pass);
    server.send(200, "text/html", 
    "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>Reiniciando</title>"
    "<style>body{background:black;color:white;text-align:center;font-family:Arial;padding:50px;}h1{color:lime;}</style>"
    "</head><body><h1>Reiniciando...</h1><p>Por favor espere unos segundos.</p></body></html>");
    delay(2000);
    ESP.restart();
  } else {
    server.send(200, "text/html", 
    "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>Error</title>"
    "<style>body{background:black;color:white;text-align:center;font-family:Arial;padding:50px;}h1{color:red;}</style>"
    "</head><body><h1>Faltan datos</h1><p>Por favor complete todos los campos e intente de nuevo.</p></body></html>");
  }
}

void borrarCredencialesWiFi() {
  for (int i = 0; i < 32; ++i) {
    EEPROM.write(SSID_ADDR + i, 0);
    EEPROM.write(PASS_ADDR + i, 0);
  }
  EEPROM.commit();
  server.send(200, "text/html",
    "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>Reiniciando</title>"
    "<style>body{background:black;color:white;text-align:center;font-family:Arial;padding:50px;}h1{color:orange;}</style>"
    "</head><body><h1>Credenciales borradas</h1><p>Reiniciando en modo configuración...</p></body></html>");
  delay(2000);
  ESP.restart();
}

void setupOTA() {
  ArduinoOTA.begin();
  Serial.println("OTA Ready");
}

void setup() {
  Serial.begin(115200);
  pinMode(RELAY1_PIN, OUTPUT);
  digitalWrite(RELAY1_PIN, LOW);

  EEPROM.begin(EEPROM_SIZE);
  cargarHorarioEEPROM();

  setupWiFi();
  setupOTA();
  timeClient.begin();
  server.begin();

  // Configurar rutas
  server.on("/", HTTP_GET, handleRoot);
  server.on("/D1/on", HTTP_GET, handleD1On);
  server.on("/D1/off", HTTP_GET, handleD1Off);
  server.on("/D2/on", HTTP_GET, handleD2On);
  server.on("/D2/off", HTTP_GET, handleD2Off);
  server.on("/configurar", HTTP_GET, handleConfigurar);
  server.on("/hora", HTTP_GET, handleHora);
  server.on("/borrar", HTTP_GET, borrarCredencialesWiFi);
}

void handleRoot() {
  String horaActual = timeClient.getFormattedTime();
  String fechaActual = obtenerFecha();
  int currentHour = timeClient.getHours();
  int currentMinute = timeClient.getMinutes();

  // Control automático para relay 1
  bool auto1 = enRangoAutomatico(currentHour, currentMinute, horaInicio1, minutoInicio1, horaFin1, minutoFin1);
  if (auto1) {
    digitalWrite(RELAY1_PIN, HIGH);
    D1State = "auto";
  } else if (D1State == "auto") {
    digitalWrite(RELAY1_PIN, LOW);
    D1State = "off";
  }

  String page = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
  page += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  page += "<style>html{font-family:Helvetica;text-align:center;background:black;color:white;}"; 
  page += ".button{background:#3364FF;border:none;color:white;padding:16px 40px;font-size:30px;margin:2px;cursor:pointer;}"; 
  page += ".button2{background:#FF3333;} .disabled{background:gray;color:white;padding:16px 40px;font-size:30px;margin:2px;border:none;}"; 
  page += "</style>";
  page += "<script>";
  page += "function hideMessage() {";
  page += "  var message = document.getElementById('mensajeGuardado');";
  page += "  message.style.display = 'none';";
  page += "}";
  page += "window.onload = function() {";
  page += "  setTimeout(hideMessage, 5000);";  // Ocultar el mensaje después de 5 segundos
  page += "  setInterval(updateTime, 1000);";  // Actualiza la hora cada segundo
  page += "};";

  page += "function updateTime() {";
  page += "  var hora = new Date();";
  page += "  var horas = hora.getHours().toString().padStart(2, '0');";
  page += "  var minutos = hora.getMinutes().toString().padStart(2, '0');";
  page += "  var segundos = hora.getSeconds().toString().padStart(2, '0');";
  page += "  document.getElementById('hora').innerHTML = horas + ':' + minutos + ':' + segundos;";
  page += "}";
  page += "</script></head><body><h1>ESP8266 Web Server</h1>";
  page += "<p>Fecha actual: <strong>" + fechaActual + "</strong></p>";
  page += "<p>Hora actual: <span id='hora'>" + horaActual + "</span></p>";
  page += "<p>Luz De Adelante - Estado " + (auto1 ? "Control Automático" : D1State) + "</p>";

  page += auto1
    ? "<p><button class='disabled' disabled>Control Automático</button></p>"
    : (D1State == "off"
      ? "<p><a href='/D1/on'><button class='button'>Encender</button></a></p>"
      : "<p><a href='/D1/off'><button class='button button2'>Apagar</button></a></p>");

  page += "<p>Luz De Atrás - Estado " + (auto1 ? "Control Automático" : D2State) + "</p>";

  page += auto1
    ? "<p><button class='disabled' disabled>Control Automático</button></p>"
    : (D2State == "off"
      ? "<p><a href='/D2/on'><button class='button'>Encender</button></a></p>"
      : "<p><a href='/D2/off'><button class='button button2'>Apagar</button></a></p>");

  page += "<h2>Configurar Horario Automático</h2>";
  page += "<form action='/configurar' method='get'>";
  page += "<h3>Luces:</h3>";
  page += "Inicio: <input type='number' name='hi1' min='0' max='23' value='" + String(horaInicio1) + "'>:<input type='number' name='mi1' min='0' max='59' value='" + String(minutoInicio1) + "'><br>";
  page += "Fin: <input type='number' name='hf1' min='0' max='23' value='" + String(horaFin1) + "'>:<input type='number' name='mf1' min='0' max='59' value='" + String(minutoFin1) + "'><br><br>";
  page += "<input class='button' type='submit' name='guardar1' value='Guardar Horario'>";
  // Mostrar mensaje si se guardó el horario
  if (horarioGuardado) {
    page += "<p id='mensajeGuardado' style='color:green;'>✔ Se ha guardado exitosamente</p>";
    horarioGuardado = false;  // Resetear el mensaje después de mostrarlo
  }
  page += "</form>";

  page += "<p><a href='/borrar'><button class='button button2'>Borrar WiFi y Reconfigurar</button></a></p>";

  server.send(200, "text/html", page);  // Usamos send en lugar de client.println
}

void handleD1On() {
  digitalWrite(RELAY1_PIN, HIGH);
  D1State = "on";
  server.sendHeader("Location", "/");  // Redirige a la página principal
  server.send(303);  // Código de estado para redirección
}

void handleD1Off() {
  digitalWrite(RELAY1_PIN, LOW);
  D1State = "off";
  server.sendHeader("Location", "/");  // Redirige a la página principal
  server.send(303);  // Código de estado para redirección
}

void handleD2On() {
  HTTPClient http;
  WiFiClient client;
  String url = "http://" + String(serverIP) + "/on"; // Asegurate que serverIP sea la IP del Wemos remoto
  http.begin(client, url);
  int httpCode = http.GET();
  http.end();
  D2State = "on";
  server.sendHeader("Location", "/");
  server.send(303);  // Redirige a la página principal
}

void handleD2Off() {
  HTTPClient http;
  WiFiClient client;
  String url = "http://" + String(serverIP) + "/off"; // Asegurate que serverIP sea la IP del Wemos remoto
  http.begin(client, url);
  int httpCode = http.GET();
  http.end();
  D2State = "off";
  server.sendHeader("Location", "/");
  server.send(303);  // Redirige a la página principal
}

void handleConfigurar() {
  String params = server.arg("hi1") + "," + server.arg("mi1") + "," + server.arg("hf1") + "," + server.arg("mf1");

  int hi1 = server.arg("hi1").toInt();
  int mi1 = server.arg("mi1").toInt();
  int hf1 = server.arg("hf1").toInt();
  int mf1 = server.arg("mf1").toInt();

  if (hi1 >= 0 && mi1 >= 0 && hf1 >= 0 && mf1 >= 0) {
    horaInicio1 = hi1;
    minutoInicio1 = mi1;
    horaFin1 = hf1;
    minutoFin1 = mf1;
    guardarHorarioEEPROM();  // Guarda los horarios
    horarioGuardado = true;  // Habilita el mensaje de éxito
    enviarHorariosAlRemoto(hi1, mi1, hf1, mf1);// Enviar al servidor remoto
  }

  server.sendHeader("Location", "/");  // Redirige a la página principal
  server.send(303);  // Código de estado para redirección
}

void handleHora() {
  String horaActual = timeClient.getFormattedTime();
  server.send(200, "text/plain", horaActual);  // Solo hora como texto
}

void loop() {
  ArduinoOTA.handle();
  timeClient.update();

  // Verifica el estado del relé y lo ajusta según el horario actual
  int currentHour = timeClient.getHours();
  int currentMinute = timeClient.getMinutes();
  bool auto1 = enRangoAutomatico(currentHour, currentMinute, horaInicio1, minutoInicio1, horaFin1, minutoFin1);

  if (auto1) {
    if (D1State != "auto") {
      digitalWrite(RELAY1_PIN, HIGH);
      D1State = "auto";
      HTTPClient http;
      WiFiClient client;
      String url = "http://" + String(serverIP) + "/modo?auto=1"; // Asegurate que serverIP sea la IP del Wemos remoto
      http.begin(client, url);
      int httpCode = http.GET();
      http.end();
      D2State = "auto";
      server.sendHeader("Location", "/");
      server.send(303);  // Redirige a la página principal
    }
  } else if (D1State == "auto") {
    digitalWrite(RELAY1_PIN, LOW);
    D1State = "off";
    HTTPClient http;
    WiFiClient client;
    String url = "http://" + String(serverIP) + "/off"; // Asegurate que serverIP sea la IP del Wemos remoto
    http.begin(client, url);
    int httpCode = http.GET();
    http.end();
    D2State = "off";
    server.sendHeader("Location", "/");
    server.send(303);  // Redirige a la página principal
  }

  server.handleClient();  // Manejar solicitudes del cliente
}

String obtenerFecha() {
  time_t rawTime = timeClient.getEpochTime();
  struct tm *tiempo = localtime(&rawTime);
  char fecha[11];
  sprintf(fecha, "%02d/%02d/%04d", tiempo->tm_mday, tiempo->tm_mon + 1, tiempo->tm_year + 1900);
  return String(fecha);
}

bool enRangoAutomatico(int h, int m, int hi, int mi, int hf, int mf) {
  int actual = h * 60 + m;
  int inicio = hi * 60 + mi;
  int fin = hf * 60 + mf;
  if (inicio < fin) {
    return actual >= inicio && actual < fin;
  } else {
    return actual >= inicio || actual < fin;
  }
}