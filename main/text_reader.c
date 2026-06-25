#include "text_reader.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "my_fns.h"
#include "ssd1306.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/_intsup.h>
#include <string.h>
#include <stdbool.h>

#define MAX_MSG_LEN    1000  // Максимальная длина одного текста
#define QUEUE_DEPTH    10    // Максимальное количество сообщений в очереди

extern SSD1306_t dev;

extern struct button_t mid_btn;
extern struct button_t up_btn;
extern struct button_t down_btn;
// extern struct button_t left_btn;
// extern struct button_t right_btn;

extern appCtx_t* appPtr;
extern uint8_t appId;

struct appElement_t readerElements[2][4];
appCtx_t reader;
char* inputText;
char* curText;

TaskHandle_t xReaderHdl = NULL;

SemaphoreHandle_t xButtonSemaphore = NULL;
QueueHandle_t xMessageQueue = NULL;

volatile bool isReadyForNextClick = false;


uint8_t curTxtPage;

void readerCtxInit() {
    memset(&readerElements[0][0], 0, 8*sizeof(struct appElement_t));

    readerElements[0][0] = (struct appElement_t) {"Line:", NULL, 0, 0};
    readerElements[0][1] = (struct appElement_t) {(char*)malloc(6 * sizeof(char)), NULL, 5, 0};
    readerElements[0][2] = (struct appElement_t) {"\x85", markAsRead, 13, 0};
    readerElements[0][3] = (struct appElement_t) {"x", exitApp, 15, 0};
    readerElements[1][0] = (struct appElement_t) {(char*)malloc(1000 * sizeof(char)), NULL, 0, 1};
    curText = readerElements[1][0].name;
    inputText = (char*)malloc(1000 * sizeof(char));
    memset(inputText, 0, 1000);
    memset(curText, '\0', 1000);

    reader.ctxPtr = &reader;
    reader.appElements = &readerElements[0][0];

    reader.appParameters = malloc(2*sizeof(int16_t));
    memset(reader.appParameters, 0, 2*sizeof(int16_t));
    reader.appParameters[0] = 2;
    reader.appParameters[1] = 4;
    
}

void readerPrinter() {
    sprintf(reader.appElements[1].name, " %d/%d   ", curTxtPage+1, (strlen(readerElements[1][0].name)/16)+1);
    for (uint8_t i = 0; i < reader.appParameters[1]; i++) {
        if (reader.appElements[i].name == NULL) continue;
        char txt[10];
        memset(txt, 0, 10);
        size_t nameLen = strlen(reader.appElements[i].name);
        size_t boxWidth = nameLen;
        if (reader.appElements[i+1].xPos != 0) {
            boxWidth = reader.appElements[i+1].xPos - reader.appElements[i].xPos;
        }
        ssd1306_display_text_box1(&dev, 0, 8*reader.appElements[i].xPos, reader.appElements[i].name, boxWidth, nameLen, (i == reader.xCursor), 0);
    }
    for (uint8_t j = 1; j < 4; j++) {
        char txt[16];
        memset(txt, 0, 16);
        memcpy(txt, (curText + (curTxtPage + j - 1)*16), 16);
        size_t len = strlen(txt);
        ssd1306_display_text(&dev, j, txt, 16, 0);
        for (size_t i = len; i < 16; i++) {
            ssd1306_display_text_box1(&dev, j, i*8, " ", 1, 1, 0, 0);
        }
        if (len < 16) {
            while (j+1 < 4) {
                ssd1306_clear_line(&dev, j+1, 0);
                j++;
            } break;
        }

    }
}

void textReaderTaskCreate() {
    if (xReaderHdl == NULL) {
        xTaskCreate(textReaderTask, "text reader", 8192, NULL, 5, &xReaderHdl);
    }
    appId = READ_ID;
    ssd1306_clear_screen(&dev, 0);
    readerPrinter();
}

void textReaderTask() {
    size_t txtLen = strlen(readerElements[1][0].name);
    while (1) {
        if (appId == READ_ID) {
            if (up_btn.cnt >= 1) {
                curTxtPage-=up_btn.cnt;
                up_btn.cnt = 0;
                if (reader.yCursor > 0) {
                    reader.yCursor = 0;
                }
                if(curTxtPage > (txtLen/16)) {
                    curTxtPage = (txtLen/16);
                }
                readerPrinter();
                // vTaskDelay(pdMS_TO_TICKS(25));
            }
            if (down_btn.cnt >= 1) {
                curTxtPage+=down_btn.cnt;
                down_btn.cnt = 0;
                if (reader.yCursor > 0) {
                    reader.yCursor = 0;
                }
                if((curTxtPage > (txtLen/16)-2) || ((curTxtPage > (txtLen/16)) && txtLen <= 2)) {
                    curTxtPage = 0;
                }
                readerPrinter();
                // vTaskDelay(pdMS_TO_TICKS(25));
            }
            if(cursorMover(&reader)) {
                readerPrinter();
                // vTaskDelay(pdMS_TO_TICKS(25));
            }
            if (mid_btn.cnt >= 1) {
                if (reader.appElements[reader.xCursor].action != NULL) {
                    reader.appElements[reader.xCursor].action(NULL, NULL);
                    readerPrinter();
                    mid_btn.cnt = 0;
                    vTaskDelay(pdMS_TO_TICKS(25));
                }
            }
            vTaskDelay(pdMS_TO_TICKS(100));
        } else {
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
    vTaskDelete(NULL);
}

void markAsRead() {
    if (xButtonSemaphore != NULL && isReadyForNextClick == true) {
        xSemaphoreGive(xButtonSemaphore);
    }
}

void decode_comma(char *str) {
    if (str == NULL) return;

    int read_idx = 0;
    int write_idx = 0;

    while (str[read_idx] != '\0') {
        // Проверяем, начинается ли подстрока %2C
        if ((str[read_idx] == '%') && str[read_idx + 1] == '2' && (str[read_idx + 2] == '1' || str[read_idx + 2] == '1')) {
            str[write_idx] = '!'; 
            read_idx += 3;
        } else if ((str[read_idx] == '%') && str[read_idx + 1] == '2' && (str[read_idx + 2] == '2' || str[read_idx + 2] == '2')) {
            str[write_idx] = '"'; 
            read_idx += 3;
        } else if ((str[read_idx] == '%') && str[read_idx + 1] == '2' && (str[read_idx + 2] == '3' || str[read_idx + 2] == '3')) {
            str[write_idx] = '#'; 
            read_idx += 3;
        } else if ((str[read_idx] == '%') && str[read_idx + 1] == '2' && (str[read_idx + 2] == '4' || str[read_idx + 2] == '4')) {
            str[write_idx] = '$'; 
            read_idx += 3;
        } else if ((str[read_idx] == '%') && str[read_idx + 1] == '2' && (str[read_idx + 2] == '5' || str[read_idx + 2] == '5')) {
            str[write_idx] = '%'; 
            read_idx += 3;
        } else if ((str[read_idx] == '%') && str[read_idx + 1] == '2' && (str[read_idx + 2] == '6' || str[read_idx + 2] == '6')) {
            str[write_idx] = '&'; 
            read_idx += 3;
        } else if ((str[read_idx] == '%') && str[read_idx + 1] == '2' && (str[read_idx + 2] == '7' || str[read_idx + 2] == '7')) {
            str[write_idx] = '\''; 
            read_idx += 3;
        } else if ((str[read_idx] == '%') && str[read_idx + 1] == '2' && (str[read_idx + 2] == '8' || str[read_idx + 2] == '8')) {
            str[write_idx] = '('; 
            read_idx += 3;
        } else if ((str[read_idx] == '%') && str[read_idx + 1] == '2' && (str[read_idx + 2] == '9' || str[read_idx + 2] == '9')) {
            str[write_idx] = ')'; 
            read_idx += 3;
        } else if ((str[read_idx] == '%') && str[read_idx + 1] == '2' && (str[read_idx + 2] == 'A' || str[read_idx + 2] == 'a')) {
            str[write_idx] = '*'; 
            read_idx += 3;
        } else if ((str[read_idx] == '%') && str[read_idx + 1] == '2' && (str[read_idx + 2] == 'B' || str[read_idx + 2] == 'b')) {
            str[write_idx] = '+'; 
            read_idx += 3;
        } else if ((str[read_idx] == '%') && str[read_idx + 1] == '2' && (str[read_idx + 2] == 'C' || str[read_idx + 2] == 'c')) {
            str[write_idx] = ','; 
            read_idx += 3;
        } else if ((str[read_idx] == '%') && str[read_idx + 1] == '2' && (str[read_idx + 2] == 'D' || str[read_idx + 2] == 'd')) {
            str[write_idx] = '-'; 
            read_idx += 3;
        } else if ((str[read_idx] == '%') && str[read_idx + 1] == '2' && (str[read_idx + 2] == 'E' || str[read_idx + 2] == 'e')) {
            str[write_idx] = '.';
            read_idx += 3;
        } else if ((str[read_idx] == '%') && str[read_idx + 1] == '2' && (str[read_idx + 2] == 'F' || str[read_idx + 2] == 'f')) {
            str[write_idx] = '/'; 
            read_idx += 3;
        } else if ((str[read_idx] == '%') && str[read_idx + 1] == '3' && (str[read_idx + 2] == 'A' || str[read_idx + 2] == 'a')) {
            str[write_idx] = ':'; 
            read_idx += 3;
        } else if ((str[read_idx] == '%') && str[read_idx + 1] == '3' && (str[read_idx + 2] == 'B' || str[read_idx + 2] == 'b')) {
            str[write_idx] = ';'; 
            read_idx += 3;
        } else if ((str[read_idx] == '%') && str[read_idx + 1] == '3' && (str[read_idx + 2] == 'C' || str[read_idx + 2] == 'c')) {
            str[write_idx] = '<'; 
            read_idx += 3;
        } else if ((str[read_idx] == '%') && str[read_idx + 1] == '3' && (str[read_idx + 2] == 'D' || str[read_idx + 2] == 'd')) {
            str[write_idx] = '='; 
            read_idx += 3;
        } else if ((str[read_idx] == '%') && str[read_idx + 1] == '3' && (str[read_idx + 2] == 'E' || str[read_idx + 2] == 'e')) {
            str[write_idx] = '>';
            read_idx += 3;
        } else if ((str[read_idx] == '%') && str[read_idx + 1] == '3' && (str[read_idx + 2] == 'F' || str[read_idx + 2] == 'f')) {
            str[write_idx] = '?'; 
            read_idx += 3;
        } else if ((str[read_idx] == '%') && str[read_idx + 1] == '4' && (str[read_idx + 2] == '0' || str[read_idx + 2] == '0')) {
            str[write_idx] = '@'; 
            read_idx += 3;
        } else if ((str[read_idx] == '%') && str[read_idx + 1] == '5' && (str[read_idx + 2] == 'B' || str[read_idx + 2] == 'b')) {
            str[write_idx] = '['; 
            read_idx += 3;
        } else if ((str[read_idx] == '%') && str[read_idx + 1] == '5' && (str[read_idx + 2] == 'C' || str[read_idx + 2] == 'c')) {
            str[write_idx] = '\\'; 
            read_idx += 3;
        } else if ((str[read_idx] == '%') && str[read_idx + 1] == '5' && (str[read_idx + 2] == 'D' || str[read_idx + 2] == 'd')) {
            str[write_idx] = ']'; 
            read_idx += 3;
        } else if ((str[read_idx] == '%') && str[read_idx + 1] == '5' && (str[read_idx + 2] == 'E' || str[read_idx + 2] == 'e')) {
            str[write_idx] = '^'; 
            read_idx += 3;
        } else if ((str[read_idx] == '%') && str[read_idx + 1] == '5' && (str[read_idx + 2] == 'F' || str[read_idx + 2] == 'f')) {
            str[write_idx] = '_'; 
            read_idx += 3;
        } else if ((str[read_idx] == '%') && str[read_idx + 1] == '6' && (str[read_idx + 2] == '0' || str[read_idx + 2] == '0')) {
            str[write_idx] = '`'; 
            read_idx += 3;
        } else if ((str[read_idx] == '%') && str[read_idx + 1] == '7' && (str[read_idx + 2] == 'B' || str[read_idx + 2] == 'b')) {
            str[write_idx] = '{'; 
            read_idx += 3;
        } else if ((str[read_idx] == '%') && str[read_idx + 1] == '7' && (str[read_idx + 2] == 'C' || str[read_idx + 2] == 'c')) {
            str[write_idx] = '|'; 
            read_idx += 3;
        } else if ((str[read_idx] == '%') && str[read_idx + 1] == '7' && (str[read_idx + 2] == 'D' || str[read_idx + 2] == 'd')) {
            str[write_idx] = '}'; 
            read_idx += 3;
        } else if ((str[read_idx] == '%') && str[read_idx + 1] == '7' && (str[read_idx + 2] == 'E' || str[read_idx + 2] == 'e')) {
            str[write_idx] = '~'; 
            read_idx += 3;
        } else {
        str[write_idx] = str[read_idx]; // Копируем текущий символ
        read_idx++;
        }
        write_idx++;
    }
    
    str[write_idx] = '\0'; // Устанавливаем новый конец строки
}



void vInitMessageQueue(void) {
    xMessageQueue = xQueueCreate(QUEUE_DEPTH, sizeof(char *));
    if (xMessageQueue == NULL) {
        for(;;);
    }
    xButtonSemaphore = xSemaphoreCreateBinary();
    if (xButtonSemaphore != NULL) {
        xSemaphoreTake(xButtonSemaphore, 0); // Изначально закрыт
    }
}

void vPostMessageToQueue(const char *pcIncomingText) {
    if (xMessageQueue == NULL || pcIncomingText == NULL) {
        return;
    }

    char *pcBuffer = (char *)pvPortMalloc(MAX_MSG_LEN);
    
    if (pcBuffer != NULL) {
        strncpy(pcBuffer, pcIncomingText, MAX_MSG_LEN - 1);
        pcBuffer[MAX_MSG_LEN - 1] = '\0'; 
        if (xQueueSend(xMessageQueue, &pcBuffer, 0) != pdPASS) {
            // Очередь полна (уже есть 10 сообщений). Освобождаем память.
            vPortFree(pcBuffer); 
        }
    }
}

void vQueueTask(void *pvParameters) {
    char *pcCurrentMessage = NULL;
    const char *TAG = "DISPLAY_TASK";

    for (;;) {
        // Шаг 1: Ждем новое сообщение из очереди
        if (xQueueReceive(xMessageQueue, &pcCurrentMessage, portMAX_DELAY) == pdPASS) {
            
            vTaskDelay(pdMS_TO_TICKS(300));

            isReadyForNextClick = false;
            curText = pcCurrentMessage;

            xSemaphoreTake(xButtonSemaphore, 0);

            isReadyForNextClick = true;
            
            xSemaphoreTake(xButtonSemaphore, portMAX_DELAY);

            isReadyForNextClick = false;

            vPortFree(pcCurrentMessage);
            pcCurrentMessage = NULL;
        }
    }
}

