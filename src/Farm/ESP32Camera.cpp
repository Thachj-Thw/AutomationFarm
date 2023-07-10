#include "ESP32Camera.h"
#include "Base64.h"
#include <Arduino.h>

ESP32Camera::ESP32Camera(cameraModel _model) {
  model = _model;
  _jpg_buf_len = 0;
  _jpg_buf = NULL;
}

void ESP32Camera::begin(framesize_t frame_size) {
  camera_config_t config;
  switch (model){
    case CAMERA_MODEL_AI_THINKER:
      config.pin_d0 = 5;
      config.pin_d1 = 18;
      config.pin_d2 = 19;
      config.pin_d3 = 21;
      config.pin_d4 = 36;
      config.pin_d5 = 39;
      config.pin_d6 = 34;
      config.pin_d7 = 35;
      config.pin_xclk = 0;
      config.pin_pclk = 22;
      config.pin_vsync = 25;
      config.pin_href = 23;
      config.pin_sscb_sda = 26;
      config.pin_sscb_scl = 27;
      config.pin_pwdn = 32;
      config.pin_reset = -1;
      break;
    case CAMERA_MODEL_M5STACK_PSRAM:
      config.pin_d0 = 32;
      config.pin_d1 = 35;
      config.pin_d2 = 34;
      config.pin_d3 = 5;
      config.pin_d4 = 39;
      config.pin_d5 = 18;
      config.pin_d6 = 36;
      config.pin_d7 = 19;
      config.pin_xclk = 27;
      config.pin_pclk = 21;
      config.pin_vsync = 22;
      config.pin_href = 26;
      config.pin_sscb_sda = 25;
      config.pin_sscb_scl = 23;
      config.pin_pwdn = -1;
      config.pin_reset = 15;
      break;
    case CAMERA_MODEL_M5STACK_WITHOUT_PSRAM:
      config.pin_d0 = 17;
      config.pin_d1 = 35;
      config.pin_d2 = 34;
      config.pin_d3 = 5;
      config.pin_d4 = 39;
      config.pin_d5 = 18;
      config.pin_d6 = 36;
      config.pin_d7 = 19;
      config.pin_xclk = 27;
      config.pin_pclk = 21;
      config.pin_vsync = 22;
      config.pin_href = 26;
      config.pin_sscb_sda = 25;
      config.pin_sscb_scl = 23;
      config.pin_pwdn = -1;
      config.pin_reset = 15;
      break;
    case CAMERA_MODEL_WROVER_KIT:
      config.pin_d0 = 4;
      config.pin_d1 = 5;
      config.pin_d2 = 18;
      config.pin_d3 = 19;
      config.pin_d4 = 36;
      config.pin_d5 = 39;
      config.pin_d6 = 34;
      config.pin_d7 = 35;
      config.pin_xclk = 21;
      config.pin_pclk = 22;
      config.pin_vsync = 25;
      config.pin_href = 23;
      config.pin_sscb_sda = 26;
      config.pin_sscb_scl = 27;
      config.pin_pwdn = -1;
      config.pin_reset = -1;
      break;
  }
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  if(psramFound()){
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    Serial.println();
    return;
    ESP.restart();
  }
  sensor_t* s = esp_camera_sensor_get();
  // initial sensors are flipped vertically and colors are a bit saturated
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 0); // flip it back
    s->set_brightness(s, 0); // up the brightness just a bit
    s->set_saturation(s, 0); // lower the saturation
  }
  // drop down frame size for higher initial frame rate
  s->set_framesize(s, frame_size);
}

JpgCapture ESP32Camera::getJPGCapture() {
  fb = esp_camera_fb_get();
  JpgCapture result;
  if (!fb) {
    Serial.println("Camera capture failed");
    esp_camera_fb_return(fb);
    return result;
  } else {
    if(fb->format != PIXFORMAT_JPEG){
      bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
      esp_camera_fb_return(fb);
      fb = NULL;
      if(!jpeg_converted){
        Serial.println("JPEG compression failed");
        return result;
      }
    } else {
      _jpg_buf_len = fb->len;
      _jpg_buf = fb->buf;
    }
  }
  result.buf = _jpg_buf;
  result.len = _jpg_buf_len;
  if(fb){
    esp_camera_fb_return(fb);
    fb = NULL;
    _jpg_buf = NULL;
  } else if(_jpg_buf){
    free(_jpg_buf);
    _jpg_buf = NULL;
  }
//  Serial.printf("MJPG: %uB\n",(uint32_t)(_jpg_buf_len));
  return result;
}

String ESP32Camera::capture2Base64() {
  fb = esp_camera_fb_get();
  if (fb) {
    char* input = (char*)fb->buf;
    char output[base64_enc_len(3)];
    String imageFile = "data:image/png;base64, ";
    for (int i=0; i<fb->len; i++) {
      base64_encode(output, (input++), 3);
      if (i%3 == 0)
        imageFile += String(output);
      }
    esp_camera_fb_return(fb);
    return imageFile;
  }
  return String();
}
