#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <qserialport.h>
#include <qserialportinfo.h>
#include <qstringlist.h>
#include <qmessagebox.h>
#include <qdebug.h>
#include <qtableview.h>
#include <qstandarditemmodel.h>
#include "Debugger.h"
#include <qtimer.h>
#include "graphwindow.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_btn_refreshSerial_clicked();
    void on_btn_switchSerial_clicked();
    void on_serial_readyRead();
    void slotTableEdit(QModelIndex topleft, QModelIndex bottomright);
    void slotTimerTrig();
    void on_btn_reqList_clicked();
    void on_btn_switchDebug_clicked();
    void on_cb_freq_currentTextChanged(const QString &arg1);
    void on_table_var_doubleClicked(const QModelIndex &index);

private:
    Ui::MainWindow *ui;
    GraphWindow *graph; //绘图窗口
    QSerialPort *port; //串口
    QStandardItemModel *tableModel; //表格数据
    QByteArray rxbuf; //串口接收缓冲区
    int varNum; //变量列表长度
    bool debugging; //当前是否正在调试
    QTimer *trigTimer; //定时器用于触发变量值查询
    QList<int> typeList; //存放每个变量的类型
    int plotIndex; //正在绘图的变量索引
    void initTable();
    void sleep(int ms);
    void parseSerial();
    void startDebug();
    void stopDebug();

};
#endif // MAINWINDOW_H
