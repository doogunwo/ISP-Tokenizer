#ifndef __NVME_IDENTIFY_H_
#define __NVME_IDENTIFY_H_

#define PCI_VENDOR_ID				0x1EDC
#define PCI_SUBSYSTEM_VENDOR_ID		0x1EDC
#define SERIAL_NUMBER				"SSDD515T"
#define MODEL_NUMBER				"DaisyPlus ZNS"
#define FIRMWARE_REVISION			"TYPE0007"

void controller_identification(unsigned int pBuffer);

void namespace_identification(unsigned int pBuffer);

void zns_namespace_identification(unsigned int pBuffer);

void zns_controller_identification(unsigned int pBuffer);

void nvme_identify_cmd_set(unsigned int pBuffer);

#endif	//__NVME_IDENTIFY_H_