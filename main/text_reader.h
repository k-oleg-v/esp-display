#include "my_fns.h"

void readerCtxInit();
void readerPrinter();
void textReaderTaskCreate();
void textReaderTask();
void markAsRead();
void decode_comma(char *str);

void vInitMessageQueue(void);
void vPostMessageToQueue(const char *pcIncomingText);
void vQueueTask(void *pvParameters);
