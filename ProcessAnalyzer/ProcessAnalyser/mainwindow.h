#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shellapi.h>

#include "animestackedwidget.h"
#include <QMainWindow>
#include <QTreeWidget>
#include <QFileDialog>
#include <QTableWidget>
#include <QProgressBar>
#include <QMessageBox>

#include "metacompute.h"
#include "helper.h"
#include "detective.h"
#include "hookermanager.h"


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);

    ~MainWindow();

public slots:
    void progressUpdated(int newVal);
    void progressMaxUpdated(int newVal);
    void functionReady(ActionType type, void* args);

    void log(const QString& info);

private slots:
    void on_connectButton_clicked();
    void on_previewButton_clicked();
    void on_refreshButton_clicked();
    void on_searchButton_clicked();
    void on_processIdLine_currentIndexChanged(int index);

    void actionPreviewHook();
    void actionHook();
    void actionUnhook();
    void actionPreview();
    void actionSpy();
    void prepareMenu(const QPoint & pos);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private:
    Ui::MainWindow *ui;
    AnimeStackedWidget* ui_sw;

    QTreeWidgetItem* m_curItem = nullptr;
    Detective detective{1024};

    // Variables temporaires
    ProcInfo m_curProcInfo;

    // Thread things
    MetaCompute computer;
    QProgressBar m_threadProgress;

    HookerManager hookerManager;

    ProcInfo parseProcInfo(QString info) const;
};
#endif // MAINWINDOW_H
