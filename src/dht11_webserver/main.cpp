#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DHT.h>
#include <LittleFS.h>
#include <PubSubClient.h>

#define DHTPIN D5
#define DHTTYPE DHT11

const char *credentialsFile = "/wifi.txt";
const char *apName = "ESP-DHT11-Setup";

DHT dht(DHTPIN, DHTTYPE);
ESP8266WebServer server(80);
// non-blocking restart scheduler (millis)
unsigned long restartAt = 0;
// Upload streaming file handle (avoid buffering upload in RAM)
File uploadFile;

// MQTT
WiFiClient espClient;
PubSubClient mqttClient(espClient);
const char *mqttServer = "test.mosquitto.org";
const uint16_t mqttPort = 1883;
// fallback broker when primary fails repeatedly
const char *mqttFallbackServer = "broker.hivemq.com";
const uint16_t mqttFallbackPort = 1883;
// track which broker is currently in use (for status reporting in the UI)
String mqttActiveBroker = "";
// update interval configurable via web UI (milliseconds)
uint32_t updateIntervalMs = 3000; // default 3s

bool mqttEnsureConnected();
String mqttCurrentBroker()
{
  return mqttActiveBroker;
}

String contentType(const String &path)
{
  String p = path;
  p.toLowerCase();
  if (p.endsWith(".html"))
    return "text/html";
  if (p.endsWith(".css"))
    return "text/css";
  if (p.endsWith(".js"))
    return "application/javascript";
  if (p.endsWith(".json"))
    return "application/json";
  if (p.endsWith(".png"))
    return "image/png";
  if (p.endsWith(".jpg") || p.endsWith(".jpeg"))
    return "image/jpeg";
  if (p.endsWith(".svg"))
    return "image/svg+xml";
  if (p.endsWith(".txt"))
    return "text/plain";
  return "application/octet-stream";
}

// main page moved to LittleFS `data/index.html`.
// Removed inline HTML to reduce firmware size and RAM usage.

// config page moved to LittleFS `data/config.html`.
// Removed inline HTML to reduce firmware size and RAM usage.

bool saveCredentials(const String &ssid, const String &pass)
{
  File f = LittleFS.open(credentialsFile, "w");
  if (!f)
    return false;
  f.println(ssid);
  f.println(pass);
  f.close();
  return true;
}

bool loadCredentials(String &ssid, String &pass)
{
  if (!LittleFS.exists(credentialsFile))
    return false;
  File f = LittleFS.open(credentialsFile, "r");
  if (!f)
    return false;
  ssid = f.readStringUntil('\n');
  ssid.trim();
  pass = f.readStringUntil('\n');
  pass.trim();
  f.close();
  return ssid.length() > 0;
}

void handleRoot()
{
  // Prefer serving `index.html` from LittleFS when present (separate page file)
  if (LittleFS.exists("/index.html"))
  {
    File f = LittleFS.open("/index.html", "r");
    if (f)
    {
      server.streamFile(f, "text/html");
      f.close();
      return;
    }
  }
  // Fallback to embedded page if no file is present
  // se existir uma página 404 no FS, servi-la com status 404
  if (LittleFS.exists("/404.html"))
  {
    File f404 = LittleFS.open("/404.html", "r");
    if (f404)
    {
      server.streamFile(f404, "text/html");
      f404.close();
      return;
    }
  }
  server.send(404, "text/plain; charset=utf-8", "404 Not Found");
}

void handleRead()
{
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  if (isnan(h) || isnan(t))
  {
    server.send(500, "application/json; charset=utf-8", "{\"error\":\"Leitura falhou\"}");
    return;
  }
  String out = "{\"temp\":" + String(t) + ",\"hum\":" + String(h) + "}";
  server.send(200, "application/json; charset=utf-8", out);
}

void handleConfig()
{
  // Prefer serving `config.html` from LittleFS when present
  if (LittleFS.exists("/config.html"))
  {
    File f = LittleFS.open("/config.html", "r");
    if (f)
    {
      server.streamFile(f, "text/html");
      f.close();
      return;
    }
  }
  // Fallback to embedded page
  if (LittleFS.exists("/404.html"))
  {
    File f404 = LittleFS.open("/404.html", "r");
    if (f404)
    {
      server.streamFile(f404, "text/html");
      f404.close();
      return;
    }
  }
  server.send(404, "text/plain; charset=utf-8", "404 Not Found");
}

void handleMqttPublish()
{
  // expects form field 'topic'
  if (!server.hasArg("topic"))
  {
    server.send(400, "text/plain; charset=utf-8", "Falta o campo topic");
    return;
  }
  String topic = server.arg("topic");
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  if (isnan(h) || isnan(t))
  {
    server.send(500, "text/plain; charset=utf-8", "Leitura do sensor falhou");
    return;
  }
  String payload = "{\"temp\":" + String(t) + ",\"hum\":" + String(h) + "}";
  // ensure MQTT server configured
  if (WiFi.status() != WL_CONNECTED)
  {
    server.send(500, "text/plain; charset=utf-8", "WiFi não conectado");
    return;
  }
  if (!mqttEnsureConnected())
  {
    server.send(500, "text/plain; charset=utf-8", "Falha ao conectar MQTT");
    return;
  }
  bool ok = mqttClient.publish(topic.c_str(), payload.c_str());
  if (ok)
    server.send(200, "text/plain; charset=utf-8", "Publicado");
  else
    server.send(500, "text/plain; charset=utf-8", "Falha ao publicar");
}

void handleSave()
{
  String s = server.arg("ssid");
  String p = server.arg("pass");
  if (s.length() == 0)
  {
    server.send(400, "text/plain; charset=utf-8", "SSID vazio");
    return;
  }
  if (!saveCredentials(s, p))
  {
    server.send(500, "text/plain; charset=utf-8", "Falha ao salvar");
    return;
  }
  // Servir página de confirmação/contagem regressiva a partir do LittleFS
  if (LittleFS.exists("/saved.html"))
  {
    File f = LittleFS.open("/saved.html", "r");
    if (f)
    {
      server.streamFile(f, "text/html");
      f.close();
    }
    else
    {
      server.send(500, "text/plain; charset=utf-8", "Erro ao abrir /saved.html");
    }
  }
  else
  {
    server.send(500, "text/plain; charset=utf-8", "Erro: /saved.html não encontrado no LittleFS");
  }
  // Safety: agendar reinício mesmo que o cliente não peça `/restart` (8s)
  restartAt = millis() + 8000UL;
}

// handleStatus removed — simplified flow: save then restart; no polling/status endpoint

void handleClear()
{
  if (LittleFS.exists(credentialsFile))
  {
    bool ok = LittleFS.remove(credentialsFile);
    if (ok)
    {
      server.send(200, "text/plain; charset=utf-8", "Credenciais apagadas. Reiniciando...");
      restartAt = millis() + 1000UL;
      return;
    }
    server.send(500, "text/plain; charset=utf-8", "Falha ao apagar credenciais");
  }
  else
  {
    server.send(200, "text/plain; charset=utf-8", "Nenhuma credencial encontrada");
  }
}

void startAPMode()
{
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(apName);
  // Log SoftAP SSID and IP for easier debugging
  Serial.print("SoftAP SSID: ");
  Serial.println(apName);
  Serial.print("SoftAP IP: ");
  Serial.println(WiFi.softAPIP());
  server.on("/", handleConfig);
  server.on("/save", HTTP_POST, handleSave);
  server.on("/mqtt/publish", HTTP_POST, handleMqttPublish);
  // simple ping for connectivity testing
  server.on("/ping", HTTP_GET, []()
            { server.send(200, "text/plain", "pong"); });
  // whoami endpoint for diagnostics
  server.on("/whoami", HTTP_GET, []()
            {
    bool connected = WiFi.status() == WL_CONNECTED;
    bool verbose = server.hasArg("verbose") && server.arg("verbose") == "1";
    static unsigned long lastWhoamiLogAP = 0;
    static bool lastWhoamiConnectedAP = false;
    bool currentConnectedAP = mqttClient.connected();
    if (verbose) {
      Serial.println("/whoami requested (AP mode) [verbose]");
      Serial.print("WiFi mode: "); Serial.println(WiFi.getMode());
      Serial.print("STA IP: "); Serial.println(WiFi.localIP());
      Serial.print("AP IP: "); Serial.println(WiFi.softAPIP());
      Serial.print("MQTT connected: "); Serial.println(currentConnectedAP);
      Serial.print("MQTT active broker: "); Serial.println(mqttActiveBroker);
      lastWhoamiConnectedAP = currentConnectedAP;
      lastWhoamiLogAP = millis();
    } else {
      // log when connected state changes, otherwise at most once every 60s
      if (currentConnectedAP != lastWhoamiConnectedAP) {
        Serial.print("/whoami (AP) state change -> mqtt_connected="); Serial.println(currentConnectedAP);
        lastWhoamiConnectedAP = currentConnectedAP;
        lastWhoamiLogAP = millis();
      } else if (millis() - lastWhoamiLogAP > 60000UL) {
        Serial.print("/whoami (AP) periodic -> mqtt_connected="); Serial.println(currentConnectedAP);
        lastWhoamiLogAP = millis();
      }
    }
    String out = "{";
    out += "\"wifi_connected\":" + String(connected?"true":"false") + ",";
    out += "\"sta_ip\":\"" + String(WiFi.localIP().toString()) + "\",";
    out += "\"ap_ip\":\"" + String(WiFi.softAPIP().toString()) + "\",";
    out += "\"mqtt_connected\":" + String(mqttClient.connected()?"true":"false") + ",";
    out += "\"mqtt_broker\":\"" + mqttActiveBroker + "\"";
    out += "}";
    server.sendHeader("Cache-Control", "no-store");
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "application/json; charset=utf-8", out); });
  // FS management: listar arquivos
  server.on("/fs/list", HTTP_GET, []()
            {
    String out = "[";
    Dir dir = LittleFS.openDir("/");
    bool first = true;
    while (dir.next()) {
      if (!first) out += ",";
      first = false;
      out += "{\"name\":\"" + dir.fileName() + "\",\"size\":" + String(dir.fileSize()) + "}";
    }
    out += "]";
    server.send(200, "application/json; charset=utf-8", out); });
  // Serve UI page for filesystem management
  server.on("/fs", HTTP_GET, []()
            {
    if (LittleFS.exists("/fs.html")){
      File f = LittleFS.open("/fs.html", "r");
      if (f){ server.streamFile(f, "text/html"); f.close(); return; }
    }
    if (LittleFS.exists("/404.html")){
      File f404 = LittleFS.open("/404.html", "r"); if (f404){ server.streamFile(f404, "text/html"); f404.close(); return; }
    }
    server.send(404, "text/plain; charset=utf-8", "404 Not Found"); });
  // FS download: /fs/download?name=/file
  server.on("/fs/download", HTTP_GET, []()
            {
    if (!server.hasArg("name")) { server.send(400, "text/plain", "Missing 'name' parameter"); return; }
    String name = server.arg("name");
    if (!name.startsWith("/")) name = "/" + name;
    if (!LittleFS.exists(name)) { server.send(404, "text/plain", "File not found"); return; }
    File f = LittleFS.open(name, "r");
    if (!f) { server.send(500, "text/plain", "Failed to open file"); return; }
    server.streamFile(f, contentType(name));
    f.close(); });
  // FS delete (POST form with field 'name')
  server.on("/fs/delete", HTTP_POST, []()
            {
    if (!server.hasArg("name")) { server.send(400, "text/plain", "Missing 'name'"); return; }
    String name = server.arg("name");
    if (!name.startsWith("/")) name = "/" + name;
    if (!LittleFS.exists(name)) { server.send(404, "text/plain", "File not found"); return; }
    bool ok = LittleFS.remove(name);
    if (ok) server.send(200, "text/plain", "Deleted"); else server.send(500, "text/plain", "Failed to delete"); });
  // Also accept GET for convenience: /fs/delete?name=/file
  server.on("/fs/delete", HTTP_GET, []()
            {
    if (!server.hasArg("name")) { server.send(400, "text/plain", "Missing 'name' parameter"); return; }
    String name = server.arg("name");
    if (!name.startsWith("/")) name = "/" + name;
    if (!LittleFS.exists(name)) { server.send(404, "text/plain", "File not found"); return; }
    bool ok = LittleFS.remove(name);
    if (ok) server.send(200, "text/plain", "Deleted"); else server.send(500, "text/plain", "Failed to delete"); });
  // Upload endpoint: stream file directly to LittleFS to avoid buffering in RAM
  server.on("/upload", HTTP_POST, []()
            { server.send(200, "text/plain; charset=utf-8", "Upload finalizado"); }, []()
            {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      String filename = upload.filename;
      if (!filename.startsWith("/")) filename = "/" + filename;
      if (filename.length() == 1) filename = "/upload.bin";
      if (uploadFile) uploadFile.close();
      uploadFile = LittleFS.open(filename, "w");
      if (!uploadFile) {
        Serial.print("Falha ao abrir para upload: "); Serial.println(filename);
      } else {
        Serial.print("Iniciando upload: "); Serial.println(filename);
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      if (uploadFile) {
        uploadFile.write(upload.buf, upload.currentSize);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (uploadFile) {
        uploadFile.close();
        Serial.print("Upload finalizado, bytes: "); Serial.println(upload.totalSize);
      }
    } });
  server.on("/restart", HTTP_POST, []()
            { server.send(200, "text/plain", "Reiniciando..."); restartAt = millis() + 200UL; });
  server.on("/scan", HTTP_GET, []()
            {
    int n = WiFi.scanNetworks();
    String out = "[";
    for(int i=0;i<n;i++){
      String ssid = WiFi.SSID(i);
      int rssi = WiFi.RSSI(i);
      bool secure = WiFi.encryptionType(i) != ENC_TYPE_NONE;
      out += "{\"ssid\":\"" + ssid + "\",\"rssi\":" + String(rssi) + ",\"secure\":" + String(secure?1:0) + "}";
      if(i < n-1) out += ",";
    }
    out += "]";
    WiFi.scanDelete();
    server.send(200, "application/json; charset=utf-8", out); });
  server.on("/clear", HTTP_POST, handleClear);
  // settings: get/set update interval (seconds)
  server.on("/settings", HTTP_GET, []()
            {
      String out = "{\"interval\":" + String(updateIntervalMs/1000) + "}";
      server.send(200, "application/json; charset=utf-8", out); });
  server.on("/settings", HTTP_POST, []()
            {
      if (!server.hasArg("interval")) { server.send(400, "text/plain", "Missing 'interval'"); return; }
      String v = server.arg("interval");
      uint32_t secs = v.toInt();
      if (secs < 1) secs = 1;
      if (secs > 3600) secs = 3600;
      updateIntervalMs = secs * 1000UL;
      server.send(200, "text/plain", "OK"); });
  // MQTT status endpoint (used by web UI)
  server.on("/mqtt/status", HTTP_GET, []()
            {
      bool connected = mqttClient.connected();
      Serial.print("/mqtt/status requested (STA mode). connected="); Serial.println(connected);
      String out = "{";
      out += "\"connected\":" + String(connected?"true":"false") + ",";
      out += "\"broker\":\"" + mqttActiveBroker + "\"";
      out += "}";
      server.sendHeader("Cache-Control", "no-store");
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.send(200, "application/json; charset=utf-8", out); });
  // Global fallback: servir página 404 customizada para qualquer rota não encontrada
  server.onNotFound([]()
                    {
    if (LittleFS.exists("/404.html")) {
      File f404 = LittleFS.open("/404.html", "r");
      if (f404) {
        server.streamFile(f404, "text/html");
        f404.close();
        return;
      }
    }
    server.send(404, "text/plain; charset=utf-8", "404 Not Found"); });
  server.begin();
}

void setup()
{
  Serial.begin(115200);
  delay(10);
  LittleFS.begin();
  // Lista arquivos presentes no LittleFS para debug/verificação
  Serial.println("LittleFS contents:");
  Dir dir = LittleFS.openDir("/");
  while (dir.next())
  {
    Serial.print(" - ");
    Serial.print(dir.fileName());
    Serial.print(" (");
    Serial.print(dir.fileSize());
    Serial.println(" bytes)");
  }
  dht.begin();

  WiFi.mode(WIFI_STA);
  String ssid, pass;
  bool haveCreds = loadCredentials(ssid, pass);
  bool connected = false;
  if (haveCreds)
  {
    WiFi.begin(ssid.c_str(), pass.c_str());
    Serial.print("Tentando conectar a: ");
    Serial.println(ssid);
    int tries = 0;
    while (WiFi.status() != WL_CONNECTED && tries < 30)
    {
      delay(500);
      Serial.print('.');
      tries++;
    }
    if (WiFi.status() == WL_CONNECTED)
    {
      Serial.println();
      Serial.print("IP: ");
      Serial.println(WiFi.localIP());
      connected = true;
    }
    else
    {
      Serial.println();
      Serial.println("Falha ao conectar com credenciais salvas");
    }
  }

  // --- Diagnostic: print network info and test internet connectivity ---
  auto printNetworkDiagnostics = [&]()
  {
    Serial.println("--- Network diagnostics ---");
    Serial.print("Status: ");
    Serial.println(WiFi.status());
    Serial.print("Local IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("Subnet: ");
    Serial.println(WiFi.subnetMask());
    Serial.print("Gateway: ");
    Serial.println(WiFi.gatewayIP());
    Serial.print("DNS: ");
    Serial.println(WiFi.dnsIP());
    Serial.print("RSSI: ");
    Serial.println(WiFi.RSSI());
    Serial.print("MAC: ");
    Serial.println(WiFi.macAddress());

    // Try DNS resolution for a known host
    IPAddress resolved;
    Serial.print("Resolving example.com... ");
    if (WiFi.hostByName("example.com", resolved))
    {
      Serial.print("OK -> ");
      Serial.println(resolved);
      // quick TCP connect test (port 80)
      WiFiClient c;
      Serial.print("Connecting to example.com:80...");
      if (c.connect(resolved, 80))
      {
        Serial.println(" connected");
        c.println("GET / HTTP/1.0\r\nHost: example.com\r\n\r\n");
        unsigned long t = millis();
        while (c.connected() && millis() - t < 2000)
        {
          while (c.available())
          {
            String line = c.readStringUntil('\n');
            Serial.println(line);
          }
        }
        c.stop();
      }
      else
      {
        Serial.println(" failed to connect");
      }
    }
    else
    {
      Serial.println("FAILED");
    }
    Serial.println("--- end diagnostics ---");
  };

  // Run diagnostics once after trying to connect (helps compare home WiFi vs notebook hotspot)
  printNetworkDiagnostics();

  // --- MQTT diagnostics: resolve and attempt direct TCP connect to MQTT broker(s) ---
  auto printMQTTDiagnostics = [&]()
  {
    Serial.println("--- MQTT diagnostics ---");
    Serial.print("Configured broker: ");
    Serial.print(mqttServer);
    Serial.print(":");
    Serial.println(mqttPort);
    IPAddress maddr;
    Serial.print("Resolving broker host... ");
    if (WiFi.hostByName(mqttServer, maddr))
    {
      Serial.print("OK -> ");
      Serial.println(maddr);
      WiFiClient mc;
      Serial.print("Connecting to broker ");
      Serial.print(maddr);
      Serial.print(":");
      Serial.print(mqttPort);
      if (mc.connect(maddr, mqttPort))
      {
        Serial.println(" connected");
        mc.stop();
      }
      else
      {
        Serial.println(" failed to connect");
      }
    }
    else
    {
      Serial.println("FAILED to resolve broker host");
    }

    // Fallback test to another public broker
    const char *altBroker = "broker.hivemq.com";
    IPAddress altAddr;
    Serial.print("Resolving fallback broker ");
    Serial.print(altBroker);
    Serial.print("... ");
    if (WiFi.hostByName(altBroker, altAddr))
    {
      Serial.print("OK -> ");
      Serial.println(altAddr);
      WiFiClient ac;
      Serial.print("Connecting to fallback ");
      Serial.print(altAddr);
      Serial.print(":1883");
      if (ac.connect(altAddr, 1883))
      {
        Serial.println(" connected");
        ac.stop();
      }
      else
      {
        Serial.println(" failed to connect");
      }
    }
    else
    {
      Serial.println("FAILED to resolve fallback broker");
    }
    Serial.println("--- end MQTT diagnostics ---");
  };

  printMQTTDiagnostics();

  if (!connected)
  {
    startAPMode();
    Serial.println("Modo AP iniciado. Conecte-se à rede do ESP para configurar.");
  }
  else
  {
    server.on("/", handleRoot);
    server.on("/read", handleRead);
    server.on("/mqtt/publish", HTTP_POST, handleMqttPublish);
    // FS management: listar arquivos
    server.on("/fs/list", HTTP_GET, []()
              {
      String out = "[";
      Dir dir = LittleFS.openDir("/");
      bool first = true;
      while (dir.next()) {
        if (!first) out += ",";
        first = false;
        out += "{\"name\":\"" + dir.fileName() + "\",\"size\":" + String(dir.fileSize()) + "}";
      }
      out += "]";
      server.send(200, "application/json; charset=utf-8", out); });
    // simple ping for connectivity testing (STA mode)
    server.on("/ping", HTTP_GET, []()
              { server.send(200, "text/plain", "pong"); });
    // whoami endpoint for diagnostics (STA mode)
    server.on("/whoami", HTTP_GET, []()
              {
      bool connected = WiFi.status() == WL_CONNECTED;
      bool verbose = server.hasArg("verbose") && server.arg("verbose") == "1";
      static unsigned long lastWhoamiLogSTA = 0;
      static bool lastWhoamiConnectedSTA = false;
      bool currentConnectedSTA = mqttClient.connected();
      if (verbose) {
        Serial.println("/whoami requested (STA mode) [verbose]");
        Serial.print("WiFi mode: "); Serial.println(WiFi.getMode());
        Serial.print("STA IP: "); Serial.println(WiFi.localIP());
        Serial.print("AP IP: "); Serial.println(WiFi.softAPIP());
        Serial.print("MQTT connected: "); Serial.println(currentConnectedSTA);
        Serial.print("MQTT active broker: "); Serial.println(mqttActiveBroker);
        lastWhoamiConnectedSTA = currentConnectedSTA;
        lastWhoamiLogSTA = millis();
      } else {
        // log when connected state changes, otherwise at most once every 60s
        if (currentConnectedSTA != lastWhoamiConnectedSTA) {
          Serial.print("/whoami (STA) state change -> mqtt_connected="); Serial.println(currentConnectedSTA);
          lastWhoamiConnectedSTA = currentConnectedSTA;
          lastWhoamiLogSTA = millis();
        } else if (millis() - lastWhoamiLogSTA > 60000UL) {
          Serial.print("/whoami (STA) periodic -> mqtt_connected="); Serial.println(currentConnectedSTA);
          lastWhoamiLogSTA = millis();
        }
      }
      String out = "{";
      out += "\"wifi_connected\":" + String(connected?"true":"false") + ",";
      out += "\"sta_ip\":\"" + String(WiFi.localIP().toString()) + "\",";
      out += "\"ap_ip\":\"" + String(WiFi.softAPIP().toString()) + "\",";
      out += "\"mqtt_connected\":" + String(mqttClient.connected()?"true":"false") + ",";
      out += "\"mqtt_broker\":\"" + mqttActiveBroker + "\"";
      out += "}";
      server.sendHeader("Cache-Control", "no-store");
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.send(200, "application/json; charset=utf-8", out); });
    // Serve UI page for filesystem management
    server.on("/fs", HTTP_GET, []()
              {
      if (LittleFS.exists("/fs.html")){
        File f = LittleFS.open("/fs.html", "r");
        if (f){ server.streamFile(f, "text/html"); f.close(); return; }
      }
      if (LittleFS.exists("/404.html")){
        File f404 = LittleFS.open("/404.html", "r"); if (f404){ server.streamFile(f404, "text/html"); f404.close(); return; }
      }
      server.send(404, "text/plain; charset=utf-8", "404 Not Found"); });
    // FS download: /fs/download?name=/file
    server.on("/fs/download", HTTP_GET, []()
              {
      if (!server.hasArg("name")) { server.send(400, "text/plain", "Missing 'name' parameter"); return; }
      String name = server.arg("name");
      if (!name.startsWith("/")) name = "/" + name;
      if (!LittleFS.exists(name)) { server.send(404, "text/plain", "File not found"); return; }
      File f = LittleFS.open(name, "r");
      if (!f) { server.send(500, "text/plain", "Failed to open file"); return; }
      server.streamFile(f, contentType(name));
      f.close(); });
    // FS delete (POST form with field 'name')
    server.on("/fs/delete", HTTP_POST, []()
              {
      if (!server.hasArg("name")) { server.send(400, "text/plain", "Missing 'name'"); return; }
      String name = server.arg("name");
      if (!name.startsWith("/")) name = "/" + name;
      if (!LittleFS.exists(name)) { server.send(404, "text/plain", "File not found"); return; }
      bool ok = LittleFS.remove(name);
      if (ok) server.send(200, "text/plain", "Deleted"); else server.send(500, "text/plain", "Failed to delete"); });
    // Also accept GET for convenience: /fs/delete?name=/file
    server.on("/fs/delete", HTTP_GET, []()
              {
      if (!server.hasArg("name")) { server.send(400, "text/plain", "Missing 'name' parameter"); return; }
      String name = server.arg("name");
      if (!name.startsWith("/")) name = "/" + name;
      if (!LittleFS.exists(name)) { server.send(404, "text/plain", "File not found"); return; }
      bool ok = LittleFS.remove(name);
      if (ok) server.send(200, "text/plain", "Deleted"); else server.send(500, "text/plain", "Failed to delete"); });
    // Upload endpoint: stream file directly to LittleFS to avoid buffering in RAM
    server.on("/upload", HTTP_POST, []()
              { server.send(200, "text/plain; charset=utf-8", "Upload finalizado"); }, []()
              {
      HTTPUpload& upload = server.upload();
      if (upload.status == UPLOAD_FILE_START) {
        String filename = upload.filename;
        if (!filename.startsWith("/")) filename = "/" + filename;
        if (filename.length() == 1) filename = "/upload.bin";
        if (uploadFile) uploadFile.close();
        uploadFile = LittleFS.open(filename, "w");
        if (!uploadFile) {
          Serial.print("Falha ao abrir para upload: "); Serial.println(filename);
        } else {
          Serial.print("Iniciando upload: "); Serial.println(filename);
        }
      } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (uploadFile) {
          uploadFile.write(upload.buf, upload.currentSize);
        }
      } else if (upload.status == UPLOAD_FILE_END) {
        if (uploadFile) {
          uploadFile.close();
          Serial.print("Upload finalizado, bytes: "); Serial.println(upload.totalSize);
        }
      } });
    server.on("/clear", HTTP_POST, handleClear);
    // settings: get/set update interval (seconds) for STA mode as well
    server.on("/settings", HTTP_GET, []()
              {
      String out = "{\"interval\":" + String(updateIntervalMs/1000) + "}";
      server.send(200, "application/json; charset=utf-8", out); });
    server.on("/settings", HTTP_POST, []()
              {
      if (!server.hasArg("interval")) { server.send(400, "text/plain", "Missing 'interval'"); return; }
      String v = server.arg("interval");
      uint32_t secs = v.toInt();
      if (secs < 1) secs = 1;
      if (secs > 3600) secs = 3600;
      updateIntervalMs = secs * 1000UL;
      server.send(200, "text/plain", "OK"); });
    server.on("/restart", HTTP_POST, []()
              { server.send(200, "text/plain", "Reiniciando..."); restartAt = millis() + 200UL; });
    // Global fallback: servir página 404 customizada para qualquer rota não encontrada
    server.onNotFound([]()
                      {
      if (LittleFS.exists("/404.html")) {
        File f404 = LittleFS.open("/404.html", "r");
        if (f404) {
          server.streamFile(f404, "text/html");
          f404.close();
          return;
        }
      }
      server.send(404, "text/plain; charset=utf-8", "404 Not Found"); });
    server.begin();
    Serial.println("HTTP server started on port 80");
    // server started in STA mode
  }
  // configure MQTT server regardless; connection attempted later when needed
  mqttClient.setServer(mqttServer, mqttPort);
}

void loop()
{
  server.handleClient();
  // maintain MQTT connection if WiFi is available
  if (WiFi.status() == WL_CONNECTED)
  {
    if (!mqttClient.connected())
    {
      mqttEnsureConnected();
    }
    mqttClient.loop();
  }
  // reinicia de forma não-bloqueante quando agendado
  if (restartAt != 0 && millis() >= restartAt)
  {
    // pequena pausa para garantir envio de resposta
    delay(20);
    ESP.restart();
  }
}

bool mqttEnsureConnected()
{
  if (mqttClient.connected())
    return true;
  if (WiFi.status() != WL_CONNECTED)
    return false;
  String clientId = "esp8266-" + String(ESP.getChipId());

  // Try primary broker up to 3 times
  Serial.print("Tentando conectar MQTT ao broker primario: ");
  Serial.print(mqttServer);
  Serial.print(":");
  Serial.println(mqttPort);
  int tries = 0;
  while (!mqttClient.connected() && tries < 3)
  {
    if (mqttClient.connect(clientId.c_str()))
    {
      Serial.println("MQTT conectado (primario)");
      mqttActiveBroker = String(mqttServer) + String(":") + String(mqttPort);
      return true;
    }
    tries++;
    Serial.print("Tentativa ");
    Serial.print(tries);
    Serial.println(" falhou, esperando antes da proxima tentativa...");
    delay(2000);
  }

  Serial.print("Primario falhou após 3 tentativas. Tentando fallback ");
  Serial.print(mqttFallbackServer);
  Serial.print(":");
  Serial.println(mqttFallbackPort);

  // attempt fallback broker (switch PubSubClient to fallback host)
  Serial.print("Configurando PubSubClient para fallback: ");
  Serial.print(mqttFallbackServer);
  Serial.print(":");
  Serial.println(mqttFallbackPort);
  // update PubSubClient to point to fallback server
  mqttClient.setServer(mqttFallbackServer, mqttFallbackPort);
  tries = 0;
  while (!mqttClient.connected() && tries < 3)
  {
    if (mqttClient.connect(clientId.c_str()))
    {
      Serial.print("MQTT conectado (fallback) -> ");
      Serial.print(mqttFallbackServer);
      Serial.print(":");
      Serial.println(mqttFallbackPort);
      mqttActiveBroker = String(mqttFallbackServer) + String(":") + String(mqttFallbackPort);
      return true;
    }
    tries++;
    Serial.print("Fallback tentativa ");
    Serial.print(tries);
    Serial.println(" falhou...");
    delay(2000);
  }

  // restore to primary server for future attempts
  mqttClient.setServer(mqttServer, mqttPort);
  Serial.println("Falha ao conectar a ambos brokers (primario e fallback)");
  mqttActiveBroker = String();
  return mqttClient.connected();
}
