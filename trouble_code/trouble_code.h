#ifndef __TROUBLE_CODE_H__
#define __TROUBLE_CODE_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "history_trouble_code.h"

// SAE1939故障码结构体
typedef struct 
{
    union _trouble_code{
        struct {		
            uint16_t SPN : 16; 
            uint8_t FMI : 5;
            uint8_t SPN1 : 3;
            uint8_t OC : 7;
            uint8_t CM : 1;
        } bit;
        uint32_t   code;           //故障码
        uint8_t byte[4];
    }trouble_code;
    uint8_t         valid_flag;       //列表中故障码有效标志
}trouble_t;

//vcu故障码结构体
typedef union _VCUFAULT_MSG{
    struct {
		uint8_t Fault_Level : 8;//VCU故障等级
		uint8_t Beep_enab : 8; //蜂鸣器使能信号
		uint16_t Fault_code : 16; //VCU故障代码
        uint16_t timeout : 16;//故障持续时间
    } bit;
    uint8_t byte[8];
}VCUFAULT_MSG;


// // 模拟数据源读取函数 (根据 param 选择 CAN 设备读取数据)
// static bool ituGetDataSource_ListBox(char* param,int len)
// {
//     if(strstr(param,"CAN0") != NULL)
//     {    
// #ifdef CFG_CAN0_ENABLE        
//         read(ITP_DEVICE_CAN0, param, len );
// #endif        
//     }
//     else if(strstr(param,"CAN1") != NULL)
//     {    
// #ifdef CFG_CAN1_ENABLE        
//         read(ITP_DEVICE_CAN1, param, len );
// #endif
//     }
//     return true;    
// }

// 定义解析函数指针：负责将原始字节转换为显示的字符串
typedef void (*FaultFormatter)(char* dest, uint8_t* rawData, int index, const char* param);

typedef struct {
    const char* protocolKey;   // 匹配 param 中的关键字 (如 "RX18FECA00")
    const char* countAddr;     // 读取故障数量的地址 (如 "CAN_4_8_8")
    int maxCount;              // 最大条目限制
    FaultFormatter formatter;  // 对应的解析函数
} FaultProtocolConfig;

// --- SAE1939 协议解析 (发动机/变速箱) ---
void SAE1939_Formatter(char* dest, uint8_t* raw, int index, const char* param) 
{
    trouble_t t;
    memcpy(t.trouble_code.byte, raw, 4);
    if (strstr(param, "Code"))      sprintf(dest, "%d", index + 1);
    else if (strstr(param, "SPN"))  sprintf(dest, "%d", (t.trouble_code.bit.SPN1 << 16) + t.trouble_code.bit.SPN);
    else if (strstr(param, "FMI"))  sprintf(dest, "%d", t.trouble_code.bit.FMI);
    else if (strstr(param, "OC"))   sprintf(dest, "%d", t.trouble_code.bit.OC);
}

// --- 后提升协议解析 (带字符串查表) ---
void RearLift_Formatter(char* dest, uint8_t* raw, int index, const char* param) 
{
    if (strstr(param, "Code")) sprintf(dest, "%d", index + 1);
    else if (strstr(param, "Part")) 
    {
        char* ptr = (char*)StringGetGuardUpgrad1(raw[3]); // 假设位域在第4字节
        if (ptr) strcpy(dest, ptr);
    }
}

// --- VCU 协议解析 ---
void VCU_Formatter(char* dest, uint8_t* raw, int index, const char* param) 
{
    VCUFAULT_MSG vcu;
    memcpy(vcu.byte, raw, 8); // 拷贝 8 字节原始数据
    if (strstr(param, "Code"))       sprintf(dest, "%d", index + 1);
    else if (strstr(param, "Part"))   sprintf(dest, "%d", vcu.bit.Fault_code);
    else if (strstr(param, "Level"))  sprintf(dest, "%d", vcu.bit.Fault_Level);
    else if (strstr(param, "MainLayerShow"))   sprintf(dest, "LEVEL=%d/CODE=%d", vcu.bit.Fault_Level,vcu.bit.Fault_code);
}

// --- 历史故障解析 ---
void History_Formatter(char* dest, uint8_t* raw ,int index, const char* param) 
{
    uint8_t type=*raw;
    if (strstr(param, "Time"))    
    {
        if(type>=1&&type<=4)
        {
            sprintf(dest, "%04u/%02u/%02u %02u:%02u", (uint16_t)history_engine_time[index*5]+2000, history_engine_time[index*5+1], history_engine_time[index*5+2], history_engine_time[index*5+3], history_engine_time[index*5+4]);
        }
        else
        {
            sprintf(dest, " ");
        }
    }   
    else if(strstr(param, "Type"))
    {
        if(type == 1)
        {
            sprintf(dest, "发动机故障");
        }
        else if(type == 2)
        {
            sprintf(dest, "变速箱故障");
        }
        else if(type == 3)
        {
            sprintf(dest, "后提升故障");
        }
        else if(type == 4)
        {
            sprintf(dest, "VCU故障");
        }
        else 
        {
            sprintf(dest, " ");
        }
    }
    else if (strstr(param, "Meaning")) 
    {
        if(type == 1)
        {
            sprintf(dest, "SPN=%u/FMI=%u",(((history_engine_fault[index*4+2]&0xE0)>>5)<<16)+(history_engine_fault[index*4+1]<<8)+history_engine_fault[index*4],history_engine_fault[index*4+2]&0x1F);
        }
        else if(type == 2)
        {
            sprintf(dest, "SPN=%u/FMI=%u",(((history_engine_fault[index*4+2]&0xE0)>>5)<<16)+(history_engine_fault[index*4+1]<<8)+history_engine_fault[index*4],history_engine_fault[index*4+2]&0x1F);
        }
        else if(type == 3)
        {
            char* ptr = (char*)StringGetGuardUpgrad1(history_engine_fault[index*4]);
            sprintf(dest, "%s", ptr);
        }
        else if(type == 4)
        {
            sprintf(dest, "Level=%u/Code=%u",history_engine_fault[index*4],(history_engine_fault[index*4+2]<<8)+history_engine_fault[index*4+1]);
        }
        else 
        {
            sprintf(dest, " ");
        }
        // sprintf(dest, "0x%02X%02X%02X%02X", theConfig.history_engine_fault[index*4], theConfig.history_engine_fault[index*4+1], theConfig.history_engine_fault[index*4+2], theConfig.history_engine_fault[index*4+3]);
    }
}

// 定义配置表
static FaultProtocolConfig protocolConfigs[] = {
    {"RX18FECA00", "CAN_4_8_8",  50, SAE1939_Formatter},
    {"RX18FECA03", "CAN_4_16_8", 50, SAE1939_Formatter},
    {"FAULTCODE",  "CAN_4_32_8", 50, RearLift_Formatter},
    {"VCUFAULT",   "CAN_4_48_8", 50, VCU_Formatter},
    {"HISTORYFAULT",   " ", 50, History_Formatter}
};
#define PROTOCOL_COUNT (sizeof(protocolConfigs) / sizeof(protocolConfigs[0]))//配置表内的项目数

#endif
