# SerialDebugger串口调试系统

---

## 项目简介

本项目实现了使用串口进行下位机程序的调试，可以实现变量的实时查看、修改和变化曲线绘制

上位机采用`QT 5.9.9`编写，下位机使用C语言编写

可以调试的下位机变量需为全局或静态变量，目前支持的数据类型：

* `int8_t uint8_t`

* `int16_t uint16_t`

* `int32_t uint32_t`

* `float`

* `bool`

---

## 使用方法

### 下位机

1. 代码工程包含`Debug.c`和`Debug.h`两个文件

2. 配置`DEBUG_MAX_VAR_NUM`、`DEBUG_TX_BUF_SIZE`、`DEBUG_SEND`三个宏定义

3. 在程序中调用`Debug_RegisterVar()`注册需要进行调试的变量，然后在收到一个数据帧后传入`Debug_DecodeFrame()`进行解析

* 详见[下位机介绍](#下位机介绍)

### 上位机

1. 先运行下位机，然后在上位机中使用串口与下位机连接（先点击`刷新串口`，选择串口后点击`打开串口`）

> 注：上位机串口配置为【115200波特率 8数据位 1停止位 无校验位】

2. 点击`加载变量列表`，上位机就会读取下位机中已经注册的变量信息

![加载变量列表](/readme_img/get_list.gif)

3. 点击`开始调试`，上位机开始定时查询变量值，在右侧的选框中可以选择查询频率（若发现软件闪退可以尝试调小频率）

![查询变量](/readme_img/read_val.gif)

4. 编辑表格中的`修改变量`一项可以修改变量

![修改变量](/readme_img/write_val.gif)

5. 取消勾选表格中的`开启调试`一项可以关闭对应变量的查询（其余变量的更新频率会提高），但此时仍可以修改该变量

![开启调试](/readme_img/enable_debug.gif)

6. 双击`变量名`一项可以调出绘图窗口，绘制该变量的变化曲线，点击`帮助`按钮可以查看绘图窗口的详细操作方法

![绘图](/readme_img/plot.gif)

---

## 下位机介绍

### 配置项

`#define DEBUG_MAX_VAR_NUM`：最大可注册的变量个数，**不能超过256**

`#define DEBUG_TX_BUF_SIZE`：发送缓冲区长度，需大于最长数据帧的长度

`#define DEBUG_SEND(buf,len)`：用于数据发送的语句，buf是需要发送的数据首地址，len是要发送的数据长度

### API

`void Debug_RegisterVar(void *addr,char *name,DebugVarType type);`：注册一个**全局或静态**变量进行调试，`addr`为该变量的地址，`name`为变量名字符串（用于在上位机显示），`type`为类型枚举值（枚举量请见定义处）

`void Debug_DecodeFrame(uint8_t *buffer);`：解析收到的数据帧，需在接收完成一帧数据时调用，`buffer`为接收到的数据的首地址

### 程序示例（STM32 & HAL）
```c
//使用USART1进行收发，发送使用DMA，接收使用RXNE和IDLE中断

/********Debug.c********/
//...
#define DEBUG_MAX_VAR_NUM 20
#define DEBUG_TX_BUF_SIZE 20
#define DEBUG_SEND(buf,len) HAL_UART_Transmit_DMA(&huart1,buf,len) //配置使用DMA发送
//...

/********main.c********/
//...
float f32; //需要调试的全局变量
//...
int main()
{
	//...
	Debug_RegisterVar(&f32,"f32",DVar_Float); //注册调试变量
	__HAL_UART_ENABLE_IT(&huart1,UART_IT_RXNE); //开启RXNE中断
	__HAL_UART_ENABLE_IT(&huart1,UART_IT_IDLE); //开启IDLE中断
	//...
	while(1)
	{
		//...
	}
}

/********stm32f4xx_it.c********/
//...
void USART1_IRQHandler()
{
	static uint8_t rxBuf[30]; //接收缓冲区
	static uint8_t rxCnt; //接收计数器
	//收到一个字节进入RXNE
	if(__HAL_UART_GET_FLAG(&huart1,UART_FLAG_RXNE)!=RESET)
	{
		//保存收到的一个字节
		rxBuf[rxCnt++]=huart1.Instance->DR;
	}
	//进入空闲中断，一帧结束，进行解析
	else if(__HAL_UART_GET_FLAG(&huart1,UART_FLAG_IDLE)!=RESET)
	{
		__HAL_UART_CLEAR_IDLEFLAG(&huart1); //清除标志位
		Debug_DecodeFrame(rxBuf); //解析数据
		rxCnt=0; //计数器归零
	}
}
```

---

## 原理介绍

上位机通过串口向下位机发送请求数据帧，下位机接收到后根据数据帧中的命令码进行解析，可能会向上位机发送反馈数据帧，上位机再进行解析和显示

### 传输协议

数据帧的统一格式如下

|帧头(1byte)|命令码(1byte)|数据内容(不定长)|
|:-:|:-:|:-:|
|固定为`0xDB`|标记数据帧的作用|每种数据帧不同|

数据帧的数据内容根据作用的不同而不同，各类型数据帧的数据内容如下表（请求由上位机发出，反馈由下位机发出）

|数据帧作用|数据长度|数据内容|
|:-:|:-:|:--|
|请求变量列表长度|0|无|
|反馈变量列表长度|1|列表长度(1byte)|
|请求变量信息|1|变量索引(1byte)|
|反馈变量信息|不定长|变量索引(1byte)+变量类型(1byte)<br>+变量名(不定长，'\0'结尾)|
|请求读取变量|1|变量索引(1byte)
|反馈读取的变量|不定长|变量索引(1byte)+变量值数据|
|请求写入变量|不定长|变量索引(1byte)+变量值数据|


### 主要运行过程

下位机程序运行时通过调用接口注册需要调试的变量，形成调试变量列表，每个变量对应一个索引号（即列表中的下标）

上下位机通过串口连接后，用户点击`加载变量列表`按钮，上位机向下位机请求变量列表长度，得到反馈后依次请求各个变量的信息并更新在界面上

用户点击`开始调试`后，上位机启动定时器，循环发送列表中各个变量的读取请求，收到反馈后更新在界面上

当用户在`修改变量`一栏的单元格中键入了数据后，上位机会将其编码为字节流并发送写入请求，下位机收到后会向该变量的地址写入数据

当用户双击`变量名`一栏的单元格时，上位机显示绘图窗口，不断记录需要绘制的变量值，进行图像的绘制，同时对用户的图像操作进行响应

---

## 参与改进

* 参与代码贡献

	1. Fork本仓库

	2. 提交代码

	3. 新建 Pull Request

* 欢迎提 Issue

---

Skythinker @TARS_Go - JLU
