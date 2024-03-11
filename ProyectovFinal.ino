#include <WiFi.h>
#include "time.h"
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include "PinDefinitionsAndMore.h"  //Configuración y definición de pines
#include <IRremote.hpp>
#include <DHT.h>  //https://github.com/adafruit/DHT-sensor-library
#include "HTTPClient.h"
#include "HTTP_Method.h"
// CONF DHT11
int DHTPIN = 2;
int WifiLED = 5;
int signalLED = 16;
#define DHTTYPE DHT11  // DHT 11
DHT dht(DHTPIN, DHTTYPE);
float tActual = 0.0;
float t = 0.0;
// CONF WIFI
#define WIFI_SSID "INFINITUM7F36"
//INFINITUM7F36
//bXMBS345TR
#define WIFI_PASSWORD "bXMBS345TR"
//Token de Telegram BOT se obtenienen desde Botfather en telegram
#define BOT_TOKEN "5939861092:AAHVRCQfBbgEfNMVJw0sa4uroxwxfqOl0h4"
const unsigned long tiempo = 1000;  //tiempo medio entre mensajes de escaneo
WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);
unsigned long tiempoAnterior, tiempoUltimaLectura;  //última vez que se realizó el análisis de mensajes
int inicio = 1;

//Variables para google sheet
String datos;
String stringT;
bool setPoint;
int aireEstado = 0;  // 1 = encendido /// 0 = apagado
bool band;
// Conf para obtener la hora
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -21600;
const int daylightOffset_sec = 0;
String chat_id;
#define ID_Chat "-939434757"  //ID_Chat se obtiene de telegram
// Variables para google sheet
HTTPClient http;
String respuesta;
//Booleans para mantener el control de los modos automatico, estandar y envio de datos.
bool automatico = false;
bool manual = false;
bool googleSheet = false;
bool activo = true;

struct tm timeinfo;
int hour = timeinfo.tm_hour;
void mensajesNuevos(int numerosMensajes) {
  for (int i = 0; i < numerosMensajes; i++) {
    String chat_id = bot.messages[i].chat_id;
    String text = bot.messages[i].text;
    //////////Comando para encender el aire acondicionado//////
    if (text == "/encender") {
      encenderAire();
      confirmSignal();
      delay(3000);
      setTemperaturaMinima();
      aireEstado = 1;
      bot.sendMessage(chat_id, "Comando encendido ejecutado", "");
    }
    //////////Comando para apagar el aire acondicionado////////
    if (text == "/apagar") {
      apagarAire();
      confirmSignal();
      aireEstado = 0;
      bot.sendMessage(chat_id, "Comando apagado ejecutado", "");
    }
    if (text == "/estado") {
      ////Ultimo comando enviado////
      if (aireEstado == 0) {
        bot.sendMessage(chat_id, "El aire está apagado.", "");
      }
      if (aireEstado == 1) {
        bot.sendMessage(chat_id, "El aire está encendido.", "");
      }
    }
    if (text == "/temperatura") {
      String msj = "La temperatura en el laboratorio es de: " + String(tActual);
      bot.sendMessage(chat_id, msj, "");
    }
    if (text == "/automatico") {
      String msj = "Modo automático ON de 10:00 AM a 16:59 PM ";
      bot.sendMessage(chat_id, msj, "");
      automatico = true;
      manual = false;
    }
    if (text == "/manual") {
      String msj = "Modo manual ON.";
      bot.sendMessage(chat_id, msj, "");
      manual = true;
      automatico = false;
    }
    if (text == "/ONGoogleSheets") {
      String msj = "Los datos del sensor serán enviados a Google Sheets.";
      bot.sendMessage(chat_id, msj, "");
      googleSheet = true;
    }
    if (text == "/OFFGoogleSheets") {
      String msj = "Ningún dato será enviado a Google Sheet.";
      bot.sendMessage(chat_id, msj, "");
      googleSheet = false;
    }
    if (text == "/iniciar") {
      String ayuda = "Bienvenidos :) \n";
      // String keyboardJson = "[[\"/encender\n\", \"/apagar\n\", \"/ayuda\", \"/estado\", \"/temperatura\", \"/automatico\", \"/manual\", \"/habilitarGoogleSheets\", \"/deshabilitarGoogleSheets\"]]";
      String keyboardJson = "[[\"/ayuda\"]";
      keyboardJson += ",[\"/encender\", \"/apagar\"]";
      keyboardJson += ",[\"/estado\", \"/temperatura\"]";
      keyboardJson += ",[\"/automatico\", \"/manual\"]";
      keyboardJson += ",[\"/ONGoogleSheets\", \"/OFFGoogleSheets\"]";
      keyboardJson += "]";
      bot.sendMessageWithReplyKeyboard(chat_id, "Selecciona una de las siguientes opciones", "", keyboardJson, true);
    }
    if (text == "/ayuda") {
      String ayuda = "";
      ayuda += "Estas son tus opciones.\n\n";
      ayuda += "/encender: para encender el aire \n";
      ayuda += "/apagar: para apagar el aire \n";
      ayuda += "/ayuda: Imprime este menú. \n";
      ayuda += "/estado: Permite saber el estado del aire acondicionado. \n";
      ayuda += "/temperatura: Muestra la temperatura actual en el laboratorio. \n";
      ayuda += "/automatico: Habilita el modo automático de 8 AM a 14 PM. \n";
      ayuda += "/manual: Habilita el modo manual. \n";
      ayuda += "/habilitarGoogleSheets: Manda los datos de la temperatura del lab a Google Sheets. \n";
      ayuda += "/deshabilitarGoogleSheets: Deja de mandar datos a Google Sheets. \n";
      ayuda += "Recuerda el sistema distingue entre mayuculas y minusculas \n";
      bot.sendMessage(chat_id, ayuda, "");
    }
  }
}

void encenderAire() {
  const uint16_t irSignal[199] = { 4388, 4332, 540, 1626, 542, 546, 540, 1630, 568,
                                   1600, 568, 520, 538, 548, 544, 1622, 566, 524, 542, 548, 540, 1628, 544, 544,
                                   566, 522, 544, 1622, 542, 1626, 542, 546, 564, 1608, 568, 1606, 544, 544,
                                   566, 1604, 542, 1624, 544, 1624, 568, 1598, 542, 1626, 566, 1604, 542, 546,
                                   542, 1626, 570, 516, 540, 546, 540, 546, 564, 520, 542, 542, 570, 520, 566,
                                   522, 540, 546, 564, 522, 542, 544, 566, 520, 566, 520, 566, 522, 562, 524,
                                   542, 1626, 544, 1622, 544, 1626, 566, 1602, 540, 1626, 542, 1624, 564, 1602,
                                   542, 1624, 538, 5194, 4396, 4332, 544, 1624, 566, 522, 568, 1602, 568, 1598,
                                   542, 544, 540, 546, 540, 1624, 544, 546, 542, 546, 544, 1624, 542, 546, 542,
                                   546, 538, 1626, 544, 1624, 542, 544, 568, 1604, 568, 1606, 542, 546, 562, 1604,
                                   572, 1596, 542, 1626, 542, 1626, 540, 1626, 568, 1602, 566, 522, 542, 1622, 570,
                                   518, 542, 546, 540, 544, 540, 546, 566, 522, 542, 546, 542, 546, 538, 546, 540,
                                   546, 542, 544, 564, 522, 544, 544, 540, 544, 544, 544, 542, 1626, 542, 1624, 542, 1626, 566, 1600, 542, 1624, 542, 1624, 544, 1624, 544, 1626, 536 };
  IrSender.sendRaw(irSignal, 199, 38);
}

void apagarAire() {
  const uint16_t irSignal2[199] = { 4354, 4388, 524, 1640, 530, 558, 528, 1644,
                                    528, 1710, 458, 582, 506, 554, 558, 1610, 532, 582, 506, 582, 506, 1642, 528,
                                    558, 526, 562, 528, 1640, 528, 1636, 584, 504, 530, 1642, 532, 558, 528, 1638,
                                    530, 1636, 532, 1638, 528, 1638, 532, 556, 530, 1652, 518, 1640, 556, 1628,
                                    522, 554, 530, 556, 532, 556, 530, 580, 506, 1634, 558, 530, 532, 558, 532,
                                    1640, 558, 1610, 532, 1636, 532, 556, 528, 558, 532, 554, 530, 554, 534, 556,
                                    534, 556, 530, 558, 530, 556, 534, 1638, 532, 1634, 532, 1636, 558, 1620, 518,
                                    1638, 528, 5228, 4358, 4370, 534, 1634, 534, 554, 532, 1640, 532, 1636, 532, 556,
                                    530, 554, 532, 1634, 534, 556, 532, 568, 546, 1612, 534, 564, 522, 554, 556, 1610,
                                    558, 1620, 520, 562, 552, 1612, 558, 530, 532, 1634, 534, 1634, 532, 1640, 528,
                                    1634, 532, 554, 532, 1638, 534, 1636, 534, 1638, 562, 526, 560, 528, 530, 556,
                                    558, 530, 530, 1634, 532, 556, 532, 554, 534, 1638, 536, 1634, 536, 1630, 534, 554,
                                    532, 554, 532, 564, 522, 552, 560, 538, 526, 556, 532, 624, 460, 556, 532, 1638, 534, 1634, 532, 1632, 534, 1634, 534, 1636, 532 };
  IrSender.sendRaw(irSignal2, 199, 38);
}

void setTemperaturaMinima() {
  const uint16_t irSignal4[199] = { 4382, 4424, 510, 1648, 526, 588, 506, 1652,
                                    528, 1676, 504, 562, 502, 590, 530, 1648, 530, 588, 532, 538, 530, 1652,
                                    532, 564, 530, 584, 508, 1650, 526, 1672, 508, 586, 506, 1652, 530, 1680,
                                    508, 558, 558, 1654, 502, 1652, 504, 1696, 504, 1676, 506, 1648, 554, 1628,
                                    530, 588, 504, 1674, 504, 588, 504, 588, 504, 588, 506, 586, 506, 564, 528,
                                    590, 504, 592, 506, 586, 506, 586, 480, 590, 504, 586, 532, 562, 528, 588,
                                    506, 590, 504, 1664, 516, 1648, 534, 1648, 502, 1678, 526, 1674, 478, 1696,
                                    536, 1620, 530, 1650, 530, 5256, 4358, 4420, 508, 1670, 506, 562, 504, 1680,
                                    528, 1676, 502, 564, 532, 562, 528, 1652, 530, 564, 530, 590, 478, 1678, 534,
                                    586, 506, 564, 530, 1672, 508, 1672, 506, 562, 532, 1678, 480, 1682, 502, 588,
                                    556, 1626, 530, 1674, 506, 1672, 506, 1674, 502, 1676, 502, 1678, 504, 566, 530,
                                    1672, 504, 564, 502, 612, 504, 566, 530, 564, 526, 590, 504, 590, 508, 588, 530,
                                    562, 556, 536, 502, 590, 480, 588, 528, 562, 558, 538, 528, 588, 480, 1700, 502,
                                    1650, 532, 1648, 532, 1646, 530, 1672, 506, 1650, 528, 1650, 504, 1700, 506 };
  IrSender.sendRaw(irSignal4, 199, 38);
}

void confirmSignal() {
  digitalWrite(signalLED, HIGH);  // turn the LED on
  delay(150);                     // wait for 500 milliseconds
  digitalWrite(signalLED, LOW);   // turn the LED off
  delay(150);
}
void setup() {
  pinMode(WifiLED, OUTPUT);
  pinMode(signalLED, OUTPUT);
  Serial.begin(115200);
  // Intenta conectarse a la red wifi
  Serial.print("Conectando a la red ");
  Serial.print(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  secured_client.setCACert(TELEGRAM_CERTIFICATE_ROOT);  //Agregar certificado raíz para api.telegram.org
  dht.begin();
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(WifiLED, HIGH);  // turn the LED on
    delay(250);                   // wait for 500 milliseconds
    digitalWrite(WifiLED, LOW);   // turn the LED off
    delay(250);
  }
  activo = true;
  digitalWrite(WifiLED, HIGH);
  Serial.print("\nConectado a la red wifi. Dirección IP: ");
  Serial.println(WiFi.localIP());
  if (inicio == 1) {
    Serial.println("Sistema preparado");
    bot.sendMessage(ID_Chat, "Sistema preparado!!!, escribe /iniciar para ver las opciones", "");  //Enviamos un mensaje a telegram para informar que el sistema está listo
    inicio = 0;
  }
  Serial.println("");
  Serial.println("\nConexion hecha");
  Serial.println("Direccion IP: ");
  Serial.println(WiFi.localIP());
  dht.begin();
  tActual = dht.readTemperature();
  Serial.println(tActual);
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}

void mandarDatosGoogleSheet() {
  if (WiFi.status() == WL_CONNECTED) {
    t = dht.readTemperature();
    if (isnan(t)) {
      Serial.println(F("Failed to read from DHT sensor!"));
      bot.sendMessage(ID_Chat, "Error al leer la temperatura... Verifica el sensor.", "");
    }
    String tem(t);
    String google = "https://script.google.com/macros/s/AKfycbye6tRSaIAjbI3AY9oHIxotlCooJyz8X-zH4L8ahcI7uAp8DnStnT8NwuAc1pcp2r1f/exec?";
    String final = google + "temperatura=" + String(t);
    Serial.println(final);
    getRequest(final);
  }
}

void getRequest(String serverName) {
  HTTPClient http;
  http.begin(serverName.c_str());
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  //Enviamos peticion HTTP
  int httpResponseCode = http.GET();

  String payload = "...";

  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
    Serial.println(payload);
  } else {
    Serial.print("Error: ");
    Serial.println(httpResponseCode);
  }
  http.end();
}

void controlAutomatico() {
  do {
    if (WiFi.status() != WL_CONNECTED) {
      digitalWrite(WifiLED, LOW);  // turn the LED off
    } else {
      hour = timeinfo.tm_hour;
      if (!getLocalTime(&timeinfo)) {
        Serial.println("Failed to obtain time");
      }
      if (hour >= 18 && aireEstado == 1) {
        bot.sendMessage(ID_Chat, "En caso de que lo hayas olvidado, apagaré el aire. Nos vemos mañana a las 10 AM :)", "");
        apagarAire();
        aireEstado = 0;
        bot.sendMessage(ID_Chat, "Aire apagado.", "");
        activo = false;
        loop();
      }
      //600000 = 10 minutos
      if (millis() - tiempoUltimaLectura > 600000) {
        tActual = dht.readTemperature();
        Serial.println(tActual);
        tiempoUltimaLectura = millis();
        if (googleSheet)
          mandarDatosGoogleSheet();
      }
      if (hour >= 17 && band == true) {
        apagarAire();
        bot.sendMessage(ID_Chat, "Aire apagado, hasta mañana:).", "");
        band = false;
        aireEstado = 0;
      }
      if (hour >= 10 && hour <= 16) {
        band = true;
        if (tActual >= 28 && aireEstado == 0) {
          bot.sendMessage(ID_Chat, "La temperatura es mayor a 28 grados, procederé a encender el aire.", "");
          encenderAire();
          setTemperaturaMinima();
          bot.sendMessage(ID_Chat, "Aire encendido.", "");
          aireEstado = 1;
        }
        if (tActual <= 26.5 && aireEstado == 1) {
          bot.sendMessage(ID_Chat, "La temperatura en el laboratorio es de 26 grados, procederé a apagar el aire.", "");
          apagarAire();
          bot.sendMessage(ID_Chat, "Aire apagado.", "");
          aireEstado = 0;
        }
      }
      if (millis() - tiempoAnterior > tiempo) {
        int numerosMensajes = bot.getUpdates(bot.last_message_received + 1);
        while (numerosMensajes) {
          Serial.println("Comando recibido");
          mensajesNuevos(numerosMensajes);
          numerosMensajes = bot.getUpdates(bot.last_message_received + 1);
        }
        tiempoAnterior = millis();
      }
    }

  } while (automatico);
}

void controlManual() {
  do {
    if (WiFi.status() != WL_CONNECTED) {
      digitalWrite(WifiLED, LOW);  // turn the LED off
    } else {
      hour = timeinfo.tm_hour;
      if (!getLocalTime(&timeinfo)) {
        Serial.println("Failed to obtain time");
      }
      if (googleSheet) {
        if (millis() - tiempoUltimaLectura > 600000) {
          tActual = dht.readTemperature();
          Serial.println(tActual);
          tiempoUltimaLectura = millis();
          if (googleSheet)
            mandarDatosGoogleSheet();
        }
      }
      if (hour >= 18 && aireEstado == 1) {
        bot.sendMessage(ID_Chat, "En caso de que lo hayas olvidado, apagaré el aire. :)", "");
        apagarAire();
        aireEstado = 0;
        bot.sendMessage(ID_Chat, "Aire apagado.", "");
        activo = false;
        loop();
      }
      if (millis() - tiempoAnterior > tiempo) {
        int numerosMensajes = bot.getUpdates(bot.last_message_received + 1);
        while (numerosMensajes) {
          Serial.println("Comando recibido");
          mensajesNuevos(numerosMensajes);
          numerosMensajes = bot.getUpdates(bot.last_message_received + 1);
        }
        tiempoAnterior = millis();
      }
    }
  } while (manual);
}
void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    digitalWrite(WifiLED, LOW);  // turn the LED off
  } else {
    hour = timeinfo.tm_hour;
    if (hour >= 1 && hour <= 6) {
      activo = true;
    }
    if (activo == true) {
      if (millis() - tiempoAnterior > tiempo) {
        int numerosMensajes = bot.getUpdates(bot.last_message_received + 1);
        while (numerosMensajes) {
          Serial.println("Comando recibido");
          mensajesNuevos(numerosMensajes);
          numerosMensajes = bot.getUpdates(bot.last_message_received + 1);
        }
        tiempoAnterior = millis();
      }
      if (manual)
        controlManual();
      else if (automatico)
        controlAutomatico();
      activo = true;
    }
  }
}
