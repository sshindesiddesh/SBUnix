#include <sys/defs.h>
#include <sys/gdt.h>
#include <sys/kprintf.h>
#include <sys/console.h>
#include <sys/tarfs.h>
#include <sys/ahci.h>

#define INTEL				0x8086
#define AHCI_CONTROLLER		0x2922
#define MSD					0x01
#define SATA				0x06

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

#define get_pci_data(val, offset)	(val >> (offset * 8))

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
	return (sys_in_long(0xCFC) & 0xFFFFFFFF);
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
					return address;
				}
				kprintf("Vendor 0x%x Device 0x%x class 0x%x sub class 0x%x\n", vendor_id, device_id, class_code, sub_class_code);
			}
		}
	}
	return 0;
}

void ahci_init()
{
	kprintf("\nAHCI : \n");
	get_ahci();
}
