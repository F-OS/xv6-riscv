#ifndef UART_H
#define UART_H
void uartinit(void);
void uartintr(void);
void uartputc(int c);
void uartputc_sync(int c);
int uartgetc(void);
#endif // UART_H