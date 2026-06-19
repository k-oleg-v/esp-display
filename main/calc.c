#include "calc.h"
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

double opnd1 = 0;
double opnd2 = 0;
char oper[4] = "";
struct appElement_t calcElements[5][10];
appCtx_t calculator;
TaskHandle_t xCalcHdl = NULL;

extern SSD1306_t dev;

extern struct button_t mid_btn;
// extern struct button_t up_btn;
// extern struct button_t down_btn;
// extern struct button_t left_btn;
// extern struct button_t right_btn;

extern appCtx_t* appPtr;
extern uint8_t appId;



void calcCtxInit() {
    memset(&calcElements[0][0], 0, 50*sizeof(struct appElement_t));

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

	calcElements[0][0] = (struct appElement_t) { (char*)malloc(15 * sizeof(char)), digitEnter, 0, 0};
	memset(calcElements[0][0].name, 0, 15);

	calcElements[1][3] = (struct appElement_t) {"0", digitEnter, 3, 1};
	calcElements[2][3] = (struct appElement_t) {".", digitEnter, 3, 2};
	calcElements[3][3] = (struct appElement_t) {"=", eval, 3, 3};
	calcElements[1][4] = (struct appElement_t) {"+", eval, 4, 1};
	calcElements[2][4] = (struct appElement_t) {"x", eval, 4, 2};
	calcElements[3][4] = (struct appElement_t) {"^", eval, 4, 3};
	calcElements[1][5] = (struct appElement_t) {"-", eval, 5, 1};
	calcElements[2][5] = (struct appElement_t) {"\x81", eval, 5, 2};
	calcElements[3][5] = (struct appElement_t) {"\x80", eval, 5, 3};
	calcElements[1][6] = (struct appElement_t) {"sin", eval, 7, 1};
	calcElements[2][6] = (struct appElement_t) {"cos", eval, 7, 2};
	calcElements[3][6] = (struct appElement_t) {"tg", eval, 7, 3};
	calcElements[1][7] = (struct appElement_t) {"asin", eval, 11, 1};
	calcElements[2][7] = (struct appElement_t) {"acos", eval, 11, 2};
	calcElements[3][7] = (struct appElement_t) {"atg", eval, 11, 3};
	calcElements[0][1] = (struct appElement_t) {"AC", eval, 12, 0};
	calcElements[0][2] = (struct appElement_t) {"x", exitApp, 15, 0};
	calculator.ctxPtr = &calculator;
	calculator.appElements = &calcElements[0][0];

    calculator.appParameters = malloc(2*sizeof(int16_t));
    memset(calculator.appParameters, 0, 2*sizeof(int16_t));
    calculator.appParameters[0] = 4;
    calculator.appParameters[1] = 10;
}

void calcPrinter(appCtx_t* curAppCtx, uint8_t curX, uint8_t curY) {
    if (curAppCtx == NULL || curAppCtx->appElements == NULL) return;

    char rowBuf[21]; 
    for (size_t j = 0; j < 4; j++) {
        memset(rowBuf, ' ', 20);
        rowBuf[20] = '\0';
        
        if (j != curY) {
            
            for (size_t i = 0; i < 10; i++) {
                struct appElement_t* element = curAppCtx->appElements + j * 10 + i;
                
                if (element == NULL || element->name == NULL) continue;

                size_t charX = element->xPos;
                
                if (charX >= 20) break;
                
                size_t nameLen = strlen(element->name);
                if (charX + nameLen > 20) nameLen = 20 - charX; 
                memcpy(rowBuf + charX, element->name, nameLen);
            }
            ssd1306_display_text(&dev, j, rowBuf, 20, 0);
        } else {
			for (int i = 0; i < 10; i++) {
				
				char txt[15];
				struct appElement_t* element = curAppCtx->appElements + j * 10 + i;
				size_t charX = element->xPos;
				size_t nameLen;
				if ((j == 0) && (i == 0) && (element->name[0] == 0)) {
					
					sprintf(txt, "%s", "              ");
					nameLen = strlen(txt);
					ESP_LOGI("calcPrinter", "Text: <%s>; Element name: <%s>", txt, element->name);
				} else {
					if (element == NULL || element->name == NULL) continue;
					nameLen = strlen(element->name);
					sprintf(txt, "%s", element->name);
				}
                if (charX >= 20) break;
				ssd1306_display_text_box1(&dev, j, 8*charX, txt, nameLen, nameLen, (i==curX)*(j==curY), 0);
				memcpy(rowBuf + charX, element->name, nameLen);
			}
		}
		ESP_LOGI("PRINTER", "Row %d: [%s]", j, rowBuf);
    }
}

void calcTaskCreate() {
	if (xCalcHdl == NULL) {
		xTaskCreate(calcTask, "calcTask", 4096, NULL, 5, &xCalcHdl);
	}
	appId = CALC_ID;
	calcPrinter(&calculator, calculator.xCursor, calculator.yCursor);
}

void digitEnter(void* vsymbol, void* vnumber) {
    if (!vsymbol || !vnumber) return;

    const char* symbol = (const char*) vsymbol;
    char* number = (char*) vnumber;
    size_t current_len = strlen(number);
    size_t symbol_len = strlen(symbol);

    if (current_len + symbol_len > 14) {
        ESP_LOGW(tag, "Экран заполнен (20/20). Ввод проигнорирован.");
        return;
    }

    strncat(number, symbol, symbol_len);

    ESP_LOGI(tag, "strlen: %zu, текст: %s", strlen(number), number);
}

void calcTask() {
	calculator.yCursor = 0;
	calcPrinter(&calculator, calculator.xCursor, calculator.yCursor);
	while (1) {
		if (appId == CALC_ID) {
			if (cursorMover(&calculator)) calcPrinter(&calculator, calculator.xCursor, calculator.yCursor);
			if (mid_btn.cnt >= 1) {
				struct appElement_t* element = calculator.appElements + calculator.yCursor * 10 + calculator.xCursor;
				if (element->action != NULL) {
					element->action(element->name, calculator.appElements->name);
				}
				mid_btn.cnt = 0;
				calcPrinter(&calculator, calculator.xCursor, calculator.yCursor);
			}
			vTaskDelay(pdMS_TO_TICKS(10));
		} else vTaskDelay(pdMS_TO_TICKS(100));
	}
	vTaskDelete(NULL);
}


void eval(void* vsymbol, void* vnumber) {
	if (!vsymbol || !vnumber) return;

    const char* symbol = (const char*) vsymbol;
    char* number = (char*) vnumber;
	
	if (oper[0] == '\0') {
			sscanf(number, "%lf", &opnd1);
			sprintf(oper, "%s", symbol);
			sprintf(number, "%s", symbol);
			if (oper[0] == '\x80') {
				opnd1 *= -1;
				oper[0] = '\0';
				sprintf(number, "%g", opnd1);
				// calcElements[0][0].name = number;
			} else if (!strcmp(oper, "sin")) {
				opnd1 = sin((M_PI / 180.0)*opnd1);
				oper[0] = '\0';
				sprintf(number, "%g", opnd1);
				// calcElements[0][0].name = number;
			} else if (!strcmp(oper, "cos")) {
				opnd1 = cos((M_PI / 180.0)*opnd1);
				oper[0] = '\0';
				sprintf(number, "%g", opnd1);
				// calcElements[0][0].name = number;
			} else if (!strcmp(oper, "tg")) {
				opnd1 = tan((M_PI / 180.0)*opnd1);
				oper[0] = '\0';
				sprintf(number, "%g", opnd1);
				// calcElements[0][0].name = number;
			} else if (!strcmp(oper, "asin")) {
				opnd1 = asin(opnd1)/(M_PI / 180.0);
				oper[0] = '\0';
				sprintf(number, "%g", opnd1);
				// calcElements[0][0].name = number;
			} else if (!strcmp(oper, "acos")) {
				opnd1 = acos(opnd1)/(M_PI / 180.0);
				oper[0] = '\0';
				sprintf(number, "%g", opnd1);
				// calcElements[0][0].name = number;
			} else if (!strcmp(oper, "atg")) {
				opnd1 = atan(opnd1)/(M_PI / 180.0);
				oper[0] = '\0';
				sprintf(number, "%g", opnd1);
				// calcElements[0][0].name = number;
			} else if (!strcmp(oper, "AC")) {
				opnd1 = 0;
				opnd2 = 0;
				memset(number, 0, 19);
				oper[0] = 0;
			}
	} else {
		if (symbol[0] == '=') {
			char d;
			sscanf(number, "%c%lf", &d, &opnd2);
			if (oper[0] == '+') {
				sprintf(number, "%g", (opnd1 + opnd2));
			} else if (oper[0] == '-') {
				sprintf(number, "%g", (opnd1 - opnd2));
			} else if (oper[0] == 'x') {
				sprintf(number, "%g", (opnd1 * opnd2));
			} else if (oper[0] == '\x81') {
				sprintf(number, "%g", (opnd1 / opnd2));
			} else if (oper[0] == '^') {
				sprintf(number, "%g", pow(opnd1, opnd2));
			} 
			oper[0] = '\0';
		}
	}
}

