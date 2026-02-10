#ifndef HISTORY_TROUBLE_CODE_H
#define HISTORY_TROUBLE_CODE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fmmem.h"
#include <sys/time.h>

// /*历史故障记录区0x800-0xC00*/
// #define HISTORY_FAULT_FIRST (0X800)
// #define HISTORY_FAULT_MAX (0XC00)

// #define History_Fault_Addr (HISTORY_FAULT_FIRST)//故障码
// #define History_Fault_Num   (450u)

// #define History_Fault_Type_Addr (History_Fault_Addr + History_Fault_Num)//故障类型
// #define History_Fault_Type_Num   (50u)

// #define History_Offset_Addr (History_Fault_Type_Addr + History_Fault_Type_Num)//故障偏移
// #define History_Offset_Num (1u)

// #define    HISTORY_FAULT_END  (History_Offset_Addr + History_Offset_Num)
// #if(HISTORY_FAULT_END >= HISTORY_FAULT_MAX)
//     #error "HISTORY_FAULT_END OVER!"
// #endif
#define HISTORY_FAULT_ENABLE    //表示历史故障存在，方便区分需要历史故障和不需要历史故障的情况

uint8_t history_engine_fault[200];  //历史故障码内存数组
uint8_t history_engine_time[250];   //历史时间内存数组
uint8_t history_type[50];           //历史故障类型内存数组

// 分区数量定义
typedef enum {
    PART_ENGINE = 0,
    PART_GEARBOX,
    PART_UPGRADE1,
    PART_VCU,
    PART_MAX_COUNT
} FaultPartition_t;

// 每个分区的配置信息
typedef struct {
    uint32_t startAddr;  // 物理起始地址
    uint8_t OffsetAddr;  // 存放偏移量的地址
    uint8_t iftime;     // 是否带时间戳, 1-带时间戳，0-不带时间戳
    uint8_t itemSize;    // 每条记录的字节数(不包含时间戳)
    uint16_t maxCount;   // 最大存储条数
} PartitionConfig_t;

// 静态配置表 (根据您的要求计算)
static const PartitionConfig_t g_PartConfigs[PART_MAX_COUNT] = {
    { History_Fault_Addr,History_Fault_Num,1,4,50 },    // Engine
    { History_Fault_Addr,History_Fault_Num,1,4,50 },  // Gearbox (假设 itemSize 为 16)
    { History_Fault_Addr,History_Fault_Num,1,4,50 },  // Upgrad1
    { History_Fault_Addr,History_Fault_Num,1,4,50 }           // VCU
};

//#define   CL16        //有 256 字节页大小限制的 EEPROM
#ifdef CL16
static uint8_t IIC_readNEW_(uint16_t addr, uint8_t n, uint8_t *pr)
{
    //int fd = open(":eprom", O_RDWR);
	uint8_t recvBuffer[255];
  int n_;
  uint16_t addr_,i,j;

  if(addr/256 == (addr + n)/256)//读取一页必须在256的倍数之内
  {
    recvBuffer[0] = addr>>8;
    recvBuffer[1] = addr;
    read(ITP_DEVICE_EPROM, &recvBuffer, n+2 );
    for (i = 0; i < n; i++)
    {
      *(pr+i) = recvBuffer[i];
    }
  }
  else
  {
    //当前页
    n_ = (addr/256+1)*256-addr;
    if(n_< 0)
    {
      return 0;
    }
    recvBuffer[0] = addr>>8;
    recvBuffer[1] = addr;
    read(ITP_DEVICE_EPROM, &recvBuffer, n_+2 );
    for (i = 0; i < n_; i++)
    {
      *(pr+i) = recvBuffer[i];
    }
    //后一页
    addr_ = (addr/256+1)*256;
    n_ = n-n_;
    if(n_< 0)
    {
      return 0;
    }
    recvBuffer[0] = addr_>>8;
    recvBuffer[1] = addr_;
    read(ITP_DEVICE_EPROM, &recvBuffer, n_+2 );
    for (j = 0; j < n_; j++)
    {
      *(pr+i+j) = recvBuffer[j];
    }
  }
    //close(fd);
    return 1;
}

static uint8_t IIC_saveNEW_(uint16_t addr, uint8_t n, uint8_t *pr)
{
	uint8_t recvBuffer[255] = {0};
  //int fd = open(":eprom", O_RDWR);
  uint8_t n_;
  uint16_t addr_,i,j;
  if(addr/256 == (addr + n)/256)//读取一页必须在256的倍数之内
  {
    for (i = 0; i < n; i++)
    {
        recvBuffer[i+2] = *(pr+i);
    }
    recvBuffer[0] = addr>>8;
    recvBuffer[1] = addr;     
    write(ITP_DEVICE_EPROM, &recvBuffer, n);
  }
  else
  {
    //当前页
    n_ = (addr/256+1)*256-addr;
    if(n_< 0)
    {
      return 0;
    }
    for (i = 0; i < n_; i++)
    {
        recvBuffer[i+2] = *(pr+i);
    }
    recvBuffer[0] = addr>>8;
    recvBuffer[1] = addr;     
    write(ITP_DEVICE_EPROM, &recvBuffer, n_);
    //后一页
    addr_ = (addr/256+1)*256;
    n_ = n-n_;
    if(n_< 0)
    {
      return 0;
    }
    for (j = 0; j < n_; j++)
    {
        recvBuffer[j+2] = *(pr+j+i);
    }
    recvBuffer[0] = addr_>>8;
    recvBuffer[1] = addr_;     
    write(ITP_DEVICE_EPROM, &recvBuffer, n_);
  }
    //close(fd);
    return 1;
}
#else
static uint8_t IIC_readNEW_(uint16_t addr, uint8_t n, uint8_t *pr)
{
    //int fd = open(":eprom", O_RDWR);
	uint8_t recvBuffer[255];
  uint8_t state = 0;
	recvBuffer[0] = addr>>8;
	recvBuffer[1] = addr;
	state = read(ITP_DEVICE_EPROM, &recvBuffer, n+2 );
  //printf("state=%d\n", state);
	for (size_t i = 0; i < n; i++)
	{
		*(pr+i) = recvBuffer[i];
	}
    //close(fd);
    return state;
}

static uint8_t IIC_saveNEW_(uint16_t addr, uint8_t n, uint8_t *pr)
{
	uint8_t recvBuffer[255] = {0};
  uint8_t state = 0;
    //int fd = open(":eprom", O_RDWR);
    for (size_t i = 0; i < n; i++)
    {
        recvBuffer[i+2] = *(pr+i);
    }
	recvBuffer[0] = addr>>8;
	recvBuffer[1] = addr;     
    state = write(ITP_DEVICE_EPROM, &recvBuffer, n);
    //printf("state=%d\n", state);
    //close(fd);
    return state;
}
// #define ErrorLog_MSG_MAX1 250    //单次 I2C 操作允许的最大数据长度
// uint8_t IIC_readNEW_(uint16_t addr, uint8_t n, uint8_t *pr)
// {
// #ifdef CFG_EPROM_ENABLE    
// 	// int fd = open(":eprom", O_RDWR);
// 	uint8_t recvBuffer[ErrorLog_MSG_MAX1+2];
//     if (n > ErrorLog_MSG_MAX1)
//     {
//         return 0;
//     }
    
// 	recvBuffer[0] = addr>>8;
// 	recvBuffer[1] = addr;
// 	read(ITP_DEVICE_EPROM, &recvBuffer, n+2 );
// 	for (size_t i = 0; i < n; i++)
// 	{
// 		*pr++ = recvBuffer[i];
// 	}
//     // for (size_t i = 0; i < (n); i++)
//     // {
//     //     printf("recvBuffer:%d\n",recvBuffer[i]);
//     // }
//     // close(fd);
//     return 1; 
// #endif      
// }
// uint8_t IIC_saveNEW_(uint16_t addr, uint8_t n, uint8_t *pr)
// {
// #ifdef CFG_EPROM_ENABLE    
// 	uint8_t		recvBuffer[ErrorLog_MSG_MAX1+2] = {0};
//     if (n > ErrorLog_MSG_MAX1)
//     {
//         return 0;
//     }    
//     // int fd = open(":eprom", O_RDWR);
//     for (size_t i = 0; i < n; i++)
//     {
//         recvBuffer[i+2] = *pr++;
//         // printf("recvBuffer:%d\n",recvBuffer[i+2]);
//     }
// 	recvBuffer[0] = addr>>8;
// 	recvBuffer[1] = addr;   
//     // for (size_t i = 0; i < (n+2); i++)
//     // {
//     //     printf("recvBuffer:%d\n",recvBuffer[i]);
//     // }    
//     write(ITP_DEVICE_EPROM, &recvBuffer, n );
//     // close(fd); 
//     return 0;
// #endif    
//}
#endif

#define ErrorLog_MSG_MAX 8  
static uint8_t IIC_readNEW(uint16_t addr, uint8_t n, uint8_t *pr, uint8_t chip)// 最大读写字节数 8
{
#ifdef CFG_EPROM_ENABLE    
	// int fd = open(":eprom", O_RDWR);
	uint8_t recvBuffer[ErrorLog_MSG_MAX+2];
    if (n > ErrorLog_MSG_MAX)
    {
        return 0;
    }
    
	recvBuffer[0] = addr>>8;
	recvBuffer[1] = addr;
	read(ITP_DEVICE_EPROM, &recvBuffer, n+2 );
	for (size_t i = 0; i < n; i++)
	{
		*pr++ = recvBuffer[i];
	}
    // for (size_t i = 0; i < (n); i++)
    // {
    //     printf("recvBuffer:%d\n",recvBuffer[i]);
    // }
    // close(fd);
    return 1; 
#endif      
}
static uint8_t IIC_saveNEW(uint16_t addr, uint8_t n, uint8_t *pr, uint8_t chip)// 最大读写字节数 8
{
#ifdef CFG_EPROM_ENABLE    
	uint8_t		recvBuffer[ErrorLog_MSG_MAX+2] = {0};
    if (n > ErrorLog_MSG_MAX)
    {
        return 0;
    }    
    // int fd = open(":eprom", O_RDWR);
    for (size_t i = 0; i < n; i++)
    {
        recvBuffer[i+2] = *pr++;
        // printf("recvBuffer:%d\n",recvBuffer[i+2]);
    }
	recvBuffer[0] = addr>>8;
	recvBuffer[1] = addr;   
    // for (size_t i = 0; i < (n+2); i++)
    // {
    //     printf("recvBuffer:%d\n",recvBuffer[i]);
    // }    
    write(ITP_DEVICE_EPROM, &recvBuffer, n );
    // close(fd); 
    return 0;
#endif    
}


/**
 * @param faultpart: 故障分区 (PART_ENGINE, PART_GEARBOX, etc.)
 * @param data: 要存储的数据指针
 */
static void History_WriteData_Generic(uint8_t faultpart,uint8_t* data,uint8_t type) 
{
    uint8_t tempBuffer[32] = {0};       // 临时缓存，最大支持 32 字节一条
    uint8_t tempOffset=0;               //偏移量缓存
    uint8_t steplen;            //步长等于每条记录的字节数
    IIC_readNEW_(g_PartConfigs[faultpart].OffsetAddr,1,(uint8_t*)&tempOffset);
    ithPrintf("[WRITE] Part:%d, ReadOffset:%d from Addr:0x%X\n", 
              faultpart, tempOffset, g_PartConfigs[faultpart].OffsetAddr);
    // uint16_t actualItemSize = withTimestamp ? (dataSize + 4) : dataSize;

    /*1-组装数据包*/
    int timeoffset = 0;
    if (g_PartConfigs[faultpart].iftime)
    {//若有时间戳
        struct timeval tv;
        struct tm *tm;
        gettimeofday(&tv, NULL);
        tm = localtime(&tv.tv_sec);//转化当前时间
        uint8_t ts[5]; //时间缓冲区
        ts[0] = (uint8_t)(tm->tm_year%100); // 年份偏移
        ts[1] = (uint8_t)(tm->tm_mon + 1);            // 月份
        ts[2] = (uint8_t)tm->tm_mday;               // 日期
        ts[3] = (uint8_t)tm->tm_hour;               // 小时
        ts[4] = (uint8_t)tm->tm_min;                // 分钟
        memcpy(tempBuffer, &ts, 5);                 //计算时间戳并放入缓冲区
        timeoffset = 5;     //时间戳偏移，前5个字节用于存放时间戳
        steplen = g_PartConfigs[faultpart].itemSize +5; //步长等于每条记录的字节数加上时间戳5字节
        // ithPrintf("[WRITE] Timestamp generated: %u/%u/%u %u:%u\n", ts[0], ts[1], ts[2], ts[3], ts[4]);
    }
    else
    {//若无时间戳
        timeoffset = 0;
        steplen = g_PartConfigs[faultpart].itemSize;    //步长等于每条记录的字节数
    }
    memcpy(tempBuffer + timeoffset, data, g_PartConfigs[faultpart].itemSize);   //数据放入缓冲区

    /*2-计算物理地址   地址 = 起始地址 + (偏移 * 步长)*/
    uint32_t writeAddr = g_PartConfigs[faultpart].startAddr + (tempOffset * steplen);
    // ithPrintf("[WRITE] Target physical Addr:0x%X, Steplen:%d\n", writeAddr, steplen);
    // for(uint8_t i = 0; i < steplen; i++) {
        // ithPrintf("  Buf[%d]: 0x%02X ", i, tempBuffer[i]);
    // }
    // ithPrintf("\n");
    /*3-写入数据和偏移*/
    IIC_saveNEW_(writeAddr, steplen, tempBuffer);                             //---写入时间戳和数据d
    // usleep(5000); 
    if(type!=0)
    {
        writeAddr = History_Fault_Type_Addr+tempOffset;
        IIC_saveNEW_(writeAddr, 1, &type);  //写入故障类型
    }
    // usleep(5000); 
    // ithPrintf("[WRITE] Writing Fault Type: %d at Addr:0x%X\n", type, writeAddr);
    tempOffset = (tempOffset + 1) % g_PartConfigs[faultpart].maxCount;              //更新偏移量（环形）
    // ithPrintf("[WRITE] Updating Offset to:%d\n", tempOffset);
    IIC_saveNEW_(g_PartConfigs[faultpart].OffsetAddr, 1, (uint8_t*)&tempOffset);     //---写入偏移量
    // usleep(5000); 
}

/**
 * @param faultpart: 故障分区 (PART_ENGINE, PART_GEARBOX, etc.)
 * @param DataBuffer: 读取到的故障内容（全部）存放在这里
 * @param timestampBuffer: 读取到的时间戳（全部）存放在这里,若无时间戳，实参给NULL即可
 */
static void History_ReadData_Generic(uint8_t faultpart, uint8_t* DataBuffer,uint8_t* timestampBuffer,uint8_t* type) 
{
    uint8_t tempOffset=0;               //偏移量缓存
    uint8_t steplen;            //步长等于每条记录的字节数
    uint8_t currentIndex;      //用于遍历整个故障列表
    uint32_t readAddr;      //物理读取地址
    uint32_t readTypeAddr;      //物理读取地址
    
    if (g_PartConfigs[faultpart].iftime)
    {//若有时间戳
        steplen = g_PartConfigs[faultpart].itemSize +5; //步长等于每条记录的字节数加上时间5字节4
    }
    else
    {//若无时间戳
        steplen = g_PartConfigs[faultpart].itemSize;    //步长等于每条记录的字节数
    }
    IIC_readNEW_(g_PartConfigs[faultpart].OffsetAddr,1,(uint8_t*)&tempOffset);//读取偏移量
    // ithPrintf("[READ] Part:%d, Current Store Offset:%d\n", faultpart, tempOffset);
    uint8_t lastIndex = (tempOffset ==0)?(g_PartConfigs[faultpart].maxCount -1):(tempOffset -1);    //计算偏移
    // ithPrintf("[READ] Starting from lastIndex:%d (most recent)\n", lastIndex);

    currentIndex = lastIndex;
    for(uint8_t i=0;i<g_PartConfigs[faultpart].maxCount;i++)
    {
        /*2-计算物理地址   地址 = 起始地址 + (偏移 * 步长)*/
        readAddr = g_PartConfigs[faultpart].startAddr + (currentIndex * steplen);
        // ithPrintf("[READ] Loop:%d, Index:%d, Addr:0x%X\n", i, currentIndex, readAddr);
        if (g_PartConfigs[faultpart].iftime && timestampBuffer != NULL)
        {//若有时间戳
            IIC_readNEW_(readAddr,5,timestampBuffer + (i * 5)); //读取时间戳到timestampBuffer中
            // ithPrintf("  Time: 0x%08X ", *(uint32_t*)(timestampBuffer + (i * 5)));
            readAddr +=5; //地址偏移5字节，指向数据部分
        }
        IIC_readNEW_(readAddr, g_PartConfigs[faultpart].itemSize, &DataBuffer[i * g_PartConfigs[faultpart].itemSize]); //读取数据和时间戳到DataBuffer中
        // ithPrintf("  Data[0-1]: 0x%02X 0x%02X\n", 
                        // DataBuffer[i * g_PartConfigs[faultpart].itemSize], 
                        // DataBuffer[i * g_PartConfigs[faultpart].itemSize + 1]);     
        if(type!=0)
        {
            readTypeAddr=History_Fault_Type_Addr+currentIndex;      //物理读取地址
            IIC_readNEW_(readTypeAddr, 1, &type[i]); //读取故障类型
        }
        // ithPrintf("  Fault Type: %d at Addr:0x%X\n", type[i], readTypeAddr);
        
        if(currentIndex==0)
        {
            currentIndex = g_PartConfigs[faultpart].maxCount -1;
        }
        else
        {
            currentIndex--;
        }
    }
}



#endif



