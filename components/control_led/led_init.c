#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led_strip.h"


#include "control_led.h"
// #include "led_config.h"

void led_strip_init(led_config_t *led_strip) {
    // Cấu hình strip (số LED, GPIO)
    led_strip_config_t strip_config = {
        .strip_gpio_num = led_strip->led_gpio,  // GPIO nối LED
        .max_leds = led_strip->led_count,       // số LED
        .led_model = LED_MODEL_WS2812,          // loại LED
        .flags.invert_out = false,
    };
    // Cấu hình RMT
    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,     // clock mặc định
        .resolution_hz = 10 * 1000 * 1000,  // 10 MHz = 100ns
        .mem_block_symbols = 64,
        .flags.with_dma = 0,
    };
    led_strip_handle_t handle;
    // Tạo device
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &handle));
    led_strip->handle = handle;
    ESP_LOGI("LED_INIT", "LED init successfully");
}

void led_strip_clear_all(led_config_t *led_strip) {
    led_strip_clear(led_strip->handle);
}

static inline rgb_t rgb_from_hex(uint32_t hex_code) {
    rgb_t color;
    color.r = (hex_code >> 16) & 0xFF;  // Get the 8 R bits
    color.g = (hex_code >> 8) & 0xFF;   // Get the 8 G bits
    color.b = hex_code & 0xFF;          // Get the 8 B bits
    return color;
}

void set_single_led(led_config_t *led_strip, int color, int index) {
    rgb_t rgb_color;
    rgb_color = rgb_from_hex(color);
    led_strip_set_pixel(led_strip->handle, index - 1, rgb_color.r, rgb_color.g, rgb_color.b);
    led_strip_refresh(led_strip->handle);
}

void set_group_led(led_config_t *led_strip, int color, int index) {
    rgb_t rgb_color;
    rgb_color = rgb_from_hex(color);
    for (int i = index * led_strip->group_size - led_strip->group_size; i < index * led_strip->group_size; i++) {
        led_strip_set_pixel(led_strip->handle, i, rgb_color.r, rgb_color.g, rgb_color.b);
        led_strip_refresh(led_strip->handle);
    }
}

void set_all_led(led_config_t *led_strip, int color) {
    rgb_t rgb_color;
    rgb_color = rgb_from_hex(color);
    for (int i = 0; i < led_strip->led_count; i++) {
        led_strip_set_pixel(led_strip->handle, i, rgb_color.r, rgb_color.g, rgb_color.b);
    }
    led_strip_refresh(led_strip->handle);
}

void clear_group_led(led_config_t *led_strip,int index){ 
    rgb_t rgb_color;
 //   rgb_color = rgb_from_hex(color);
    for (int i = index * led_strip->group_size - led_strip->group_size; i < index * led_strip->group_size; i++) {
        led_strip_set_pixel(led_strip->handle, i, 0, 0, 0);
    }
    led_strip_refresh(led_strip->handle);
}

