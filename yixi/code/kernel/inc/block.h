#ifndef __BLOCK_H__
#define __BLOCK_H__

struct BolckDeviceOperation
{
    long (* m_open)();
    long (* m_close)();
    long (* m_ioctl)(long cmd, long arg);
    long (* m_transfer)(long cmd, unsigned long blocks, long count, unsigned char* buffer);
};

#endif