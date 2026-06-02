#include "my_fns.h"
#include "esp_log.h"
#include "freertos/idf_additions.h"
#include "iot_button.h"
#include "portmacro.h"
#include "ssd1306.h"
#include <stdint.h>
#include <string.h>

struct menuElement_t screenSettingsPage[] = {
	{"Screen settings", "", 1, NULL},
	{"Change theme", "", 1, NULL}
};

struct menuElement_t mainPage[] = {
	{"    Main menu","", 4, NULL},
	{"Turn off screen", "", 1, clearScr},
	{"Hidden text", "topsecret", 2, NULL},
	{"Calculator", "", 3, calcTaskCreate},
	{"Screen settings", "", 4, NULL, screenSettingsPage}
};

struct appElement_t calcElements[5][10];
uint8_t currentId = 0;
struct menuElement_t* currentPage = mainPage;
extern SSD1306_t dev;
size_t curpageSize;
bool theme = 0;

appCtx_t* appPtr;
appCtx_t menu;
appCtx_t calculator; 
uint8_t appId;

TaskHandle_t xCalcHdl = NULL;

struct button_t mid_btn = {
	.cfg = {0},
	.gpio_cfg = {
	.gpio_num = BTN_PIN_CENTER,
	.active_level = 1,
	},
	.gpio_hdl = NULL,
	.cnt = 0
};
struct button_t down_btn = {
	.cfg = {0},
	.gpio_cfg = {
	.gpio_num = BTN_PIN_DOWN,
	.active_level = 1,
	},
	.gpio_hdl = NULL,
	.cnt = 0
};
struct button_t up_btn = {
	.cfg = {0},
	.gpio_cfg = {
	.gpio_num = BTN_PIN_UP,
	.active_level = 1,
	},
	.gpio_hdl = NULL,
	.cnt = 0
};
struct button_t left_btn = {
	.cfg = {0},
	.gpio_cfg = {
	.gpio_num = BTN_PIN_LEFT,
	.active_level = 1,
	},
	.gpio_hdl = NULL,
	.cnt = 0
};
struct button_t right_btn = {
	.cfg = {0},
	.gpio_cfg = {
	.gpio_num = BTN_PIN_RIGHT,
	.active_level = 1,
	},
	.gpio_hdl = NULL,
	.cnt = 0
};

void gpio_init() {
	esp_err_t ret = iot_button_new_gpio_device(&mid_btn.cfg, &mid_btn.gpio_cfg, &mid_btn.gpio_hdl);
	if(NULL == mid_btn.gpio_hdl || ret != ESP_OK) {
		ESP_LOGE(tag, "Button create failed");
	}

	
	ret = iot_button_new_gpio_device(&down_btn.cfg, &down_btn.gpio_cfg, &down_btn.gpio_hdl);
	if(NULL == down_btn.gpio_hdl || ret != ESP_OK) {
		ESP_LOGE(tag, "Low button create failed");
	}

	
	ret = iot_button_new_gpio_device(&up_btn.cfg, &up_btn.gpio_cfg, &up_btn.gpio_hdl);
	if(NULL == up_btn.gpio_hdl || ret != ESP_OK) {
		ESP_LOGE(tag, "Top button create failed");
	}

	
	gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;      // Отключить прерывания
    io_conf.mode = GPIO_MODE_INPUT;            // Режим входа
    io_conf.pin_bit_mask = (1ULL << BTN_PIN_UP); // Выбор пина
    io_conf.pull_down_en = 1;                   // Отключить pull-down
    io_conf.pull_up_en = 0;                     // Включить pull-up
    gpio_config(&io_conf);

    io_conf.intr_type = GPIO_INTR_DISABLE;      // Отключить прерывания
    io_conf.mode = GPIO_MODE_INPUT;            // Режим входа
    io_conf.pin_bit_mask = (1ULL << BTN_PIN_CENTER); // Выбор пина
    io_conf.pull_down_en = 0;                   // Отключить pull-down
    io_conf.pull_up_en = 0;                     // Включить pull-up
    gpio_config(&io_conf);

    io_conf.intr_type = GPIO_INTR_DISABLE;      // Отключить прерывания
    io_conf.mode = GPIO_MODE_INPUT;            // Режим входа
    io_conf.pin_bit_mask = (1ULL << BTN_PIN_DOWN); // Выбор пина
    io_conf.pull_down_en = 1;                   // Отключить pull-down
    io_conf.pull_up_en = 0;                     // Включить pull-up
    gpio_config(&io_conf);
}

void cb_reg() {
	esp_err_t ret;
	ret = iot_button_register_cb(mid_btn.gpio_hdl, BUTTON_PRESS_UP, NULL, midClk, NULL);
	if(ret != ESP_OK){
		ESP_LOGE(tag, "midClk failed");
	}
	ret = iot_button_register_cb(down_btn.gpio_hdl, BUTTON_PRESS_UP, NULL, downClk, NULL);
	if(ret != ESP_OK){
		ESP_LOGE(tag, "downClk failed");
	}
	ret = iot_button_register_cb(up_btn.gpio_hdl, BUTTON_PRESS_UP, NULL, upClk, NULL);
	if(ret != ESP_OK){
		ESP_LOGE(tag, "upClk failed");
	}
	ESP_LOGI(tag, "btn cfg ok");
	xTaskCreate(menuTask, "menu", 2048, NULL, 5, NULL);
}

void process_button_events() {
	button_event_t event = iot_button_get_event(mid_btn.gpio_hdl);
	if (event != BUTTON_NONE_PRESS) {
		ESP_LOGI(tag, "mid event is %s", iot_button_get_event_str(event));
	}
		event = iot_button_get_event(down_btn.gpio_hdl);
	if (event != BUTTON_NONE_PRESS) {
		ESP_LOGI(tag, "low event is %s", iot_button_get_event_str(event));
	}
		event = iot_button_get_event(up_btn.gpio_hdl);
	if (event != BUTTON_NONE_PRESS) {
		ESP_LOGI(tag, "top event is %s", iot_button_get_event_str(event));
	}
		
}

void menuPrinter(struct menuElement_t *page, uint8_t curId) {
	// size_t menuSize = ARRAY_SIZE(page);
	uint8_t curshowId = curId % 4;
	ssd1306_clear_screen(&dev, 0);
	for (int i = curId - curshowId; i < curId - curshowId + 4; i++){
		if (!strcmp(page[i].name, "") || (i <= currentPage->id)) {
			ssd1306_display_text(&dev, i%4, page[i].name, 20, (i == curId));
		} else {
			break;
		}
	}
}

void clearScr(void *dev) {
	SSD1306_t* device = (SSD1306_t*)dev;
	ssd1306_clear_screen(device, 0);
	ssd1306_display_text(device, 0, "screen cleared", 20, 0);
}

void midClk() {
	mid_btn.cnt++;
}
void downClk() {
	down_btn.cnt++;
}
void upClk() {
	up_btn.cnt++;
}
void leftClk() {
	left_btn.cnt++;
}
void rightClk() {
	right_btn.cnt++;
}



void menuTask() {
	menuPrinter(currentPage, currentId);
	while (1) {
		if (appId == MENU_ID) {
			appPtr = &menu;
			if (mid_btn.cnt >= 1) {
				if (currentPage[currentId].trgtPage != NULL) {
					currentPage = currentPage[currentId].trgtPage;
					currentId = 0;
					menuPrinter(currentPage, currentId);
				} else if (currentPage[currentId].trgtPage == NULL && currentPage[currentId].action != NULL) {
					currentPage[currentId].action(NULL);
				}
				mid_btn.cnt = 0;
				vTaskDelay(100);
				
			}
			if (down_btn.cnt >= 1) {
				currentId+=down_btn.cnt;
				down_btn.cnt = 0;
				if (currentId > currentPage->id) {
					currentId = 0;
				}
				menuPrinter(currentPage, currentId);
				
			}
			if (up_btn.cnt >= 1) {
				currentId-=up_btn.cnt;
				up_btn.cnt = 0;
				if (currentId > currentPage->id) {
					currentId = currentPage->id;
				}
				menuPrinter(currentPage, currentId);
			}
			vTaskDelay(10);  
		} else vTaskDelay(pdMS_TO_TICKS(100));
	}
	vTaskDelete(NULL);
}


void digitEnter(void* vsymbol, void* vnumber) {
	char* symbol = (char*) vsymbol;
	char* number = (char*) vnumber;
	char outNum[20];
	sprintf(outNum, "%s%s", number, symbol);
	number = outNum;
}

void appCtxInit() {
	appId = MENU_ID;

	menu.ctxPtr = &menu;
	menu.menuElements = mainPage;
	sprintf(menu.name, "Main menu");
	menu.appParameters = malloc(sizeof(int16_t)*20);


	// memset(&calcElements[0][0], 0, 50*sizeof(struct appElement_t));

	for (int i = 0; i < 3; i++) {
		for (int j = 1; j < 4; j++) {
			char* uniqueStr = (char*)malloc(2 * sizeof(char));
			if (uniqueStr != NULL) {
				uniqueStr[0] = 0x30 + i + 3 * (j - 1) + 1;
				uniqueStr[1] = '\0'; // Гарантируем корректное завершение строки
			} else {
				ESP_LOGE("INIT", "Не удалось выделить память для цифры!");
				continue;
			}
			calcElements[j][i] = (struct appElement_t) {uniqueStr, digitEnter, i, j};
			ESP_LOGI(tag, "Name(%d,%d): [%s]", j, i, calcElements[j][i].name);
		}
	}

	calcElements[0][0] = (struct appElement_t) { "     ", digitEnter, 0, 0};
	calcElements[1][3] = (struct appElement_t) {"0", digitEnter, 3, 1};
	calcElements[2][3] = (struct appElement_t) {".", digitEnter, 3, 2};
	calcElements[3][3] = (struct appElement_t) {"=", digitEnter, 3, 3};
	calcElements[1][4] = (struct appElement_t) {"+", digitEnter, 4, 1};
	calcElements[2][4] = (struct appElement_t) {"x", digitEnter, 4, 2};
	calcElements[3][4] = (struct appElement_t) {"^", digitEnter, 4, 3};
	calcElements[1][5] = (struct appElement_t) {"-", digitEnter, 5, 1};
	calcElements[2][5] = (struct appElement_t) {"\x81", digitEnter, 5, 2};
	calcElements[3][5] = (struct appElement_t) {"\x80", digitEnter, 5, 3};
	calcElements[0][1] = (struct appElement_t) {"x", exitApp, 18, 0};
	// ssd1306_display_text(&dev, 0, calcElements[5][2].name, 2, 0);
	calculator.ctxPtr = &calculator;
	calculator.appElements = &calcElements[0][0];
	appPrinter(calculator.ctxPtr, 2, 2);
}

void appPrinter(appCtx_t* curAppCtx, uint8_t curX, uint8_t curY) {
    if (curAppCtx == NULL || curAppCtx->appElements == NULL) return;

    char rowBuf[21]; 
    
    // j - строки экрана (0..3)
    for (size_t j = 0; j < 4; j++) {
        
        // Полностью очищаем буфер пробелами
        memset(rowBuf, ' ', 20);
        rowBuf[20] = '\0';
        
        if (j != curY) {
            
            // ИСПРАВЛЕНО: перебираем строго 10 элементов, которые принадлежат этой строке
            for (size_t i = 0; i < 10; i++) {
                struct appElement_t* element = curAppCtx->appElements + j * 10 + i;
                
                if (element == NULL || element->name == NULL) continue;
                
                // РАСПРЕДЕЛЕНИЕ ПО КООРДИНАТЕ X:
                // Здесь задается, где на экране (в каком символе по счету) напечатается элемент.
                // Например, i * 5 разнесет 4 элемента на позиции: 0, 5, 10, 15.
                // Если нужно, чтобы они стояли плотнее, измените шаг (например, i * 2 или i * 3)
                size_t charX = element->xPos; 
                
                if (charX >= 20) break; // Защита границ экрана
                
                size_t nameLen = strlen(element->name);
                if (charX + nameLen > 20) nameLen = 20 - charX; 
                
                // Копируем имя элемента ровно в его координату X
                memcpy(rowBuf + charX, element->name, nameLen);
            }
            
            // Лог покажет, что лишние девятки пропали
            ESP_LOGI("PRINTER", "Row %d: [%s]", j, rowBuf);
            
            // Вывод ровной строки на дисплей
            ssd1306_display_text(&dev, j, rowBuf, 20, 0);
        } else {
			for (int i = 0; i < 10; i++) {
				struct appElement_t* element = curAppCtx->appElements + j * 10 + i;
				if (element == NULL || element->name == NULL) continue;
				char txt[5];
				size_t charX = element->xPos;
				size_t nameLen = strlen(element->name);
				if (nameLen == 0) continue;
                
                if (charX >= 20) break;
				ssd1306_display_text_box1(&dev, j, 8*charX, element->name, nameLen, nameLen, (i==curX), 0);
				ESP_LOGI(tag, "Text[%d][%d]: [%s]; Name lenght [%d]",i,j, element->name, nameLen);
			}
		}
    }
}

void calcTaskCreate() {
	if (xCalcHdl == NULL) {
		xTaskCreate(calcTask, "calcTask", 2048, NULL, 5, &xCalcHdl);
	}
	appId = CALC_ID;
}

void calcTask() {
	calculator.yCursor = 0;
	appPrinter(&calculator, calculator.xCursor, calculator.yCursor);
	while (1) {
		if (appId == CALC_ID) {
			cursorMover();
			if (mid_btn.cnt >= 1) {
				struct appElement_t* element = calculator.appElements + calculator.yCursor * 10 + calculator.xCursor;
				// calculator.xCursor = 0;
				// calculator.yCursor = 0;
				if (element->action != NULL) {
					element->action(element->name, calculator.appElements->name);
				}
				mid_btn.cnt = 0;
				appPrinter(&calculator, calculator.xCursor, calculator.yCursor);
			}
			vTaskDelay(pdMS_TO_TICKS(10));
		} else vTaskDelay(pdMS_TO_TICKS(100));
	}
	vTaskDelete(NULL);
}

void exitApp() {
	appId = MENU_ID;
	menuPrinter(currentPage, currentId);
}

void cursorMover() {
	if (up_btn.cnt >= 1) {
		calculator.yCursor -= up_btn.cnt;
		up_btn.cnt = 0;
		appPrinter(&calculator, calculator.xCursor, calculator.yCursor);
	}
	if (down_btn.cnt >= 1) {
		calculator.yCursor += down_btn.cnt;
		down_btn.cnt = 0;
		appPrinter(&calculator, calculator.xCursor, calculator.yCursor);
	}
	if (left_btn.cnt >= 1) {
		calculator.xCursor -= up_btn.cnt;
		left_btn.cnt = 0;
		appPrinter(&calculator, calculator.xCursor, calculator.yCursor);
	}
	if (right_btn.cnt >= 1) {
		calculator.xCursor += down_btn.cnt;
		right_btn.cnt = 0;
		appPrinter(&calculator, calculator.xCursor, calculator.yCursor);
	}

}


