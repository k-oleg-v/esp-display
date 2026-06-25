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
#include "web_server.h"
#include "text_reader.h"

#include <stdio.h>
#include <esp_log.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

static const char *TAG = "main_app";

// Локальный буфер, куда приложение будет копировать данные из очереди
static char app_rx_buffer[MAX_TEXT_LEN + 1];

SSD1306_t dev;

extern QueueHandle_t tx_to_phone_queue;
extern char* inputText;
extern char* outputText;

extern bool needToSend;

SemaphoreHandle_t display_mutex = NULL;

static void send_to_phone_task(void *pvParameters) {
    int counter = 0;
    
    // ДОБАВЛЕНО static: теперь буфер не нагружает стек задачи FreeRTOS
    static char tx_buffer[MAX_TEXT_LEN + 1];
	

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(5000)); // Проверка каждые 5 секунд
        
        if (needToSend) {
            counter++;
            snprintf(tx_buffer, sizeof(tx_buffer), "%s", outputText);
            
            // Отправляем массив в очередь (ждем до 10 тиков, если веб-сервер занят)
            if (xQueueSend(tx_to_phone_queue, tx_buffer, pdMS_TO_TICKS(10)) == pdPASS) {
                ESP_LOGI(TAG, "Данные отправлены на телефон: %s", tx_buffer);
                
                // Сбрасываем флаг, чтобы не слать текст повторно в следующем цикле
                needToSend = 0; 
            } else {
                ESP_LOGE(TAG, "Ошибка: не удалось отправить данные в веб-очередь!");
            }
        }
    }
}


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
	vInitMessageQueue();
	display_mutex = xSemaphoreCreateMutex();
	// ssd1306_display_text_box1(&dev, 0, 10, "12", 2, 2, 0, 1);
	init_web_system();
	xTaskCreate(send_to_phone_task, "send_to_phone", 2048, NULL, 4, NULL);
	xTaskCreate(vQueueTask, "text queue update", 8192, NULL, 5, NULL);

	while (1) {
		// process_button_events();
		vTaskDelay(pdMS_TO_TICKS(100));
		if (xQueueReceive(web_text_queue, &app_rx_buffer, portMAX_DELAY) == pdPASS) {
            
            ESP_LOGW(TAG, "Получен новый текст из main.c!");
            sprintf(inputText, "%s", app_rx_buffer);
			decode_comma(inputText);

			vPostMessageToQueue(inputText);
            
            printf("--- НАЧАЛО ТЕКСТА ---\n%s\n--- КОНЕЦ ТЕКСТА ---\n", app_rx_buffer);
		}
	}
}



