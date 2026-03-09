# ESP8266 Web Relay Controller

Sistema de control de relés mediante **ESP8266 (Wemos D1 Mini)** con interfaz web, comunicación entre dispositivos y automatización por horarios.

Este proyecto permite controlar dispositivos eléctricos de forma remota a través de WiFi utilizando microcontroladores ESP8266. El sistema utiliza una arquitectura cliente-servidor donde un ESP8266 actúa como controlador principal y otro como servidor de relé.

---

## Características

* Interfaz web para control remoto
* Comunicación HTTP entre dispositivos ESP8266
* Control manual de relés
* Automatización por horarios
* Sincronización de hora mediante NTP
* Configuración WiFi desde la interfaz web
* Almacenamiento de configuración en EEPROM
* Actualización remota de firmware (OTA)

---

## Arquitectura del sistema

El sistema utiliza dos dispositivos ESP8266:

Usuario
↓
ESP8266 Cliente (Interfaz Web)
↓ HTTP Requests
ESP8266 Servidor
↓
Relé
↓
Dispositivo eléctrico

---

## Hardware utilizado

* 2 × Wemos D1 Mini (ESP8266)
* 2 x Módulo relé
* Fuente de alimentación 5V
* Conexión WiFi

---

## Instalación

1. Instalar Arduino IDE
2. Agregar soporte para ESP8266 en el Board Manager
3. Instalar las siguientes librerías:
    * ESP8266WiFi
    * ESP8266WebServer
    * EEPROM
    * NTPClient
    * WiFiUdp
    * ArduinoOTA

4. Cargar:

* `Wemos_Cliente_Final.ino` en el ESP8266 cliente
* `Wemos_Servidor_Final.ino` en el ESP8266 servidor

5. Conectar ambos dispositivos a la misma red WiFi.

---

## Uso

1. Acceder a la interfaz web del ESP8266 cliente desde el navegador.
2. Desde la interfaz se puede:
    * Encender y apagar relés
    * Configurar horarios automáticos
    * Controlar relé remoto
    * Configurar red WiFi

3. El sistema también puede funcionar automáticamente según los horarios configurados.

---

## Fotos del proyecto

Agregar aquí fotos del circuito y del sistema funcionando.

---

## Video demostración

Agregar aquí el link del video mostrando el funcionamiento del proyecto.

---

## Tecnologías utilizadas

* ESP8266
* Arduino IDE
* C++
* HTTP
* WiFi
* NTP

---

## Autor

Proyecto desarrollado por Mauri como proyecto personal de IoT.
