#ifndef MY_FNS
#define MY_FNS

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

#define CONFIG_SDA_GPIO 20
#define CONFIG_SCL_GPIO 21

#define BTN_PIN_UP 3
#define BTN_PIN_CENTER 1
#define BTN_PIN_DOWN 0
#define BTN_PIN_LEFT 10
#define BTN_PIN_RIGHT 8


#define CONFIG_SSD1306_128x32 1
#define CONFIG_SSD1306_128x64 0

#define tag "SSD1306"

enum APP_ID {
	MENU_ID,
	CALC_ID,
};

struct button_t {
	const button_config_t cfg;
	const button_gpio_config_t gpio_cfg;
	button_handle_t gpio_hdl;
	uint8_t cnt;
};

struct menuElement_t {
	char name[20];
	char hiddenText[20];
	uint8_t id;
	void(*action)(void*);
	struct menuElement_t* trgtPage;
};

struct appElement_t {
	char* name;
	void(*action)(void*, void*);
	uint8_t xPos;
	uint8_t yPos;
};

typedef struct appCtx_t {
	void* ctxPtr;
	char name[20];
	uint8_t xCursor;
	uint8_t yCursor;
	struct appElement_t* appElements;
	struct menuElement_t* menuElements;
	int16_t* appParameters;
} appCtx_t;

void menuPrinter(struct menuElement_t *el, uint8_t curId);
static void button_event_cb(void *arg, void *data);
void clearScr(void* dev);
// void openSettings(void* ptr);
void gpio_init();
void midClk();
void downClk();
void upClk();
void leftClk();
void rightClk();
void test_cb(void* arg, void* strct);
void menuTask();
void digitEnter(void* symbol, void* number);
void appCtxInit();
void process_button_events();
void cb_reg();
void appPrinter(appCtx_t* curAppCtx, uint8_t curX, uint8_t curY);
void calcTaskCreate();
void calcTask();
void exitApp();
void cursorMover();





#endif