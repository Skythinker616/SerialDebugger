#ifndef GRAPHWINDOW_H
#define GRAPHWINDOW_H

#include <QDialog>
#include <qlist.h>
#include <qtimer.h>
#include <qpainter.h>
#include <QKeyEvent>
#include <qdebug.h>
#include <qmessagebox.h>

namespace Ui {
class GraphWindow;
}

class GraphWindow : public QDialog
{
    Q_OBJECT

public:
    explicit GraphWindow(QWidget *parent = nullptr);
    ~GraphWindow();
    void paintEvent(QPaintEvent*);
    void updateData(float newData);
    void wheelEvent(QWheelEvent *event);
    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *event);
    void setVarName(QString str);
    void setPlotState(bool enable);
    bool isPloting();
    void closeEvent(QCloseEvent *);
    void clearData();
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);

private slots:
    void on_btn_clear_clicked();
    void onTrig();
    void on_sb_samp_valueChanged(double arg1);

    void on_btn_help_clicked();

private:
    Ui::GraphWindow *ui;
    QList<float> data; //存放变量历史数据用于绘图
    QTimer *trigTimer; //定时器用于触发图像更新
    bool ctrlFlag,shiftFlag,altFlag; //组合键按下标志
    bool ploting; //当前是否正在绘图
    bool looking; //当前是否正在查看变量
    QPoint lookPos; //查看变量值时的鼠标坐标
};

#endif // GRAPHWINDOW_H
