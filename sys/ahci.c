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
#define NEW_ABAR	0x10000000

#define ATA_CMD_READ_DMA_EX	0x25
#define	ATA_CMD_WRITE_DMA_EX	0x35

/* TODO : Try bringing it closer to the ABAR */
#define AHCI_BASE	0x20000000

#define get_pci_data(val, offset)	(val >> (offset * 8))

uint64_t abar;

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
	return bar5;
}


uint64_t get_ahci()
{
	uint8_t bus, device;
	uint64_t vendor_id = 0, device_id = 0, class_code = 0, sub_class_code = 0;
	for (bus = 0; bus < 255; bus++) {
		for (device = 0; device < 32; device++) {
			vendor_id = pci_config_read_word(bus, device, 0, 0);
			vendor_id = get_pci_data(vendor_id, 0) & 0xFFFF;
			device_id = pci_config_read_word(bus, device, 0, 2);
			device_id = get_pci_data(device_id, 2) & 0xFFFF;
			class_code = pci_config_read_word(bus, device, 0, 11);
			class_code = get_pci_data(class_code, 11 % 4) & 0xFF;
			sub_class_code = pci_config_read_word(bus, device, 0, 10);
			sub_class_code = get_pci_data(sub_class_code, 10 % 4) & 0xFF;

			if (vendor_id != 0xFFFF) {
				if (vendor_id == INTEL && device_id == AHCI_CONTROLLER && class_code == MSD && sub_class_code == SATA) {
					kprintf("!!! Found AHCI Controller !!!\n");
					uint64_t address = pci_config_read_word(bus, device, 0, 0x24);
					address = get_pci_data(address, 0);
					kprintf(" 0x%x\n", address);
					address = remap_bar(bus, device, 0, 0x24);
					kprintf("  bar5 0x%x\n", address);
					return address;
				}
				kprintf("Vendor 0x%x Device 0x%x class 0x%x sub class 0x%x\n", vendor_id, device_id, class_code, sub_class_code);
			}
		}
	}
	return 0;
}

void search_disk(uint64_t address)
{
	hba_mem_t *abar = (hba_mem_t *)address;
	uint32_t pi = abar->pi;
	kprintf("0x%x pi\n", pi);
	int i = 0;
	while (i < 32) {
		if (pi & 1)
			kprintf("%p\n", abar->ports[i++]);
		pi >>= 1;
	}
}

/* Check device type */
static int check_type(hba_port_t *port)
{
		uint32_t ssts = port->ssts;

		uint8_t ipm = (ssts >> 8) & 0x0F;
		uint8_t det = ssts & 0x0F;

		if (det != hba_port_t_DET_PRESENT)	/* Check drive status */
				return AHCI_DEV_NULL;
		if (ipm != hba_port_t_IPM_ACTIVE)
				return AHCI_DEV_NULL;

		switch (port->sig) {
				case SATA_SIG_ATAPI:
					return AHCI_DEV_SATAPI;
				case SATA_SIG_SEMB:
					return AHCI_DEV_SEMB;
				case SATA_SIG_PM:
					return AHCI_DEV_PM;
				default:
					return AHCI_DEV_SATA;
		}
}

void port_rebase(hba_port_t *port, int portno)
{
	//stop_cmd(port);	// Stop command engine

	// Command list offset: 1K*portno
	// Command list entry size = 32
	// Command list entry maxim count = 32
	// Command list maxim size = 32*32 = 1K per port
	port->clb = AHCI_BASE + (portno<<10);
	memset((void*)(port->clb), 0, 1024);

	// FIS offset: 32K+256*portno
	// FIS entry size = 256 bytes per port
	port->fb = AHCI_BASE + (32<<10) + (portno<<8);
	memset((void*)(port->fb), 0, 256);

	// Command table offset: 40K + 8K*portno
	// Command table size = 256*32 = 8K per port
	hba_cmd_header_t *cmdheader = (hba_cmd_header_t*)(port->clb);
	for (int i=0; i<32; i++) {
		cmdheader[i].prdtl = 8;	// 8 prdt entries per command table
		// 256 bytes per command table, 64+16+48+16*8
		// Command table offset: 40K + 8K*portno + cmdheader_index*256
		cmdheader[i].ctba = AHCI_BASE + (40<<10) + (portno<<13) + (i<<8);
		memset((void*)cmdheader[i].ctba, 0, 256);
	}

	//start_cmd(port);	// Start command engine
}

// Find a free command list slot
int find_cmdslot(hba_port_t *port)
{
	// If not set in SACT and CI, the slot is free
	uint32_t slots = (port->sact | port->ci);
	/* Intel SATA Manual 1.3.1 : Bits 8 - 12 */
	uint8_t cmdslots = ((hba_mem_t *)abar)->cap & 0x0f00 >> 8;
	for (int i = 0; i < cmdslots; i++) {
		if ((slots&1) == 0)
			return i;
		slots >>= 1;
	}
	kprintf("Cannot find free command list entry\n");
	return -1;
}

void probe_port(hba_mem_t *abar)
{
	/* Search disk in impelemented ports */
	uint32_t pi = abar->pi;
	int i = 0;
	while (i < 32) {
		if (pi & 1) {
			int dt = check_type(&abar->ports[i]);
			if (dt == AHCI_DEV_SATA) {
				kprintf("SATA drive found at port %d\n", i);
				port_rebase(abar->ports, i);
				kprintf("Free Slot : 0x%x\n", find_cmdslot(&abar->ports[i]));
				return;
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

#define ATA_DEV_BUSY 0x80
#define ATA_DEV_DRQ 0x08
int read(hba_port_t *port, uint32_t startl, uint32_t starth, uint32_t count, uint16_t *buf)
{
	port->is_rwc = 0xFFFFFFFF;		// Clear pending interrupt bits
	int spin = 0; // Spin lock timeout counter

	int slot = find_cmdslot(port);
	slot++;
	if (slot == -1)
		return -1;

	kprintf("Slots present\n");

	hba_cmd_header_t *cmdheader = (hba_cmd_header_t*)port->clb;
	cmdheader += slot;
	cmdheader->cfl = sizeof(fis_reg_h2d_t)/sizeof(uint32_t);	// Command FIS size
	cmdheader->w = 0;		// Read from device
	cmdheader->prdtl = (uint16_t)((count-1)>>4) + 1;	// PRDT entries count
 
	hba_cmd_tbl_t *cmdtbl = (hba_cmd_tbl_t*)(cmdheader->ctba);

	memset(cmdtbl, 0, sizeof(hba_cmd_tbl_t) + (cmdheader->prdtl-1)*sizeof(hba_prdt_entry_t));
 
	int i;
	// 8K bytes (16 sectors) per PRDT
	for (i=0; i<cmdheader->prdtl-1; i++) {
		cmdtbl->prdt_entry[i].dba = (uint64_t)buf;
		cmdtbl->prdt_entry[i].dbc = 8*1024;	// 8K bytes
		cmdtbl->prdt_entry[i].i = 1;
		buf += 4*1024;	// 4K words
		count -= 16;	// 16 sectors
	}

	// Last entry
	cmdtbl->prdt_entry[i].dba = (uint64_t)buf;
	cmdtbl->prdt_entry[i].dbc = count<<9;	// 512 bytes per sector
	cmdtbl->prdt_entry[i].i = 1;
 
	// Setup command
	fis_reg_h2d_t *cmdfis = (fis_reg_h2d_t*)(&cmdtbl->cfis);
 
	cmdfis->fis_type = FIS_TYPE_REG_H2D;
	cmdfis->c = 1;	// Command
	cmdfis->command = ATA_CMD_READ_DMA_EX;
 
	cmdfis->lba0 = (uint8_t)startl;
	cmdfis->lba1 = (uint8_t)(startl>>8);
	cmdfis->lba2 = (uint8_t)(startl>>16);
	cmdfis->device = 1<<6;	// LBA mode
 
	cmdfis->lba3 = (uint8_t)(startl>>24);
	cmdfis->lba4 = (uint8_t)starth;
	cmdfis->lba5 = (uint8_t)(starth>>8);
 
	cmdfis->count = count;
 
	// The below loop waits until the port is no longer busy before issuing a new command
	while ((port->tfd & (ATA_DEV_BUSY | ATA_DEV_DRQ)) && spin < 1000000)
	{
		spin++;
	}
	if (spin == 1000000)
	{
		kprintf("Port is hung\n");
		return -1;
	}
 
	port->ci = 1<<slot;	// Issue command
 
	// Wait for completion
	while (1) {
		// In some longer duration reads, it may be helpful to spin on the DPS bit 
		// in the PxIS port field as well (1 << 5)
		if ((port->ci & (1<<slot)) == 0) 
			break;
		if (port->is_rwc & HBA_PxIS_TFES)	// Task file error
		{
			kprintf("Read disk error\n");
			return -1;
		}
	}
 
	// Check again
	if (port->is_rwc & HBA_PxIS_TFES) {
		kprintf("Read disk error\n");
		return -1;
	}
 
	return 1;
}

int write(hba_port_t *port, uint32_t startl, uint32_t starth, uint32_t count, uint64_t buf)  
{
       port->is_rwc = 0xffff;              // Clear pending interrupt bits
       // int spin = 0;           // Spin lock timeout counter
        int slot = find_cmdslot(port);
        if (slot == -1)
                return 0;
        uint64_t addr = 0;
     //   kprintf("\n clb %x clbu %x", port->clb, port->clbu);
        addr = port->clb;
        hba_cmd_header_t *cmdheader = (hba_cmd_header_t*)(addr);
 
        //hba_cmd_header_t *cmdheader = (hba_cmd_header_t*)(port->clb);
        cmdheader += slot;
       cmdheader->cfl = sizeof(fis_reg_h2d_t)/sizeof(uint32_t);     // Command FIS size
        cmdheader->w = 1;               // Read from device
        cmdheader->c = 1;               // Read from device
        cmdheader->p = 1;               // Read from device
        // 8K bytes (16 sectors) per PRDT
        cmdheader->prdtl = (uint16_t)((count-1)>>4) + 1;    // PRDT entries count
 
        addr=0;
        addr= cmdheader->ctba;
        hba_cmd_tbl_t *cmdtbl = (hba_cmd_tbl_t*)(addr);
        
        //memset(cmdtbl, 0, sizeof(hba_cmd_tbl_t) + (cmdheader->prdtl-1)*sizeof(HBA_PRDT_ENTRY));
        int i = 0; 
    //    kprintf("[PRDTL][%d]", cmdheader->prdtl);
        // 8K bytes (16 sectors) per PRDT
        for (i=0; i<cmdheader->prdtl-1; i++)
        {
               cmdtbl->prdt_entry[i].dba = (uint64_t)(buf);
                cmdtbl->prdt_entry[i].dbc = 8*1024-1;     // 8K bytes
                cmdtbl->prdt_entry[i].i = 0;
                buf += 4*1024;  // 4K words
                count -= 16;    // 16 sectors
       }
        /**If the final Data FIS transfer in a command is for an odd number of 16-bit words, the transmitter�s
Transport layer is responsible for padding the final Dword of a FIS with zeros. If the HBA receives one
more word than is indicated in the PRD table due to this padding requirement, the HBA shall not signal
this as an overflow condition. In addition, if the HBA inserts padding as required in a FIS it is transmitting,
an overflow error shall not be indicated. The PRD Byte Count field shall be updated based on the
number of words specified in the PRD table, ignoring any additional padding.**/
        
        // Last entry

        cmdtbl->prdt_entry[i].dba = (uint64_t)(buf);
        cmdtbl->prdt_entry[i].dbc = count << 9 ;   // 512 bytes per sector
        cmdtbl->prdt_entry[i].i = 0;
        

        // Setup command
        fis_reg_h2d_t *cmdfis = (fis_reg_h2d_t*)(&cmdtbl->cfis);
 
        cmdfis->fis_type = FIS_TYPE_REG_H2D;
        cmdfis->c = 1;  // Command
        cmdfis->command = ATA_CMD_WRITE_DMA_EX;
 
        cmdfis->lba0 = (uint8_t)startl;
        cmdfis->lba1 = (uint8_t)(startl>>8);
        cmdfis->lba2 = (uint8_t)(startl>>16);
        cmdfis->device = 1<<6;  // LBA mode
 
        cmdfis->lba3 = (uint8_t)(startl>>24);
        cmdfis->lba4 = (uint8_t)starth;
        cmdfis->lba5 = (uint8_t)(starth>>8);
 
        cmdfis->count = count;
 
    //    kprintf("[slot]{%d}", slot);
        port->ci = 1;    // Issue command
    //    kprintf("\n[Port ci ][%d]", port->ci);
   //     kprintf("\nafter issue : %d" , port->tfd);
    //    kprintf("\nafter issue : %d" , port->tfd);

    //    kprintf("\nbefore while 1--> %d", slot); 
        // Wait for completion
        while (1)
        {
                // In some longer duration reads, it may be helpful to spin on the DPS bit 
                // in the PxIS port field as well (1 << 5)
                if ((port->ci & (1<<slot)) == 0) 
                        break;
                if (port->is_rwc & HBA_PxIS_TFES)   // Task file error
                {
                        kprintf("Read disk error\n");
                        return 0;
                }
        }
     //   kprintf("\n after while 1"); 
     //   kprintf("\nafter issue : %d" , port->tfd);
        // Check again
        if (port->is_rwc & HBA_PxIS_TFES)
        {
                kprintf("Read disk error\n");
                return 0;
        }
        
     //   kprintf("\n[Port ci ][%d]", port->ci);
       // int k = 0;
        while(port->ci != 0)
        {
       //     kprintf("[%d]", k++);
        }
        return 1;
}

void ahci_init()
{
	uint64_t address = get_ahci();
	/* Set global abar address  */
	abar = address;
	/* If in problem, try NEW_ABAR NEW_ABAR); */
	probe_port((hba_mem_t *)(0xFFFFFFFF80000000 + address));
}
