// =============================================================
//  ESP32 Soil Sensor Dashboard (Access Point Mode)
//  Sensor: CWT-SOIL-NPKPHCTH-S via MAX485 (RS485)
//  Author: Dr. Zainal Abidin Arsat
// =============================================================

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ModbusMaster.h>
#include <Preferences.h>

// ---------- RS485 ----------
#define RX2_PIN   16
#define TX2_PIN   17
#define DE_RE_PIN  4   // MAX485 Driver Enable / Receiver Enable

ModbusMaster node;
Preferences prefs;

void preTransmission()  { digitalWrite(DE_RE_PIN, HIGH); }
void postTransmission() { digitalWrite(DE_RE_PIN, LOW);  }

// ---------- WiFi ----------
const char* ssid     = "SoilSensor";
const char* password = "12345678";
WebServer server(80);

// ---------- Variables ----------
float N = -1, P = -1, K = -1, pH = -1, EC = -1, Moist = -1, Temp = -1;
float offset_N = 0, offset_P = 0, offset_K = 0;
float offset_pH = 0, offset_EC = 0, offset_Moist = 0, offset_Temp = 0;

unsigned long lastRead = 0;
const unsigned long READ_INTERVAL = 1000; // ms between sensor reads

// ---------- HTML Dashboard ----------
String htmlPage() {
  String page = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Soil Sensor Dashboard</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:'Segoe UI',Arial,sans-serif;background:#0d1117;color:#e6edf3;min-height:100vh;padding:16px}
header{display:flex;align-items:center;justify-content:space-between;margin-bottom:20px;padding-bottom:12px;border-bottom:1px solid #21262d}
header h1{font-size:1.1rem;font-weight:600;color:#58a6ff}
#status{display:flex;align-items:center;gap:6px;font-size:0.75rem;color:#8b949e}
#dot{width:8px;height:8px;border-radius:50%;background:#3fb950;box-shadow:0 0 6px #3fb950;transition:background 0.3s}
#dot.err{background:#f85149;box-shadow:0 0 6px #f85149}
.grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(140px,1fr));gap:12px;margin-bottom:20px}
.card{background:#161b22;border:1px solid #21262d;border-radius:10px;padding:14px 12px;text-align:center;transition:border-color 0.3s}
.card.err{border-color:#f85149}
.card .label{font-size:0.7rem;color:#8b949e;text-transform:uppercase;letter-spacing:0.05em;margin-bottom:6px}
.card .val{font-size:1.6rem;font-weight:700;color:#58a6ff;line-height:1;transition:color 0.3s}
.card.err .val{color:#f85149;font-size:1rem;padding-top:6px}
.card .unit{font-size:0.65rem;color:#6e7681;margin-top:4px}
.card .icon{font-size:1.1rem;margin-bottom:4px}
section{background:#161b22;border:1px solid #21262d;border-radius:10px;padding:16px}
section h2{font-size:0.85rem;color:#8b949e;text-transform:uppercase;letter-spacing:0.05em;margin-bottom:14px}
.offset-grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(130px,1fr));gap:10px;margin-bottom:14px}
.offset-item label{display:block;font-size:0.7rem;color:#8b949e;margin-bottom:4px}
.offset-item input{width:100%;background:#0d1117;border:1px solid #30363d;border-radius:6px;padding:6px 8px;color:#e6edf3;font-size:0.85rem}
.offset-item input:focus{outline:none;border-color:#58a6ff}
.btn-row{display:flex;gap:10px}
button{padding:8px 20px;border:none;border-radius:6px;font-size:0.85rem;font-weight:600;cursor:pointer;transition:opacity 0.2s}
button:hover{opacity:0.85}
#btnApply{background:#238636;color:#fff}
#btnReset{background:#21262d;color:#e6edf3;border:1px solid #30363d}
#toast{position:fixed;bottom:20px;right:20px;background:#238636;color:#fff;padding:8px 16px;border-radius:8px;font-size:0.8rem;opacity:0;transition:opacity 0.4s;pointer-events:none}
</style>
</head>
<body>
<header>
  <h1>&#127807; Soil Sensor Dashboard</h1>
  <div id="status"><div id="dot"></div><span id="statusTxt">Connecting...</span></div>
</header>

<div class="grid">
  <div class="card" id="cN"><div class="icon">🌿</div><div class="label">Nitrogen</div><div class="val" id="nVal">--</div><div class="unit">mg/kg</div></div>
  <div class="card" id="cP"><div class="icon">🔴</div><div class="label">Phosphorus</div><div class="val" id="pVal">--</div><div class="unit">mg/kg</div></div>
  <div class="card" id="cK"><div class="icon">🟡</div><div class="label">Potassium</div><div class="val" id="kVal">--</div><div class="unit">mg/kg</div></div>
  <div class="card" id="cPH"><div class="icon">⚗️</div><div class="label">pH</div><div class="val" id="phVal">--</div><div class="unit">&nbsp;</div></div>
  <div class="card" id="cEC"><div class="icon">⚡</div><div class="label">EC</div><div class="val" id="ecVal">--</div><div class="unit">µS/cm</div></div>
  <div class="card" id="cM"><div class="icon">💧</div><div class="label">Moisture</div><div class="val" id="mVal">--</div><div class="unit">%</div></div>
  <div class="card" id="cT"><div class="icon">🌡️</div><div class="label">Temperature</div><div class="val" id="tVal">--</div><div class="unit">°C</div></div>
</div>

<section>
  <h2>Calibration Offsets</h2>
  <form id="offsetForm">
    <div class="offset-grid">
      <div class="offset-item"><label>Nitrogen (N)</label><input type="number" step="0.1" name="n" id="in_n" value="0"></div>
      <div class="offset-item"><label>Phosphorus (P)</label><input type="number" step="0.1" name="p" id="in_p" value="0"></div>
      <div class="offset-item"><label>Potassium (K)</label><input type="number" step="0.1" name="k" id="in_k" value="0"></div>
      <div class="offset-item"><label>pH</label><input type="number" step="0.01" name="ph" id="in_ph" value="0"></div>
      <div class="offset-item"><label>EC</label><input type="number" step="0.1" name="ec" id="in_ec" value="0"></div>
      <div class="offset-item"><label>Moisture</label><input type="number" step="0.1" name="moist" id="in_moist" value="0"></div>
      <div class="offset-item"><label>Temperature</label><input type="number" step="0.1" name="temp" id="in_temp" value="0"></div>
    </div>
    <div class="btn-row">
      <button type="button" id="btnApply" onclick="sendOffsets()">Apply Offsets</button>
      <button type="button" id="btnReset" onclick="resetOffsets()">Reset</button>
    </div>
  </form>
</section>

<div id="toast">Offsets applied!</div>

<script>
function set(id,v,cardId){
  var el=document.getElementById(id);
  var card=document.getElementById(cardId);
  if(v==-1){el.innerText='ERR';card.classList.add('err');}
  else{el.innerText=v;card.classList.remove('err');}
}
async function fetchData(){
  try{
    var r=await fetch('/data');
    var j=await r.json();
    set('nVal',j.N,'cN');
    set('pVal',j.P,'cP');
    set('kVal',j.K,'cK');
    set('phVal',j.pH,'cPH');
    set('ecVal',j.EC,'cEC');
    set('mVal',j.Moist,'cM');
    set('tVal',j.Temp,'cT');
    document.getElementById('dot').className='';
    document.getElementById('statusTxt').innerText='Updated '+new Date().toLocaleTimeString();
  }catch(e){
    document.getElementById('dot').className='err';
    document.getElementById('statusTxt').innerText='No response';
  }
}
async function sendOffsets(){
  var data=new URLSearchParams(new FormData(document.getElementById('offsetForm')));
  await fetch('/offsets',{method:'POST',body:data});
  var t=document.getElementById('toast');
  t.style.opacity=1;setTimeout(function(){t.style.opacity=0;},2000);
}
function resetOffsets(){
  document.getElementById('offsetForm').querySelectorAll('input').forEach(function(i){i.value=0;});
  sendOffsets();
}
async function loadOffsets(){
  try{
    var r=await fetch('/offsets');
    var j=await r.json();
    document.getElementById('in_n').value=j.n;
    document.getElementById('in_p').value=j.p;
    document.getElementById('in_k').value=j.k;
    document.getElementById('in_ph').value=j.ph;
    document.getElementById('in_ec').value=j.ec;
    document.getElementById('in_moist').value=j.moist;
    document.getElementById('in_temp').value=j.temp;
  }catch(e){}
}
setInterval(fetchData,2000);
fetchData();
loadOffsets();
</script>
</body>
</html>
)rawliteral";
  return page;
}

// ---------- Setup ----------
void setup() {
  Serial.begin(115200);
  Serial.println("Starting ESP32 Soil Sensor Access Point...");

  // Load saved offsets from NVS
  prefs.begin("offsets", true); // read-only
  offset_N     = prefs.getFloat("n",    0);
  offset_P     = prefs.getFloat("p",    0);
  offset_K     = prefs.getFloat("k",    0);
  offset_pH    = prefs.getFloat("ph",   0);
  offset_EC    = prefs.getFloat("ec",   0);
  offset_Moist = prefs.getFloat("moist",0);
  offset_Temp  = prefs.getFloat("temp", 0);
  prefs.end();

  // RS485 DE/RE pin
  pinMode(DE_RE_PIN, OUTPUT);
  digitalWrite(DE_RE_PIN, LOW); // default receive mode

  // RS485 + Modbus
  Serial2.begin(4800, SERIAL_8N1, RX2_PIN, TX2_PIN);
  node.begin(1, Serial2);
  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);

  // WiFi AP
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  Serial.print("Access Point IP: ");
  Serial.println(WiFi.softAPIP());

  // Web routes
  server.on("/", []() { server.send(200, "text/html", htmlPage()); });

  server.on("/data", []() {
    String json = "{";
    json += "\"N\":"     + String(N)     + ",";
    json += "\"P\":"     + String(P)     + ",";
    json += "\"K\":"     + String(K)     + ",";
    json += "\"pH\":"    + String(pH)    + ",";
    json += "\"EC\":"    + String(EC)    + ",";
    json += "\"Moist\":" + String(Moist) + ",";
    json += "\"Temp\":"  + String(Temp);
    json += "}";
    server.send(200, "application/json", json);
  });

  server.on("/offsets", HTTP_GET, []() {
    String json = "{";
    json += "\"n\":"    + String(offset_N)     + ",";
    json += "\"p\":"    + String(offset_P)     + ",";
    json += "\"k\":"    + String(offset_K)     + ",";
    json += "\"ph\":"   + String(offset_pH)    + ",";
    json += "\"ec\":"   + String(offset_EC)    + ",";
    json += "\"moist\":" + String(offset_Moist) + ",";
    json += "\"temp\":" + String(offset_Temp);
    json += "}";
    server.send(200, "application/json", json);
  });

  server.on("/offsets", HTTP_POST, []() {
    if (server.hasArg("n"))     offset_N     = server.arg("n").toFloat();
    if (server.hasArg("p"))     offset_P     = server.arg("p").toFloat();
    if (server.hasArg("k"))     offset_K     = server.arg("k").toFloat();
    if (server.hasArg("ph"))    offset_pH    = server.arg("ph").toFloat();
    if (server.hasArg("ec"))    offset_EC    = server.arg("ec").toFloat();
    if (server.hasArg("moist")) offset_Moist = server.arg("moist").toFloat();
    if (server.hasArg("temp"))  offset_Temp  = server.arg("temp").toFloat();
    prefs.begin("offsets", false); // read-write
    prefs.putFloat("n",     offset_N);
    prefs.putFloat("p",     offset_P);
    prefs.putFloat("k",     offset_K);
    prefs.putFloat("ph",    offset_pH);
    prefs.putFloat("ec",    offset_EC);
    prefs.putFloat("moist", offset_Moist);
    prefs.putFloat("temp",  offset_Temp);
    prefs.end();
    server.send(200, "text/plain", "OK");
  });

  server.begin();
}

// ---------- Loop ----------
void loop() {
  server.handleClient();

  if (millis() - lastRead >= READ_INTERVAL) {
    lastRead = millis();

    uint8_t result = node.readHoldingRegisters(0x0000, 7);
    if (result == node.ku8MBSuccess) {
      Moist = node.getResponseBuffer(0) / 10.0 + offset_Moist;
      Temp  = node.getResponseBuffer(1) / 10.0 + offset_Temp;
      EC    = node.getResponseBuffer(2) + offset_EC;
      pH    = node.getResponseBuffer(3) / 10.0 + offset_pH;
      N     = node.getResponseBuffer(4) / 10.0 + offset_N;
      P     = node.getResponseBuffer(5) / 10.0 + offset_P;
      K     = node.getResponseBuffer(6) / 10.0 + offset_K;
    } else {
      // Reset to -1 on failed read so dashboard shows ERR
      N = P = K = pH = EC = Moist = Temp = -1;
      Serial.printf("Modbus error: 0x%02X\n", result);
    }
  }
}
