#include "xil_printf.h"
#include "debug.h"
#include "io_access.h"

#include "nvme.h"
#include "host_lld.h"
#include "nvme_io_cmd.h"

#include "../ftl_config.h"
#include "../request_transform.h"

#include "zns.h"

static void handle_nvme_io_read(unsigned int cmdSlotTag, NVME_IO_COMMAND *nvmeIOCmd)
{
	IO_READ_COMMAND_DW12 readInfo12;
	//IO_READ_COMMAND_DW13 readInfo13;
	//IO_READ_COMMAND_DW15 readInfo15;
	unsigned int startLba[2];
	unsigned int nlb;
	NVME_COMPLETION nvmeCPL;
    uint16_t status;

	readInfo12.dword = nvmeIOCmd->dword[12];
	//readInfo13.dword = nvmeIOCmd->dword[13];
	//readInfo15.dword = nvmeIOCmd->dword[15];

	startLba[0] = nvmeIOCmd->dword[10];
	startLba[1] = nvmeIOCmd->dword[11];
	nlb = readInfo12.NLB;

	ASSERT(startLba[0] < storageCapacity_L && (startLba[1] < STORAGE_CAPACITY_H || startLba[1] == 0));
	//ASSERT(nlb < MAX_NUM_OF_NLB);
	ASSERT((nvmeIOCmd->PRP1[0] & 0x3) == 0 && (nvmeIOCmd->PRP2[0] & 0x3) == 0); //error
	ASSERT(nvmeIOCmd->PRP1[1] < 0x10000 && nvmeIOCmd->PRP2[1] < 0x10000);

    status = nvme_read(startLba[0], nlb);
    if(status)
    {
    	nvmeCPL.statusFieldWord = 0;
    	nvmeCPL.statusField.SCT = SCT_GENERIC_COMMAND_STATUS;
    	nvmeCPL.statusField.SC = status;
        nvmeCPL.specific = 0x0;
        set_auto_nvme_cpl(cmdSlotTag, nvmeCPL.specific, nvmeCPL.statusFieldWord);
    }
    else
    {
    	ReqTransNvmeToSlice(cmdSlotTag, startLba[0], nlb, IO_NVM_READ, 0);
    }
}


static void handle_nvme_io_write(unsigned int cmdSlotTag, NVME_IO_COMMAND *nvmeIOCmd)
{
	IO_READ_COMMAND_DW12 writeInfo12;
	//IO_READ_COMMAND_DW13 writeInfo13;
	//IO_READ_COMMAND_DW15 writeInfo15;
	unsigned int startLba[2];
	unsigned int nlb;
	NVME_COMPLETION nvmeCPL;
    uint16_t status;

	writeInfo12.dword = nvmeIOCmd->dword[12];
	//writeInfo13.dword = nvmeIOCmd->dword[13];
	//writeInfo15.dword = nvmeIOCmd->dword[15];

	//if(writeInfo12.FUA == 1)
	//	xil_printf("write FUA\r\n");

	startLba[0] = nvmeIOCmd->dword[10];
	startLba[1] = nvmeIOCmd->dword[11];
	nlb = writeInfo12.NLB;

	ASSERT(startLba[0] < storageCapacity_L && (startLba[1] < STORAGE_CAPACITY_H || startLba[1] == 0));
	//ASSERT(nlb < MAX_NUM_OF_NLB);
	ASSERT((nvmeIOCmd->PRP1[0] & 0xF) == 0 && (nvmeIOCmd->PRP2[0] & 0xF) == 0);
	ASSERT(nvmeIOCmd->PRP1[1] < 0x10000 && nvmeIOCmd->PRP2[1] < 0x10000);

    status = nvme_do_write(&startLba[0], nlb, 0, NULL);
    if(status)
    {
    	nvmeCPL.statusFieldWord = 0;
    	nvmeCPL.statusField.SCT = SCT_GENERIC_COMMAND_STATUS;
    	nvmeCPL.statusField.SC = status;
        nvmeCPL.specific = 0x0;
        set_auto_nvme_cpl(cmdSlotTag, nvmeCPL.specific, nvmeCPL.statusFieldWord);
    }
    else
    {
    	ReqTransNvmeToSlice(cmdSlotTag, startLba[0], nlb, IO_NVM_WRITE, 0);
    }
}

static void handle_nvme_io_zone_append(unsigned int cmdSlotTag, NVME_IO_COMMAND *nvmeIOCmd)
{
	IO_READ_COMMAND_DW12 writeInfo12;
	//IO_READ_COMMAND_DW13 writeInfo13;
	//IO_READ_COMMAND_DW15 writeInfo15;
	unsigned int startLba[2];
	unsigned int nlb;
	NVME_COMPLETION nvmeCPL;
    uint16_t status;
	unsigned int w_ptr;

	writeInfo12.dword = nvmeIOCmd->dword[12];
	//writeInfo13.dword = nvmeIOCmd->dword[13];
	//writeInfo15.dword = nvmeIOCmd->dword[15];

	//if(writeInfo12.FUA == 1)
	//	xil_printf("write FUA\r\n");

	startLba[0] = nvmeIOCmd->dword[10];
	startLba[1] = nvmeIOCmd->dword[11];
	nlb = writeInfo12.NLB;

	ASSERT(startLba[0] < storageCapacity_L && (startLba[1] < STORAGE_CAPACITY_H || startLba[1] == 0));
	//ASSERT(nlb < MAX_NUM_OF_NLB);
	ASSERT((nvmeIOCmd->PRP1[0] & 0xF) == 0 && (nvmeIOCmd->PRP2[0] & 0xF) == 0);
	ASSERT(nvmeIOCmd->PRP1[1] < 0x10000 && nvmeIOCmd->PRP2[1] < 0x10000);

    status = nvme_do_write(&startLba[0], nlb, 1, &w_ptr);
    if(status)
    {
    	nvmeCPL.statusFieldWord = 0;
    	nvmeCPL.statusField.SCT = SCT_GENERIC_COMMAND_STATUS;
    	nvmeCPL.statusField.SC = status;
        nvmeCPL.specific = 0x0;
        set_auto_nvme_cpl(cmdSlotTag, nvmeCPL.specific, nvmeCPL.statusFieldWord);
    }
    else
    {
    	ReqTransNvmeToSlice(cmdSlotTag, startLba[0], nlb, IO_NVM_WRITE, w_ptr);
    }
}

static void handle_nvme_io_zone_mgmt_send(unsigned int cmdSlotTag, NVME_IO_COMMAND *nvmeIOCmd)
{
    char send_cpl = 1;
    uint16_t status = nvme_zone_mgmt_send(cmdSlotTag, (void *)nvmeIOCmd, &send_cpl);
    NVME_COMPLETION nvmeCPL;

    if(send_cpl)
    {
    	nvmeCPL.statusFieldWord = 0;
    	nvmeCPL.statusField.SCT = SCT_GENERIC_COMMAND_STATUS;
    	nvmeCPL.statusField.SC = status;
    	set_auto_nvme_cpl(cmdSlotTag, 0, nvmeCPL.statusFieldWord);
    }
}

static void handle_nvme_io_zone_mgmt_recv(unsigned int cmdSlotTag, NVME_IO_COMMAND *nvmeIOCmd)
{
    unsigned int pZoneManagementData = ZONE_MANAGEMENT_RECV_ADDR;
    unsigned int data_size = 0;
    uint16_t status = nvme_zone_mgmt_recv(pZoneManagementData, (void *)nvmeIOCmd, &data_size);
	unsigned int prp[2];
    NVME_COMPLETION nvmeCPL;

    prp[0] = nvmeIOCmd->PRP1[0];
    prp[1] = nvmeIOCmd->PRP1[1];

//  xil_printf("prpLen = %X, prp[1] = %X, prp[0] = %X\r\n",prpLen, prp[1], prp[0]);
    set_direct_tx_dma(pZoneManagementData, prp[1], prp[0], data_size);
    check_direct_tx_dma_done();

    nvmeCPL.statusFieldWord = 0;
    nvmeCPL.statusField.SCT = SCT_GENERIC_COMMAND_STATUS;
    nvmeCPL.statusField.SC = status;

	set_auto_nvme_cpl(cmdSlotTag, 0, nvmeCPL.statusFieldWord);
}

void handle_nvme_io_cmd(NVME_COMMAND *nvmeCmd)
{
	NVME_IO_COMMAND *nvmeIOCmd;
	NVME_COMPLETION nvmeCPL;
	unsigned int opc;

	nvmeIOCmd = (NVME_IO_COMMAND*)nvmeCmd->cmdDword;

	opc = (unsigned int)nvmeIOCmd->OPC;

	switch(opc)
	{
		case IO_NVM_FLUSH:
		{
			PRINT("IO Flush Command\r\n");
			nvmeCPL.dword[0] = 0;
			nvmeCPL.specific = 0x0;
			set_auto_nvme_cpl(nvmeCmd->cmdSlotTag, nvmeCPL.specific, nvmeCPL.statusFieldWord);
			break;
		}
		case IO_NVM_WRITE:
		{
			PRINT("IO Write Command\r\n");
			handle_nvme_io_write(nvmeCmd->cmdSlotTag, nvmeIOCmd);
			break;
		}
		case IO_NVM_READ:
		{
			PRINT("IO Read Command\r\n");
			handle_nvme_io_read(nvmeCmd->cmdSlotTag, nvmeIOCmd);
			break;
		}
		case IO_NVM_ZONE_MGMT_SEND:
		{
			PRINT("IO Zone Management Send Command\r\n");
			handle_nvme_io_zone_mgmt_send(nvmeCmd->cmdSlotTag, nvmeIOCmd);
			break;
		}
		case IO_NVM_ZONE_MGMT_RECV:
		{
			PRINT("IO Zone Management Receive Command\r\n");
			handle_nvme_io_zone_mgmt_recv(nvmeCmd->cmdSlotTag, nvmeIOCmd);
			break;
		}
		case IO_NVM_ZONE_APPEND:
		{
			PRINT("IO Zone Append Command\r\n");
			handle_nvme_io_zone_append(nvmeCmd->cmdSlotTag, nvmeIOCmd);
			break;
		}
		case IO_NVM_WRITE_ZEROS:
		{
			PRINT("IO Write Zeros Command\r\n");
			nvmeCPL.dword[0] = 0;
			nvmeCPL.specific = 0x0;
			set_auto_nvme_cpl(nvmeCmd->cmdSlotTag, nvmeCPL.specific, nvmeCPL.statusFieldWord);
			break;
		}

		default:
		{
			xil_printf("Not Support IO Command OPC: 0x%X\r\n", opc);
			ASSERT(0);
			break;
		}
	}

#if (__IO_CMD_DONE_MESSAGE_PRINT)
    xil_printf("OPC = 0x%X\r\n", nvmeIOCmd->OPC);
    xil_printf("PRP1[63:32] = 0x%X, PRP1[31:0] = 0x%X\r\n", nvmeIOCmd->PRP1[1], nvmeIOCmd->PRP1[0]);
    xil_printf("PRP2[63:32] = 0x%X, PRP2[31:0] = 0x%X\r\n", nvmeIOCmd->PRP2[1], nvmeIOCmd->PRP2[0]);
    xil_printf("dword10 = 0x%X\r\n", nvmeIOCmd->dword10);
    xil_printf("dword11 = 0x%X\r\n", nvmeIOCmd->dword11);
    xil_printf("dword12 = 0x%X\r\n", nvmeIOCmd->dword12);
#endif
}