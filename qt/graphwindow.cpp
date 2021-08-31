#include "graphwindow.h"
#include "ui_graphwindow.h"

GraphWindow::GraphWindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::GraphWindow),
    ctrlFlag(false),
    shiftFlag(false),
    altFlag(false),
    ploting(false),
    looking(false)
{
    ui->setupUi(this);
    setWindowTitle("Graph");
    setWindowFlags(Qt::WindowCloseButtonHint);
    //设定触发定时器
    trigTimer=new QTimer(this);
    trigTimer->setInterval(30); //30ms绘制一帧图像
    trigTimer->start();
    connect(trigTimer,SIGNAL(timeout()),this,SLOT(onTrig()));
}

GraphWindow::~GraphWindow()
{
    delete trigTimer;
    delete ui;
}

//绘图事件，进行图像重绘
void GraphWindow::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    if(data.isEmpty())
        return;

    //设定水平拖动条范围
    if(data.size()>ui->sb_samp->value())
        ui->bar_hori->setMaximum(data.size()-ui->sb_samp->value());
    else
        ui->bar_hori->setMaximum(0);
    //若勾选实时更新则设定拖动条到末端
    if(ui->cb_update->checkState()==Qt::Checked)
        ui->bar_hori->setValue(ui->bar_hori->maximum());

    ui->lab_curval->setText(QString("当前值：%1").arg(data.at(data.size()-1))); //显示当前值

    //获取界面上控件数据
    float min=ui->sb_min->value(),max=ui->sb_max->value(),mid=(max+min)/2;
    int range=ui->sb_samp->value(),startIndex=ui->bar_hori->value();

    //窗口宽高
    float hei=height(),wid=width();

    QPainter painter(this); //画笔

    //绘制零标记线
    if(min<0 && max >0)
    {
        painter.setPen(QPen(QColor(184,184,184),3,Qt::DotLine));
        float zeroY=hei/2-(0-mid)/(max-min)*hei;
        painter.drawLine(0,zeroY,wid,zeroY);
    }

    //绘制查看线及查看值
    if(looking)
    {
        painter.setPen(QPen(QColor(184,184,184),2));
        painter.drawLine(lookPos.x(),0,lookPos.x(),hei);
        int index=(lookPos.x()*1.0/wid)*(ui->sb_samp->value())+ui->bar_hori->value();
        if(index<data.size() && lookPos.x()>0 && lookPos.x()<wid)
            ui->lab_lookval->setText(QString("查看值：%1").arg(data.at(index)));
        else
            ui->lab_lookval->setText("查看值：NaN");
    }
    else
    {
        ui->lab_lookval->setText("");
    }

    //绘制图线
    painter.setPen(QPen(QColor(48,51,53),2));
    for(int i=startIndex;i<startIndex+range-1 && i<data.size()-1;i++)
    {
        //将每两个采样点用直线相连
        painter.drawLine((i-startIndex+0.5)*(wid/range), hei/2-(data[i]-mid)/(max-min)*hei, (i-startIndex+1.5)*(wid/range), hei/2-(data[i+1]-mid)/(max-min)*hei);
    }
}

//更新变量的最新值
void GraphWindow::updateData(float newData)
{
    data.append(newData); //将最新值追加到存储区
}

//滚轮事件
void GraphWindow::wheelEvent(QWheelEvent *event)
{
    if(ctrlFlag) //ctrl按下则竖直缩放
    {
        float dist=(ui->sb_max->value()-ui->sb_min->value())*0.01; //最大/最小值增减量
        if(event->delta()>0)
        {
            ui->sb_max->setValue(ui->sb_max->value()-dist);
            ui->sb_min->setValue(ui->sb_min->value()+dist);
        }
        else if(event->delta()<0)
        {
            ui->sb_max->setValue(ui->sb_max->value()+dist);
            ui->sb_min->setValue(ui->sb_min->value()-dist);
        }
    }
    if(shiftFlag) //shift按下则上下平移
    {
        float dist=(ui->sb_max->value()-ui->sb_min->value())*0.01; //最大最小值增减量
        if(event->delta()>0)
        {
            ui->sb_max->setValue(ui->sb_max->value()+dist);
            ui->sb_min->setValue(ui->sb_min->value()+dist);
        }
        else if(event->delta()<0)
        {
            ui->sb_max->setValue(ui->sb_max->value()-dist);
            ui->sb_min->setValue(ui->sb_min->value()-dist);
        }
    }
    if(altFlag) //alt按下则水平伸缩
    {
        float dist=ui->sb_samp->value()*0.01+1; //采样数增减量*0.5
        if(event->delta()>0)
        {
            if(ui->sb_samp->value()>10)
            {
                ui->sb_samp->setValue(ui->sb_samp->value()-dist*2);
                if(ui->bar_hori->value()<ui->bar_hori->maximum()-dist)
                    ui->bar_hori->setValue(ui->bar_hori->value()+dist);
            }
        }
        if(event->delta()<0)
        {
            ui->sb_samp->setValue(ui->sb_samp->value()+dist*2);
            if(ui->bar_hori->value()>ui->bar_hori->minimum()+dist)
                ui->bar_hori->setValue(ui->bar_hori->value()-dist);
        }
    }
    if(!ctrlFlag && !shiftFlag && !altFlag) //无任何组合键按下则时间轴左右滚动
    {
        float dist=ui->sb_samp->value()*0.01+1; //水平拖动条位置增减量
        if(event->delta()>0 && ui->bar_hori->value()>0)
        {
            if(ui->bar_hori->value()>dist)
                ui->bar_hori->setValue(ui->bar_hori->value()-dist);
            else
                ui->bar_hori->setValue(0);
        }
        else if(event->delta()<0 && ui->bar_hori->value()<ui->bar_hori->maximum())
        {
            if(ui->bar_hori->maximum()-ui->bar_hori->value()>dist)
                ui->bar_hori->setValue(ui->bar_hori->value()+dist);
            else
                ui->bar_hori->setValue(ui->bar_hori->maximum());
        }

    }
}

//按键按下事件
void GraphWindow::keyPressEvent(QKeyEvent *event)
{
    //三种组合键按下时对应标记置true
    if(event->key()==Qt::Key_Control)
        ctrlFlag=true;
    else if(event->key()==Qt::Key_Shift)
        shiftFlag=true;
    else if(event->key()==Qt::Key_Alt)
        altFlag=true;
}

//按键松开事件
void GraphWindow::keyReleaseEvent(QKeyEvent *event)
{
    //三种组合键松开时对应标记置false
    if(event->key()==Qt::Key_Control)
        ctrlFlag=false;
    else if(event->key()==Qt::Key_Shift)
        shiftFlag=false;
    else if(event->key()==Qt::Key_Alt)
        altFlag=false;
}

//设定变量名
void GraphWindow::setVarName(QString str)
{
    ui->lab_name->setText("变量名："+str);
}

//设置绘图开启状态
void GraphWindow::setPlotState(bool enable)
{
    ploting=enable;
}

//获取绘图开启状态
bool GraphWindow::isPloting()
{
    return ploting;
}

//关闭窗口事件
void GraphWindow::closeEvent(QCloseEvent *)
{
    ploting=false; //关闭绘图
}

//清空历史数据，也即清空图像
void GraphWindow::clearData()
{
    data.clear();
}

//鼠标按下事件
void GraphWindow::mousePressEvent(QMouseEvent *event)
{
    if(event->button()==Qt::LeftButton) //如果左键按下则开启查看
    {
        looking=true;
        lookPos.setX(event->x()); //记录鼠标坐标
        lookPos.setY(event->y());
    }
}

//鼠标松开事件
void GraphWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if(event->button()==Qt::LeftButton) //如果左键松开则关闭查看
        looking=false;
}

//鼠标移动事件（仅按下时触发）
void GraphWindow::mouseMoveEvent(QMouseEvent *event)
{
    if(looking) //若在查看状态则记录鼠标位置
    {
        lookPos.setX(event->x());
        lookPos.setY(event->y());
    }
}

//清空数据按钮点击
void GraphWindow::on_btn_clear_clicked()
{
    clearData();
}

//定时器槽函数
void GraphWindow::onTrig()
{
    update(); //触发图像更新
}

//采样数spinbox内容发生更改
void GraphWindow::on_sb_samp_valueChanged(double value)
{
    if(data.size()>ui->sb_samp->value()) //修改水平拖动条最大值
        ui->bar_hori->setMaximum(data.size()-value);
    else
        ui->bar_hori->setMaximum(0);
}

//帮助按钮点击
void GraphWindow::on_btn_help_clicked()
{
    //弹出messagebox显示帮助信息
    QString str="设置说明\n"
                "采样点数：画面内显示的采样点数量\n"
                "最小值/最大值：画面内显示的采样值范围\n"
                "清空数据：清除历史数据，清空图像\n"
                "实时更新：始终显示时间轴末端，跟随采样值的更新而变化\n\n"
                "滚轮操作\n"
                "1.无按键+滚轮：时间轴左右滚动\n"
                "2.Ctrl+滚轮：图像竖直缩放\n"
                "3.Alt+滚轮：图像水平缩放\n"
                "4.Shift+滚轮：图像上下平移\n\n"
                "查看操作\n"
                "按住鼠标左键并拖动可以查看图像上的指定值";
    QMessageBox box;
    box.setWindowTitle("绘图界面说明");
    box.setText(str);
    box.exec();
}
