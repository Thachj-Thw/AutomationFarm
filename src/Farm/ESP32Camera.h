#ifndef _ESP32CAMERA_H_
#define _ESP32CAMERA_H_

#include "Arduino.h"
#include "esp_camera.h"

typedef enum {
    CAMERA_MODEL_AI_THINKER,
    CAMERA_MODEL_M5STACK_PSRAM,
    CAMERA_MODEL_M5STACK_WITHOUT_PSRAM,
    CAMERA_MODEL_WROVER_KIT
} cameraModel;
/*
FRAMESIZE_96X96     (96x96)
FRAMESIZE_SVGA      (800x600)
FRAMESIZE_VGA       (640x480)
FRAMESIZE_CIF       (400x296)
FRAMESIZE_QVGA      (320x240)
FRAMESIZE_HQVGA     (240x176)
FRAMESIZE_QQVGA     (160x120)
FRAMESIZE_QXGA      (2048x1564 for OV3660)
*/
typedef struct {
  unsigned char* buf;
  size_t len;
} JpgCapture;

class ESP32Camera {
    public:
        ESP32Camera(cameraModel _model);
        cameraModel model;
        void begin(framesize_t frame_size);
        String capture2Base64();
        JpgCapture getJPGCapture();
    private:
        camera_fb_t *fb = NULL;
        unsigned char* _jpg_buf;
        size_t _jpg_buf_len;
};

#endif // _ESP32CAMERA_H_