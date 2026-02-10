#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scene.h"
#include "ctrlboard.h"
#include "ite/itp.h"
#include <sys/ioctl.h>
#include <mqueue.h>
#include "scene.h"
#include "ite/itu.h"
#include "ite/ith.h"
#include "capture_handler.h"
#include <sys/time.h>
#include "SDL/SDL.h"
#include "guard.h"
#include "guardVcu.h"
#include "analog.h"
#include "trouble_code.h"
#include "history_trouble_code.h"
// --- SAE J1939 故障码协议解析 ---
// typedef struct 
// {
//     union _trouble_code{
//         struct {		
//             uint16_t SPN : 16; 
//             uint8_t FMI : 5;
//             uint8_t SPN1 : 3;
//             uint8_t OC : 7;
//             uint8_t CM : 1;
//         } bit;
//         uint32_t   code;           //故障码
//         uint8_t byte[4];
//     }trouble_code;
//     uint8_t         valid_flag;       //列表中故障码有效标志
// }trouble_t;
typedef union _FAULTCODE_MSG{
    struct {
		uint8_t Instrument_Total : 8;
		uint8_t Instrument_code : 8;
		uint8_t upgrad_Total : 8;
		uint8_t upgrad_code : 8;
        uint8_t Reserve1 : 8;
        uint8_t Box_Total : 8;
		uint16_t Box_code : 16;
    } bit;
    uint8_t byte[8];
}FAULTCODE_MSG;
// typedef union _VCUFAULT_MSG{
//     struct {
// 		uint8_t Fault_Level : 8;//VCU故障等级
// 		uint8_t Beep_enab : 8; //蜂鸣器使能信号
// 		uint16_t Fault_code : 16; //VCU故障代码
//         uint16_t timeout : 16;//故障持续时间
//     } bit;
//     uint8_t byte[8];
// }VCUFAULT_MSG;


static const char* stringPartArray[] =
{
"VCU",
"BMS",
"TMS",
"DCDC",
"OBC",
"PDU",
"MCU",
"FAN"
};//8

/**********************************************************菜单故障列表****************************************************************/
bool ituScrollListBoxCanFaultcodeOnLoad(ITUWidget* widget, char* param)
{
    ITUScrollListBox* slistbox = (ITUScrollListBox*)widget;
    ITUListBox* listbox = (ITUListBox*)widget;
    char rxd[ITU_ACTION_PARAM_SIZE] = {0};
    char textBuf[1024] = {0};
    ITCTree* node;
    int i,j;
    int entryCount;

    sprintf(rxd, "%s",param);
    ituGetDataSource_ListBox((char*) rxd,0);

    // 1. 自动匹配协议配置
    FaultProtocolConfig* cfg = NULL;    
    for (int i = 0; i < PROTOCOL_COUNT; i++) 
    {
        if (strstr(param, protocolConfigs[i].protocolKey)) 
        {
            cfg = &protocolConfigs[i];
            break;
        }
    }
    if (cfg==NULL) return false;

    // 2. 获取故障总数 (从配置的地址读取位域数值)
    if(cfg->protocolKey == "HISTORYFAULT")
    {//如果显示的是历史故障，显示数量固定为故障总数
        entryCount = cfg->maxCount;
    }
    else
    {//实时故障数量通过CAN信号确认
        entryCount = read(ITP_DEVICE_CANPOOL, cfg->countAddr, 0);
    }
    if (entryCount > cfg->maxCount) entryCount = cfg->maxCount;
    // ithPrintf("entryCount=%d\n",entryCount);

    // 3. 计算分页与节点初始化
    int count = ituScrollListBoxGetItemCount(slistbox);//每页显示条数
    node = ituScrollListBoxGetLastPageItem(slistbox);//获取最后一页的第一个节点
    listbox->pageCount = entryCount ? (entryCount + count - 1) / count : 1;//计算总页数
    if (listbox->pageIndex == 0)//页码从1开始
    {
        listbox->pageIndex = 1;//第一页
		listbox->focusIndex = -1;//没有选中任何项
    }
    if (listbox->pageIndex <= 1)
    {
        for (i = 0; i < count; i++)
        {
            ITUScrollText* scrolltext = (ITUScrollText*) node;//获取节点
            ituScrollTextSetString(scrolltext, "");

            node = node->sibling;
        }
    }

    // 4. 计算当前页起始索引
    i = 0;
    j = count * (listbox->pageIndex - 2);
    if (j < 0)
        j = 0;

    // 5. 数据填充循环
    for (; j < entryCount && node; j++) {
        ITUScrollText* scrolltext = (ITUScrollText*)node;
        // // 打印控件的：名称、坐标(x,y)、可见性(visible)、背景颜色(alpha)
        // ITUWidget* w = (ITUWidget*)node;
        // ithPrintf("Node[%d]: Name=%s, Pos=(%d,%d), Visible=%d, ColorAlpha=%d\n", 
        //         j, w->name, w->rect.x, w->rect.y, ituWidgetIsVisible(w), w->alpha);
        if(cfg->protocolKey == "HISTORYFAULT")
        {
            #ifdef HISTORY_FAULT_ENABLE
                cfg->formatter(textBuf,  &history_type[j], j, param);// 调用对应的格式化函数生成文字
            #endif
        }
        else
        {
            sprintf(rxd, "%s",param);
            ituGetDataSource_ListBox((char* )rxd,j);//获取第j条故障码数据
            cfg->formatter(textBuf, (uint8_t*)rxd, j, param);// 调用对应的格式化函数生成文字
        }
        char* currentStr = ituTextGetString(scrolltext);
        // ithPrintf("j=%d,currentStr=%s,NewText=%s\n",j,currentStr,textBuf);
        if (!currentStr || strcmp(currentStr, textBuf) != 0) //仅当文字改变时才更新，防止滚动效果被重置
        {
            ituTextSetString(scrolltext, textBuf);
            ituWidgetSetCustomData(scrolltext, j);
        }
        i++;
        node = node->sibling;

        if (node == NULL)
            break;
    }

    // 6. 清理多余节点
    for (; node; node = node->sibling) 
    {
        ITUScrollText* scrolltext = (ITUScrollText*) node;
        ituScrollTextSetString((ITUScrollText*)node, "");
    }

    // 7. 更新列表条目计数
    if (listbox->pageIndex == listbox->pageCount)
    {
        if (i == 0)
        {
            listbox->itemCount = i;
        }
        else
        {
            listbox->itemCount = i % count;
            if (listbox->itemCount == 0)
                listbox->itemCount = count;
        }
    }
    else
        listbox->itemCount = count;

    return true;
}




