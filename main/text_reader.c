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

extern SSD1306_t dev;

extern struct button_t mid_btn;
extern struct button_t up_btn;
extern struct button_t down_btn;
// extern struct button_t left_btn;
// extern struct button_t right_btn;

extern appCtx_t* appPtr;
extern uint8_t appId;

struct appElement_t readerElements[2][3];
appCtx_t reader;
char* inputText;

TaskHandle_t xReaderHdl = NULL;

uint8_t curTxtPage;

void readerCtxInit() {
    memset(&readerElements[0][0], 0, 6*sizeof(struct appElement_t));

    readerElements[0][0] = (struct appElement_t) {"Line:", NULL, 0, 0};
    readerElements[0][1] = (struct appElement_t) {(char*)malloc(6 * sizeof(char)), NULL, 5, 0};
    readerElements[0][2] = (struct appElement_t) {"x", exitApp, 15, 0};
    readerElements[1][0] = (struct appElement_t) {(char*)malloc(1000 * sizeof(char)), NULL, 0, 1};
    inputText = readerElements[1][0].name;
    memset(inputText, 0, 1000);

    reader.ctxPtr = &reader;
    reader.appElements = &readerElements[0][0];

    reader.appParameters = malloc(2*sizeof(int16_t));
    memset(reader.appParameters, 0, 2*sizeof(int16_t));
    reader.appParameters[0] = 2;
    reader.appParameters[1] = 3;
    
}

void readerPrinter() {
    sprintf(reader.appElements[1].name, " %d/%d     ", curTxtPage+1, (strlen(readerElements[1][0].name)/16)-1);
    for (uint8_t i = 0; i < 3; i++) {
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
        memcpy(txt, (inputText + (curTxtPage + j - 1)*16), 16);
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
                if(curTxtPage > (txtLen/16)-2) {
                    curTxtPage = (txtLen/16)-2;
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
                if(curTxtPage > (txtLen/16)-2) {
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
