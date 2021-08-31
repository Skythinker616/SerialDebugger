#include "Debug.h"
#include <string.h>
#include "usart.h"

//֡ͷ
#define DEBUG_FRAME_HEADER 0xDB
//��ȡ�����ͳ��ȵĺ꣬typeӦ����DebugVarTypeö��ֵ
#define DEBUG_TYPE_LEN(type) (((int[]){1,2,4,1,2,4,4,1})[(type)])

/**********������**********/

//����ע��ı������������ɳ���256
#define DEBUG_MAX_VAR_NUM 20
//���ͻ���������
#define DEBUG_TX_BUF_SIZE 20
//�������ݷ��͵����
#define DEBUG_SEND(buf,len) HAL_UART_Transmit_DMA(&huart1,buf,len)

/**********ȫ�ֱ���**********/

//�����б�
DebugVarInfo varList[DEBUG_MAX_VAR_NUM];
//��ע��ı�������
uint8_t varNum=0;

/**********����ʵ��**********/

//ע��һ������
//����Ϊ(������ַ,��������,��������)
void Debug_RegisterVar(void *addr,char *name,DebugVarType type)
{
	varList[varNum].addr=addr;
	varList[varNum].name=name;
	varList[varNum].type=type;
	varNum++;
}

//������յ���һ֡�ֽ���
void Debug_DecodeFrame(uint8_t *buffer)
{
	if(buffer[0]!=DEBUG_FRAME_HEADER) //֡ͷ����ֱ�ӷ���
		return;
	
	static uint8_t txBuf[DEBUG_TX_BUF_SIZE]; //������
	
	uint8_t cmd=buffer[1]; //������
	
	if(cmd==DCmd_ListLenReq) //�յ��б������󣬷����б���
	{
		txBuf[0]=DEBUG_FRAME_HEADER;
		txBuf[1]=DCmd_ListLenRes;
		txBuf[2]=varNum;
		DEBUG_SEND(txBuf,3);
	}
	else if(cmd==DCmd_VarInfoReq) //�յ�������Ϣ���󣬷��ر�����Ϣ
	{
		uint8_t varIndex=buffer[2]; //����ı�������(varList�е��±�)
		txBuf[0]=DEBUG_FRAME_HEADER;
		txBuf[1]=DCmd_VarInfoRes;
		txBuf[2]=varIndex; //��������
		txBuf[3]=varList[varIndex].type; //��������
		uint8_t nameLen=strlen(varList[varIndex].name); //������
		memcpy(txBuf+4,varList[varIndex].name,nameLen+1);
		DEBUG_SEND(txBuf,4+nameLen+1);
	}
	else if(cmd==DCmd_ReadReq) //�յ��������ݶ�ȡ���󣬷��ر�������
	{
		uint8_t varIndex=buffer[2]; //����ı�������
		txBuf[0]=DEBUG_FRAME_HEADER;
		txBuf[1]=DCmd_ReadRes;
		txBuf[2]=varIndex; //��������
		memcpy(txBuf+3,varList[varIndex].addr,DEBUG_TYPE_LEN(varList[varIndex].type)); //����ַ������ֱ�ӷ���
		DEBUG_SEND(txBuf,3+DEBUG_TYPE_LEN(varList[varIndex].type));
	}
	else if(cmd==DCmd_WriteReq) //�յ�д���������
	{
		uint8_t varIndex=buffer[2]; //����ı�������
		memcpy(varList[varIndex].addr,buffer+3,DEBUG_TYPE_LEN(varList[varIndex].type)); //ֱ�����ַд������
	}
}
