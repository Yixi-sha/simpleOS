#include "../inc/keyboard.h"
#include "../inc/APIC.h"
#include "../inc/interrupt.h"
#include "../inc/printk.h"
#include "../inc/process.h"

static struct keyboard_inputbuffer* p_kb = NULL;
static int shift_l,shift_r,ctrl_l,ctrl_r,alt_l,alt_r;

static struct IO_APIC_RET_entry keyboardEntry;

hw_int_controller keyboardIntController = 
{
    .enable = IOAPIC_enable,
    .disable = IOAPIC_disable,
	.install = IOAPIC_install,
    .uninstall = IOAPIC_uninstall,
    .ack = IOAPIC_edge_ack,
};

void keyboard_handler(unsigned long nr, unsigned long parament, struct pt_regs* reg)
{
    unsigned char x;
    x= io_in8(0x60);
    
    if(p_kb->count != 100)
    {
        p_kb->count++;
        *p_kb->m_head = x;
        if(p_kb->m_head == p_kb->buf + KB_BUF_SIZE - 1)
            p_kb->m_head = p_kb->buf;
        else
            p_kb->m_head++;
    }
    
}

void keyboard_init()
{
    unsigned int i = 0, j =0;
    p_kb = (struct keyboard_inputbuffer*)kmalloc(sizeof(struct keyboard_inputbuffer), 0);

    if(NULL == p_kb)
    {
        color_printk(RED, BLACK, "keyboard_init() init fail %s %d\n", __FILE__, __LINE__);
        return;
    }
    p_kb->m_head = p_kb->buf;
    p_kb->m_tail = p_kb->buf;
    p_kb->count = 0;
    memset(p_kb->buf, 0, KB_BUF_SIZE);

    keyboardEntry.vector = 0x21;
	keyboardEntry.deliver_mode = APIC_ICR_IOAPIC_Fixed ;
	keyboardEntry.dest_mode = ICR_IOAPIC_DELV_PHYSICAL;
	keyboardEntry.deliver_status = APIC_ICR_IOAPIC_Idle;
	keyboardEntry.polarity = APIC_IOAPIC_POLARITY_HIGH;
	keyboardEntry.irr = APIC_IOAPIC_IRR_RESET;
	keyboardEntry.trigger = APIC_ICR_IOAPIC_Edge;
	keyboardEntry.mask = APIC_ICR_IOAPIC_Masked;
	keyboardEntry.reserved = 0;

	keyboardEntry.destination.physical.reserved1 = 0;
	keyboardEntry.destination.physical.phy_dest = 0;
	keyboardEntry.destination.physical.reserved2 = 0;

    wait_KB_write();
    io_out8(PORT_KB_CMD, KBCMD_WRITE_CMD);
    wait_KB_write();
    io_out8(PORT_KB_DATA, KB_INIT_MODE);

    for(i = 0; i < 1000; i++)
        for(j = 0; j < 1000; j++)
            nop();

    shift_l = 0;
    shift_r = 0;
    ctrl_l = 0;
    ctrl_r = 0;
    alt_l = 0;
    alt_r = 0;

    register_irq(0x21, &keyboardEntry , &keyboard_handler, (unsigned long)p_kb, &keyboardIntController, "ps/2 keyboard");
}

void keyboard_exit()
{
    unregister_irq(0x21);
    kfree(p_kb);
}


unsigned char pausebreakScode[] = {0xE1, 0x1D, 0x45, 0xE1, 0x9D, 0xc5};

unsigned int keycode_map_normal[NR_SCAN_CODES * MAP_COLS] = //
{
/*scan-code	unShift		Shift		*/
/*--------------------------------------------------------------*/
/*0x00*/	0,		0,
/*0x01*/	0,		0,		//ESC
/*0x02*/	'1',		'!',
/*0x03*/	'2',		'@',
/*0x04*/	'3',		'#',
/*0x05*/	'4',		'$',
/*0x06*/	'5',		'%',
/*0x07*/	'6',		'^',
/*0x08*/	'7',		'&',
/*0x09*/	'8',		'*',
/*0x0a*/	'9',		'(',
/*0x0b*/	'0',		')',
/*0x0c*/	'-',		'_',
/*0x0d*/	'=',		'+',
/*0x0e*/	0,		0,		//BACKSPACE	
/*0x0f*/	0,		0,		//TAB

/*0x10*/	'q',		'Q',
/*0x11*/	'w',		'W',
/*0x12*/	'e',		'E',
/*0x13*/	'r',		'R',
/*0x14*/	't',		'T',
/*0x15*/	'y',		'Y',
/*0x16*/	'u',		'U',
/*0x17*/	'i',		'I',
/*0x18*/	'o',		'O',
/*0x19*/	'p',		'P',
/*0x1a*/	'[',		'{',
/*0x1b*/	']',		'}',
/*0x1c*/	0,		0,		//ENTER
/*0x1d*/	0x1d,		0x1d,		//CTRL Left
/*0x1e*/	'a',		'A',
/*0x1f*/	's',		'S',

/*0x20*/	'd',		'D',
/*0x21*/	'f',		'F',
/*0x22*/	'g',		'G',
/*0x23*/	'h',		'H',
/*0x24*/	'j',		'J',
/*0x25*/	'k',		'K',
/*0x26*/	'l',		'L',
/*0x27*/	';',		':',
/*0x28*/	'\'',		'"',
/*0x29*/	'`',		'~',
/*0x2a*/	0x2a,		0x2a,		//SHIFT Left
/*0x2b*/	'\\',		'|',
/*0x2c*/	'z',		'Z',
/*0x2d*/	'x',		'X',
/*0x2e*/	'c',		'C',
/*0x2f*/	'v',		'V',

/*0x30*/	'b',		'B',
/*0x31*/	'n',		'N',
/*0x32*/	'm',		'M',
/*0x33*/	',',		'<',
/*0x34*/	'.',		'>',
/*0x35*/	'/',		'?',
/*0x36*/	0x36,		0x36,		//SHIFT Right
/*0x37*/	'*',		'*',
/*0x38*/	0x38,		0x38,		//ALT Left
/*0x39*/	' ',		' ',
/*0x3a*/	0,		0,		//CAPS LOCK
/*0x3b*/	0,		0,		//F1
/*0x3c*/	0,		0,		//F2
/*0x3d*/	0,		0,		//F3
/*0x3e*/	0,		0,		//F4
/*0x3f*/	0,		0,		//F5

/*0x40*/	0,		0,		//F6
/*0x41*/	0,		0,		//F7
/*0x42*/	0,		0,		//F8
/*0x43*/	0,		0,		//F9
/*0x44*/	0,		0,		//F10
/*0x45*/	0,		0,		//NUM LOCK
/*0x46*/	0,		0,		//SCROLL LOCK
/*0x47*/	'7',		0,		/*PAD HONE*/
/*0x48*/	'8',		0,		/*PAD UP*/
/*0x49*/	'9',		0,		/*PAD PAGEUP*/
/*0x4a*/	'-',		0,		/*PAD MINUS*/
/*0x4b*/	'4',		0,		/*PAD LEFT*/
/*0x4c*/	'5',		0,		/*PAD MID*/
/*0x4d*/	'6',		0,		/*PAD RIGHT*/
/*0x4e*/	'+',		0,		/*PAD PLUS*/
/*0x4f*/	'1',		0,		/*PAD END*/

/*0x50*/	'2',		0,		/*PAD DOWN*/
/*0x51*/	'3',		0,		/*PAD PAGEDOWN*/
/*0x52*/	'0',		0,		/*PAD INS*/
/*0x53*/	'.',		0,		/*PAD DOT*/
/*0x54*/	0,		0,
/*0x55*/	0,		0,
/*0x56*/	0,		0,
/*0x57*/	0,		0,		//F11
/*0x58*/	0,		0,		//F12
/*0x59*/	0,		0,		
/*0x5a*/	0,		0,
/*0x5b*/	0,		0,
/*0x5c*/	0,		0,
/*0x5d*/	0,		0,
/*0x5e*/	0,		0,
/*0x5f*/	0,		0,

/*0x60*/	0,		0,
/*0x61*/	0,		0,
/*0x62*/	0,		0,
/*0x63*/	0,		0,
/*0x64*/	0,		0,
/*0x65*/	0,		0,
/*0x66*/	0,		0,
/*0x67*/	0,		0,
/*0x68*/	0,		0,
/*0x69*/	0,		0,
/*0x6a*/	0,		0,
/*0x6b*/	0,		0,
/*0x6c*/	0,		0,
/*0x6d*/	0,		0,
/*0x6e*/	0,		0,
/*0x6f*/	0,		0,

/*0x70*/	0,		0,
/*0x71*/	0,		0,
/*0x72*/	0,		0,
/*0x73*/	0,		0,
/*0x74*/	0,		0,
/*0x75*/	0,		0,
/*0x76*/	0,		0,
/*0x77*/	0,		0,
/*0x78*/	0,		0,
/*0x79*/	0,		0,
/*0x7a*/	0,		0,
/*0x7b*/	0,		0,
/*0x7c*/	0,		0,
/*0x7d*/	0,		0,
/*0x7e*/	0,		0,
/*0x7f*/	0,		0,
};

unsigned char analysis_keycode()
{
    unsigned char x = get_scancode();
    unsigned char key = 0;
    unsigned int i = 0;
    int make = 0;

    if(0xE1 == x)
    {
        key = PAUSERREAK;
        for(i = 0;i < 6; i++)
            if(get_scancode() != pausebreakScode[i])
            {
                key = 0;
                break;
            }
    }
    else if(0xE0 == x)
    {
        x = get_scancode();
        switch (x)
        {
        case 0x2a: //press printscreen
            if(0xE0 == get_scancode())
            {
                key = PRINTSCREEN;
                make = 1;
            }
            break;
        
        case 0xb7: // unpress printscreen
            if(0xE0 == get_scancode())
            {
                key = PRINTSCREEN;
                make = 0;
            }
            break;

        case 0x1d: // press right ctrl
            ctrl_l = 1;
            key = OTHERKEY;
            break;

        case 0x9d: //unpress right ctrl
            ctrl_l = 0;
            key = OTHERKEY;
            break;
            
        case 0x38: // press right alt
            alt_l = 1;
            key = OTHERKEY;
            break;
        
        case 0xb8: // unpress right alt
            alt_l = 0;
            key = OTHERKEY;
            break;

        default:
            key = OTHERKEY;
            break;
        }
    }

    if(0 == key)
    {
        unsigned int* keyrow = NULL;
        int column = 0;

        make = x & FLAG_BREAK ? 0 : 1;
        keyrow = &keycode_map_normal[(x & 0x7f) * MAP_COLS];
        if(shift_l || shift_r)
            column = 1;
        
        key = keyrow[column];
        switch (x & 0x7f)
        {
        case 0x2a: // shift_l
            shift_l = make;
            key = 0;
            break;
        
        case 0x36: // shift_r
            shift_r = make;
            key = 0;
            break;
        
        case 0x1d: // ctrl_l
            ctrl_l = make;
            key = 0;
            break;
        
        case 0x38: // ctrl_r
            ctrl_r = make;
            key = 0;
            break;
        
        default:
            if(!make)
                key = 0;
            break;
        }
    }
    if(key)
        color_printk(RED, BLACK, "(k:%c)\t",key);
    
    return key;
}

unsigned char get_scancode()
{
    unsigned char ret = 0;

    while(!p_kb->count)
        nop();
    ret = *p_kb->m_tail;
    p_kb->count--;
    if(p_kb->m_tail == p_kb->buf + KB_BUF_SIZE - 1)
        p_kb->m_tail = p_kb->buf;
    else
        p_kb->m_tail++; 
    
        
    return ret;
}

static struct keyboard_inputbuffer* mouseBuff = NULL;
static int mouseCount = 0;
static struct IO_APIC_RET_entry mouseEntry;

hw_int_controller mouseIntController = 
{
    .enable = IOAPIC_enable,
    .disable = IOAPIC_disable,
	.install = IOAPIC_install,
    .uninstall = IOAPIC_uninstall,
    .ack = IOAPIC_edge_ack,
};

void mouse_handler(unsigned long nr, unsigned long parament, struct pt_regs* reg)
{
    unsigned char x;
    x= io_in8(PORT_KB_DATA);
    
    
    if(mouseBuff->count != KB_BUF_SIZE)
    {
        *mouseBuff->m_head = x;
        mouseBuff->count++;
        if(mouseBuff->m_head == mouseBuff->buf + KB_BUF_SIZE - 1)
            mouseBuff->m_head = mouseBuff->buf;
        else
            mouseBuff->m_head++;
    }
       
}

void mouse_exit()
{
	unregister_irq(0x2c);
	kfree((unsigned long *)mouseBuff);
}

void mouse_init()
{
	unsigned long i,j;

	mouseBuff = (struct keyboard_inputbuffer *)kmalloc(sizeof(struct keyboard_inputbuffer),0);
	
	mouseBuff->m_head = mouseBuff->buf;
	mouseBuff->m_tail = mouseBuff->buf;
	mouseBuff->count  = 0;
	memset(mouseBuff->buf,0,KB_BUF_SIZE);

	mouseEntry.vector = 0x2c;
	mouseEntry.deliver_mode = APIC_ICR_IOAPIC_Fixed ;
	mouseEntry.dest_mode = ICR_IOAPIC_DELV_PHYSICAL;
	mouseEntry.deliver_status = APIC_ICR_IOAPIC_Idle;
	mouseEntry.polarity = APIC_IOAPIC_POLARITY_HIGH;
	mouseEntry.irr = APIC_IOAPIC_IRR_RESET;
	mouseEntry.trigger = APIC_ICR_IOAPIC_Edge;
	mouseEntry.mask = APIC_ICR_IOAPIC_Masked;
	mouseEntry.reserved = 0;

	mouseEntry.destination.physical.reserved1 = 0;
	mouseEntry.destination.physical.phy_dest = 0;
	mouseEntry.destination.physical.reserved2 = 0;

	mouseCount = 0;

	register_irq(0x2c, &mouseEntry , &mouse_handler, (unsigned long)mouseBuff, &mouseIntController, "ps/2 mouse");

	wait_KB_write();
	io_out8(PORT_KB_CMD,KBCMD_EN_MOUSE_INTFACE);

	for(i = 0;i<1000;i++)
		for(j = 0;j<1000;j++)
			nop();

	wait_KB_write();
	io_out8(PORT_KB_CMD,KBCMD_SENDTO_MOUSE);
	wait_KB_write();
	io_out8(PORT_KB_DATA,MOUSE_ENABLE);

	for(i = 0;i<1000;i++)
		for(j = 0;j<1000;j++)
			nop();

	wait_KB_write();
	io_out8(PORT_KB_CMD,KBCMD_WRITE_CMD);
	wait_KB_write();
	io_out8(PORT_KB_DATA,KB_INIT_MODE);
    
    
}

unsigned char get_mouse_code()
{
    unsigned char ret = 0;

    while(!mouseBuff->count)
        nop();
    ret = *mouseBuff->m_tail;
    mouseBuff->count--;
    if(mouseBuff->m_tail == mouseBuff->buf + KB_BUF_SIZE - 1)
        mouseBuff->m_tail = mouseBuff->buf;
    else
        mouseBuff->m_tail++; 
    color_printk(GREEN,WHITE,"(M:%02x)",ret);    
    return ret;
}
static struct MousePacket mouse;
unsigned char keyboard_exit_data()
{
    return p_kb->count ? 0 : 1;
}
unsigned char mouse_exit_data()
{
    return mouseBuff->count ? 0 : 1;
}
void analysis_mousecode()
{
    unsigned char x = get_mouse_code();

   switch(mouseCount)
	{
		case 0:
			mouseCount++;
			break;

		case 1:
			mouse.m_byte0 = x;
			mouseCount++;
			break;
		
		case 2:
			mouse.m_byte1 = (char)x;
			mouseCount++;
			break;

		case 3:
			mouse.m_byte2 = (char)x;
			mouseCount = 1;
			color_printk(RED,GREEN,"(M:%02x,X:%3d,Y:%3d)\n",mouse.m_byte0,mouse.m_byte1,mouse.m_byte2);
			break;

		default:			
			break;
	}
}