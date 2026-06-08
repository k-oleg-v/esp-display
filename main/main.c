#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "button_interface.h"
#include "button_types.h"
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "button_gpio.h"

#include "iot_button.h"
#include "ssd1306.h"
#include "font8x8_basic.h"
#include "my_fns.h"

SSD1306_t dev;
// appCtx_t calculator; 

void app_main(void)
{
	gpio_init();
	

	int center, top, bottom;
	char lineChar[20];

	ESP_LOGI(tag, "INTERFACE is i2c");
	ESP_LOGI(tag, "CONFIG_SDA_GPIO=%d",CONFIG_SDA_GPIO);
	ESP_LOGI(tag, "CONFIG_SCL_GPIO=%d",CONFIG_SCL_GPIO);
	ESP_LOGI(tag, "CONFIG_RESET_GPIO=%d",CONFIG_RESET_GPIO);
	i2c_master_init(&dev, CONFIG_SDA_GPIO, CONFIG_SCL_GPIO, CONFIG_RESET_GPIO);

#if CONFIG_FLIP
	dev._flip = true;
	ESP_LOGW(tag, "Flip upside down");
#endif

	ESP_LOGI(tag, "Panel is 128x32");
	ssd1306_init(&dev, 128, 32);
	ssd1306_clear_screen(&dev, false);
	ssd1306_contrast(&dev, 0xff);
	appCtxInit();
	cb_reg();
	// ssd1306_display_text_box1(&dev, 0, 10, "12", 2, 2, 0, 1);
	
	// char pm = 0x80;
	// char t = 0x7E;
	// sprintf(lineChar, "%c %c", pm, t);
	// ssd1306_display_text(&dev, 0, lineChar, 20, 0);

	while (1) {
		// process_button_events();
		vTaskDelay(pdMS_TO_TICKS(100));
	}
}



