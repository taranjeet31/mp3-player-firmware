#pragma once

#if defined(TARGET_THMI)

#include "hal/IDisplay.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "xpt2046.h"
#include <lvgl.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

class THMIDisplay : public IDisplay {
private:
    esp_lcd_panel_handle_t panel_handle;
    esp_lcd_panel_io_handle_t io_handle;
    esp_lcd_i80_bus_handle_t i80_bus;
    
public:
    THMIDisplay();
    bool init() override;
    void update() override;

    void setLvglTaskHandle(TaskHandle_t handle);

    // Static helper for LVGL flush callback
    static void lvgl_flush_cb(struct _lv_disp_drv_t * drv, const lv_area_t * area, lv_color_t * color_map);
    // Static helper for LVGL touch read callback
    static void lvgl_touch_read(struct _lv_indev_drv_t * indev_driver, lv_indev_data_t * data);
};

#endif

