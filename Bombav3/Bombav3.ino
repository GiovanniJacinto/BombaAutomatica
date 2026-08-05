#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>      // Reemplaza a ESPAsyncWebServer
#include <ArduinoJson.h>

//==============================
// Configuración WiFi
//==============================
const char* ssid     = "JacintoLopez";
const char* password = "_LbJc01031719*";

IPAddress local_IP(192, 168, 100, 200);
IPAddress gateway(192, 168, 100, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8);
IPAddress secondaryDNS(8, 8, 4, 4);

//==============================
// Declaración del Servidor Síncrono Oficial
//==============================
WebServer server(80);

//==============================
// Pines 
//==============================
const int SELECTOR_BOMBA1 =   15;
const int SELECTOR_BOMBA2 =   02;
const int FLOTADOR_CISTERNA = 04;
const int FLOTADOR_TINACO =   05;
const int SENSOR_FLUJO =      18;
const int SENSOR_PRESION =    19;
const int RELE_BOMBA1 = 32;
const int RELE_BOMBA2 = 33;
const int RELE_BOMBA3 = 27;
const int RELE_BOMBA4 = 14;
const int RELE_BOMBA5 = 12;
const int RELE_BOMBA6 = 13;

//==============================
// Estado compartido y Variables
//==============================
volatile bool bomba1_encendida = false;
volatile bool bomba2_encendida = false;
volatile bool ultimo_selector1 = LOW;
volatile bool ultimo_selector2 = LOW;
volatile bool estado_selector1 = LOW;
volatile bool estado_selector2 = LOW;
volatile unsigned long tiempo_inicio_flujo = 0;
volatile bool verificando_flujo = false;
volatile bool bloqueo_bomba1 = false;
volatile bool bloqueo_bomba2 = false;
volatile bool modo_manual_bomba1 = false;
volatile bool modo_manual_bomba2 = false;

portMUX_TYPE estadoMux = portMUX_INITIALIZER_UNLOCKED;
TaskHandle_t Task1;

//==============================
// Variables para Tiempo Extra (Web)
//==============================
volatile bool habilitar_tiempo_extra = false;
volatile unsigned long tiempo_extra_llenado = 5000; 

volatile bool temporizador_bomba1_activo = false;
volatile unsigned long tiempo_flotador1_alto = 0;

volatile bool temporizador_bomba2_activo = false;
volatile unsigned long tiempo_flotador2_alto = 0;

//==============================
// WiFi robusto y Auto-Reconexión
//==============================
unsigned long lastWifiCheck = 0;
const unsigned long WIFI_CHECK_MS = 5000;
uint8_t reconnectTries = 0;
const uint8_t MAX_TRIES_BEFORE_AP = 3;
bool apBackupActivo = false;

//==============================
// Prototipos
//==============================
void TaskControlHardware(void * pvParameters);
void startWiFiSTA();
void ensureWiFi();
void startAPBackup();
void stopAPBackup();
void configureWebServer();
void encenderBomba1();
void apagarBomba1();
void encenderBomba2();
void apagarBomba2();
void encenderBomba1Manual();
void apagarBomba1Manual();
void encenderBomba2Manual();
void apagarBomba2Manual();

//==============================
// Interfaz Web HTML
//==============================
const char ROOT_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang='es'>
<head>
<meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>
<title>Control de Bombas - Sistema Inteligente</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:'Segoe UI',Tahoma,Geneva,Verdana,sans-serif;background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);min-height:100vh;color:#333;line-height:1.6}
.container{max-width:1400px;margin:0 auto;padding:20px}
.header{text-align:center;color:#fff;margin-bottom:30px;padding:20px 0}
.header h1{font-size:2.5rem;font-weight:700;margin-bottom:10px;text-shadow:2px 2px 4px rgba(0,0,0,.3)}
.header p{font-size:1.2rem;opacity:.9}
.dashboard{display:grid;grid-template-columns:repeat(auto-fit,minmax(300px,1fr));gap:25px;margin-bottom:30px}
.card{background:rgba(255,255,255,.95);border-radius:20px;padding:25px;box-shadow:0 15px 35px rgba(0,0,0,.1);backdrop-filter:blur(10px);border:1px solid rgba(255,255,255,.2);transition:.3s}
.card:hover{transform:translateY(-5px);box-shadow:0 20px 40px rgba(0,0,0,.15)}
.card-title{font-size:1.4rem;font-weight:600;margin-bottom:20px;color:#333;display:flex;align-items:center;gap:10px}
.status-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(250px,1fr));gap:15px}
.status-item{display:flex;justify-content:space-between;align-items:center;padding:15px 20px;background:linear-gradient(135deg,#f8f9ff 0%,#e8f0ff 100%);border-radius:12px;border-left:4px solid #667eea;transition:.3s}
.status-item:hover{transform:scale(1.02);box-shadow:0 5px 15px rgba(102,126,234,.2)}
.status-label{font-weight:500;color:#555}
.status-value{font-weight:700;padding:5px 12px;border-radius:20px;color:#fff;font-size:.9rem;text-transform:uppercase}
.status-on{background:linear-gradient(135deg,#4CAF50,#45a049)}
.status-off{background:linear-gradient(135deg,#757575,#616161)}
.status-full{background:linear-gradient(135deg,#2196F3,#1976D2)}
.status-empty{background:linear-gradient(135deg,#FF9800,#F57C00)}
.status-high{background:linear-gradient(135deg,#f44336,#d32f2f)}
.status-normal{background:linear-gradient(135deg,#4CAF50,#45a049)}
.status-detected{background:linear-gradient(135deg,#00BCD4,#0097A7)}
.status-not-detected{background:linear-gradient(135deg,#9E9E9E,#757575)}
.status-active{background:linear-gradient(135deg,#FF5722,#E64A19)}
.status-inactive{background:linear-gradient(135deg,#9E9E9E,#757575)}
.status-manual{background:linear-gradient(135deg,#9C27B0,#7B1FA2)}
.pump-controls{display:grid;grid-template-columns:repeat(auto-fit,minmax(320px,1fr));gap:25px}
.pump-card{background:rgba(255,255,255,.95);border-radius:20px;padding:30px;box-shadow:0 15px 35px rgba(0,0,0,.1);backdrop-filter:blur(10px);border:1px solid rgba(255,255,255,.2);text-align:center;position:relative;overflow:hidden}
.pump-card::before{content:'';position:absolute;top:0;left:0;right:0;height:4px;background:linear-gradient(90deg,#667eea,#764ba2)}
.pump-title{font-size:1.5rem;font-weight:700;margin-bottom:20px;color:#333}
.pump-status{font-size:1.1rem;font-weight:600;margin-bottom:25px;padding:10px;border-radius:25px;color:#fff;text-transform:uppercase;letter-spacing:1px}
.pump-status.on{background:linear-gradient(135deg,#4CAF50,#45a049);animation:pulse 2s infinite}
.pump-status.off{background:linear-gradient(135deg,#757575,#616161)}
.selector-status{font-size:0.9rem;font-weight:600;margin-bottom:15px;padding:8px;border-radius:20px;color:#fff;text-transform:uppercase;letter-spacing:0.5px}
.selector-status.active{background:linear-gradient(135deg,#FF5722,#E64A19)}
.selector-status.inactive{background:linear-gradient(135deg,#9E9E9E,#757575)}
.mode-indicator{font-size:0.8rem;font-weight:600;margin-bottom:15px;padding:6px 12px;border-radius:15px;color:#fff;text-transform:uppercase;letter-spacing:0.3px;display:inline-block}
.mode-auto{background:linear-gradient(135deg,#4CAF50,#45a049)}
.mode-manual{background:linear-gradient(135deg,#9C27B0,#7B1FA2);animation:flashManual 1.5s infinite}
@keyframes pulse{0%{box-shadow:0 0 0 0 rgba(76,175,80,.7)}70%{box-shadow:0 0 0 10px rgba(76,175,80,0)}100%{box-shadow:0 0 0 0 rgba(76,175,80,0)}}
@keyframes flashManual{0%{opacity:1}50%{opacity:0.7}100%{opacity:1}}
.control-section{margin-bottom:20px;padding:15px;border-radius:15px;background:rgba(240,240,240,.3)}
.section-title{font-size:1rem;font-weight:600;color:#333;margin-bottom:10px;text-align:left}
.button-group{display:flex;gap:10px;justify-content:center;margin-bottom:10px}
.btn{flex:1;padding:12px 20px;border:none;border-radius:20px;font-size:0.9rem;font-weight:600;text-decoration:none;text-align:center;cursor:pointer;transition:.3s;text-transform:uppercase;letter-spacing:.3px;position:relative;overflow:hidden;min-width:100px}
.btn::before{content:'';position:absolute;top:0;left:-100%;width:100%;height:100%;background:linear-gradient(90deg,transparent,rgba(255,255,255,.3),transparent);transition:left .5s}
.btn:hover::before{left:100%}
.btn-on{background:linear-gradient(135deg,#4CAF50,#45a049);color:#fff;box-shadow:0 6px 15px rgba(76,175,80,.3)}
.btn-on:hover{transform:translateY(-2px);box-shadow:0 8px 20px rgba(76,175,80,.4)}
.btn-off{background:linear-gradient(135deg,#f44336,#d32f2f);color:#fff;box-shadow:0 6px 15px rgba(244,67,54,.3)}
.btn-off:hover{transform:translateY(-2px);box-shadow:0 8px 20px rgba(244,67,54,.4)}
.btn-manual{background:linear-gradient(135deg,#9C27B0,#7B1FA2);color:#fff;box-shadow:0 6px 15px rgba(156,39,176,.3);border:2px solid rgba(255,255,255,.3)}
.btn-manual:hover{transform:translateY(-2px);box-shadow:0 8px 20px rgba(156,39,176,.4)}
.warning{background:linear-gradient(135deg,#FF6B6B,#FF5252);color:#fff;padding:10px;border-radius:10px;font-size:0.85rem;margin-top:10px;text-align:center;font-weight:500}
.system-info{background:rgba(255,255,255,.95);border-radius:20px;padding:25px;margin-top:25px;box-shadow:0 15px 35px rgba(0,0,0,.1);backdrop-filter:blur(10px);border:1px solid rgba(255,255,255,.2)}
.info-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(200px,1fr));gap:15px}
.info-item{padding:15px;background:linear-gradient(135deg,#f0f4ff,#e1ecff);border-radius:10px;text-align:center;border-left:4px solid #667eea}
.info-label{font-size:.9rem;color:#666;margin-bottom:5px;text-transform:uppercase;letter-spacing:.5px}
.info-value{font-size:1.1rem;font-weight:700;color:#333}
.footer{text-align:center;color:rgba(255,255,255,.8);margin-top:40px;padding:20px;font-size:.9rem}
.loading-indicator{position:fixed;top:20px;right:20px;background:rgba(255,255,255,.9);padding:10px 20px;border-radius:25px;box-shadow:0 5px 15px rgba(0,0,0,.1);font-size:.9rem;color:#667eea;z-index:1000; transition: all 0.5s;}
.loading-indicator.error{background: #f44336; color: white;}
.loading-indicator.ok{background: #4CAF50; color: white;}
</style>
</head>
<body>
<div class='loading-indicator' id='loading-indicator'>🔄 Obteniendo datos...</div>
<div class='container'>
<div class='header'>
<h1>💧 Control de Bombas</h1>
<p>Sistema Inteligente de Gesti&oacute;n de Agua</p>
</div>
<div class='dashboard'><div class='card'><div class='card-title'>📊 Estado del Sistema</div><div class='status-grid'>
<div class='status-item'><span class='status-label'>Bomba Cisterna</span><span id='status-bomba1' class='status-value'></span></div>
<div class='status-item'><span class='status-label'>Bomba Tinaco</span><span id='status-bomba2' class='status-value'></span></div>
<div class='status-item'><span class='status-label'>Modo Bomba Cisterna</span><span id='status-modo1' class='status-value'></span></div>
<div class='status-item'><span class='status-label'>Modo Bomba Tinaco</span><span id='status-modo2' class='status-value'></span></div>
<div class='status-item'><span class='status-label'>Selector de Cisterna</span><span id='status-selector1' class='status-value'></span></div>
<div class='status-item'><span class='status-label'>Selector de Tinaco</span><span id='status-selector2' class='status-value'></span></div>
<div class='status-item'><span class='status-label'>Cisterna</span><span id='status-cisterna' class='status-value'></span></div>
<div class='status-item'><span class='status-label'>Tinaco</span><span id='status-tinaco' class='status-value'></span></div>
<div class='status-item'><span class='status-label'>Flujo</span><span id='status-flujo' class='status-value'></span></div>
<div class='status-item'><span class='status-label'>Presi&oacute;n</span><span id='status-presion' class='status-value'></span></div>
</div></div></div>

<div class='pump-controls'>
<div class='pump-card'>
<h3 class='pump-title'>🚰 Bomba 1 - Cisterna</h3>
<div id='card-selector1' class='selector-status'></div>
<div id='card-modo1' class='mode-indicator'></div>
<div id='card-status1' class='pump-status'></div>
<div class='control-section'><div class='section-title'>🤖 Control Autom&aacute;tico</div><div class='button-group'><a href='/bomba1on' class='btn btn-on'>⚡ Encender</a><a href='/bomba1off' class='btn btn-off'>⏹ Apagar</a></div></div>
<div class='control-section'><div class='section-title'>🔧 Control Manual (Sin Protecciones)</div><div class='button-group'><a href='/bomba1manualon' class='btn btn-manual'>🔥 Manual ON</a><a href='/bomba1manualoff' class='btn btn-manual'>🛑 Manual OFF</a></div><div class='warning'>⚠️ ATENCI&Oacute;N: El modo manual ignora todas las protecciones de seguridad</div></div>
</div>

<div class='pump-controls' style='margin-top: 25px;'>
  <div class='pump-card' style='grid-column: 1 / -1; max-width: 800px; margin: 0 auto; width: 100%;'>
    <h3 class='pump-title'>⏳ Llenado extra para cisterna</h3>
    <div class='control-section'>
      <div class='section-title'>⏱️ Parámetros del Temporizador</div>
      <div style='display: flex; flex-wrap: wrap; justify-content: center; gap: 30px; align-items: center; margin-bottom: 20px;'>
        <label style='cursor: pointer; font-size: 1.1rem; font-weight: 600; display: flex; align-items: center; gap: 10px; color: #333;'>
          <input type='checkbox' id='checkTimer' style='transform: scale(1.6); accent-color: #4CAF50;'>
          Activar tiempo extra de llenado
        </label>
        <div style='display: flex; align-items: center; gap: 10px;'>
          <label style='font-size: 1.1rem; font-weight: 600; color: #333;'>Minutos extra:</label>
          <input type='number' id='inputMinutos' value='60' min='1' max='120' style='padding: 10px 15px; width: 90px; font-size: 1.1rem; border: 2px solid #e1ecff; border-radius: 12px; text-align: center; outline: none; background: #fff;'>
        </div>
      </div>
      <div class='button-group' style='max-width: 300px; margin: 0 auto;'>
        <button onclick='guardarTimer()' class='btn btn-on' style='width: 100%; font-family: inherit;'>💾 Guardar Configuración</button>
      </div>
    </div>
  </div>
</div>

<div class='pump-card'>
<h3 class='pump-title'>🏠 Bomba 2 - Tinaco</h3>
<div id='card-selector2' class='selector-status'></div>
<div id='card-modo2' class='mode-indicator'></div>
<div id='card-status2' class='pump-status'></div>
<div class='control-section'><div class='section-title'>🤖 Control Autom&aacute;tico</div><div class='button-group'><a href='/bomba2on' class='btn btn-on'>⚡ Encender</a><a href='/bomba2off' class='btn btn-off'>⏹ Apagar</a></div></div>
<div class='control-section'><div class='section-title'>🔧 Control Manual (Sin Protecciones)</div><div class='button-group'><a href='/bomba2manualon' class='btn btn-manual'>🔥 Manual ON</a><a href='/bomba2manualoff' class='btn btn-manual'>🛑 Manual OFF</a></div><div class='warning'>⚠️ ATENCI&Oacute;N: El modo manual ignora todas las protecciones de seguridad</div></div>
</div>
</div>

<div class='system-info'>
<div class='card-title'>⚙️ Informaci&oacute;n del Sistema</div>
<div class='info-grid'>
<div class='info-item'><div class='info-label'>Direcci&oacute;n IP</div><div id='info-ip' class='info-value'>-</div></div>
<div class='info-item'><div class='info-label'>Memoria Libre</div><div id='info-heap' class='info-value'>-</div></div>
<div class='info-item'><div class='info-label'>Verificando Flujo</div><div id='info-verif-flujo' class='info-value'>-</div></div>
<div class='info-item'><div class='info-label'>Tiempo Verificaci&oacute;n</div><div id='info-tiempo-flujo' class='info-value'>-</div></div>
<div class='info-item'><div class='info-label'>Bloqueo Bomba 1</div><div id='info-bloqueo1' class='info-value'>-</div></div>
<div class='info-item'><div class='info-label'>Bloqueo Bomba 2</div><div id='info-bloqueo2' class='info-value'>-</div></div>
</div></div>
<div class='footer'>
<p>🔧 Sistema de Gesti&oacute;n de Agua Inteligente</p>
<p>👷🏼 Desarrollado por Ing. Cristian Giovanni Jacinto Lopez</p>
<p>&Uacute;ltima actualizaci&oacute;n: <span id='timestamp'></span></p>
</div>
</div>
<script>
const indicator=document.getElementById('loading-indicator');function updateElement(id,text,className){const el=document.getElementById(id);if(el){if(el.textContent!==text)el.textContent=text;if(el.className!==className)el.className=className}}
function actualizarDatos(){fetch('/status').then(response=>{if(!response.ok)throw new Error('Network response was not ok');return response.json()}).then(data=>{indicator.textContent='✔️ Datos actualizados';indicator.className='loading-indicator ok';setTimeout(()=>{indicator.className='loading-indicator';indicator.textContent='🔄 Obteniendo datos...'},1500);updateElement('status-bomba1',data.bomba1?'Encendida':'Apagada','status-value '+(data.bomba1?'status-on':'status-off'));updateElement('status-bomba2',data.bomba2?'Encendida':'Apagada','status-value '+(data.bomba2?'status-on':'status-off'));updateElement('status-modo1',data.modo_manual_bomba1?'Manual':'Automático','status-value '+(data.modo_manual_bomba1?'status-manual':'status-on'));updateElement('status-modo2',data.modo_manual_bomba2?'Manual':'Automático','status-value '+(data.modo_manual_bomba2?'status-manual':'status-on'));updateElement('status-selector1',data.selector1?'Activo':'Inactivo','status-value '+(data.selector1?'status-active':'status-inactive'));updateElement('status-selector2',data.selector2?'Activo':'Inactivo','status-value '+(data.selector2?'status-active':'status-inactive'));updateElement('status-cisterna',data.cisterna==='llena'?'Llena':'Vacía','status-value '+(data.cisterna==='llena'?'status-full':'status-empty'));updateElement('status-tinaco',data.tinaco==='lleno'?'Lleno':'Vacío','status-value '+(data.tinaco==='lleno'?'status-full':'status-empty'));updateElement('status-flujo',data.flujo?'Detectado':'Sin Flujo','status-value '+(data.flujo?'status-detected':'status-not-detected'));updateElement('status-presion',data.presion==='alta'?'Alta':'Normal','status-value '+(data.presion==='alta'?'status-high':'status-normal'));updateElement('card-selector1',data.selector1?'🔘 Selector Activo':'⚫ Selector Inactivo','selector-status '+(data.selector1?'active':'inactive'));updateElement('card-modo1',data.modo_manual_bomba1?'🔧 Modo Manual':'🤖 Modo Automático','mode-indicator '+(data.modo_manual_bomba1?'mode-manual':'mode-auto'));updateElement('card-status1',data.bomba1?'● Operando':'● Detenida','pump-status '+(data.bomba1?'on':'off'));updateElement('card-selector2',data.selector2?'🔘 Selector Activo':'⚫ Selector Inactivo','selector-status '+(data.selector2?'active':'inactive'));updateElement('card-modo2',data.modo_manual_bomba2?'🔧 Modo Manual':'🤖 Modo Automático','mode-indicator '+(data.modo_manual_bomba2?'mode-manual':'mode-auto'));updateElement('card-status2',data.bomba2?'● Operando':'● Detenida','pump-status '+(data.bomba2?'on':'off'));updateElement('info-ip',data.ip,'info-value');updateElement('info-heap',(data.heap_libre/1024).toFixed(2)+' KB','info-value');updateElement('info-verif-flujo',data.verificando_flujo?'Sí':'No','info-value');updateElement('info-tiempo-flujo',data.tiempo_verificacion+' seg','info-value');updateElement('info-bloqueo1',data.bloqueo_bomba1?'Sí':'No','info-value');updateElement('info-bloqueo2',data.bloqueo_bomba2?'Sí':'No','info-value');document.getElementById('timestamp').textContent=new Date().toLocaleString('es-ES')}).catch(error=>{console.error('Error al obtener datos:',error);indicator.textContent='❌ Error de conexión';indicator.className='loading-indicator error'})}
document.addEventListener('DOMContentLoaded',function(){actualizarDatos();setInterval(actualizarDatos,2000)});

function guardarTimer() {
  let activado = document.getElementById('checkTimer').checked;
  let minutos = document.getElementById('inputMinutos').value;
  
  fetch(`/setTimer?activado=${activado}&minutos=${minutos}`)
    .then(response => {
      if(response.ok) {
        alert("Configuración de tiempo guardada correctamente en el ESP32.");
      } else {
        alert("Hubo un error al comunicar con el ESP32.");
      }
    })
    .catch(error => {
      alert("Fallo de red: " + error);
    });
}
</script>
</body></html>
)rawliteral";

//==============================
// Setup
//==============================
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

  digitalWrite(RELE_BOMBA1, LOW);
  digitalWrite(RELE_BOMBA2, LOW);
  digitalWrite(RELE_BOMBA3, LOW);
  digitalWrite(RELE_BOMBA4, LOW);
  digitalWrite(RELE_BOMBA5, LOW);
  digitalWrite(RELE_BOMBA6, LOW);

  // Configuración WiFi
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.setAutoReconnect(false); 
  WiFi.setHostname("esp32-bombas");

  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("Error al configurar IP fija");
  }

  // Cuando logramos conectar a la red original, se apaga el AP
  WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info){
    Serial.print("\n>>> CONECTADO A LA RED PRINCIPAL. IP: ");
    Serial.println(WiFi.localIP());
    reconnectTries = 0;
    stopAPBackup();
  }, ARDUINO_EVENT_WIFI_STA_GOT_IP);
  
  WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info){
    Serial.printf("WiFi desconectado. Motivo: %d\n", info.wifi_sta_disconnected.reason);
  }, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);

  startWiFiSTA();
  configureWebServer();

  server.on("/setTimer", HTTP_GET, []() {
    if (server.hasArg("activado") && server.hasArg("minutos")) {
      String strActivado = server.arg("activado");
      String strMinutos = server.arg("minutos");
      
      portENTER_CRITICAL(&estadoMux);
      habilitar_tiempo_extra = (strActivado == "true");
      tiempo_extra_llenado = strMinutos.toInt() * 60000; 
      portEXIT_CRITICAL(&estadoMux);
      
      Serial.print("Timer Web - Activado: ");
      Serial.print(strActivado);
      Serial.print(", Milisegundos totales: ");
      Serial.println(tiempo_extra_llenado);
      
      server.send(200, "text/plain", "OK");
    } else {
      server.send(400, "text/plain", "Faltan datos");
    }
  });

  server.begin();
  Serial.println("Servidor HTTP Síncrono iniciado");

  xTaskCreatePinnedToCore(
    TaskControlHardware, "ControlHardware", 10000, NULL, 1, &Task1, 0
  );

  delay(500);
  Serial.println("Sistema iniciado - Core 1: Web Server, Core 0: Control Hardware");
}

//==============================
// Loop (Core 1: Web)
//==============================
void loop() {
  server.handleClient(); 
  ensureWiFi();
  delay(2);
}

//==============================
// Configuración de rutas web
//==============================
void configureWebServer() {
  
  server.on("/", HTTP_GET, [](){
    server.send_P(200, "text/html", ROOT_HTML);
  });

  server.on("/status", HTTP_GET, [](){
    JsonDocument doc; 
    doc["bomba1"] = bomba1_encendida;
    doc["bomba2"] = bomba2_encendida;
    doc["modo_manual_bomba1"] = modo_manual_bomba1;
    doc["modo_manual_bomba2"] = modo_manual_bomba2;
    doc["selector1"] = (digitalRead(SELECTOR_BOMBA1) == HIGH);
    doc["selector2"] = (digitalRead(SELECTOR_BOMBA2) == HIGH);
    doc["cisterna"] = (digitalRead(FLOTADOR_CISTERNA) == HIGH) ? "llena" : "vacia";
    doc["tinaco"] = (digitalRead(FLOTADOR_TINACO) == HIGH) ? "lleno" : "vacio";
    doc["flujo"] = (digitalRead(SENSOR_FLUJO) == HIGH);
    doc["presion"] = (digitalRead(SENSOR_PRESION) == HIGH) ? "alta" : "normal";
    doc["verificando_flujo"] = verificando_flujo;
    doc["tiempo_verificacion"] = verificando_flujo ? (millis() - tiempo_inicio_flujo) / 1000 : 0;
    doc["bloqueo_bomba1"] = bloqueo_bomba1;
    doc["bloqueo_bomba2"] = bloqueo_bomba2;
    doc["ip"] = apBackupActivo ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
    doc["heap_libre"] = ESP.getFreeHeap();
    doc["modo"] = apBackupActivo ? "AP+STA" : "STA";
    
    String output;
    serializeJson(doc, output);
    server.send(200, "application/json", output);
  });

  server.on("/bomba1on", HTTP_GET, [](){
    encenderBomba1();
    server.sendHeader("Location", "/");
    server.send(303);
  });

  server.on("/bomba1off", HTTP_GET, [](){
    apagarBomba1();
    portENTER_CRITICAL(&estadoMux);
    modo_manual_bomba1 = false;
    portEXIT_CRITICAL(&estadoMux);
    server.sendHeader("Location", "/");
    server.send(303);
  });

  server.on("/bomba2on", HTTP_GET, [](){
    encenderBomba2();
    server.sendHeader("Location", "/");
    server.send(303);
  });

  server.on("/bomba2off", HTTP_GET, [](){
    apagarBomba2();
    portENTER_CRITICAL(&estadoMux);
    modo_manual_bomba2 = false;
    portEXIT_CRITICAL(&estadoMux);
    server.sendHeader("Location", "/");
    server.send(303);
  });

  server.on("/bomba1manualon", HTTP_GET, [](){
    encenderBomba1Manual();
    server.sendHeader("Location", "/");
    server.send(303);
  });

  server.on("/bomba1manualoff", HTTP_GET, [](){
    apagarBomba1Manual();
    server.sendHeader("Location", "/");
    server.send(303);
  });

  server.on("/bomba2manualon", HTTP_GET, [](){
    encenderBomba2Manual();
    server.sendHeader("Location", "/");
    server.send(303);
  });

  server.on("/bomba2manualoff", HTTP_GET, [](){
    apagarBomba2Manual();
    server.sendHeader("Location", "/");
    server.send(303);
  });
}

//==================================================================
// LÓGICA DE HARDWARE
//==================================================================

void TaskControlHardware(void * pvParameters) {
  Serial.println("Tarea de control de hardware iniciada en Core 0");
  for(;;) {
    bool selector1_actual = digitalRead(SELECTOR_BOMBA1);
    bool selector2_actual = digitalRead(SELECTOR_BOMBA2);

    portENTER_CRITICAL(&estadoMux);
    estado_selector1 = selector1_actual;
    estado_selector2 = selector2_actual;
    bool m_manual1 = modo_manual_bomba1;
    bool u_sel1 = ultimo_selector1;
    bool b_bomba1 = bloqueo_bomba1;
    bool m_manual2 = modo_manual_bomba2;
    bool u_sel2 = ultimo_selector2;
    bool b_bomba2 = bloqueo_bomba2;
    
    bool hab_timer = habilitar_tiempo_extra;
    unsigned long t_extra = tiempo_extra_llenado;
    portEXIT_CRITICAL(&estadoMux);

    if (!m_manual1) {
      if (selector1_actual == HIGH && u_sel1 == LOW && !b_bomba1) {
        encenderBomba1();
      } else if (selector1_actual == LOW && u_sel1 == HIGH) {
        apagarBomba1();
        portENTER_CRITICAL(&estadoMux);
        bloqueo_bomba1 = false;
        portEXIT_CRITICAL(&estadoMux);
      }
    }

    if (!m_manual2) {
      if (selector2_actual == HIGH && u_sel2 == LOW && !b_bomba2) {
        encenderBomba2();
      } else if (selector2_actual == LOW && u_sel2 == HIGH) {
        apagarBomba2();
        portENTER_CRITICAL(&estadoMux);
        bloqueo_bomba2 = false;
        portEXIT_CRITICAL(&estadoMux);
      }
    }

    portENTER_CRITICAL(&estadoMux);
    ultimo_selector1 = selector1_actual;
    ultimo_selector2 = selector2_actual;
    
    bool b1_enc = bomba1_encendida;
    bool v_flujo = verificando_flujo;
    unsigned long t_flujo = tiempo_inicio_flujo;
    bool b2_enc = bomba2_encendida;
    portEXIT_CRITICAL(&estadoMux);
    
    if (b1_enc && !m_manual1) {
      if (digitalRead(FLOTADOR_CISTERNA) == HIGH) {
        if (hab_timer) {
          if (!temporizador_bomba1_activo) {
            temporizador_bomba1_activo = true;
            tiempo_flotador1_alto = millis();
            Serial.println("Cisterna llena. Iniciando tiempo extra (Bomba 1)...");
          } else if (millis() - tiempo_flotador1_alto >= t_extra) {
            Serial.println("Tiempo extra web completado, apagando bomba 1");
            apagarBomba1();
            portENTER_CRITICAL(&estadoMux);
            bloqueo_bomba1 = true;
            verificando_flujo = false;
            temporizador_bomba1_activo = false; 
            portEXIT_CRITICAL(&estadoMux);
          }
        } else {
          Serial.println("Cisterna llena, apagando bomba 1 inmediatamente");
          apagarBomba1();
          portENTER_CRITICAL(&estadoMux);
          bloqueo_bomba1 = true;
          verificando_flujo = false;
          temporizador_bomba1_activo = false; 
          portEXIT_CRITICAL(&estadoMux);
        }
      } else {
        temporizador_bomba1_activo = false; 
        
        if (digitalRead(SENSOR_PRESION) == HIGH) {
          Serial.println("Alta presión detectada, apagando bomba 1");
          apagarBomba1();
          portENTER_CRITICAL(&estadoMux);
          bloqueo_bomba1 = true;
          verificando_flujo = false;
          portEXIT_CRITICAL(&estadoMux);
        } else if (!v_flujo) {
          portENTER_CRITICAL(&estadoMux);
          tiempo_inicio_flujo = millis();
          verificando_flujo = true;
          portEXIT_CRITICAL(&estadoMux);
        } else if (v_flujo && (millis() - t_flujo > 300000)) {
          if (digitalRead(SENSOR_FLUJO) == LOW) {
            Serial.println("No se detecto flujo en 5 min, apagando Bomba 1");
            apagarBomba1();
            portENTER_CRITICAL(&estadoMux);
            bloqueo_bomba1 = true;
            verificando_flujo = false;
            portEXIT_CRITICAL(&estadoMux);
          } else {
            portENTER_CRITICAL(&estadoMux);
            tiempo_inicio_flujo = millis();
            portEXIT_CRITICAL(&estadoMux);
          }
        }
      }
    }

    if (b2_enc && !m_manual2) {
      if (digitalRead(FLOTADOR_TINACO) == HIGH) {
        if (hab_timer) {
          if (!temporizador_bomba2_activo) {
            temporizador_bomba2_activo = true;
            tiempo_flotador2_alto = millis();
            Serial.println("Tinaco lleno. Iniciando tiempo extra (Bomba 2)...");
          } else if (millis() - tiempo_flotador2_alto >= t_extra) {
            Serial.println("Tiempo extra web completado, apagando bomba 2");
            apagarBomba2();
            portENTER_CRITICAL(&estadoMux);
            bloqueo_bomba2 = true;
            temporizador_bomba2_activo = false; 
            portEXIT_CRITICAL(&estadoMux);
          }
        } else {
          Serial.println("Tinaco lleno, apagando bomba 2 inmediatamente");
          apagarBomba2();
          portENTER_CRITICAL(&estadoMux);
          bloqueo_bomba2 = true;
          temporizador_bomba2_activo = false; 
          portEXIT_CRITICAL(&estadoMux);
        }
      } else {
        temporizador_bomba2_activo = false; 
      }
    }

    delay(50);
  }
}

void encenderBomba1() {
  if (digitalRead(FLOTADOR_CISTERNA) == LOW) {
    Serial.println("Encendiendo bomba 1");
    digitalWrite(RELE_BOMBA1, HIGH);
    portENTER_CRITICAL(&estadoMux);
    bomba1_encendida = true;
    verificando_flujo = false;
    portEXIT_CRITICAL(&estadoMux);
  } else {
    Serial.println("No se puede encender bomba 1, cisterna llena");
  }
}

void apagarBomba1() {
  Serial.println("Apagando bomba 1");
  digitalWrite(RELE_BOMBA1, LOW);
  portENTER_CRITICAL(&estadoMux);
  bomba1_encendida = false;
  verificando_flujo = false;
  portEXIT_CRITICAL(&estadoMux);
}

void encenderBomba2() {
  if (digitalRead(FLOTADOR_TINACO) == LOW) {
    Serial.println("Encendiendo bomba 2");
    digitalWrite(RELE_BOMBA2, HIGH);
    portENTER_CRITICAL(&estadoMux);
    bomba2_encendida = true;
    portEXIT_CRITICAL(&estadoMux);
  } else {
    Serial.println("No se puede encender bomba 2, tinaco lleno");
  }
}

void apagarBomba2() {
  Serial.println("Apagando bomba 2");
  digitalWrite(RELE_BOMBA2, LOW);
  portENTER_CRITICAL(&estadoMux);
  bomba2_encendida = false;
  portEXIT_CRITICAL(&estadoMux);
}

void encenderBomba1Manual() {
  Serial.println("MODO MANUAL: Encendiendo bomba 1");
  digitalWrite(RELE_BOMBA1, HIGH);
  portENTER_CRITICAL(&estadoMux);
  bomba1_encendida = true;
  modo_manual_bomba1 = true;
  verificando_flujo = false;
  bloqueo_bomba1 = false;
  portEXIT_CRITICAL(&estadoMux);
}

void apagarBomba1Manual() {
  Serial.println("MODO MANUAL: Apagando bomba 1");
  digitalWrite(RELE_BOMBA1, LOW);
  portENTER_CRITICAL(&estadoMux);
  bomba1_encendida = false;
  modo_manual_bomba1 = false;
  verificando_flujo = false;
  portEXIT_CRITICAL(&estadoMux);
}

void encenderBomba2Manual() {
  Serial.println("MODO MANUAL: Encendiendo bomba 2");
  digitalWrite(RELE_BOMBA2, HIGH);
  portENTER_CRITICAL(&estadoMux);
  bomba2_encendida = true;
  modo_manual_bomba2 = true;
  bloqueo_bomba2 = false;
  portEXIT_CRITICAL(&estadoMux);
}

void apagarBomba2Manual() {
  Serial.println("MODO MANUAL: Apagando bomba 2");
  digitalWrite(RELE_BOMBA2, LOW);
  portENTER_CRITICAL(&estadoMux);
  bomba2_encendida = false;
  modo_manual_bomba2 = false;
  portEXIT_CRITICAL(&estadoMux);
}

//========================================================
// WiFi Robusto - Lógica de Auto-Reconexión en Fondo
//========================================================
void startWiFiSTA() {
  Serial.printf("Conectando a SSID '%s' ...\n", ssid);
  WiFi.begin(ssid, password);
  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 15000) {
    delay(250);
    Serial.print('.');
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    apBackupActivo = false;
    Serial.print("WiFi STA OK. IP: ");
    Serial.println(WiFi.localIP());
    reconnectTries = 0;
  } else {
    Serial.println("No conecto en el primer intento. El Loop intentará después.");
  }
}

void ensureWiFi() {
  // Si estamos conectados de forma exitosa a la red
  if (WiFi.status() == WL_CONNECTED) {
    if (apBackupActivo) {
       stopAPBackup(); // Por si acaso nos reconectamos y el AP seguía vivo
    }
    reconnectTries = 0;
    return; // Todo perfecto, salimos
  }

  // Si estamos desconectados, definimos el tiempo de espera entre intentos
  // Si el AP está activo, damos 15 segundos para no trabar a los usuarios del AP
  // Si el AP no está activo, intentamos cada 5 segundos
  unsigned long currentInterval = apBackupActivo ? 15000 : WIFI_CHECK_MS;

  if (millis() - lastWifiCheck < currentInterval) return;
  lastWifiCheck = millis();

  if (reconnectTries < 250) { // Límite de conteo para no desbordar la variable
    reconnectTries++;
  }
  
  Serial.printf("WiFi principal caido. Intento de reconexion en fondo #%u\n", reconnectTries);
  
  // Si ya falló demasiadas veces y el AP aún NO está activo, lo encendemos
  if (reconnectTries >= MAX_TRIES_BEFORE_AP && !apBackupActivo) {
    startAPBackup();
  }

  // Ordenamos al chip de red intentar conectar a tu módem nuevamente (no bloquea el código)
  WiFi.disconnect(false); // Desconecta STA sin apagar la radio
  WiFi.begin(ssid, password);
}

void startAPBackup() {
  Serial.println("\n[!] Activando AP de respaldo: ESP32-Backup (clave: 12345678)");
  // LA MAGIA OCURRE AQUÍ: Modo AP + STA permite tener la red de respaldo activa 
  // MIENTRAS el ESP32 sigue buscando conectarse a tu módem en segundo plano.
  WiFi.mode(WIFI_AP_STA); 
  
  bool ok = WiFi.softAP("ESP32-Backup", "12345678", 1, 0, 4);
  if (ok) {
    apBackupActivo = true;
    IPAddress apIP = WiFi.softAPIP();
    Serial.print("AP activo. Conectate a: ");
    Serial.println(apIP);
  } else {
    Serial.println("Fallo al iniciar AP de respaldo.");
  }
}

void stopAPBackup() {
  if (apBackupActivo) {
    Serial.println("Desactivando AP de respaldo porque ya hay WiFi.");
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA); // Volvemos a modo cliente puro
    apBackupActivo = false;
  }
}