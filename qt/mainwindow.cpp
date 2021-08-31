#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle("SerialDebugger");

    graph=new GraphWindow();
    port=new QSerialPort();
    tableModel=new QStandardItemModel(this);
    trigTimer=new QTimer(this);

    trigTimer->stop();
    trigTimer->setInterval(10); //设置定时器间隔10ms，触发频率100Hz
    connect(trigTimer,SIGNAL(timeout()),this,SLOT(slotTimerTrig()));
    ui->cb_freq->setToolTip("数据刷新频率");
    ui->cb_freq->setCurrentText("100Hz");

    initTable(); //初始化表头
}

MainWindow::~MainWindow()
{
    delete ui;
    delete trigTimer;
    delete port;
    delete tableModel;
    delete graph;
}

//刷新串口按键点击
void MainWindow::on_btn_refreshSerial_clicked()
{
    ui->combo_serialNum->clear(); //清除当前列表
    foreach(const QSerialPortInfo &info,QSerialPortInfo::availablePorts()) //遍历并添加获取到的串口名称
    {
        ui->combo_serialNum->addItem(info.portName());
    }
}

//开关串口按钮点击
void MainWindow::on_btn_switchSerial_clicked()
{
    if(port->isOpen()) //关闭串口
    {
        if(debugging)
            stopDebug(); //停止调试
        port->clear(); //清除串口数据
        port->close(); //关闭串口
        //设置各按钮样式
        ui->btn_reqList->setEnabled(false);
        ui->btn_switchDebug->setEnabled(false);
        ui->btn_switchSerial->setText("打开串口");
    }
    else //打开串口
    {
        if(ui->combo_serialNum->currentText().isEmpty()) //没有可用的串口号
        {
            QMessageBox::information(NULL,"错误","请选择串口号");
            return;
        }
        port->setPortName(ui->combo_serialNum->currentText());
        if(!port->open(QIODevice::ReadWrite)) //打开串口
        {
            QMessageBox::information(NULL,"错误","串口打开失败");
            return;
        }
        port->setBaudRate(QSerialPort::Baud115200,QSerialPort::AllDirections); //波特率115200
        port->setDataBits(QSerialPort::Data8); //8数据位
        port->setFlowControl(QSerialPort::NoFlowControl);
        port->setParity(QSerialPort::NoParity); //无校验位
        port->setStopBits(QSerialPort::OneStop); //1停止位
        connect(port,SIGNAL(readyRead()),this,SLOT(on_serial_readyRead()));
        //设置各按钮样式
        ui->btn_switchSerial->setText("关闭串口");
        ui->btn_reqList->setEnabled(true);
    }
}

//接收到串口数据
void MainWindow::on_serial_readyRead()
{
    rxbuf.append(port->readAll()); //收到的数据追加到缓冲区
    parseSerial(); //解析数据
}

//表格发生数据变更事件
void MainWindow::slotTableEdit(QModelIndex topleft, QModelIndex bottomright)
{
    Q_UNUSED(bottomright);

    if(!debugging) //没有在调试则退出
        return;

    if(topleft.column()==3) //修改的是“修改数据”列
    {
        int index=topleft.row(); //获取修改的变量索引

        QString str=tableModel->item(index,3)->text(); //获取修改后字符串
        if(str.isEmpty())
            return;

        DebugVarTrans trans; //用于数据转换为字节流
        switch(typeList[index]) //根据该变量类型转换
        {
            case DVar_Int8: trans.varInt8=str.toInt(); break;
            case DVar_Int16: trans.varInt16=str.toInt(); break;
            case DVar_Int32: trans.varInt32=str.toInt(); break;
            case DVar_Uint8: trans.varUint8=str.toUInt(); break;
            case DVar_Uint16: trans.varUint16=str.toUInt(); break;
            case DVar_Uint32: trans.varUint32=str.toUInt(); break;
            case DVar_Float: trans.varFloat=str.toFloat(); break;
            case DVar_Bool: trans.varBool=(str=="true"?true:false); break;
        }

        trigTimer->stop(); //暂停变量数据查询
        sleep(10); //确保之前的数据发送完毕

        //创建一个数据帧
        QByteArray arr;
        int varLen=LenOfDType(typeList[index]); //获取变量长度
        arr.resize(3+varLen);
        arr[0]=DEBUG_FRAME_HEADER; //帧头
        arr[1]=DCmd_WriteReq; //命令码：写入请求
        arr[2]=index; //请求写入的变量索引
        for(int i=0;i<varLen;i++) //变量数据
            arr[i+3]=trans.bytes[i];

        port->write(arr); //串口发送
        sleep(10); //确保发送完毕

        trigTimer->start(); //恢复变量数据查询
        tableModel->item(index,3)->setText(""); //清空该单元格
    }
}

//定时器触发变量查询请求
void MainWindow::slotTimerTrig()
{
    static int curIndex; //当前在查询的变量索引

    //查找下一个开启了调试的变量索引（这里写的挺乱）
    int trialIndex=((curIndex==varNum-1)?0:curIndex+1);
    while(tableModel->item(trialIndex++,4)->checkState()!=Qt::Checked)
    {
        if(trialIndex>=varNum)
            trialIndex=0;
        if(trialIndex-1==curIndex)
            return;
    }
    curIndex=trialIndex-1;

    //创建数据帧
    QByteArray arr;
    arr.resize(3);
    arr[0]=DEBUG_FRAME_HEADER; //帧头
    arr[1]=DCmd_ReadReq; //命令码：读取变量请求
    arr[2]=curIndex; //要查询的变量索引
    port->write(arr); //串口发送
}

//初始化表格数据
void MainWindow::initTable()
{
    tableModel->setColumnCount(5); //5列
    //各列表头
    tableModel->setHeaderData(0,Qt::Horizontal,"变量名");
    tableModel->setHeaderData(1,Qt::Horizontal,"类型");
    tableModel->setHeaderData(2,Qt::Horizontal,"当前值");
    tableModel->setHeaderData(3,Qt::Horizontal,"修改变量");
    tableModel->setHeaderData(4,Qt::Horizontal,"开启调试");

    connect(tableModel,SIGNAL(dataChanged(QModelIndex,QModelIndex)),this,SLOT(slotTableEdit(QModelIndex,QModelIndex)));
    ui->table_var->setModel(tableModel);//绑定表格控件与表格数据
}

//程序等待（使用子任务循环）
void MainWindow::sleep(int ms)
{
    QEventLoop el;
    QTimer::singleShot(ms,&el,SLOT(quit()));
    el.exec();
}

//解析串口接收的数据
void MainWindow::parseSerial()
{
    if(rxbuf.size()>=2 && rxbuf[0]==(char)DEBUG_FRAME_HEADER) //帧头正确且缓冲区长度>=2(帧头+命令码)则进入解析，否则帧错误或不完整
    {
        char cmd=rxbuf[1]; //命令码

        if(cmd==DCmd_ListLenRes) //变量列表长度反馈
        {
            if(rxbuf.size()<3) //至少要三个字节
                return;
            varNum=rxbuf[2]; //列表长度
            rxbuf.remove(0,3); //移除缓冲区这一帧数据
            parseSerial(); //递归解析
        }
        else if(cmd==DCmd_VarInfoRes) //变量信息反馈
        {
            if(rxbuf.size()<5) //至少要5个字节（帧头+命令码+索引+类型+变量名）
                return;
            for(int i=4;i<rxbuf.size();i++) //寻找'\0'（变量名结尾）以判断帧结束
            {
                if(rxbuf[i]=='\0') //找到'\0'说明数据帧完整
                {
                    int index=rxbuf[2]; //变量索引
                    int type=rxbuf[3]; //变量类型
                    QString name=QString(rxbuf.mid(4,i-4)); //变量名

                    //更新表格数据，添加一行
                    tableModel->setItem(index,0,new QStandardItem(name)); //变量名
                    tableModel->setItem(index,1,new QStandardItem(QString(NameOfDType(type)))); //类型
                    tableModel->setItem(index,2,new QStandardItem("NaN")); //当前值
                    QStandardItem *checkItem=new QStandardItem(); //是否开启调试
                    checkItem->setCheckable(true);
                    checkItem->setCheckState(Qt::Checked); //默认开启调试
                    tableModel->setItem(index,4,checkItem);
                    tableModel->item(index,0)->setFlags(Qt::ItemIsEnabled);//前三列不可编辑
                    tableModel->item(index,1)->setFlags(Qt::ItemIsEnabled);
                    tableModel->item(index,2)->setFlags(Qt::ItemIsEnabled);

                    typeList.append(type); //追加到变量类型列表

                    rxbuf.remove(0,i+1); //从缓冲区内去除该帧数据
                    parseSerial(); //递归解析
                    return;
                }
            }
        }
        else if(cmd==DCmd_ReadRes) //变量值读取反馈
        {
            if(rxbuf.size()<4) //至少四个字节（帧头+命令码+索引+值内容）
                return;

            int index=rxbuf[2]; //索引
            int varLen=LenOfDType(typeList[index]); //变量长度

            if(rxbuf.size()<3+varLen) //至少3+varLen个字节（帧头+命令码+索引+值内容）
                return;

            //转换字节流到变量
            DebugVarTrans trans;
            for(int i=0;i<varLen;i++)
                trans.bytes[i]=rxbuf[i+3];
            QStandardItem *item=tableModel->item(index,2); //当前值单元格
            switch(typeList[index]) //根据变量类型转换字节流，并显示在单元格中
            {
                case DVar_Int8: item->setText(QString::number(trans.varInt8)); break;
                case DVar_Int16: item->setText(QString::number(trans.varInt16)); break;
                case DVar_Int32: item->setText(QString::number(trans.varInt32)); break;
                case DVar_Uint8: item->setText(QString::number(trans.varUint8)); break;
                case DVar_Uint16: item->setText(QString::number(trans.varUint16)); break;
                case DVar_Uint32: item->setText(QString::number(trans.varUint32)); break;
                case DVar_Float: item->setText(QString::number(trans.varFloat)); break;
                case DVar_Bool: item->setText(trans.varBool?"true":"false"); break;
            }

            //发送数据到绘图窗口
            if(index==plotIndex && graph->isPloting())
            {
                switch(typeList[index])
                {
                    case DVar_Int8: graph->updateData(trans.varInt8); break;
                    case DVar_Int16: graph->updateData(trans.varInt16); break;
                    case DVar_Int32: graph->updateData(trans.varInt32); break;
                    case DVar_Uint8: graph->updateData(trans.varUint8); break;
                    case DVar_Uint16: graph->updateData(trans.varUint16); break;
                    case DVar_Uint32: graph->updateData(trans.varUint32); break;
                    case DVar_Float: graph->updateData(trans.varFloat); break;
                    case DVar_Bool: graph->updateData(trans.varBool); break;
                }
            }

            rxbuf.remove(0,3+varLen); //从缓冲区内去除该帧数据
            parseSerial(); //递归解析
        }
    }
    else //帧错误或不完整
    {
        while(rxbuf[0]!=(char)DEBUG_FRAME_HEADER && rxbuf.size()>0) //去除错误数据
            rxbuf.remove(0,1);
        if(rxbuf.size()>0) //若缓冲区仍有数据说明后面还有数据帧，递归解析
            parseSerial();
    }
}

//开始调试
void MainWindow::startDebug()
{
    if(!debugging)
    {
        trigTimer->start();
        ui->btn_switchDebug->setText("停止调试");
        debugging=true;
    }
}

//停止调试
void MainWindow::stopDebug()
{
    if(debugging)
    {
        trigTimer->stop();
        ui->btn_switchDebug->setText("开始调试");
        debugging=false;
    }
}

//请求变量列表按钮点击
void MainWindow::on_btn_reqList_clicked()
{
    if(debugging)
        stopDebug(); //停止调试

    sleep(10); //确保之前的数据已发出

    tableModel->clear(); //清除表格数据
    initTable(); //初始化表格

    varNum=0; //变量数清零
    typeList.clear(); //类型列表清空
    ui->btn_switchDebug->setEnabled(false); //失能开关调试按钮

    //创建数据帧请求列表长度
    QByteArray arr;
    arr.resize(2);
    arr[0]=DEBUG_FRAME_HEADER;
    arr[1]=DCmd_ListLenReq;
    port->write(arr); //串口发送

    for(int i=0;i<500;i++) //等待列表长度获取完成，超时5秒会直接返回
    {
        if(varNum!=0)
            break;
        if(i==499)
            return;
        sleep(10);
    }

    for(int index=0;index<varNum;index++) //依次请求变量信息
    {
        //创建数据帧
        arr.resize(3);
        arr[0]=DEBUG_FRAME_HEADER; //帧头
        arr[1]=DCmd_VarInfoReq; //命令码：变量信息请求
        arr[2]=index; //请求的索引
        port->write(arr); //串口写入
        sleep(10); //等待10ms保证串口发出
    }

    ui->btn_switchDebug->setEnabled(true); //列表获取完成，使能开关调试按钮
}

//开关调试按钮点击
void MainWindow::on_btn_switchDebug_clicked()
{
    if(!debugging)
    {
        startDebug();
    }
    else
    {
        stopDebug();
    }
}

//修改变量查看频率
void MainWindow::on_cb_freq_currentTextChanged(const QString &str)
{
    //根据选择的频率设定定时器间隔时间
    if(str=="200Hz")
    {
        trigTimer->setInterval(5);
    }
    else if(str=="100Hz")
    {
        trigTimer->setInterval(10);
    }
    else if(str=="50Hz")
    {
        trigTimer->setInterval(20);
    }
    else if(str=="10Hz")
    {
        trigTimer->setInterval(100);
    }
    else if(str=="1Hz")
    {
        trigTimer->setInterval(1000);
    }
}

//单元格双击事件
void MainWindow::on_table_var_doubleClicked(const QModelIndex &index)
{
    if(index.column()==0) //若点击在变量名上则进入绘图
    {
        graph->setVarName(tableModel->item(index.row(),0)->text()); //设定变量名
        plotIndex=index.row(); //设定要绘制的变量索引
        graph->clearData(); //清空图像
        graph->setPlotState(true); //开启绘图
        graph->show(); //显示绘图窗口
    }
}
