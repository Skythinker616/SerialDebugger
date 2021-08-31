#include "Debug.h"
#include <string.h>
#include "usart.h"

//帧头
#define DEBUG_FRAME_HEADER 0xDB
//获取各类型长度的宏，type应传入DebugVarType枚举值
#define DEBUG_TYPE_LEN(type) (((int[]){1,2,4,1,2,4,4,1})[(type)])

/**********配置区**********/

//最大可注册的变量个数，不可超过256
#define DEBUG_MAX_VAR_NUM 20
//发送缓冲区长度
#define DEBUG_TX_BUF_SIZE 20
//用于数据发送的语句
#define DEBUG_SEND(buf,len) HAL_UART_Transmit_DMA(&huart1,buf,len)

/**********全局变量**********/

//变量列表
DebugVarInfo varList[DEBUG_MAX_VAR_NUM];
//已注册的变量个数
uint8_t varNum=0;

/**********函数实现**********/

//注册一个变量
//参数为(变量地址,变量名字,变量类型)
void Debug_RegisterVar(void *addr,char *name,DebugVarType type)
{
	varList[varNum].addr=addr;
	varList[varNum].name=name;
	varList[varNum].type=type;
	varNum++;
}

//解码接收到的一帧字节流
void Debug_DecodeFrame(uint8_t *buffer)
{
	if(buffer[0]!=DEBUG_FRAME_HEADER) //帧头不对直接返回
		return;
	
	static uint8_t txBuf[DEBUG_TX_BUF_SIZE]; //发送区
	
	uint8_t cmd=buffer[1]; //命令码
	
	if(cmd==DCmd_ListLenReq) //收到列表长度请求，返回列表长度
	{
		txBuf[0]=DEBUG_FRAME_HEADER;
		txBuf[1]=DCmd_ListLenRes;
		txBuf[2]=varNum;
		DEBUG_SEND(txBuf,3);
	}
	else if(cmd==DCmd_VarInfoReq) //收到变量信息请求，返回变量信息
	{
		uint8_t varIndex=buffer[2]; //请求的变量索引(varList中的下标)
		txBuf[0]=DEBUG_FRAME_HEADER;
		txBuf[1]=DCmd_VarInfoRes;
		txBuf[2]=varIndex; //变量索引
		txBuf[3]=varList[varIndex].type; //变量类型
		uint8_t nameLen=strlen(varList[varIndex].name); //变量名
		memcpy(txBuf+4,varList[varIndex].name,nameLen+1);
		DEBUG_SEND(txBuf,4+nameLen+1);
	}
	else if(cmd==DCmd_ReadReq) //收到变量数据读取请求，返回变量数据
	{
		uint8_t varIndex=buffer[2]; //请求的变量索引
		txBuf[0]=DEBUG_FRAME_HEADER;
		txBuf[1]=DCmd_ReadRes;
		txBuf[2]=varIndex; //变量索引
		memcpy(txBuf+3,varList[varIndex].addr,DEBUG_TYPE_LEN(varList[varIndex].type)); //将地址中数据直接发送
		DEBUG_SEND(txBuf,3+DEBUG_TYPE_LEN(varList[varIndex].type));
	}
	else if(cmd==DCmd_WriteReq) //收到写入变量请求
	{
		uint8_t varIndex=buffer[2]; //请求的变量索引
		memcpy(varList[varIndex].addr,buffer+3,DEBUG_TYPE_LEN(varList[varIndex].type)); //直接向地址写入数据
	}
}
