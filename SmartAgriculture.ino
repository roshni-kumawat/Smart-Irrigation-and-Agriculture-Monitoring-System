#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "DHT.h"

#define DHTPIN D2       // DHT11 connected to D2
#define DHTTYPE DHT11
#define SOIL_PIN A0     // Soil moisture sensor analog pin
#define PUMP_PIN D1     // Pump relay control pin

// WiFi credentials
const char* ssid = "ToRR";
const char* password = "torr@12345";

// Objects
ESP8266WebServer server(80);
DHT dht(DHTPIN, DHTTYPE);

// Globals
float temperature = 0.0;
float humidity = 0.0;
int soilMoisture = 0;
bool pumpStatus = false;

void setup() {
  Serial.begin(115200);
  dht.begin();

  pinMode(PUMP_PIN, OUTPUT);
  digitalWrite(PUMP_PIN, HIGH); // Pump OFF initially (active LOW relay)

  // WiFi Connection
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Routes
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.on("/pump", handlePump);

  server.begin();
  Serial.println("Server started!");
}

void loop() {
  server.handleClient();
}

// Dashboard Page
void handleRoot() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<script src='https://cdn.jsdelivr.net/npm/chart.js'></script>";
  html += "<style>";
  html += "body { font-family: Arial; text-align: center; background: #f4f4f4; }";
  html += "h1 { background: #16a085; color: white; padding: 15px; border-radius: 10px; }";
  html += ".card { display:inline-block; margin:20px; padding:20px; background:white; border-radius:15px; box-shadow:0 4px 6px rgba(0,0,0,0.2); }";
  html += "button { font-size:20px; padding:12px 25px; margin:15px; border:none; border-radius:8px; cursor:pointer; }";
  html += ".on { background:#2ecc71; color:white; }";
  html += ".off { background:#e74c3c; color:white; }";
  html += "</style></head><body>";
  html += "<h1>🌱 Smart Agriculture Dashboard</h1>";

  // Cards with charts
  html += "<div class='card'><canvas id='tempChart' width='200' height='200'></canvas><p>Temperature (°C)</p></div>";
  html += "<div class='card'><canvas id='humChart' width='200' height='200'></canvas><p>Humidity (%)</p></div>";
  html += "<div class='card'><canvas id='soilChart' width='200' height='200'></canvas><p>Soil Moisture (%)</p></div>";

  // Pump Control
  html += "<div class='card'><h2>💧 Water Pump</h2>";
  html += "<p id='pumpStatus'>Status: OFF</p>";
  html += "<button id='pumpBtn' class='on' onclick='togglePump()'>Turn ON</button></div>";

  // Script
  html += "<script>";
  html += "var tempChart, humChart, soilChart;";
  html += "function createChart(id, label, color){ return new Chart(document.getElementById(id), { type:'doughnut', data:{ labels:[label], datasets:[{ data:[0,100], backgroundColor:[color,'#ecf0f1'] }]}, options:{ cutout:'70%', plugins:{ legend:{display:false} } } }); }";
  html += "function updateCharts(data){ tempChart.data.datasets[0].data=[data.temperature,100-data.temperature]; tempChart.update(); humChart.data.datasets[0].data=[data.humidity,100-data.humidity]; humChart.update(); soilChart.data.datasets[0].data=[data.soil,100-data.soil]; soilChart.update(); document.getElementById('pumpStatus').innerHTML = 'Status: ' + (data.pump ? '<span style=color:green>ON</span>' : '<span style=color:red>OFF</span>'); document.getElementById('pumpBtn').innerHTML = data.pump ? 'Turn OFF' : 'Turn ON'; document.getElementById('pumpBtn').className = data.pump ? 'off' : 'on'; }";
  html += "function fetchData(){ fetch('/data').then(res=>res.json()).then(data=>{ updateCharts(data); }); }";
  html += "function togglePump(){ fetch('/pump').then(res=>res.json()).then(data=>{ updateCharts(data); }); }";
  html += "window.onload=function(){ tempChart=createChart('tempChart','Temp','#e67e22'); humChart=createChart('humChart','Hum','#3498db'); soilChart=createChart('soilChart','Soil','#2ecc71'); setInterval(fetchData,2000); }";
  html += "</script>";

  html += "</body></html>";
  server.send(200, "text/html", html);
}

// JSON Data API
void handleData() {
  // Read sensors
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();
  soilMoisture = analogRead(SOIL_PIN);
  int soilPercent = map(soilMoisture, 1023, 200, 0, 100); // Adjust calibration!

  String json = "{";
  json += "\"temperature\":" + String(temperature) + ",";
  json += "\"humidity\":" + String(humidity) + ",";
  json += "\"soil\":" + String(soilPercent) + ",";
  json += "\"pump\":" + String(pumpStatus ? "true" : "false");
  json += "}";
  server.send(200, "application/json", json);
}

// Pump Toggle API
void handlePump() {
  pumpStatus = !pumpStatus;
  digitalWrite(PUMP_PIN, pumpStatus ? LOW : HIGH); // Relay active LOW
  handleData(); // Return updated status
}
