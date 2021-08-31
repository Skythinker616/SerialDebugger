#ifndef DEBUGGER_H
#define DEBUGGER_H

#include <stdint.h>

//帧头
#define DEBUG_FRAME_HEADER 0xDB
//根据类型枚举(DebugVarType)获取长度
#define LenOfDType(type) (((int[]){1,2,4,1,2,4,4,1})[(type)])
//根据类型枚举获取名字
#define NameOfDType(type) (((const char*[]){"Int8","Int16","Int32","Uint8","Uint16","Uint32","Float","Bool"})[(type)])

//命令码
typedef enum{
    DCmd_ListLenReq, //变量列表长度请求
    DCmd_ListLenRes, //变量列表长度反馈
    DCmd_VarInfoReq, //变量信息请求
    DCmd_VarInfoRes, //变量信息反馈
    DCmd_ReadReq, //读取变量请求
    DCmd_ReadRes, //读取变量反馈
    DCmd_WriteReq //写入变量请求
}DebugFrameCmd;

//类型枚举
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

//用于字节流与各类型数据的相互转换
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

#endif // DEBUGGER_H
