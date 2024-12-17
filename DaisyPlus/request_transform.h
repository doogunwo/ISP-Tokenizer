#ifndef REQUEST_TRANSFORM_H_
#define REQUEST_TRANSFORM_H_

#include "ftl_config.h"
#include "nvme/nvme.h"

#define NVME_COMMAND_AUTO_COMPLETION_OFF	0
#define NVME_COMMAND_AUTO_COMPLETION_ON		1

#define BUF_DEPENDENCY_REPORT_BLOCKED		0
#define BUF_DEPENDENCY_REPORT_PASS			1

void ReqTransNvmeToSlice(unsigned int cmdSlotTag, unsigned int startLba, unsigned int nlb, unsigned int cmdCode, unsigned int w_ptr);
void ReqTransSliceToLowLevel();
void IssueNvmeDmaReq(unsigned int reqSlotTag);
void CheckDoneNvmeDmaReq();

void SelectLowLevelReqQ(unsigned int reqSlotTag);
void ReleaseBlockedByBufDepReq(unsigned int reqSlotTag);

#endif /* REQUEST_TRANSFORM_H_ */