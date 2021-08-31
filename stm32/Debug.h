#ifndef _DEBUG_H_
#define _DEBUG_H_

#include <stdint.h>
#include <stdbool.h>

//�����룬����ָ������֡������
typedef enum{
	DCmd_ListLenReq, //�����б�������
	DCmd_ListLenRes, //�����б��ȷ���
	DCmd_VarInfoReq, //������Ϣ����
	DCmd_VarInfoRes, //������Ϣ����
	DCmd_ReadReq, //��ȡ��������
	DCmd_ReadRes, //��ȡ��������
	DCmd_WriteReq //д���������
}DebugFrameCmd;

//����ö��
typedef enum{
	DVar_Int8,
	DVar_Int16,
	DVar_Int32,
	DVar_Uint8,
	DVar_Uint16,
	DVar_Uint32,
	DVar_Float,
	DVar_Bool
}DebugVarType;

//�����ֽ�������������ݵ��໥ת��
typedef union{
	uint8_t bytes[4];
	int8_t varInt8;
	int16_t varInt16;
	int32_t varInt32;
	uint8_t varUint8;
	uint16_t varUint16;
	uint32_t varUint32;
	float varFloat;
	bool varBool;
}DebugVarTrans;

//������Ϣ
typedef struct{
	void *addr;
	char *name;
	DebugVarType type;
}DebugVarInfo;

//ע��һ������
void Debug_RegisterVar(void *addr,char *name,DebugVarType type);
//�����յ�������֡
void Debug_DecodeFrame(uint8_t *buffer);

#endif
