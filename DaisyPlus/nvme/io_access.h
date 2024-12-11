#ifndef __IO_ACCESS_H_
#define __IO_ACCESS_H_

#define IO_WRITE32(addr, val)		*((volatile unsigned int *)(addr)) = val
#define IO_READ32(addr)				*((volatile unsigned int *)(addr))

#endif	//__IO_ACCESS_H_