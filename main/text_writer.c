#include "text_writer.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "my_fns.h"
#include "ssd1306.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/_intsup.h>
#include <ctype.h>

extern SSD1306_t dev;
extern appCtx_t* appPtr;
extern uint8_t appId;

extern SemaphoreHandle_t display_mutex;

bool needToSend = 0;

extern struct button_t mid_btn;
// extern struct button_t up_btn;
// extern struct button_t down_btn;
// extern struct button_t left_btn;
// extern struct button_t right_btn;

struct appElement_t writerElements[7][15];
appCtx_t writer;

char* outputText;
bool cpslk = false;
bool shift = false;


TaskHandle_t xWriterHdl = NULL;

uint8_t writerCurTxtPage;

void writerCtxInit() {
    outputText = malloc(1000*sizeof(char));
    memset(outputText, 0, 1000*sizeof(char));
    memset(&writerElements[0][0], 0, 15*7*sizeof(struct appElement_t));

    // writerElements[0][0]  = (struct appElement_t) {"              ",   NULL, 15, 0};
    // writerElements[0][1]  = (struct appElement_t) {"x",   exitApp, 15, 0};
    writerElements[1][0]  = (struct appElement_t) {"~",   textEnter, 0, 1};
    writerElements[1][1]  = (struct appElement_t) {"0",   textEnter, 1, 1};
    writerElements[1][2]  = (struct appElement_t) {"1",   textEnter, 2, 1};
    writerElements[1][3]  = (struct appElement_t) {"2",   textEnter, 3, 1};
    writerElements[1][4]  = (struct appElement_t) {"3",   textEnter, 4, 1};
    writerElements[1][5]  = (struct appElement_t) {"4",   textEnter, 5, 1};
    writerElements[1][6]  = (struct appElement_t) {"5",   textEnter, 6, 1};
    writerElements[1][7]  = (struct appElement_t) {"6",   textEnter, 7, 1};
    writerElements[1][8]  = (struct appElement_t) {"7",   textEnter, 8, 1};
    writerElements[1][9]  = (struct appElement_t) {"8",   textEnter, 9, 1};
    writerElements[1][10] = (struct appElement_t) {"9",   textEnter, 10, 1};
    writerElements[1][11] = (struct appElement_t) {"-", textEnter, 11, 1};
    writerElements[1][12] = (struct appElement_t) {"+", textEnter, 12, 1};
    writerElements[1][13] = (struct appElement_t) {"del", textEnter, 13, 1};
    writerElements[2][0]  = (struct appElement_t) {"tab ", textEnter, 0, 2};
    writerElements[2][1]  = (struct appElement_t) {"(", textEnter, 4, 2};
    writerElements[2][2]  = (struct appElement_t) {")", textEnter, 5, 2};
    writerElements[2][3]  = (struct appElement_t) {"[", textEnter, 6, 2};
    writerElements[2][4]  = (struct appElement_t) {"]", textEnter, 7, 2};
    writerElements[2][5]  = (struct appElement_t) {"{", textEnter, 8, 2};
    writerElements[2][6]  = (struct appElement_t) {"}", textEnter, 9, 2};
    writerElements[2][7]  = (struct appElement_t) {",", textEnter, 10, 2};
    writerElements[2][8]  = (struct appElement_t) {".", textEnter, 11, 2};
    writerElements[2][9]  = (struct appElement_t) {"?", textEnter, 12, 2};
    writerElements[2][10] = (struct appElement_t) {"!", textEnter, 13, 2};
    writerElements[2][11] = (struct appElement_t) {"/", textEnter, 14, 2};
    writerElements[2][12] = (struct appElement_t) {";", textEnter, 15, 2};
    writerElements[3][0]  = (struct appElement_t) {"'", textEnter, 0, 3};
    writerElements[3][1]  = (struct appElement_t) {"\"", textEnter, 1, 3};
    writerElements[3][2]  = (struct appElement_t) {"[Space] ", textEnter, 2, 3};
    writerElements[3][3]  = (struct appElement_t) {"chars\x82", NULL, 10, 3};


    
    writerElements[4][0] = (struct appElement_t) {"tab", textEnter, 0, 1};
    writerElements[4][1] = (struct appElement_t) {"Q", textEnter, 3, 1};
    writerElements[4][2] = (struct appElement_t) {"W", textEnter, 4, 1};
    writerElements[4][3] = (struct appElement_t) {"E", textEnter, 5, 1};
    writerElements[4][4] = (struct appElement_t) {"R", textEnter, 6, 1};
    writerElements[4][5] = (struct appElement_t) {"T", textEnter, 7, 1};
    writerElements[4][6] = (struct appElement_t) {"Y", textEnter, 8, 1};
    writerElements[4][7] = (struct appElement_t) {"U", textEnter, 9, 1};
    writerElements[4][8] = (struct appElement_t) {"I", textEnter, 10, 1};
    writerElements[4][9] = (struct appElement_t) {"O", textEnter, 11, 1};
    writerElements[4][10] = (struct appElement_t) {"P", textEnter, 12, 1};
    writerElements[4][11] = (struct appElement_t) {"del", textEnter, 13, 1};

    writerElements[5][0] = (struct appElement_t) {"CpLk", textEnter, 0, 2};
    writerElements[5][1] = (struct appElement_t) {"A", textEnter, 4, 2};
    writerElements[5][2] = (struct appElement_t) {"S", textEnter, 5, 2};
    writerElements[5][3] = (struct appElement_t) {"D", textEnter, 6, 2};
    writerElements[5][4] = (struct appElement_t) {"F", textEnter, 7, 2};
    writerElements[5][5] = (struct appElement_t) {"G", textEnter, 8, 2};
    writerElements[5][6] = (struct appElement_t) {"H", textEnter, 9, 2};
    writerElements[5][7] = (struct appElement_t) {"J", textEnter, 10, 2};
    writerElements[5][8] = (struct appElement_t) {"K", textEnter, 11, 2};
    writerElements[5][9] = (struct appElement_t) {"L", textEnter, 12, 2};
    writerElements[5][10] = (struct appElement_t) {"\x84", sendText, 13, 2};

    writerElements[6][0] = (struct appElement_t) {"Shift", textEnter, 0, 3};
    writerElements[6][1] = (struct appElement_t) {"Z", textEnter, 5, 3};
    writerElements[6][2] = (struct appElement_t) {"X", textEnter, 6, 3};
    writerElements[6][3] = (struct appElement_t) {"C", textEnter, 7, 3};
    writerElements[6][4] = (struct appElement_t) {"V", textEnter, 8, 3};
    writerElements[6][5] = (struct appElement_t) {"B", textEnter, 9, 3};
    writerElements[6][6] = (struct appElement_t) {"N", textEnter, 10, 3};
    writerElements[6][7] = (struct appElement_t) {"M", textEnter, 11, 3};

    // writerElements[5][1] = (struct appElement_t) {"Line:", NULL, 10, 3};
    writerElements[6][8] = (struct appElement_t) {"\x82", NULL, 14, 3};
    writerElements[6][9] = (struct appElement_t) {"\x83", NULL, 15, 3};
    // writerElements[5][0] = (struct appElement_t) {"[Space] ", textEnter, 0, 3};

    writer.ctxPtr = &writer;
	writer.appElements = &writerElements[0][0];

    writer.appParameters = malloc(2*sizeof(int16_t));
    memset(writer.appParameters, 0, 2*sizeof(int16_t));
    writer.appParameters[0] = 7;
    writer.appParameters[1] = 15;
    // writer.yCursor = 4;
    writerPrinter();
}

void textEnter(void* vsymbol, void* vtext) {
    if (!vsymbol || !vtext) return;

    char* symbol = (char*) vsymbol;
    char* text = (char*) vtext;
    
    char buf[16];
    memset(buf, 0, sizeof(buf));

    bool is_special = false;

    // 1. Блок системных исключений
    if (!strcmp(symbol, "[Space] ")) {
        sprintf(buf, " ");
        is_special = true;
    } else if (!strcmp(symbol, "tab")) {
        sprintf(buf, "   ");
        is_special = true;
    } else if (!strcmp(symbol, "Shift")) {
        shift = !shift;
        return;
    } else if (!strcmp(symbol, "CpLk")) {
        cpslk = !cpslk;
        return;
    }

    // 2. Если это обычный символ (буква/цифра), копируем её в буфер
    if (!is_special) {
        snprintf(buf, sizeof(buf), "%s", symbol);
    }

    size_t current_len = strlen(text);

    // 3. Обработка клавиши удаления
    if (!strcmp(symbol, "del")) {
        if (current_len > 0) {
            text[current_len - 1] = '\0'; // Стираем последний символ
        }
        return;
    }

    // КРИТИЧЕСКИЙ СДВИГ: Считаем длину только ПОСЛЕ того, как buf гарантированно заполнен!
    size_t symbol_len = strlen(buf); 

    // 4. Проверка на переполнение буфера текста
    if (current_len + symbol_len >= 1000) {
        ESP_LOGW(tag, "Буфер текста заполнен! Ввод проигнорирован.");
        return;
    }

    // 5. Обработка регистра букв (Shift / CapsLock)
    if ((shift || cpslk) && (shift != cpslk)) {
        shift = 0; // Сбрасываем одинарный Shift после ввода одной буквы
        buf[0] = toupper((unsigned char)buf[0]);
    } else if (shift == cpslk) {
        shift = 0; // Сбрасываем одинарный Shift после ввода одной буквы
        buf[0] = tolower((unsigned char)buf[0]);
    }

    // 6. Вставка готового символа или строки в текст
    strncat(text, buf, symbol_len);
    current_len = strlen(text);    
    writerCurTxtPage = current_len / 16;
    ESP_LOGI(tag, "strlen: %zu, текст: %s", current_len, text);
}




void sendText() {
    needToSend = 1;
}
void writerPrinter() {

    if (display_mutex == NULL) return;

    // Захватываем мьютекс дисплея. Ждем, если шина занята.
    if (xSemaphoreTake(display_mutex, portMAX_DELAY) == pdTRUE) {
            char txt[17]; // 16 символов + 1 для нуля
        
        // 1. БЕЗОПАСНАЯ отрисовка верхней текстовой строки с учетом страниц
        memset(txt, 0, sizeof(txt));
        
        size_t total_len = strlen(outputText);
        size_t char_offset = writerCurTxtPage * 16;

        if (char_offset < total_len) {
            // Вычисляем, сколько символов осталось до конца строки на текущей странице
            size_t bytes_to_copy = total_len - char_offset;
            if (bytes_to_copy > 16) {
                bytes_to_copy = 16; // Берем максимум одну строку дисплея
            }
            memcpy(txt, outputText + char_offset, bytes_to_copy);
        }
        
        // Очищаем и выводим текст (если на странице пусто, выведутся пробелы)
        ssd1306_clear_line(&dev, 0, 0);
        size_t txtLen = strlen(txt);
        if (txtLen > 0) {
            ssd1306_display_text(&dev, 0, txt, txtLen, 0);
    }
        for (size_t j = 1; j < 4; j++) {
            memset(txt, ' ', 16); 
            txt[15] = '\0';
            size_t y = j;
            if (writer.yCursor > 3) {
                y = j+3;
            }

            if (y != writer.yCursor) {
                
                for (uint8_t i = 0; i < writer.appParameters[1]; i++) {
                    if (writerElements[y][i].name == NULL) continue;
                    size_t nameLen = strlen(writerElements[y][i].name);
                    size_t charX = writerElements[y][i].xPos;
                    memcpy(txt+charX, writerElements[y][i].name, nameLen);
                }
                txtLen = strlen(txt);
                ssd1306_display_text(&dev, j, txt, txtLen, 0);
                
            } else {
                for (size_t i = 0; i < writer.appParameters[1]; i++) {
                    size_t nameLen;
                    size_t charX = writerElements[y][i].xPos;
                    if ((j == 0) && (i == 0) && (writerElements[y][i].name[0] == 0)) {
                        
                        sprintf(txt, "%s", " ");
                        nameLen = strlen(txt);
                        ESP_LOGI("writerPrinter", "Text: <%s>; Element name: <%s>", txt, writerElements[y][i].name);
                    } else {
                        if (writerElements[y][i].name == NULL) continue;
                        nameLen = strlen(writerElements[y][i].name);
                        sprintf(txt, "%s", writerElements[y][i].name);
                    }
                    if (charX >= 20) break;
                    ssd1306_display_text_box1(&dev, j, 8*charX, txt, nameLen, nameLen, (i==writer.xCursor)*(y==writer.yCursor), 0);
                    vTaskDelay(pdMS_TO_TICKS(10));
                }
            }
        }
        xSemaphoreGive(display_mutex);
    }
    
}
void writerTaskCreate() {
    if (xWriterHdl == NULL) {
        xTaskCreate(writerTask, "writer task", 8192, NULL, 5, &xWriterHdl);
    }
    appId = WRIT_ID;
    writerPrinter();
}
void writerTask() {
    while (1) {
        if (appId == WRIT_ID) {
            if (cursorMover(&writer)) writerPrinter();
			if (mid_btn.cnt >= 1) {
				if (writerElements[writer.yCursor][writer.xCursor].action != NULL) {
					writerElements[writer.yCursor][writer.xCursor].action(writerElements[writer.yCursor][writer.xCursor].name, outputText);
				}
				mid_btn.cnt = 0;
				writerPrinter();
			}
            vTaskDelay(pdMS_TO_TICKS(10));
        } else {
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
    vTaskDelete(NULL);
}