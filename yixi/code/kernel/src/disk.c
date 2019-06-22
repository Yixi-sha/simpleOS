#include "../inc/disk.h"
#include "../inc/APIC.h"
#include "../inc/interrupt.h"
#include "../inc/printk.h"

hw_int_controller disk_int_controller = 
{
	.enable = IOAPIC_enable,
	.disable = IOAPIC_disable,
	.install = IOAPIC_install,
	.uninstall = IOAPIC_uninstall,
	.ack = IOAPIC_edge_ack,
};

unsigned char wirte = 0;

void disk_handler(unsigned long nr, unsigned long parameter, struct pt_regs * regs)
{
	if(wirte == 0)
	{
		color_printk(WHITE,BLACK,"Write One Sector Finished:%02x\n",io_in8(PORT_DISK0_STATUS_CMD));
		wirte = 1;
		io_out8(PORT_DISK0_ALT_STA_CTL,0);

		while(io_in8(PORT_DISK0_STATUS_CMD) & DISK_STATUS_BUSY);
		color_printk(WHITE,BLACK,"Read One Sector Starting:%02x\n",io_in8(PORT_DISK0_STATUS_CMD));

		io_out8(PORT_DISK0_DEVICE,0x40);

		io_out8(PORT_DISK0_ERR_FEATURE,0);
		io_out8(PORT_DISK0_SECTOR_CNT,0);
		io_out8(PORT_DISK0_SECTOR_LOW,0x01);
		io_out8(PORT_DISK0_SECTOR_MID,0);
		io_out8(PORT_DISK0_SECTOR_HIGH,0);

		io_out8(PORT_DISK0_ERR_FEATURE,0);
		io_out8(PORT_DISK0_SECTOR_CNT,1);
		io_out8(PORT_DISK0_SECTOR_LOW,0x78);
		io_out8(PORT_DISK0_SECTOR_MID,0x56);
		io_out8(PORT_DISK0_SECTOR_HIGH,0x34);

		while(!(io_in8(PORT_DISK0_STATUS_CMD) & DISK_STATUS_READY));
		color_printk(WHITE,BLACK,"Send CMD:%02x\n",io_in8(PORT_DISK0_STATUS_CMD));
	
		io_out8(PORT_DISK0_STATUS_CMD,0x24);	////read
	}
	else
	{
		int i = 0;
		unsigned char b[512];
		port_insw(PORT_DISK0_DATA,&b,256);
		color_printk(WHITE,BLACK,"Read One Sector Finished:%02x\n",io_in8(PORT_DISK0_STATUS_CMD));
		for(i = 0;i<512;i++)
			color_printk(WHITE,BLACK,"%02x ",b[i]);
	}
	
}

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

	register_irq(0x2e, &entry , &disk_handler, 0, &disk_int_controller, "disk0");

	io_out8(PORT_DISK0_ALT_STA_CTL,0);
	
	while(io_in8(PORT_DISK0_STATUS_CMD) & DISK_STATUS_BUSY)
	{
		printk("busy");
	}
	color_printk(WHITE,BLACK,"Write One Sector Starting:%02x\n",io_in8(PORT_DISK0_STATUS_CMD));

	io_out8(PORT_DISK0_DEVICE,0x40);

	io_out8(PORT_DISK0_ERR_FEATURE,0);
	io_out8(PORT_DISK0_SECTOR_CNT,0);
	io_out8(PORT_DISK0_SECTOR_LOW,0x01);
	io_out8(PORT_DISK0_SECTOR_MID,0x00);
	io_out8(PORT_DISK0_SECTOR_HIGH,0x00);


	io_out8(PORT_DISK0_ERR_FEATURE,0);
	io_out8(PORT_DISK0_SECTOR_CNT,1);
	io_out8(PORT_DISK0_SECTOR_LOW,0x78);
	io_out8(PORT_DISK0_SECTOR_MID,0x56);
	io_out8(PORT_DISK0_SECTOR_HIGH,0x34);

	while(!(io_in8(PORT_DISK0_STATUS_CMD) & DISK_STATUS_READY));
	color_printk(WHITE,BLACK,"Send CMD:%02x\n",io_in8(PORT_DISK0_STATUS_CMD));

	io_out8(PORT_DISK0_STATUS_CMD,0x34);	////write
	
	while(!(io_in8(PORT_DISK0_STATUS_CMD) & DISK_STATUS_REQ));
	memset(&a,0x78,512);
	port_outsw(PORT_DISK0_DATA,&a,256);
}

void disk_exit()
{
	unregister_irq(0x2e);
}