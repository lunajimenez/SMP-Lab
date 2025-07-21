#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

/// --- Configuración Wi-Fi ---
const char* ssid = "Wokwi-GUEST";      //  nombre red Wi-Fi
const char* password = "";            // contraseña Wi-Fi

// --- Configuración MQTT ---
const char* mqtt_server = "p49a8ed4.ala.us-east-1.emqxsl.com"; // Host broker
const int mqtt_port = 8883;                       // Puerto broker
const char* clientID = "nodoUTB";                 // ID de cliente MQTT único
const char* mqtt_username = "domotica2025_UTB";   //usuario MQTT
const char* mqtt_password = "123456";             // contraseña MQTT
// Si broker no requiere usuario/contraseña, dejar vacíos: ""
// o usar client.connect(clientID) en la función reconnect_mqtt()

// ----- certificado CA broker EMQX   ---
const char* ca_cert = \
"-----BEGIN CERTIFICATE-----\n" \
"MIIDrzCCApegAwIBAgIQCDvgVpBCRrGhdWrJWZHHSjANBgkqhkiG9w0BAQUFADBh\n" \
"MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n" \
"d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBD\n" \
"QTAeFw0wNjExMTAwMDAwMDBaFw0zMTExMTAwMDAwMDBaMGExCzAJBgNVBAYTAlVT\n" \
"MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j\n" \
"b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IENBMIIBIjANBgkqhkiG\n" \
"9w0BAQEFAAOCAQ8AMIIBCgKCAQEA4jvhEXLeqKTTo1eqUKKPC3eQyaKl7hLOllsB\n" \
"CSDMAZOnTjC3U/dDxGkAV53ijSLdhwZAAIEJzs4bg7/fzTtxRuLWZscFs3YnFo97\n" \
"nh6Vfe63SKMI2tavegw5BmV/Sl0fvBf4q77uKNd0f3p4mVmFaG5cIzJLv07A6Fpt\n" \
"43C/dxC//AH2hdmoRBBYMql1GNXRor5H4idq9Joz+EkIYIvUX7Q6hL+hqkpMfT7P\n" \
"T19sdl6gSzeRntwi5m3OFBqOasv+zbMUZBfHWymeMr/y7vrTC0LUq7dBMtoM1O/4\n" \
"gdW7jVg/tRvoSSiicNoxBN33shbyTApOB6jtSj1etX+jkMOvJwIDAQABo2MwYTAO\n" \
"BgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUA95QNVbR\n" \
"TLtm8KPiGxvDl7I90VUwHwYDVR0jBBgwFoAUA95QNVbRTLtm8KPiGxvDl7I90VUw\n" \
"DQYJKoZIhvcNAQEFBQADggEBAMucN6pIExIK+t1EnE9SsPTfrgT1eXkIoyQY/Esr\n" \
"hMAtudXH/vTBH1jLuG2cenTnmCmrEbXjcKChzUyImZOMkXDiqw8cvpOp/2PV5Adg\n" \
"06O/nVsJ8dWO41P0jmP6P6fbtGbfYmbW0W5BjfIttep3Sp+dWOIrWcBAI+0tKIJF\n" \
"PnlUkiaY4IBIqDfv8NZ5YBberOgOzW6sRBc4L0na4UU+Krk2U886UAb3LujEV0ls\n" \
"YSEY1QSteDwsOoBrp+uvFRTp2InBuThs4pFsiv9kuXclVzDAGySj4dzp30d8tbQk\n" \
"CAUw7C29C79Fv1C5qfPrmAESrciIxpg0X40KPMbp1ZWVbd4=\n" \
"-----END CERTIFICATE-----\n";

// Tópicos MQTT
const char* led_control_topic = "esp32/led/control"; // Tópico al que se suscribirá el esp32
const char* esp32_user_input_topic = "esp32/userInput/data";// Tópico al que se publicará el esp32

// --- Configuración Hardware ---
const int LED_PIN = 2;
bool led_status = LOW;

// --- Variables Globales ---
WiFiClientSecure espClientSecure; // WiFiClientSecure
PubSubClient client(espClientSecure); // Pasa cliente seguro a PubSubClient

unsigned long lastPublishTime = 0;
const long publishInterval = 3000;
unsigned int counter = 0;

// --- Función de Conexión Wi-Fi ---
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Conectando a ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  int retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries < 30) {
    delay(500);
    Serial.print(".");
    retries++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("");
    Serial.println("WiFi conectado");
    Serial.println("Dirección IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("");
    Serial.println("Falló la conexión Wi-Fi.");
    while(true) { delay(1000); }
  }
}

// --- Función Callback para Mensajes MQTT Entrantes ---
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Mensaje recibido [");
  Serial.print(topic);
  Serial.print("] ");

  String messageTemp;
  for (int i = 0; i < length; i++) {
    messageTemp += (char)payload[i];
  }
  Serial.println(messageTemp);

  if (String(topic) == led_control_topic) {
    if (messageTemp == "1") {
      Serial.println("Comando Recibido: Encender LED");
      digitalWrite(LED_PIN, HIGH);
      led_status = HIGH;
    } else if (messageTemp == "0") {
      Serial.println("Comando Recibido: Apagar LED");
      digitalWrite(LED_PIN, LOW);
      led_status = LOW;
    } else {
      Serial.println("Comando no reconocido para el LED.");
    }
  }
}

// --- Función de Reconexión MQTT --- 
void reconnect_mqtt() {
  while (!client.connected()) {
    Serial.print("Intentando conexión MQTT segura...");
    if (client.connect(clientID, mqtt_username, mqtt_password)) {
      Serial.println("conectado al broker MQTT (TLS/SSL).");
      client.subscribe(led_control_topic);
      Serial.print("Suscrito a: ");
      Serial.println(led_control_topic);
    } else {
      Serial.print("falló la conexión MQTT, rc=");
      Serial.print(client.state());
      Serial.println(" intentando de nuevo en 5 segundos");
      
      // Imprimir error de SSL si está disponible (útil para depurar certificados)
      char ssl_error_buf[256];
      espClientSecure.lastError(ssl_error_buf, sizeof(ssl_error_buf));
      Serial.print("Último error SSL: ");
      Serial.println(ssl_error_buf);
      
      delay(5000);
    }
  }
}

// --- Configuración Inicial ---
void setup() {
  Serial.begin(115200);
  while (!Serial);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  setup_wifi();

  if (WiFi.status() == WL_CONNECTED) {
    // Configurar el certificado CA para el cliente seguro ANTES de conectar
    espClientSecure.setCACert(ca_cert);
    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(callback);
    Serial.println("----------------------------------------------------");
    Serial.println("Cliente MQTT listo para recibir comandos para el LED.");
    Serial.println("Escribe un mensaje en esta terminal y presiona Enter");
    Serial.println("para enviarlo por MQTT al tópico: " + String(esp32_user_input_topic));
    Serial.println("----------------------------------------------------");
  } else {
    Serial.println("No se pudo conectar a Wi-Fi, MQTT no se iniciará.");
  }
}

// --- Bucle Principal ---
void loop() {

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wi-Fi desconectado.");
    delay(1000);
    // setup_wifi(); // Considera una estrategia de reconexión Wi-Fi más robusta
    return;
  }

  if (!client.connected()) {
    reconnect_mqtt();
  }
  if (!client.connected()){
      delay(1000);
      return;
  }
  
  client.loop();

  // Verificar si hay datos disponibles en el Monitor Serie
  if (Serial.available() > 0) {
    // Leer la cadena de caracteres hasta que se presione Enter
    String userInput = Serial.readStringUntil('\n');
    
    // Limpiar espacios en blanco al inicio/final (incluyendo el carácter de nueva línea)
    userInput.trim();

    if (userInput.length() > 0) {
      Serial.print("Texto recibido del Serial: '");
      Serial.print(userInput);
      Serial.println("'");

      Serial.print("Publicando en MQTT al tópico '");
      Serial.print(esp32_user_input_topic);
      Serial.print("': ");
      Serial.println(userInput);

      // Publicar el mensaje. El método publish toma const char*,
      // así que usamos .c_str() para convertir el objeto String.
      if (client.publish(esp32_user_input_topic, userInput.c_str())) {
        Serial.println("Mensaje MQTT enviado exitosamente.");
      } else {
        Serial.println("Error al enviar mensaje MQTT.");
      }
    }
  }

}