/*
 * Edge Impulse Arduino example
 * Adapted for Seeed Studio XIAO ESP32S3 Sense
 * Model: VlastitiDataset
 * Input: 160x160
 * Classes: 4
 */

#include <VlastitiDatasetCrop_inferencing.h>
#include "edge-impulse-sdk/dsp/image/image.hpp"

#include "esp_camera.h"

/* OLED display */
#include <U8g2lib.h>
#include <Wire.h>


#define PWDN_GPIO_NUM     -1
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM     10
#define SIOD_GPIO_NUM     40
#define SIOC_GPIO_NUM     39

#define Y9_GPIO_NUM       48
#define Y8_GPIO_NUM       11
#define Y7_GPIO_NUM       12
#define Y6_GPIO_NUM       14
#define Y5_GPIO_NUM       16
#define Y4_GPIO_NUM       18
#define Y3_GPIO_NUM       17
#define Y2_GPIO_NUM       15

#define VSYNC_GPIO_NUM    38
#define HREF_GPIO_NUM     47
#define PCLK_GPIO_NUM     13


#define EI_CAMERA_RAW_FRAME_BUFFER_COLS   320
#define EI_CAMERA_RAW_FRAME_BUFFER_ROWS   240
#define EI_CAMERA_FRAME_BYTE_SIZE         3


static bool debug_nn = false;
static bool is_initialised = false;

uint8_t *snapshot_buf;


/* -------------------------------------------------------------------------
 * OLED variables
 * ------------------------------------------------------------------------- */

U8G2_SSD1306_72X40_ER_1_HW_I2C u8g2(
    U8G2_R2,
    U8X8_PIN_NONE
);

String predicted_class = "NONE";
float confidence = 0.0;


/* -------------------------------------------------------------------------
 * Camera configuration
 * ------------------------------------------------------------------------- */

static camera_config_t camera_config = {

    .pin_pwdn = PWDN_GPIO_NUM,
    .pin_reset = RESET_GPIO_NUM,

    .pin_xclk = XCLK_GPIO_NUM,

    .pin_sscb_sda = SIOD_GPIO_NUM,
    .pin_sscb_scl = SIOC_GPIO_NUM,

    .pin_d7 = Y9_GPIO_NUM,
    .pin_d6 = Y8_GPIO_NUM,
    .pin_d5 = Y7_GPIO_NUM,
    .pin_d4 = Y6_GPIO_NUM,
    .pin_d3 = Y5_GPIO_NUM,
    .pin_d2 = Y4_GPIO_NUM,
    .pin_d1 = Y3_GPIO_NUM,
    .pin_d0 = Y2_GPIO_NUM,

    .pin_vsync = VSYNC_GPIO_NUM,
    .pin_href = HREF_GPIO_NUM,
    .pin_pclk = PCLK_GPIO_NUM,

    .xclk_freq_hz = 20000000,

    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format = PIXFORMAT_JPEG,

    .frame_size = FRAMESIZE_QVGA,

    .jpeg_quality = 12,

    .fb_count = 1,

    .fb_location = CAMERA_FB_IN_PSRAM,

    .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
};


/* -------------------------------------------------------------------------
 * Function declarations
 * ------------------------------------------------------------------------- */

bool ei_camera_init(void);

void ei_camera_deinit(void);

bool ei_camera_capture(
    uint32_t img_width,
    uint32_t img_height,
    uint8_t *out_buf
);

static int ei_camera_get_data(
    size_t offset,
    size_t length,
    float *out_ptr
);

void display_inference_result(void);


/* -------------------------------------------------------------------------
 * Setup
 * ------------------------------------------------------------------------- */

void setup()
{
    Serial.begin(115200);

    while (!Serial);

    Serial.println();
    Serial.println("Edge Impulse Inferencing Demo");
    Serial.println("Model: VlastitiDatasetiSplit");


    /* Initialize OLED */

    u8g2.begin();
    u8g2.clearDisplay();

    u8g2.firstPage();

    do {

        u8g2.setFont(
            u8g2_font_6x10_tr
        );

        u8g2.setCursor(
            8,
            15
        );

        u8g2.print(
            "Starting"
        );

        u8g2.setCursor(
            12,
            30
        );

        u8g2.print(
            "model..."
        );

    } while (
        u8g2.nextPage()
    );


    Serial.print("Model input: ");
    Serial.print(EI_CLASSIFIER_INPUT_WIDTH);
    Serial.print("x");
    Serial.println(EI_CLASSIFIER_INPUT_HEIGHT);

    Serial.print("Number of classes: ");
    Serial.println(EI_CLASSIFIER_LABEL_COUNT);


    /*
     * Verify model dimensions.
     */

    if (
        EI_CLASSIFIER_INPUT_WIDTH != 160 ||
        EI_CLASSIFIER_INPUT_HEIGHT != 160
    ) {

        Serial.println(
            "ERR: Loaded model is not 160x160!"
        );

        while (true) {
            delay(1000);
        }
    }


    if (EI_CLASSIFIER_LABEL_COUNT != 4) {

        Serial.println(
            "ERR: Loaded model does not have 4 classes!"
        );

        while (true) {
            delay(1000);
        }
    }


    if (ei_camera_init() == false) {

        ei_printf(
            "Failed to initialize Camera!\r\n"
        );


        u8g2.firstPage();

        do {

            u8g2.setFont(
                u8g2_font_6x10_tr
            );

            u8g2.setCursor(
                8,
                15
            );

            u8g2.print(
                "Camera"
            );

            u8g2.setCursor(
                10,
                30
            );

            u8g2.print(
                "ERROR!"
            );

        } while (
            u8g2.nextPage()
        );
    }

    else {

        ei_printf(
            "Camera initialized\r\n"
        );
    }


    ei_printf(
        "\nStarting continuous inference in 2 seconds...\n"
    );

    ei_sleep(2000);
}


/* -------------------------------------------------------------------------
 * Main loop
 * ------------------------------------------------------------------------- */

void loop()
{
    if (ei_sleep(5) != EI_IMPULSE_OK) {
        return;
    }


    snapshot_buf =
        (uint8_t *)malloc(
            EI_CAMERA_RAW_FRAME_BUFFER_COLS *
            EI_CAMERA_RAW_FRAME_BUFFER_ROWS *
            EI_CAMERA_FRAME_BYTE_SIZE
        );


    if (snapshot_buf == nullptr) {

        ei_printf(
            "ERR: Failed to allocate snapshot buffer!\n"
        );

        return;
    }


    /* Prepare Edge Impulse signal */

    ei::signal_t signal;

    signal.total_length =
        EI_CLASSIFIER_INPUT_WIDTH *
        EI_CLASSIFIER_INPUT_HEIGHT;

    signal.get_data =
        &ei_camera_get_data;


    /* Capture and preprocess image */

    if (
        ei_camera_capture(
            (size_t)EI_CLASSIFIER_INPUT_WIDTH,
            (size_t)EI_CLASSIFIER_INPUT_HEIGHT,
            snapshot_buf
        ) == false
    ) {

        ei_printf(
            "Failed to capture image\r\n"
        );

        free(snapshot_buf);

        snapshot_buf = nullptr;

        return;
    }


    /* Run classifier */

    ei_impulse_result_t result = { 0 };


    EI_IMPULSE_ERROR err =
        run_classifier(
            &signal,
            &result,
            debug_nn
        );


    if (err != EI_IMPULSE_OK) {

        ei_printf(
            "ERR: Failed to run classifier (%d)\n",
            err
        );

        free(snapshot_buf);

        snapshot_buf = nullptr;

        return;
    }


    /* ---------------------------------------------------------
     * Find class with highest confidence for OLED
     * --------------------------------------------------------- */

    float max_confidence = 0.0;

    String best_class = "UNKNOWN";


    for (
        uint16_t i = 0;
        i < EI_CLASSIFIER_LABEL_COUNT;
        i++
    ) {

        if (
            result.classification[i].value >
            max_confidence
        ) {

            max_confidence =
                result.classification[i].value;

            best_class =
                String(
                    ei_classifier_inferencing_categories[i]
                );
        }
    }


    predicted_class =
        best_class;

    confidence =
        max_confidence;


    display_inference_result();


    /* Timing */

    ei_printf(
        "Predictions (DSP: %d ms., Classification: %d ms., Anomaly: %d ms.):\n",
        result.timing.dsp,
        result.timing.classification,
        result.timing.anomaly
    );


#if EI_CLASSIFIER_OBJECT_DETECTION == 1

    ei_printf(
        "Object detection bounding boxes:\r\n"
    );


    for (
        uint32_t i = 0;
        i < result.bounding_boxes_count;
        i++
    ) {

        ei_impulse_result_bounding_box_t bb =
            result.bounding_boxes[i];


        if (bb.value == 0) {
            continue;
        }


        ei_printf(
            "  %s (%f) [x: %u, y: %u, width: %u, height: %u]\r\n",
            bb.label,
            bb.value,
            bb.x,
            bb.y,
            bb.width,
            bb.height
        );
    }


#else


    ei_printf(
        "Predictions:\r\n"
    );


    for (
        uint16_t i = 0;
        i < EI_CLASSIFIER_LABEL_COUNT;
        i++
    ) {

        ei_printf(
            "  %s: %.5f\r\n",
            ei_classifier_inferencing_categories[i],
            result.classification[i].value
        );
    }


    ei_printf(
        "Best: %s (%.1f%%)\r\n",
        best_class.c_str(),
        max_confidence * 100.0
    );


#endif


#if EI_CLASSIFIER_HAS_ANOMALY

    ei_printf(
        "Anomaly prediction: %.3f\r\n",
        result.anomaly
    );

#endif


    free(snapshot_buf);

    snapshot_buf = nullptr;

    Serial.println();
}


/* -------------------------------------------------------------------------
 * OLED display
 * ------------------------------------------------------------------------- */

void display_inference_result(void)
{
    u8g2.firstPage();

    do {

        u8g2.setFont(
            u8g2_font_6x10_tr
        );


        int class_width =
            u8g2.getStrWidth(
                predicted_class.c_str()
            );


        int class_x =
            (72 - class_width) / 2;


        if (class_x < 0) {
            class_x = 0;
        }


        u8g2.setCursor(
            class_x,
            15
        );


        u8g2.print(
            predicted_class
        );


        String confidence_text =
            String(
                confidence * 100.0,
                1
            )
            +
            "%";


        int confidence_width =
            u8g2.getStrWidth(
                confidence_text.c_str()
            );


        int confidence_x =
            (72 - confidence_width) / 2;


        if (confidence_x < 0) {
            confidence_x = 0;
        }


        u8g2.setCursor(
            confidence_x,
            30
        );


        u8g2.print(
            confidence_text
        );


        u8g2.drawFrame(
            0,
            0,
            72,
            40
        );


    } while (
        u8g2.nextPage()
    );
}


/* -------------------------------------------------------------------------
 * Camera initialization
 * ------------------------------------------------------------------------- */

bool ei_camera_init(void)
{
    if (is_initialised) {
        return true;
    }


    esp_err_t err =
        esp_camera_init(
            &camera_config
        );


    if (err != ESP_OK) {

        Serial.printf(
            "Camera init failed with error 0x%x\n",
            err
        );

        return false;
    }


    sensor_t *s =
        esp_camera_sensor_get();


    if (s->id.PID == OV3660_PID) {

        s->set_vflip(
            s,
            1
        );

        s->set_brightness(
            s,
            1
        );

        s->set_saturation(
            s,
            0
        );
    }


#if defined(CAMERA_MODEL_M5STACK_WIDE)

    s->set_vflip(
        s,
        1
    );

    s->set_hmirror(
        s,
        1
    );

#elif defined(CAMERA_MODEL_ESP_EYE)

    s->set_vflip(
        s,
        1
    );

    s->set_hmirror(
        s,
        1
    );

    s->set_awb_gain(
        s,
        1
    );

#endif


    is_initialised = true;

    return true;
}


/* -------------------------------------------------------------------------
 * Camera deinitialization
 * ------------------------------------------------------------------------- */

void ei_camera_deinit(void)
{
    esp_err_t err =
        esp_camera_deinit();


    if (err != ESP_OK) {

        ei_printf(
            "Camera deinit failed\n"
        );

        return;
    }


    is_initialised = false;
}


/* -------------------------------------------------------------------------
 * Capture + resize image
 * ------------------------------------------------------------------------- */

bool ei_camera_capture(
    uint32_t img_width,
    uint32_t img_height,
    uint8_t *out_buf
)
{
    bool do_resize = false;

    if (!is_initialised) {

        ei_printf(
            "ERR: Camera is not initialized\r\n"
        );

        return false;
    }


    camera_fb_t *fb =
        esp_camera_fb_get();


    if (!fb) {

        ei_printf(
            "Camera capture failed\n"
        );

        return false;
    }


    /*
     * JPEG -> RGB888
     */

    bool converted =
        fmt2rgb888(
            fb->buf,
            fb->len,
            PIXFORMAT_JPEG,
            out_buf
        );


    esp_camera_fb_return(
        fb
    );


    if (!converted) {

        ei_printf(
            "Conversion failed\n"
        );

        return false;
    }


    /*
     * Resize image to model input size.
     *
     * For the current model:
     * 320x240 -> 160x160
     */

    if (
        (img_width != EI_CAMERA_RAW_FRAME_BUFFER_COLS) ||
        (img_height != EI_CAMERA_RAW_FRAME_BUFFER_ROWS)
    ) {

        do_resize = true;
    }


    if (do_resize) {

        ei::image::processing::crop_and_interpolate_rgb888(

            out_buf,

            EI_CAMERA_RAW_FRAME_BUFFER_COLS,
            EI_CAMERA_RAW_FRAME_BUFFER_ROWS,

            out_buf,

            img_width,
            img_height
        );
    }


    return true;
}


/* -------------------------------------------------------------------------
 * Edge Impulse image callback
 * ------------------------------------------------------------------------- */

static int ei_camera_get_data(
    size_t offset,
    size_t length,
    float *out_ptr
)
{
    size_t pixel_ix =
        offset * 3;

    size_t pixels_left =
        length;

    size_t out_ptr_ix =
        0;


    while (pixels_left != 0) {

        /*
         * Same BGR -> RGB conversion as before.
         */

        out_ptr[out_ptr_ix] =
            (
                snapshot_buf[pixel_ix + 2]
                << 16
            )
            +
            (
                snapshot_buf[pixel_ix + 1]
                << 8
            )
            +
            snapshot_buf[pixel_ix];


        out_ptr_ix++;

        pixel_ix += 3;

        pixels_left--;
    }


    return 0;
}


/* -------------------------------------------------------------------------
 * Verify sensor type
 * ------------------------------------------------------------------------- */

#if !defined(EI_CLASSIFIER_SENSOR) || \
    EI_CLASSIFIER_SENSOR != EI_CLASSIFIER_SENSOR_CAMERA

#error "Invalid model for current sensor"

#endif