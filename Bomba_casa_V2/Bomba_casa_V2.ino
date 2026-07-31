#include <WiFi.h>
#include <WebServer.h>

// Configuración WiFi
const char* ssid = "JacintoLopez";
const char* password = "_LbJc01031719*";
WebServer server(80);

// Definición de pines
const int SELECTOR_BOMBA1 = 17;      // Selector físico para bomba 1
const int SELECTOR_BOMBA2 = 18;      // Selector físico para bomba 2
const int FLOTADOR_CISTERNA = 19;    // Flotador de cisterna (HIGH = llena, LOW = vacía)
const int FLOTADOR_TINACO = 21;      // Flotador de tinaco (HIGH = lleno, LOW = vacío)
const int SENSOR_FLUJO = 22;         // Sensor de flujo (HIGH = hay flujo, LOW = no hay flujo)
const int SENSOR_PRESION = 23;       // Sensor de presión (HIGH = presión alta, LOW = presión normal)
const int RELE_BOMBA1 = 1;          // Relé bomba 1
const int RELE_BOMBA2 = 2;          // Relé bomba 2
const int RELE_BOMBA3 = 3;          // Relé bomba 1
const int RELE_BOMBA4 = 4;          // Relé bomba 2
const int RELE_BOMBA5 = 5;          // Relé bomba 1
const int RELE_BOMBA6 = 12;          // Relé bomba 2

// Variables de estado
bool bomba1_encendida = false;       // Estado actual de la bomba 1
bool bomba2_encendida = false;       // Estado actual de la bomba 2
bool ultimo_selector1 = LOW;         // Último estado del selector 1
bool ultimo_selector2 = LOW;         // Último estado del selector 2
unsigned long tiempo_inicio_flujo = 0; // Tiempo cuando se inició la verificación de flujo
bool verificando_flujo = false;      // Estado de verificación de flujo
bool bloqueo_bomba1 = false;         // Bloqueo para requerer cambio en selector
bool bloqueo_bomba2 = false;         // Bloqueo para requerer cambio en selector

void setup() {
  Serial.begin(115200);
  
  // Configuración de pines
  pinMode(SELECTOR_BOMBA1, INPUT);
  pinMode(SELECTOR_BOMBA2, INPUT);
  pinMode(FLOTADOR_CISTERNA, INPUT);
  pinMode(FLOTADOR_TINACO, INPUT);
  pinMode(SENSOR_FLUJO, INPUT);
  pinMode(SENSOR_PRESION, INPUT);
  pinMode(RELE_BOMBA1, OUTPUT);
  pinMode(RELE_BOMBA2, OUTPUT);
  pinMode(RELE_BOMBA3, OUTPUT);
  pinMode(RELE_BOMBA4, OUTPUT);
  pinMode(RELE_BOMBA5, OUTPUT);
  pinMode(RELE_BOMBA6, OUTPUT);
  
  // Inicializar relés en estado apagado
  digitalWrite(RELE_BOMBA1, LOW); // Apagado
  digitalWrite(RELE_BOMBA2, LOW); // Apagado
  digitalWrite(RELE_BOMBA3, LOW); // Apagado
  digitalWrite(RELE_BOMBA4, LOW); // Apagado
  digitalWrite(RELE_BOMBA5, LOW); // Apagado
  digitalWrite(RELE_BOMBA6, LOW); // Apagado
  
  // Conectar a WiFi
  WiFi.begin(ssid, password);
  Serial.print("Conectando a WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Conectado a WiFi, IP: ");
  Serial.println(WiFi.localIP());
  
  // Configurar rutas del servidor web
  server.on("/", HTTP_GET, handleRoot);
  server.on("/bomba1on", HTTP_GET, handleBomba1On);
  server.on("/bomba1off", HTTP_GET, handleBomba1Off);
  server.on("/bomba2on", HTTP_GET, handleBomba2On);
  server.on("/bomba2off", HTTP_GET, handleBomba2Off);
  server.on("/status", HTTP_GET, handleStatus);
  
  // Iniciar servidor web
  server.begin();
  Serial.println("Servidor HTTP iniciado");
}

void loop() {
  server.handleClient();
  
  // Control de Bomba 1 con selector físico
  if (digitalRead(SELECTOR_BOMBA1) == HIGH && ultimo_selector1 == LOW && !bloqueo_bomba1) {
    // Transición de LOW a HIGH en selector 1
    encenderBomba1();
  } else if (digitalRead(SELECTOR_BOMBA1) == LOW && ultimo_selector1 == HIGH) {
    // Transición de HIGH a LOW en selector 1
    apagarBomba1();
    bloqueo_bomba1 = false; // Desbloquear cuando bajamos el selector
  }
  ultimo_selector1 = digitalRead(SELECTOR_BOMBA1);
  
  // Control de Bomba 2 con selector físico
  if (digitalRead(SELECTOR_BOMBA2) == HIGH && ultimo_selector2 == LOW && !bloqueo_bomba2) {
    // Transición de LOW a HIGH en selector 2
    encenderBomba2();
  } else if (digitalRead(SELECTOR_BOMBA2) == LOW && ultimo_selector2 == HIGH) {
    // Transición de HIGH a LOW en selector 2
    apagarBomba2();
    bloqueo_bomba2 = false; // Desbloquear cuando bajamos el selector
  }
  ultimo_selector2 = digitalRead(SELECTOR_BOMBA2);
  
  // Lógica de protección para Bomba 1
  if (bomba1_encendida) {
    // 1. Si la cisterna se llena mientras está en funcionamiento la bomba 1
    if (digitalRead(FLOTADOR_CISTERNA) == HIGH) {
      Serial.println("Cisterna llena, apagando bomba 1");
      apagarBomba1();
      bloqueo_bomba1 = true;
      verificando_flujo = false;
    }
    // 2. Si se detecta alta presión mientras está en funcionamiento la bomba 1
    else if (digitalRead(SENSOR_PRESION) == HIGH) {
      Serial.println("Alta presión detectada, apagando bomba 1");
      apagarBomba1();
      bloqueo_bomba1 = true;
      verificando_flujo = false;
    }
    // 3. Control de flujo después de encendido
    else if (!verificando_flujo) {
      tiempo_inicio_flujo = millis();
      verificando_flujo = true;
      Serial.println("Iniciando verificación de flujo por 120 segundos");
    }
    else if (verificando_flujo && ((millis() - tiempo_inicio_flujo) > 120000)) {
      verificando_flujo = false;
      if (digitalRead(SENSOR_FLUJO) == LOW) {
        Serial.println("No se detecta flujo después de 120 segundos, apagando bomba 1");
        apagarBomba1();
        bloqueo_bomba1 = true;
      } else {
        Serial.println("Flujo detectado, bomba 1 sigue operando");
        // Seguimos verificando flujo continuamente
        verificando_flujo = true;
        tiempo_inicio_flujo = millis();
      }
    }
  }
  
  // Lógica de protección para Bomba 2 - Independiente de la bomba 1
  if (bomba2_encendida) {
    // Si el tinaco se llena mientras está en funcionamiento la bomba 2
    if (digitalRead(FLOTADOR_TINACO) == HIGH) {
      Serial.println("Tinaco lleno, apagando bomba 2");
      apagarBomba2();
      bloqueo_bomba2 = true;
    }
  }
  
  delay(100); // Pequeña pausa para estabilidad
}

// Funciones de control de bombas
void encenderBomba1() {
  // Solo enciende si la cisterna no está llena
  if (digitalRead(FLOTADOR_CISTERNA) == LOW) {
    Serial.println("Encendiendo bomba 1");
    digitalWrite(RELE_BOMBA1, HIGH); // Encendido con HIGH
    bomba1_encendida = true;
    verificando_flujo = false; // Reiniciar verificación de flujo
  } else {
    Serial.println("No se puede encender bomba 1, cisterna llena");
  }
}

void apagarBomba1() {
  Serial.println("Apagando bomba 1");
  digitalWrite(RELE_BOMBA1, LOW); // Apagado con LOW
  bomba1_encendida = false;
  verificando_flujo = false;
}

void encenderBomba2() {
  // Solo enciende si el tinaco no está lleno
  if (digitalRead(FLOTADOR_TINACO) == LOW) {
    Serial.println("Encendiendo bomba 2");
    digitalWrite(RELE_BOMBA2, HIGH); // Encendido con HIGH
    bomba2_encendida = true;
  } else {
    Serial.println("No se puede encender bomba 2, tinaco lleno");
  }
}

void apagarBomba2() {
  Serial.println("Apagando bomba 2");
  digitalWrite(RELE_BOMBA2, LOW); // Apagado con LOW
  bomba2_encendida = false;
}

// Manejadores para servidor web
void handleRoot() {
  String html = "<html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<meta http-equiv='refresh' content='5'>";  // Actualizar cada 5 segundos
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; margin: 20px; }";
  html += ".container { max-width: 600px; margin: 0 auto; }";
  html += ".status { margin-bottom: 20px; padding: 10px; border: 1px solid #ddd; }";
  html += ".controls { display: flex; justify-content: space-between; margin-top: 20px; }";
  html += ".control-group { width: 48%; }";
  html += "button { width: 100%; padding: 10px; margin: 5px 0; cursor: pointer; }";
  html += ".on { background-color: #4CAF50; color: white; border: none; }";
  html += ".off { background-color: #f44336; color: white; border: none; }";
  html += "</style>";
  html += "</head><body>";
  html += "<div class='container'>";
  html += "<h1>Control de Bombas</h1>";
  
  html += "<div class='status'>";
  html += "<h2>Estado Actual</h2>";
  html += "<p>Bomba 1: " + String(bomba1_encendida ? "ENCENDIDA" : "APAGADA") + "</p>";
  html += "<p>Bomba 2: " + String(bomba2_encendida ? "ENCENDIDA" : "APAGADA") + "</p>";
  html += "<p>Cisterna: " + String(digitalRead(FLOTADOR_CISTERNA) == HIGH ? "LLENA" : "VACIA") + "</p>";
  html += "<p>Tinaco: " + String(digitalRead(FLOTADOR_TINACO) == HIGH ? "LLENO" : "VACIO") + "</p>";
  html += "<p>Flujo de agua: " + String(digitalRead(SENSOR_FLUJO) == HIGH ? "DETECTADO" : "NO DETECTADO") + "</p>";
  html += "<p>Presion: " + String(digitalRead(SENSOR_PRESION) == HIGH ? "ALTA" : "NORMAL") + "</p>";
  html += "</div>";
  
  html += "<div class='controls'>";
  html += "<div class='control-group'>";
  html += "<h3>Bomba 1 (Cisterna)</h3>";
  html += "<a href='/bomba1on'><button class='on'>ENCENDER</button></a>";
  html += "<a href='/bomba1off'><button class='off'>APAGAR</button></a>";
  html += "</div>";
  
  html += "<div class='control-group'>";
  html += "<h3>Bomba 2 (Tinaco)</h3>";
  html += "<a href='/bomba2on'><button class='on'>ENCENDER</button></a>";
  html += "<a href='/bomba2off'><button class='off'>APAGAR</button></a>";
  html += "</div>";
  html += "</div>";
  
  html += "</div></body></html>";
  
  server.send(200, "text/html", html);
}

void handleBomba1On() {
  // La web puede encender independientemente del selector físico
  if (digitalRead(FLOTADOR_CISTERNA) == LOW) {
    digitalWrite(RELE_BOMBA1, HIGH);
    bomba1_encendida = true;
    verificando_flujo = false;
    tiempo_inicio_flujo = millis();
    Serial.println("Bomba 1 encendida vía web");
  } else {
    Serial.println("No se puede encender bomba 1 vía web, cisterna llena");
  }
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleBomba1Off() {
  digitalWrite(RELE_BOMBA1, LOW);
  bomba1_encendida = false;
  verificando_flujo = false;
  Serial.println("Bomba 1 apagada vía web");
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleBomba2On() {
  // La web puede encender independientemente del selector físico
  if (digitalRead(FLOTADOR_TINACO) == LOW) {
    digitalWrite(RELE_BOMBA2, HIGH);
    bomba2_encendida = true;
    Serial.println("Bomba 2 encendida vía web");
  } else {
    Serial.println("No se puede encender bomba 2 vía web, tinaco lleno");
  }
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleBomba2Off() {
  digitalWrite(RELE_BOMBA2, LOW);
  bomba2_encendida = false;
  Serial.println("Bomba 2 apagada vía web");
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleStatus() {
  String status = "{";
  status += "\"bomba1\":" + String(bomba1_encendida ? "true" : "false") + ",";
  status += "\"bomba2\":" + String(bomba2_encendida ? "true" : "false") + ",";
  status += "\"cisterna\":" + String(digitalRead(FLOTADOR_CISTERNA) == HIGH ? "\"llena\"" : "\"vacia\"") + ",";
  status += "\"tinaco\":" + String(digitalRead(FLOTADOR_TINACO) == HIGH ? "\"lleno\"" : "\"vacio\"") + ",";
  status += "\"flujo\":" + String(digitalRead(SENSOR_FLUJO) == HIGH ? "true" : "false") + ",";
  status += "\"presion\":" + String(digitalRead(SENSOR_PRESION) == HIGH ? "\"alta\"" : "\"normal\"") + ",";
  status += "\"verificando_flujo\":" + String(verificando_flujo ? "true" : "false") + ",";
  status += "\"tiempo_verificacion\":" + String(verificando_flujo ? (millis() - tiempo_inicio_flujo) / 1000 : 0);
  status += "}";
  
  server.send(200, "application/json", status);
}