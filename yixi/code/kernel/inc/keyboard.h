#ifndef __KEYBOARD_H__
#define __KEYBOARD_H__
#include "process.h"

#define KB_BUF_SIZE 100

#define PORT_KB_DATA	0x60
#define PORT_KB_STATUS	0x64
#define PORT_KB_CMD	0x64

#define KBSTATUS_IBF	0x02
#define KBSTATUS_OBF	0x01

#define KBCMD_WRITE_CMD	0x60
#define KBCMD_READ_CMD	0x20

#define KB_INIT_MODE	0x47

#define  wait_KB_write()	while(io_in8(PORT_KB_STATUS) & KBSTATUS_IBF)
#define  wait_KB_read()		while(io_in8(PORT_KB_STATUS) & KBSTATUS_OBF)

struct keyboard_inputbuffer
{
    unsigned char* m_head;
    unsigned char* m_tail;
    int count;
    unsigned char buf[KB_BUF_SIZE];
};

void keyboard_handler(unsigned long nr, unsigned long parament, struct pt_regs* reg);
void keyboard_init();

#define NR_SCAN_CODES 0x80
#define MAP_COLS  2

#define PAUSERREAK 1
#define PRINTSCREEN 2
#define OTHERKEY    4
#define FLAG_BREAK  0x80

unsigned char analysis_keycode();
unsigned char get_scancode();
unsigned char keyboard_exit_data();

#define KBCMD_SENDTO_MOUSE   0xd4
#define MOUSE_ENABLE         0xf4

#define KBCMD_EN_MOUSE_INTFACE 0xa8

struct MousePacket
{
    unsigned char m_byte0; //7:Y overflow 6:X overflow 5:Y sign bit 4:X sign bit
                           //3:Always 2:middle btn 1:right btn 0:left btn
    char m_byte1;
    char m_byte2;
};

void mouse_init();
void mouse_handler(unsigned long nr, unsigned long parament, struct pt_regs* reg);
void mouse_exit();
unsigned char get_mouse_code();
void analysis_mousecode();
unsigned char mouse_exit_data();

#endif