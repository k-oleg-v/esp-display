#include "my_fns.h"
#include "calc.h"
#include "text_reader.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "text_writer.h"

struct menuElement_t screenSettingsPage[] = {
	{"Screen settings", "", 1, NULL},
	{"Change theme", "", 1, NULL}
};

struct menuElement_t mainPage[] = {
	{"   Main menu","", 4, NULL},
	// {"Turn off screen", "", 1, NULL},
	{"Reader", "", 1, textReaderTaskCreate, NULL},
	{"Calculator", "", 2, calcTaskCreate, NULL},
	{"Writer", "", 3, writerTaskCreate, NULL}
};

uint8_t currentId = 0;
struct menuElement_t* currentPage = mainPage;
extern SSD1306_t dev;
size_t curpageSize;
bool theme = 0;


appCtx_t* appPtr;
uint8_t appId;
appCtx_t menu;



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

// инициализация кнопок
void gpio_init() {
	esp_err_t ret = iot_button_new_gpio_device(&mid_btn.cfg, &mid_btn.gpio_cfg, &mid_btn.gpio_hdl);
	if(NULL == mid_btn.gpio_hdl || ret != ESP_OK) {
		ESP_LOGE(tag, "Button create failed");
	}

	
	ret = iot_button_new_gpio_device(&down_btn.cfg, &down_btn.gpio_cfg, &down_btn.gpio_hdl);
	if(NULL == down_btn.gpio_hdl || ret != ESP_OK) {
		ESP_LOGE(tag, "Down button create failed");
	}

	
	ret = iot_button_new_gpio_device(&up_btn.cfg, &up_btn.gpio_cfg, &up_btn.gpio_hdl);
	if(NULL == up_btn.gpio_hdl || ret != ESP_OK) {
		ESP_LOGE(tag, "Up button create failed");
	}

	ret = iot_button_new_gpio_device(&left_btn.cfg, &left_btn.gpio_cfg, &left_btn.gpio_hdl);
	if(NULL == left_btn.gpio_hdl || ret != ESP_OK) {
		ESP_LOGE(tag, "Left button create failed");
	}
	ret = iot_button_new_gpio_device(&right_btn.cfg, &right_btn.gpio_cfg, &right_btn.gpio_hdl);
	if(NULL == right_btn.gpio_hdl || ret != ESP_OK) {
		ESP_LOGE(tag, "right button create failed");
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
    io_conf.pull_down_en = 1;                   // Отключить pull-down
    io_conf.pull_up_en = 0;                     // Включить pull-up
    gpio_config(&io_conf);

    io_conf.intr_type = GPIO_INTR_DISABLE;      // Отключить прерывания
    io_conf.mode = GPIO_MODE_INPUT;            // Режим входа
    io_conf.pin_bit_mask = (1ULL << BTN_PIN_DOWN); // Выбор пина
    io_conf.pull_down_en = 1;                   // Отключить pull-down
    io_conf.pull_up_en = 0;                     // Включить pull-up
    gpio_config(&io_conf);

	io_conf.intr_type = GPIO_INTR_DISABLE;      // Отключить прерывания
    io_conf.mode = GPIO_MODE_INPUT;            // Режим входа
    io_conf.pin_bit_mask = (1ULL << BTN_PIN_LEFT); // Выбор пина
    io_conf.pull_down_en = 1;                   // Отключить pull-down
    io_conf.pull_up_en = 0;                     // Включить pull-up
    gpio_config(&io_conf);

	io_conf.intr_type = GPIO_INTR_DISABLE;      // Отключить прерывания
    io_conf.mode = GPIO_MODE_INPUT;            // Режим входа
    io_conf.pin_bit_mask = (1ULL << BTN_PIN_RIGHT); // Выбор пина
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
	ret = iot_button_register_cb(left_btn.gpio_hdl, BUTTON_PRESS_UP, NULL, leftClk, NULL);
	if(ret != ESP_OK){
		ESP_LOGE(tag, "leftClk failed");
	}
	ret = iot_button_register_cb(right_btn.gpio_hdl, BUTTON_PRESS_UP, NULL, rightClk, NULL);
	if(ret != ESP_OK){
		ESP_LOGE(tag, "rightClk failed");
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
		ESP_LOGI(tag, "down event is %s", iot_button_get_event_str(event));
	}
		event = iot_button_get_event(up_btn.gpio_hdl);
	if (event != BUTTON_NONE_PRESS) {
		ESP_LOGI(tag, "up event is %s", iot_button_get_event_str(event));
	}
		event = iot_button_get_event(left_btn.gpio_hdl);
	if (event != BUTTON_NONE_PRESS) {
		ESP_LOGI(tag, "left event is %s", iot_button_get_event_str(event));
	}
		event = iot_button_get_event(right_btn.gpio_hdl);
	if (event != BUTTON_NONE_PRESS) {
		ESP_LOGI(tag, "right event is %s", iot_button_get_event_str(event));
	}
	
		
}


//функции кнопок
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


// для приложений

void appCtxInit() {
	appId = MENU_ID;

	menu.ctxPtr = &menu;
	menu.menuElements = mainPage;
	sprintf(menu.name, "Main menu");
	menu.appParameters = malloc(sizeof(int16_t)*20);

	calcCtxInit();
    readerCtxInit();
	writerCtxInit();

	writerPrinter();
}

uint8_t cursorMover(appCtx_t* ctx) {
    uint8_t rows = ctx->appParameters[0]; // Высота сетки (4)
    uint8_t cols = ctx->appParameters[1]; // Ширина сетки (10)
    
    // 1. Фиксируем параметры исходного элемента (ДО перемещения)
    struct appElement_t* element = ctx->appElements + ctx->yCursor * cols + ctx->xCursor;
    uint8_t fxPos = element->xPos;
    uint8_t flen = (element->name != NULL) ? strlen(element->name) : 0;
	uint8_t fyPos = element->yPos;
    
    bool ret = false;
    bool verticalMovement = false;

    // --- БЛОК ДВИЖЕНИЯ ВВЕРХ ---
    if (up_btn.cnt >= 1) {
        uint8_t steps = up_btn.cnt;
        up_btn.cnt = 0;
        verticalMovement = true;
        
        for (uint8_t i = 0; i < steps; i++) {
            uint8_t startY = ctx->yCursor;
            bool rowFound = false;
            
            do {
                if (ctx->yCursor == 0) ctx->yCursor = rows - 1;
                else ctx->yCursor--;
                
                if (ctx->yCursor == startY) break; // Защита от бесконечного цикла
                
                // Ищем любую непустую ячейку на этой строке
                for (uint8_t x = 0; x < cols; x++) {
                    if ((ctx->appElements + ctx->yCursor * cols + x)->name != NULL) {
                        rowFound = true;
                        ctx->xCursor = x; // Временно встаем сюда
                        break;
                    }
                }
            } while (!rowFound);
        }
        ret = true;
    }

    // --- БЛОК ДВИЖЕНИЯ ВНИЗ ---
    if (down_btn.cnt >= 1) {
        uint8_t steps = down_btn.cnt;
        down_btn.cnt = 0;
        verticalMovement = true;
        
        for (uint8_t i = 0; i < steps; i++) {
            uint8_t startY = ctx->yCursor;
            bool rowFound = false;
            
            do {
                ctx->yCursor++;
                if (ctx->yCursor >= rows) ctx->yCursor = 0;
                
                if (ctx->yCursor == startY) break;
                
                // Ищем любую непустую ячейку на этой строке
                for (uint8_t x = 0; x < cols; x++) {
                    if ((ctx->appElements + ctx->yCursor * cols + x)->name != NULL) {
                        rowFound = true;
                        ctx->xCursor = x;
                        break;
                    }
                }
            } while (!rowFound);
        }
        ret = true;
    }

    // --- БЛОК ДВИЖЕНИЯ ВЛЕВО ---
    if (left_btn.cnt >= 1) {
        uint8_t steps = left_btn.cnt;
        left_btn.cnt = 0;
        
        for (uint8_t i = 0; i < steps; i++) {
            uint8_t startX = ctx->xCursor;
            do {
                if (ctx->xCursor == 0) ctx->xCursor = cols - 1;
                else ctx->xCursor--;
                
                if (ctx->xCursor == startX) return 1;
                element = ctx->appElements + ctx->yCursor * cols + ctx->xCursor;
            } while (element->name == NULL);
        }
        ret = true;
    }
    
    // --- БЛОК ДВИЖЕНИЯ ВПРАВО ---
    if (right_btn.cnt >= 1) {
        uint8_t steps = right_btn.cnt;
        right_btn.cnt = 0;
        
        for (uint8_t i = 0; i < steps; i++) {
            uint8_t startX = ctx->xCursor;
            do {
                ctx->xCursor++;
                if (ctx->xCursor >= cols) ctx->xCursor = 0;
                
                if (ctx->xCursor == startX) return 1;
                element = ctx->appElements + ctx->yCursor * cols + ctx->xCursor;
            } while (element->name == NULL);
        }
        ret = true;
    }

    // --- ИЗМЕНЕННЫЙ БЛОК ИНТУИТИВНОГО ПЕРЕМЕЩЕНИЯ ---
    if (ret && verticalMovement) { 
        uint8_t bestX = ctx->xCursor;
        int minDistance = 9999; // Задаем заведомо большое расстояние

        // 1. Вычисляем визуальную длину старого элемента на экране
        uint8_t visual_flen = flen;
        // Корректируем длину для спецсимволов, которые в памяти весят 2 байта, а на экране занимают 1 символ
        if (element->name != NULL && strchr(element->name, '\x82') != NULL) visual_flen = 6; // "chars" + стрелочка = 6 символов
        if (element->name != NULL && strchr(element->name, '\x83') != NULL) visual_flen = 1;
        if (element->name != NULL && strchr(element->name, '\x84') != NULL) visual_flen = 1;

        // Находим геометрический центр старого элемента на экране (умножаем на 2, чтобы избежать дробных чисел)
        int oldCenter = (fxPos * 2) + visual_flen;

        // 2. Сканируем всю новую строку (yCursor уже изменен кнопками ВВЕРХ/ВНИЗ)
        for (uint8_t x = 0; x < cols; x++) {
            struct appElement_t* evalElem = ctx->appElements + ctx->yCursor * cols + x;
            
            if (evalElem->name != NULL) {
                uint8_t sxPos = evalElem->xPos;
                uint8_t slen = strlen(evalElem->name);
                
                // Точно так же определяем визуальную длину оцениваемого элемента
                uint8_t visual_slen = slen;
                if (strchr(evalElem->name, '\x82') != NULL) visual_slen = 6;
                if (strchr(evalElem->name, '\x83') != NULL) visual_slen = 1;
                if (strchr(evalElem->name, '\x84') != NULL) visual_slen = 1;

                // Находим центр оцениваемого элемента на экране
                int newCenter = (sxPos * 2) + visual_slen;

                // Считаем абсолютное расстояние между центрами кнопок по горизонтали
                int distance = abs(newCenter - oldCenter);

                // Если этот элемент находится ближе к старому центру, чем предыдущие найденные
                if (distance < minDistance) {
                    minDistance = distance;
                    bestX = x; // Запоминаем индекс элемента в массиве ОЗУ
                }
            }
        }

        // Применяем самый близкий по геометрии индекс столбца
        ctx->xCursor = bestX;
    }
    
    return ret;
}
uint8_t cursorMover1(appCtx_t *ctx) {
	bool yMove = 0;
	uint8_t ret = 0;
	uint8_t rows = ctx->appParameters[0];
    uint8_t cols = ctx->appParameters[1];
	struct appElement_t* element = (ctx->appElements + ctx->yCursor*ctx->appParameters[1] + ctx->xCursor);

	uint8_t fxPos = element->xPos;
	uint8_t flen = strlen(element->name);

	if (up_btn.cnt >= 1) {
		ctx->yCursor-= up_btn.cnt;
		up_btn.cnt = 0;
		if (ctx->yCursor > rows) {
			ctx->yCursor = rows;
			element = (ctx->appElements + ctx->yCursor*cols + ctx->xCursor);
			while (element->name == NULL || element == NULL) {
				ctx->yCursor--;
			}
		}

		element = (ctx->appElements + ctx->yCursor*ctx->appParameters[1] + ctx->xCursor);
		while (element->name == NULL || element == NULL) {
		ctx->xCursor--;
		element = (ctx->appElements + ctx->yCursor*ctx->appParameters[1] + ctx->xCursor);
		}
		ret = 1;
		yMove = 1;
	} else if (down_btn.cnt >= 1) {
		ctx->yCursor-= down_btn.cnt;
		down_btn.cnt = 0;
		if (ctx->yCursor > ctx->appParameters[0]) {
			ctx->yCursor = 0;
		}

		element = (ctx->appElements + ctx->yCursor*ctx->appParameters[1] + ctx->xCursor);
		while (element->name == NULL || element == NULL) {
		ctx->xCursor--;
		element = (ctx->appElements + ctx->yCursor*ctx->appParameters[1] + ctx->xCursor);
		}
		ret = 1;
		yMove = 1;
	}
	if (left_btn.cnt >= 1) {
		ctx->xCursor-= left_btn.cnt;
		left_btn.cnt = 0;
		ret = 1;
		if (ctx->xCursor > ctx->appParameters[1]) {
			ctx->xCursor = ctx->appParameters[1];

			element = (ctx->appElements + ctx->yCursor*ctx->appParameters[1] + ctx->xCursor);
			while (element->name == NULL || element == NULL) {
				ctx->xCursor--;
				element = (ctx->appElements + ctx->yCursor*ctx->appParameters[1] + ctx->xCursor);
			}
		}
	} else if (right_btn.cnt >= 1) {
		ctx->xCursor+= left_btn.cnt;
		right_btn.cnt = 0;
		ret = 1;
		if (ctx->xCursor > ctx->appParameters[1]) {
			ctx->xCursor = 0;
		}
	}

	if (yMove && ret) {

		yMove = 0;
		element = (ctx->appElements + ctx->yCursor*ctx->appParameters[1] + ctx->xCursor);

		uint8_t dist = 255;
		uint8_t sxPos = element->xPos;
		uint8_t slen = strlen(element->name);
		
		uint8_t fxCenter = fxPos*2 + flen;
		uint8_t sxCenter = sxPos*2 + flen;

	}

	return ret;
}


void exitApp() {
	appId = MENU_ID;
	currentId = 0;
	menuPrinter(currentPage, currentId);
}

//меню
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




