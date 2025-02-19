#include "esp_camera.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "esp_camera.h"
#include "UniversalTelegramBot.h"
#include <ThingerESP32.h>
String status;
const int pinIR =13;

//
// WARNING!!! PSRAM IC required for UXGA resolution and high JPEG quality
//            Ensure ESP32 Wrover Module or other board with PSRAM is selected
//            Partial images will be transmitted if image exceeds buffer size
//
//            You must select partition scheme from the board menu that has at least 3MB APP space.
//            Face Recognition is DISABLED for ESP32 and ESP32-S2, because it takes up from 15 
//            seconds to process single frame. Face Detection is ENABLED if PSRAM is enabled as well

// ===================
// Select camera model
// ===================
//#define CAMERA_MODEL_WROVER_KIT // Has PSRAM
//#define CAMERA_MODEL_ESP_EYE // Has PSRAM
//#define CAMERA_MODEL_ESP32S3_EYE // Has PSRAM
//#define CAMERA_MODEL_M5STACK_PSRAM // Has PSRAM
//#define CAMERA_MODEL_M5STACK_V2_PSRAM // M5Camera version B Has PSRAM
//#define CAMERA_MODEL_M5STACK_WIDE // Has PSRAM
//#define CAMERA_MODEL_M5STACK_ESP32CAM // No PSRAM
//#define CAMERA_MODEL_M5STACK_UNITCAM // No PSRAM
#define CAMERA_MODEL_AI_THINKER // Has PSRAM
//#define CAMERA_MODEL_TTGO_T_JOURNAL // No PSRAM
// ** Espressif Internal Boards **
#define CAMERA_MODEL_ESP32_CAM_BOARD
#define CAMERA_MODEL_ESP32S2_CAM_BOARD
#define CAMERA_MODEL_ESP32S3_CAM_LCD

#include "camera_pins.h"
#define USERNAME "rizaldipardede"
#define DEVICE_ID "Node_MCU"
#define DEVICE_CREDENTIAL "bBU-k9n5SJHNmTAX"
#define SSID "Kos Ceria 2"
#define SSID_PASSWORD "KosGajayana23B"
ThingerESP32 thing(USERNAME, DEVICE_ID, DEVICE_CREDENTIAL);
// #define BOT_TOKEN "5858729054:AAG8VN4J6-qXNmOdxvP8pBVdBxynJHHr16k"
// String chat_id="5317564137";

// WiFiClientSecure client;
// UniversalTelegramBot bot(BOT_TOKEN, client);

// bool hasMoreData;
// camera_fb_t * fb = NULL;

// bool hasMoreDataAvailable();
// byte* getNextBuffer();
// int getBufferLen();


// ===========================
// Enter your WiFi credentials
// ===========================
const char* ssid = "Kos Ceria 2";
const char* password = "KosGajayana23B";

void startCameraServer();

void setup() {
  WiFi.begin(SSID,SSID_PASSWORD);
  thing.add_wifi(SSID, SSID_PASSWORD);

  Serial.begin(115200);
  pinMode(pinIR, INPUT);
  Serial.setDebugOutput(true);
  Serial.println();

  
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.frame_size = FRAMESIZE_UXGA;
  config.pixel_format = PIXFORMAT_JPEG; // for streaming
  //config.pixel_format = PIXFORMAT_RGB565; // for face detection/recognition
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;
  
  // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
  //                      for larger pre-allocated frame buffer.
  if(config.pixel_format == PIXFORMAT_JPEG){
    if(psramFound()){
      config.jpeg_quality = 10;
      config.fb_count = 2;
      config.grab_mode = CAMERA_GRAB_LATEST;
    } else {
      // Limit the frame size when PSRAM is not available
      config.frame_size = FRAMESIZE_SVGA;
      config.fb_location = CAMERA_FB_IN_DRAM;
    }
  } else {
    // Best option for face detection/recognition
    config.frame_size = FRAMESIZE_240X240;
#if CONFIG_IDF_TARGET_ESP32S3
    config.fb_count = 2;
#endif
  }

#if defined(CAMERA_MODEL_ESP_EYE)
  pinMode(13, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
#endif

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t * s = esp_camera_sensor_get();
  // initial sensors are flipped vertically and colors are a bit saturated
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1); // flip it back
    s->set_brightness(s, 1); // up the brightness just a bit
    s->set_saturation(s, -2); // lower the saturation
  }
  // drop down frame size for higher initial frame rate
  if(config.pixel_format == PIXFORMAT_JPEG){
    s->set_framesize(s, FRAMESIZE_QVGA);
  }

#if defined(CAMERA_MODEL_M5STACK_WIDE) || defined(CAMERA_MODEL_M5STACK_ESP32CAM)
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);
#endif

#if defined(CAMERA_MODEL_ESP32S3_EYE)
  s->set_vflip(s, 1);
#endif

  WiFi.begin(ssid, password);
  WiFi.setSleep(false);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  startCameraServer();

  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");

  

  thing["SensorInfrared"] >> [](pson & out){
    out["Status_sensor"] = status;
  };
}

void loop() {
  
  int sensorState=digitalRead(pinIR);
  if(sensorState== LOW){
    Serial.println("Terdeteksi");
    status = "Terdeteksi";
    // // alerts2Telegram("5685013857:AAHu8neOBJN3O5nYvME4maPrDZsviLlhils","1677558177");
    // Serial.println("Chat id:");
    // Serial.println("Msg: Terdeteksi");
    // // sendImage("1677558177");
  }
  else {
    Serial.println("Tidak Terdeteksi");
    status = "Tidak Terdeteksi";

  }
  delay(50);
  
}

// void sendImage(String chat_id) { 
//   Serial.println("Sending Image");
//   fb = NULL;
//   fb = esp_camera_fb_get();
//   hasMoreData = true;

//   Serial.println(fb->len);
  
//   bot.sendPhotoByBinary(chat_id, "image/jpeg", fb->len, hasMoreDataAvailable, nullptr, getNextBuffer, getBufferLen);
  
//   esp_camera_fb_return(fb);

// }


// int getBufferLen() {
// Serial.println("Buffer len");
//  if (fb)
//    return fb->len;

//   return 0;
// }

// byte* getNextBuffer() {
//   Serial.println("Next Buffer ");
//   if (fb)
//     return fb->buf;

//   return nullptr;
// }

// bool hasMoreDataAvailable() {
//   Serial.println("Has more data");
//   if (hasMoreData) {
//     hasMoreData = false;
//     return true;
//   }

//   return false;
// }
// String alerts2Telegram(String token, String chat_id) 
// {
//   const char* myDomain = "api.telegram.org";
//   String getAll="", getBody = "";

//   camera_fb_t * fb = NULL;
//   fb = esp_camera_fb_get();  
//   if(!fb) 
// {
//     Serial.println("Camera capture failed");
//     delay(1000);
//     ESP.restart();
//     return "Camera capture failed";
//   }  
  

// WiFiClientSecure client_tcp;
  
//   if (client_tcp.connect(myDomain, 443)) 
// {
//     Serial.println("Connected to " + String(myDomain));
    
//     String head = "--India\r\nContent-Disposition: form-data; name=\"chat_id\"; \r\n\r\n" + chat_id + "\r\n--India\r\nContent-Disposition: form-data; name=\"photo\"; filename=\"esp32-cam.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
//     String tail = "\r\n--India--\r\n";

//     uint16_t imageLen = fb->len;
//     uint16_t extraLen = head.length() + tail.length();
//     uint16_t totalLen = imageLen + extraLen;
  
//     client_tcp.println("POST /bot"+token+"/sendPhoto HTTP/1.1");
//     client_tcp.println("Host: " + String(myDomain));
//     client_tcp.println("Content-Length: " + String(totalLen));
//     client_tcp.println("Content-Type: multipart/form-data; boundary=India");
//     client_tcp.println();
//     client_tcp.print(head);
  
//     uint8_t *fbBuf = fb->buf;
//     size_t fbLen = fb->len;


//     for (size_t n=0;n<fbLen;n=n+1024)
//  {

//       if (n+1024<fbLen) 
// {
//         client_tcp.write(fbBuf, 1024);
//         fbBuf += 1024;
//       }
//       else if (fbLen%1024>0) 
// {
//         size_t remainder = fbLen%1024;
//         client_tcp.write(fbBuf, remainder);
//       }
//     }  
    
//     client_tcp.print(tail);
    
//     esp_camera_fb_return(fb);
    
//     int waitTime = 10000;   // timeout 10 seconds
//     long startTime = millis();
//     boolean state = false;
    
//     while ((startTime + waitTime) > millis())
//     {
//       Serial.print(".");
//       delay(100);      
//       while (client_tcp.available()) 
//       {
//           char c = client_tcp.read();
//           if (c == '\n') 
//           {
//             if (getAll.length()==0) state=true; 
//             getAll = "";
//           } 
//           else if (c != '\r')
//             getAll += String(c);
//           if (state==true) getBody += String(c);
//           startTime = millis();
//        }
//        if (getBody.length()>0) break;
//     }
//     client_tcp.stop();
//     Serial.println(getBody);
//   }
//   else {
//     getBody = "Connection to telegram failed.";
//     Serial.println("Connection to telegram failed.");
//   }
  
//   return getBody;
// }
