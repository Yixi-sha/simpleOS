#include "../inc/disk.h"
#include "../inc/APIC.h"
#include "../inc/interrupt.h"
#include "../inc/printk.h"
#include "../inc/block.h"
#include "../inc/mm.h"
#include "../inc/lib.h"

hw_int_controller disk_int_controller = 
{
	.enable = IOAPIC_enable,
	.disable = IOAPIC_disable,
	.install = IOAPIC_install,
	.uninstall = IOAPIC_uninstall,
	.ack = IOAPIC_edge_ack,
};

struct RequestQueue diskRequest;

long IDE_open()
{
	color_printk(BLACK,WHITE,"DISK0 Opened\n");
	return 0;
}

long IDE_close()
{
	color_printk(BLACK,WHITE,"DISK0 Closed\n");
	return 0;
}

long IDE_ioctl(long cmd,long arg)
{
	struct BlockBufferNode * node = NULL;
	
	if(cmd == GET_IDENTIFY_DISK_CMD)
	{
		node = make_request(cmd,0,0,(unsigned char *)arg);
		submit(node);
		wait_for_finish();
		return 1;
	}
	
	return 0;
}

struct BolckDeviceOperation IDEDeviceOperation = 
{
	.m_open = IDE_open,
	.m_close = IDE_close,
	.m_ioctl = IDE_ioctl,
	.m_transfer = IDE_transfer,
};

static int diskFlags = 0;
struct IO_APIC_RET_entry entry;
unsigned char a[512];
void disk_init()
{

	entry.vector = 0x2e;
	entry.deliver_mode = APIC_ICR_IOAPIC_Fixed ;
	entry.dest_mode = ICR_IOAPIC_DELV_PHYSICAL;
	entry.deliver_status = APIC_ICR_IOAPIC_Idle;
	entry.polarity = APIC_IOAPIC_POLARITY_HIGH;
	entry.irr = APIC_IOAPIC_IRR_RESET;
	entry.trigger = APIC_ICR_IOAPIC_Edge;
	entry.mask = APIC_ICR_IOAPIC_Masked;
	entry.reserved = 0;

	entry.destination.physical.reserved1 = 0;
	entry.destination.physical.phy_dest = 0;
	entry.destination.physical.reserved2 = 0;

	register_irq(0x2e, &entry , &disk_handler, (unsigned long)&diskRequest, &disk_int_controller, "disk0");

	io_out8(PORT_DISK0_ALT_STA_CTL,0);
	
	list_init(&diskRequest.m_queueList);
	diskRequest.m_inUsing = NULL;
	diskRequest.m_blockRequestCount = 0;
	
	diskFlags = 0;
}

void disk_exit()
{
	unregister_irq(0x2e);
}

long IDE_transfer(long cmd, unsigned long blocks, long count, unsigned char* buffer)
{
	struct BlockBufferNode* node = NULL;

	if(cmd == ATA_READ_CMD || cmd == ATA_WRITE_CMD)
	{
		node = make_request(cmd, blocks, count, buffer);
		submit(node);
		wait_for_finish();
		color_printk(RED,BLACK,"IDE_transfer\n");
	}
	else
	{
		return -1;
	}
	
	return 0;
}

void submit(struct BlockBufferNode* node)
{
	add_request(node);
	if(diskRequest.m_inUsing == NULL)
	{
		cmd_out();
	}
}


void wait_for_finish()
{
	//diskFlags = 1;
	while(diskFlags)
		nop();
}

long cmd_out()
{
	struct BlockBufferNode * node = diskRequest.m_inUsing = container_of(list_next(&diskRequest.m_queueList),struct BlockBufferNode,m_list);
	list_del(&diskRequest.m_inUsing->m_list);
	diskRequest.m_blockRequestCount--;
	while(io_in8(PORT_DISK0_STATUS_CMD) & DISK_STATUS_BUSY)
		nop();
	switch(node->m_cmd)
	{
		diskFlags = 1;
		case ATA_WRITE_CMD:	
			io_out8(PORT_DISK0_DEVICE,0x40);

			io_out8(PORT_DISK0_ERR_FEATURE,0);
			io_out8(PORT_DISK0_SECTOR_CNT,(node->m_count >> 8) & 0xff);
			io_out8(PORT_DISK0_SECTOR_LOW ,(node->m_LBA >> 24) & 0xff);
			io_out8(PORT_DISK0_SECTOR_MID ,(node->m_LBA >> 32) & 0xff);
			io_out8(PORT_DISK0_SECTOR_HIGH,(node->m_LBA >> 40) & 0xff);

			io_out8(PORT_DISK0_ERR_FEATURE,0);
			io_out8(PORT_DISK0_SECTOR_CNT,node->m_count & 0xff);
			io_out8(PORT_DISK0_SECTOR_LOW,node->m_LBA & 0xff);
			io_out8(PORT_DISK0_SECTOR_MID,(node->m_LBA >> 8) & 0xff);
			io_out8(PORT_DISK0_SECTOR_HIGH,(node->m_LBA >> 16) & 0xff);
			while(!(io_in8(PORT_DISK0_STATUS_CMD) & DISK_STATUS_READY))
				nop();
			io_out8(PORT_DISK0_STATUS_CMD,node->m_cmd);
			while(!(io_in8(PORT_DISK0_STATUS_CMD) & DISK_STATUS_REQ))
				nop();
			port_outsw(PORT_DISK0_DATA,node->m_buffer,256);
			break;

		case ATA_READ_CMD:

			io_out8(PORT_DISK0_DEVICE,0x40);

			io_out8(PORT_DISK0_ERR_FEATURE,0);
			io_out8(PORT_DISK0_SECTOR_CNT,(node->m_count >> 8) & 0xff);
			io_out8(PORT_DISK0_SECTOR_LOW ,(node->m_LBA >> 24) & 0xff);
			io_out8(PORT_DISK0_SECTOR_MID ,(node->m_LBA >> 32) & 0xff);
			io_out8(PORT_DISK0_SECTOR_HIGH,(node->m_LBA >> 40) & 0xff);

			io_out8(PORT_DISK0_ERR_FEATURE,0);
			io_out8(PORT_DISK0_SECTOR_CNT,node->m_count & 0xff);
			io_out8(PORT_DISK0_SECTOR_LOW,node->m_LBA & 0xff);
			io_out8(PORT_DISK0_SECTOR_MID,(node->m_LBA >> 8) & 0xff);
			io_out8(PORT_DISK0_SECTOR_HIGH,(node->m_LBA >> 16) & 0xff);

			while(!(io_in8(PORT_DISK0_STATUS_CMD) & DISK_STATUS_READY))
				nop();
			io_out8(PORT_DISK0_STATUS_CMD,node->m_cmd);
			break;
			
		case GET_IDENTIFY_DISK_CMD:

			io_out8(PORT_DISK0_DEVICE,0xe0);
			
			io_out8(PORT_DISK0_ERR_FEATURE,0);
			io_out8(PORT_DISK0_SECTOR_CNT,node->m_count & 0xff);
			io_out8(PORT_DISK0_SECTOR_LOW,node->m_LBA & 0xff);
			io_out8(PORT_DISK0_SECTOR_MID,(node->m_LBA >> 8) & 0xff);
			io_out8(PORT_DISK0_SECTOR_HIGH,(node->m_LBA >> 16) & 0xff);

			while(!(io_in8(PORT_DISK0_STATUS_CMD) & DISK_STATUS_READY))
				nop();			
			io_out8(PORT_DISK0_STATUS_CMD,node->m_cmd);

		default:
			diskFlags = 0;
			color_printk(BLACK,WHITE,"ATA CMD Error\n");
			break;
	}
	return 0;
}

void add_request(struct BlockBufferNode* node)
{
	list_add_to_before(&diskRequest.m_queueList, &node->m_list);
	diskRequest.m_blockRequestCount++;
}

struct BlockBufferNode* make_request(long cmd, unsigned long blocks, long count, unsigned char* buffer)
{
	struct BlockBufferNode* ret = (struct BlockBufferNode*)kmalloc(sizeof(struct BlockBufferNode), 0);
	if(ret != NULL)
	{
		list_init(&ret->m_list);

		switch (cmd)
		{
		case ATA_READ_CMD:
			ret->m_end_handler = read_handler;
			ret->m_cmd = ATA_READ_CMD;
			break;
		
		case ATA_WRITE_CMD:
			ret->m_end_handler = wirte_handler;
			ret->m_cmd = ATA_WRITE_CMD;
			break;

		default:
			ret->m_end_handler = other_handler;
			ret->m_cmd = cmd;
			break;
		}

		ret ->m_LBA = blocks;
		ret->m_count = count;
		ret->m_buffer = buffer;
	}
	return ret;
}

void disk_handler(unsigned long nr, unsigned long parameter, struct pt_regs * regs)
{
	struct BlockBufferNode* node = ((struct RequestQueue*)parameter)->m_inUsing;
	node->m_end_handler(nr, parameter);
}

void read_handler(unsigned long nr, unsigned long parameter)
{
	struct BlockBufferNode * node = ((struct RequestQueue*)parameter)->m_inUsing;
	
	if(io_in8(PORT_DISK0_STATUS_CMD) & DISK_STATUS_ERROR)
		color_printk(RED,BLACK,"read_handler:%#010x\n",io_in8(PORT_DISK0_ERR_FEATURE));
	else
		port_insw(PORT_DISK0_DATA,node->m_buffer,256);
	end_request();
}

void wirte_handler(unsigned long nr, unsigned long parameter)
{
	if(io_in8(PORT_DISK0_STATUS_CMD) & DISK_STATUS_ERROR)
		color_printk(RED,BLACK,"write_handler:%#010x\n",io_in8(PORT_DISK0_ERR_FEATURE));
	end_request();
}

void other_handler(unsigned long nr, unsigned long parameter)
{
	struct BlockBufferNode * node = ((struct RequestQueue*)parameter)->m_inUsing;

	if(io_in8(PORT_DISK0_STATUS_CMD) & DISK_STATUS_ERROR)
		color_printk(RED,BLACK,"other_handler:%#010x\n",io_in8(PORT_DISK0_ERR_FEATURE));
	else
		port_insw(PORT_DISK0_DATA,node->m_buffer,256);

	end_request();
}

void end_request()
{
	kfree(diskRequest.m_inUsing);
	diskRequest.m_inUsing = NULL;
	diskFlags = 0;
	if(diskRequest.m_blockRequestCount)
	{
		cmd_out();
	}
		
}