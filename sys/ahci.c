#include <sys/defs.h>
#include <sys/gdt.h>
#include <sys/kprintf.h>
#include <sys/console.h>
#include <sys/tarfs.h>
#include <sys/ahci.h>
#include <sys/idt.h>

#define INTEL				0x8086
#define AHCI_CONTROLLER		0x2922
#define MSD					0x01
#define SATA				0x06

#define	SATA_SIG_ATA	0x00000101	// SATA drive
#define	SATA_SIG_ATAPI	0xEB140101	// SATAPI drive
#define	SATA_SIG_SEMB	0xC33C0101	// Enclosure management bridge
#define	SATA_SIG_PM	0x96690101	// Port multiplier

#define	hba_port_t_DET_PRESENT	3
#define hba_port_t_IPM_ACTIVE	1

#define	AHCI_DEV_NULL		0
#define NEW_ABAR	0xA6000

#define ATA_CMD_READ_DMA_EX	0x25
#define	ATA_CMD_WRITE_DMA_EX	0x35

#define ATA_DEV_BUSY 0x80
#define ATA_DEV_DRQ 0x08

/* TODO : Try bringing it closer to the ABAR */
#define AHCI_BASE	0x800000

#define get_pci_data(val, offset)	(val >> (offset * 8))

#define NO_OF_BLOCKS	100

uint64_t abar;

int ahci_write(hba_port_t *port, uint32_t startl, uint32_t starth, uint32_t count, uint8_t *buf);
int ahci_read(hba_port_t *port, uint32_t startl, uint32_t starth, uint32_t count, uint8_t *buf);

uint32_t sys_in_long(uint16_t port)
{
	uint32_t result;
	__asm__ __volatile__ ("inl %1, %0" : "=a" (result) : "Nd" (port));
	return result;
}

void sys_out_long(uint16_t port, uint32_t data)
{
	__asm__ __volatile__ ("outl %0, %1" : : "a" (data), "Nd" (port));
}

/* Not used */
void delay()
{
	int spin = 0;
	for (uint64_t i = 0; i < 1000000; i++) {
		for (uint64_t j = 0; j < 10000; j++) {
			spin++;
		}
	}
}

uint64_t pci_config_read_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset)
{
	uint32_t address;
	uint32_t l_bus = (uint32_t)bus;
	uint32_t l_slot = (uint32_t)slot;
	uint32_t l_func = (uint32_t)func;

	address = (uint32_t)((l_bus << 16) |	/* Shift lbus 23 - 16 */
			(l_slot << 11) |			/* Shift lbus 15 - 11 */
			(l_func << 8) |				/* Shift Function Number 10 - 8 */
			(offset & 0xFC) | 			/* Last two bits must be 0 */
			(uint32_t)0x80000000);		/* Set Enable Bit  */

	sys_out_long(0x0CF8, address);
	/* Each offset gives 8 bit depth of the 32bit register.
	 * So every register has 4 offsets and are devided into
	 * information as device id, vendor id, class, etc
	 */
	/*
	 * Offsets and size are mentioned
	 * Device ID : 31-16 : 2 Bytes : 2
	 * Vendor ID : 15-0 : 2 Bytes : 0
	 * Class Code : 31:14 : 1 Byte : 11
	 * Subclass Code : 23:16 : 1 Byte : 10
	 * BAR5 : Prefetchable Memory Limit : 32
	 */
	return (sys_in_long(0xCFC) & 0xFFFFFFFFFFFFFFFF);
}

/* 0x5000000 */
uint64_t remap_bar(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset)
{
	uint32_t address;
	uint32_t l_bus = (uint32_t)bus;
	uint32_t l_slot = (uint32_t)slot;
	uint32_t l_func = (uint32_t)func;

	address = (uint32_t)((l_bus << 16) |	/* Shift lbus 23 - 16 */
			(l_slot << 11) |			/* Shift lbus 15 - 11 */
			(l_func << 8) |				/* Shift Function Number 10 - 8 */
			(offset & 0xFC) | 			/* Last two bits must be 0 */
			(uint32_t)0x80000000);		/* Set Enable Bit  */

	sys_out_long(0x0CF8, address);

	/* Each offset gives 8 bit depth of the 32bit register.
	 * So every register has 4 offsets and are devided into
	 * information as device id, vendor id, class, etc
	 */
	/*
	 * Offsets and size are mentioned
	 * Device ID : 31-16 : 2 Bytes : 2
	 * Vendor ID : 15-0 : 2 Bytes : 0
	 * Class Code : 31:14 : 1 Byte : 11
	 * Subclass Code : 23:16 : 1 Byte : 10
	 * BAR5 : Prefetchable Memory Limit : 0x24
	 */
	uint64_t bar5 = sys_in_long(0xCFC) & 0xFFFFFFFFFFFFFFFF;
	sys_out_long(0x0CFC, NEW_ABAR);
	bar5 = sys_in_long(0xCFC) & 0xFFFFFFFFFFFFFFFF;
	kprintf("bar5 : 0x%x", bar5);
	return bar5;
}


uint64_t get_ahci()
{
	uint8_t bus, device;
	uint64_t vendor_id = 0, device_id = 0, class_code = 0, sub_class_code = 0, func = 0;
	for (bus = 0; bus < 255; bus++) {
		for (device = 0; device < 32; device++) {
			for (func = 0; func < 8; func++) {
				vendor_id = pci_config_read_word(bus, device, func, 0);
				vendor_id = get_pci_data(vendor_id, 0) & 0xFFFF;
				device_id = pci_config_read_word(bus, device, func, 2);
				device_id = get_pci_data(device_id, 2) & 0xFFFF;
				class_code = pci_config_read_word(bus, device, func, 11);
				class_code = get_pci_data(class_code, 11 % 4) & 0xFF;
				sub_class_code = pci_config_read_word(bus, device, func, 10);
				sub_class_code = get_pci_data(sub_class_code, 10 % 4) & 0xFF;

				if (vendor_id != 0xFFFF) {
					//if (vendor_id == INTEL && device_id == AHCI_CONTROLLER && class_code == MSD && sub_class_code == SATA) {
					if (class_code == MSD && sub_class_code == SATA) {
						kprintf("!!! Found AHCI Controller !!!\n");
						kprintf("Vendor 0x%x Device 0x%x class 0x%x sub class 0x%x\n", vendor_id, device_id, class_code, sub_class_code);
						uint64_t address = pci_config_read_word(bus, device, 0, 0x24);
						address = get_pci_data(address, 0);
						address = remap_bar(bus, device, func, 0x24);
						return address;
					}
				}
			}
		}
	}
	return 0;
}


/* Check device type */
static int check_type(hba_port_t *port)
{
#if 0
		uint32_t ssts = port->ssts;

		uint8_t ipm = (ssts >> 8) & 0x0F;
		uint8_t det = ssts & 0x0F;
		if (det != hba_port_t_DET_PRESENT)	/* Check drive status */
				return AHCI_DEV_NULL;
		if (ipm != hba_port_t_IPM_ACTIVE)
				return AHCI_DEV_NULL;

#endif
		switch (port->sig) {
			case SATA_SIG_ATAPI:
				return AHCI_DEV_SATAPI;
			case SATA_SIG_SEMB:
				return AHCI_DEV_SEMB;
			case SATA_SIG_PM:
				return AHCI_DEV_PM;
			case SATA_SIG_ATA:
				return AHCI_DEV_SATA;
			default:
				kprintf("Problem Default\n");
				return -1;
		}
}

#define SUD	1
#define POD	2
#define FRE	4

#define set_bit(x, y)	(x |= 1 << y)
#define clr_bit(x, y)	(x &= ~(1 << y))

void port_init(hba_port_t *port)
{
	uint32_t t = 0;
#if 0
	/* Partial and slumber State Disable */
	t = port->sctl;
	t &= 0xFFFFF0FF;
	t |= 0x300;
	port->sctl = t;
#endif
	/* Set SUD */
	set_bit(port->cmd, SUD);

	delay();

	/* Set POD */
	set_bit(port->cmd, POD);
	delay();

	/* Set ICC : Bit 31:28 */
	t = port->cmd;
	t &= 0x0FFFFFFF;
	t |= 0x10000000;
	port->cmd = t;
	delay();

	/* Set FRE */
	//set_bit(port->cmd, FRE);

	/* Write all 1s to SERR Register */
	port->serr_rwc = 0xFFFFFFFF;
	delay();

	//port->ie = 0xFFFFFFFF;
	//kprintf("cmd:%x ", port->cmd);
	/* IS */
	port->is_rwc = 0xFFFFFFFF;
}

/*  Reset the port as per INTEL specification 10.4.2 */
void port_reset(hba_port_t *port)
{
	port->sctl = 0x301;

	/* Wait atleast for 500 ms */
	delay();

	port->sctl = 0x300;

	port_init(port);

	delay();

	//kprintf("st:%x ", port->ssts);
	/* Wait for DET in SSTS to become 3H  */
	while ((port->ssts & 0xF) != 0x3);
	//kprintf("st:%x ", port->ssts);
}

/* Start Command Engine */
void start_cmd(hba_port_t *port)
{
	/* Reset Port : Recommended only when system is not fault. */
	/* TODO: When something goes wrong on hardware, check here. */
	port_reset(port);

	/* Wait until CR (bit15) is cleared */
	while (port->cmd & HBA_PxCMD_CR);

	/* Set FRE (bit4) and ST (bit0) */
	port->cmd |= HBA_PxCMD_FRE;
	delay();
	port->cmd |= HBA_PxCMD_ST;
	delay();
}

/* Stop Command Engine */
void stop_cmd(hba_port_t *port)
{
	/* Clear ST (bit0) */
	port->cmd &= ~HBA_PxCMD_ST;
	delay();

	while (port->cmd & HBA_PxCMD_CR);

	/* Clear FRE (bit4) */
	port->cmd &= ~HBA_PxCMD_FRE;
	delay();

	while (port->cmd & HBA_PxCMD_FR);
}

void port_rebase(hba_port_t *port, int portno)
{
/* HBA reset should not be used in ideal condition */
#if 1
	/* Reset AHCI control, enable AHCI controller and Interrupts */
	hba_mem_t *t_abar = ((hba_mem_t *)abar);
	t_abar->ghc = t_abar->ghc | 0X01;
	t_abar->ghc = t_abar->ghc | 0X80000000;
	t_abar->ghc = t_abar->ghc | 0X02;

	/* Wait untill reset successfull */
	while (((t_abar->ghc) & 0x01) != 0);
#endif
	/* Stop Command Engine */
	stop_cmd(port);

	/* Command list offset: 1K*portno
	 * Command list entry size = 32
	 * Command list entry maxim count = 32
	 * Command list maxim size = 32*32 = 1K per port
	 */
	port->clb = AHCI_BASE + (portno << 10);
	memset((void*)(port->clb), 0, 1024);

	/* FIS offset: 32K+256*portno
	 * FIS entry size = 256 bytes per port
	 */
	port->fb = AHCI_BASE + (32<<10) + (portno << 8);
	memset((void*)(port->fb), 0, 256);

	/* Command table offset: 40K + 8K*portno
	 * Command table size = 256*32 = 8K per port
	 */
	hba_cmd_header_t *cmd_header = (hba_cmd_header_t*)(port->clb);
	for (int i = 0; i < 32; i++) {
		/* 8 PRDT entries per comamand tabled */
		cmd_header[i].prdtl = 8;
		/* 256 bytes per command table, 64+16+48+16*8
		 * Command table offset: 40K + 8K*portno + cmdheader_index*256
		 */
		cmd_header[i].ctba = AHCI_BASE + (40 << 10) + (portno << 13) + (i << 8);
		memset((void*)cmd_header[i].ctba, 0, 256);
	}

	/* Start Command Engine */
	start_cmd(port);
}

/* Find free command list slot */
int find_cmdslot(hba_port_t *port)
{
	/* If bit not set in SACT and CI, the slot is free */
	uint32_t slots = (port->sact | port->ci);
	/* Intel SATA Manual 1.3.1 : Bits 8 - 12 */
	uint8_t cmdslots = (((hba_mem_t *)abar)->cap >> 8) & 0x1F;
	for (int i = 0; i < cmdslots; i++) {
		if ((slots & 1) == 0) {
			//kprintf("cs:%x ", i);
			return i;
		}
		slots >>= 1;
	}
	kprintf("Cannot find free command list entry\n");
	return -1;
}

int read_write_lba(int port_no, uint8_t *write_buf, uint8_t *read_buf)
{
	/*Write*/
	int i = 0;
	int j = 0;
	for (i = 0; i < NO_OF_BLOCKS; i++) {
		/* Write to the LBA */
		memset(write_buf, i, 4096);
		ahci_write(&((hba_mem_t *)abar)->ports[port_no], i * 8, 0, 8, write_buf);
	}
	/* Read */
	kprintf("Verifying LBAs... ");
	for (i = 0; i < NO_OF_BLOCKS; i++) {
		/* Read from LBA */
		ahci_read(&((hba_mem_t *)abar)->ports[port_no], i * 8, 0, 8, read_buf);
		/* Check the data */
		for (j = 0; j < 4096; j++) {
			if (read_buf[j] != i) {
				kprintf(" r : %d i : %d ", read_buf[j], i);
				kprintf("Error in read LBA %d Byte %d\n read %d ... %p %p %p %p\n", i, j, read_buf[j], read_buf, read_buf + j, write_buf, write_buf + j);
				break;
			}
			
		}
	}
	kprintf("Verification Complete\n");
	return 1;

}

void probe_port(hba_mem_t *abar)
{
	/* Search disk in impelemented ports */
	uint32_t pi = abar->pi;
	kprintf("pi : %x", pi);
	int i = 0;
	while (i < 32) {
		if ((pi & 1)) {
			int dt = check_type(&abar->ports[i]);
			if (dt == AHCI_DEV_SATA || dt == AHCI_DEV_SATAPI) {
				/* Currently we read/write on port 1 */
				if (i == 0) {
					uint8_t *write_buf = (uint8_t *)0x400000;
					uint8_t *read_buf = (uint8_t *)0x500000;
					kprintf("SATA drive found at port %d\n", i);
					port_rebase(&abar->ports[i], i);
					read_write_lba(i, write_buf, read_buf);
					return;
				}
			} else if (dt == AHCI_DEV_SATAPI)
				kprintf("SATAPI drive found at port %d\n", i);
			else if (dt == AHCI_DEV_SEMB)
				kprintf("SEMB drive found at port %d\n", i);
			else if (dt == AHCI_DEV_PM)
				kprintf("PM drive found at port %d\n", i);
			else
				kprintf("No drive found at port %d\n", i);
		}
		pi >>= 1;
		i++;
	}
}

int ahci_read(hba_port_t *port, uint32_t startl, uint32_t starth, uint32_t count, uint8_t *buf)
{
	/* Clear Interrupt Bits */
	port->is_rwc = 0xFFFFFFFF;
	int spin = 0, cnt = count;

	int slot = find_cmdslot(port);
	if (slot == -1)
		return -1;

	hba_cmd_header_t *cmd_header = (hba_cmd_header_t*)port->clb;
	cmd_header += slot;

	/* FIS size  */
	cmd_header->cfl = sizeof(fis_reg_h2d_t)/sizeof(uint32_t);
	cmd_header->w = 0;
	//cmd_header->c = 1;
	//cmd_header->p = 1;

	/* PRDT entries count  */
	cmd_header->prdtl = (uint16_t)((count - 1) >> 3) + 1;

	hba_cmd_tbl_t *cmd_tbl = (hba_cmd_tbl_t*)(cmd_header->ctba);

	memset(cmd_tbl, 0, sizeof(hba_cmd_tbl_t) + (cmd_header->prdtl - 1) * sizeof(hba_prdt_entry_t));

	int i;
	for (i = 0; i < cmd_header->prdtl - 1; i++) {
		cmd_tbl->prdt_entry[i].dba = (uint64_t)buf;
		cmd_tbl->prdt_entry[i].dbc = 4*1024;
		cmd_tbl->prdt_entry[i].i = 1;
		buf += 4*1024;
		count -= 8;
	}

	/* Last entry */
	cmd_tbl->prdt_entry[i].dba = (uint64_t)buf;
	/* 512 bytes per sector */
	cmd_tbl->prdt_entry[i].dbc = count << 9;
	cmd_tbl->prdt_entry[i].i = 1;

	/* setup command */
	fis_reg_h2d_t *cmd_fis = (fis_reg_h2d_t*)(&cmd_tbl->cfis);

	cmd_fis->fis_type = FIS_TYPE_REG_H2D;
	cmd_fis->c = 1;
	cmd_fis->command = ATA_CMD_READ_DMA_EX;

	cmd_fis->lba0 = (uint8_t)startl;
	cmd_fis->lba1 = (uint8_t)(startl >> 8);
	cmd_fis->lba2 = (uint8_t)(startl >> 16);
	/* Set 6th Bit for LBA mode */
	cmd_fis->device = 1 << 6;

	cmd_fis->lba3 = (uint8_t)(startl >> 24);
	cmd_fis->lba4 = (uint8_t)starth;
	cmd_fis->lba5 = (uint8_t)(starth >> 8);

	cmd_fis->count = cnt;

	//kprintf("R:BUSY ");
	/* The below loop waits until the port is no longer busy before issuing a new command */
	while ((port->tfd & (ATA_DEV_BUSY | ATA_DEV_DRQ)) && spin < 1000000)
		spin++;

	if (spin == 1000000) {
		kprintf("Port is Hung\n");
		return -1;
	}

	port->ci = 1 << slot;

	/* Wait until CI is cleared */
	while (1) {
		//kprintf("a ");
		if ((port->ci & (1 << slot)) == 0) {
			//kprintf("pc %x  ", port->ci);
			break;
		}
		if (port->is_rwc & HBA_PxIS_TFES) {
			//kprintf("Read Error\n");
			return 0;
		}
	}

	if (port->is_rwc & HBA_PxIS_TFES) {
		kprintf("Read Error\n");
		return 0;
	}

	//kprintf("R:CI ");
	/* Wait until CI is cleared */
	//while(port->ci != 0);
	//kprintf("R:CI ");

	return 1;
}

int ahci_write(hba_port_t *port, uint32_t startl, uint32_t starth, uint32_t count, uint8_t *buf)  
{
	/* Clear Interrupt Bits */
	port->is_rwc = 0xFFFFFFFF;
	int spin = 0, cnt = count;

	int slot = find_cmdslot(port);
	if (slot == -1)
		return 0;

	hba_cmd_header_t *cmd_header = (hba_cmd_header_t*)port->clb;
	cmd_header += slot;

	/* FIS size  */
	cmd_header->cfl = sizeof(fis_reg_h2d_t)/sizeof(uint32_t);
	cmd_header->w = 1;
	//cmd_header->c = 1;
	//cmd_header->p = 1;

	/* PRDT entries count  */
	cmd_header->prdtl = (uint16_t)((count - 1) >> 3) + 1;

	hba_cmd_tbl_t *cmd_tbl = (hba_cmd_tbl_t*)(cmd_header->ctba);

	int i;
	for (i = 0; i < cmd_header->prdtl - 1; i++) {
		cmd_tbl->prdt_entry[i].dba = (uint64_t)(buf);
		cmd_tbl->prdt_entry[i].dbc = 4*1024;
		cmd_tbl->prdt_entry[i].i = 0;
		buf += 4*1024;
		count -= 8;
	}

	/* Last entry */
	cmd_tbl->prdt_entry[i].dba = (uint64_t)(buf);
	/* 512 bytes per sector */
	cmd_tbl->prdt_entry[i].dbc = count << 9;
	cmd_tbl->prdt_entry[i].i = 1;

	/* Setup command FIS */
	fis_reg_h2d_t *cmd_fis = (fis_reg_h2d_t*)(&cmd_tbl->cfis);

	cmd_fis->fis_type = FIS_TYPE_REG_H2D;
	cmd_fis->c = 1;
	cmd_fis->command = ATA_CMD_WRITE_DMA_EX;

	cmd_fis->lba0 = (uint8_t)startl;
	cmd_fis->lba1 = (uint8_t)(startl >> 8);
	cmd_fis->lba2 = (uint8_t)(startl >> 16);
	/* Set 6th Bit for LBA mode */
	cmd_fis->device = 1 << 6;

	cmd_fis->lba3 = (uint8_t)(startl >> 24);
	cmd_fis->lba4 = (uint8_t)starth;
	cmd_fis->lba5 = (uint8_t)(starth >> 8);

	cmd_fis->count = cnt;

	//kprintf("W:BUSY ");
	/* The below loop waits until the port is no longer busy before issuing a new command */
	while ((port->tfd & (ATA_DEV_BUSY | ATA_DEV_DRQ)) && spin < 1000000)
		spin++;

	if (spin == 1000000) {
		kprintf("Port is hung\n");
		return -1;
	}

	/* Issue Command */
	port->ci = 1 << slot;

	while (1) {
		if ((port->ci & (1<<slot)) == 0) {
			//kprintf("pc: ", port->ci);
			break;
		}
		if (port->is_rwc & HBA_PxIS_TFES) {
			kprintf("Write Error\n");
			return 0;
		}
	}

	if (port->is_rwc & HBA_PxIS_TFES) {
		kprintf("Write Error\n");
		return 0;
	}

	return 1;
}

void ahci_init()
{
	uint64_t address = get_ahci();
	/* Set global abar address  */
	abar = address;

	/* If in problem, try NEW_ABAR NEW_ABAR); */
	probe_port((hba_mem_t *)(address));
}
