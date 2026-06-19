#ifndef WEB_SYSTEM_H
#define WEB_SYSTEM_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#define MAX_TEXT_LEN 1000

// Очередь, через которую будут передаваться строки
extern QueueHandle_t web_text_queue;

// Функция инициализации системы
void init_web_system(void);

#endif // WEB_SYSTEM_H
