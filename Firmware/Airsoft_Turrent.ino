#include <WiFi.h>
#include "esp_camera.h"
#include <ESPAsyncWebServer.h>
#include <Servo.h>


const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

#define PWDN_GPIO_NUM     -1
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

#define PAN_SERVO_PIN   14
#define TILT_SERVO_PIN  15
#define BLDC_PIN        12
#define BLDC_CH         0
#define BLDC_FREQ       50
#define BLDC_RES        16

Servo panServo;
Servo tiltServo;

AsyncWebServer server(80);


const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>ESP32-CAM Control</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
</head>
<body>
  <h2>ESP32-CAM with Pan/Tilt and BLDC Control</h2>
  <img src="/stream" width="320" />

  <h3>Servo Control</h3>
  Pan: <input type="range" id="pan" min="0" max="180" value="90" onchange="updateServo()"><br>
  Tilt: <input type="range" id="tilt" min="0" max="180" value="90" onchange="updateServo()"><br>

  <h3>BLDC Control</h3>
  Speed (µs): <input type="range" id="speed" min="1000" max="2000" value="1000" onchange="updateBLDC()"><br>

<script>
function updateServo() {
  var pan = document.getElementById("pan").value;
  var tilt = document.getElementById("tilt").value;
  fetch(`/servo?pan=${pan}&tilt=${tilt}`);
}
function updateBLDC() {
  var speed = document.getElementById("speed").value;
  fetch(`/bldc?speed=${speed}`);
}
</script>
</body>
</html>
)rawliteral";

void setupCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;
  config.pin_d0       = Y2_GPIO_NUM;
  config.pin_d1       = Y3_GPIO_NUM;
  config.pin_d2       = Y4_GPIO_NUM;
  config.pin_d3       = Y5_GPIO_NUM;
  config.pin_d4       = Y6_GPIO_NUM;
  config.pin_d5       = Y7_GPIO_NUM;
  config.pin_d6       = Y8_GPIO_NUM;
  config.pin_d7       = Y9_GPIO_NUM;
  config.pin_xclk     = XCLK_GPIO_NUM;
  config.pin_pclk     = PCLK_GPIO_NUM;
  config.pin_vsync    = VSYNC_GPIO_NUM;
  config.pin_href     = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn     = PWDN_GPIO_NUM;
  config.pin_reset    = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if(psramFound()) {
    config.frame_size = FRAMESIZE_QVGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_QQVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed: 0x%x", err);
    return;
  }
}

void startWebServer() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });

  server.on("/servo", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("pan")) {
      int pan = request->getParam("pan")->value().toInt();
      pan = constrain(pan, 0, 180);
      panServo.write(pan);
    }
    if (request->hasParam("tilt")) {
      int tilt = request->getParam("tilt")->value().toInt();
      tilt = constrain(tilt, 0, 180);
      tiltServo.write(tilt);
    }
    request->send(200, "text/plain", "Servo updated");
  });

  server.on("/bldc", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("speed")) {
      int speed = request->getParam("speed")->value().toInt();
      speed = constrain(speed, 1000, 2000);
      int duty = map(speed, 1000, 2000, 3277, 6554); // 16-bit duty
      ledcWrite(BLDC_CH, duty);
    }
    request->send(200, "text/plain", "BLDC updated");
  });

  server.on("/stream", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", "Streaming on /stream");
  });

  server.begin();
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.println(WiFi.localIP());

  panServo.attach(PAN_SERVO_PIN);
  tiltServo.attach(TILT_SERVO_PIN);
  panServo.write(90);
  tiltServo.write(90);

 
  ledcSetup(BLDC_CH, BLDC_FREQ, BLDC_RES);
  ledcAttachPin(BLDC_PIN, BLDC_CH);
  ledcWrite(BLDC_CH, 3277); // 1000us to arm ESC

  setupCamera();

  
  startWebServer();
}

void loop() {

}
