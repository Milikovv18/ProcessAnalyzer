#include "mainwindow.h"
#include "ui_mainwindow.h"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->dllViewer->setItemDelegateForColumn(1, new TreeWidgetFontDelegate{ui->dllViewer});

    // Setting up stacked widget separatelly
    ui_sw = new AnimeStackedWidget(this);
    ((QBoxLayout*)centralWidget()->layout())->insertWidget(1, ui_sw);

    ui_sw->addWidget(ui->dllViewer);
    ui_sw->addWidget(ui->logText);
    ui_sw->show();

    log("Getting started, finding all running processes.");
    on_refreshButton_clicked();

    // Setting up custom context menu to 'preview' and to 'hook' etc.
    ui->dllViewer->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->dllViewer, &QTreeWidget::customContextMenuRequested,
            this, &MainWindow::prepareMenu);

    // Its easier to press enter than button
    connect(ui->searchLine, SIGNAL(returnPressed()),
            this, SLOT(on_searchButton_clicked()));
    connect(ui->processIdLine->lineEdit(), SIGNAL(returnPressed()),
            this, SLOT(on_previewButton_clicked()));

    // Signals about progress bar from heavy thread
    connect(&computer, SIGNAL(progressUpdated(int)),
            this, SLOT(progressUpdated(int)));
    connect(&computer, SIGNAL(progressMaxUpdated(int)),
            this, SLOT(progressMaxUpdated(int)));

    // Signal that heavy thread has finished
    connect(&computer, SIGNAL(functionReady(ActionType,void*)),
            this, SLOT(functionReady(ActionType,void*)));

    log("Ready to work.");
}


void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Alt) {
        ui_sw->slideInIdx(1);
    }
}

void MainWindow::keyReleaseEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Alt) {
        ui_sw->slideInIdx(0);
    }
}


void MainWindow::actionPreviewHook()
{
    QString injectionData(m_curItem->parent()->text(1) + '\2' +
                          m_curItem->text(0) + '\0');

    auto args = new ArgHookPreview{m_curProcInfo.id, injectionData};
    computer.apply(ActionType::HOOK_PREVIEW, args);

    //log("Function " + m_curItem->text(0) + " successfully hooked by " +
    //    funcToHook + " with " + dllObj.getName());
}

void MainWindow::actionHook(){
    log("");
    log("Getting user dll path.");

    // Obtenir le chemin de dll utilisateur
    auto dllPath = QFileDialog::
            getOpenFileName(this,
                            "Choose library (dll) with payload",
                            QDir::currentPath(),
                            "Library (*.dll *.DLL)");

    if (dllPath.isEmpty()) {
        log("Operation cancelled: no file choosen.");
        return;
    }

    log("Retrieving dll exported functions.");

    auto dllObj = ModuleObject(dllPath, Helper::getNameFromPath(dllPath));
    auto funcs = detective.getStaticExport(dllObj);
    if (!dllObj.exports()) {
        log("Operation failed: dll does not export any function");
        return;
    }

    // Crérr un tableau
    auto funcWnd = new QDialog;
    auto layout = new QHBoxLayout;
    auto funcContainer = new QTableWidget(funcs.size(), 1);
    funcWnd->setWindowTitle("Choose function to replace the hooked one");
    funcWnd->setAttribute(Qt::WA_DeleteOnClose);
    funcContainer->setEditTriggers(QAbstractItemView::NoEditTriggers);
    funcContainer->verticalHeader()->setDefaultSectionSize(24);
    funcContainer->horizontalHeader()->setStretchLastSection(true);
    funcContainer->horizontalHeader()->setVisible(false);

    for (int i(0); i < funcs.size(); ++i) {
        funcContainer->setItem(i, 0, new QTableWidgetItem(funcs[i]));
    }

    layout->addWidget(funcContainer);
    funcWnd->setLayout(layout);

    connect(funcContainer, SIGNAL(cellDoubleClicked(int,int)), funcWnd, SLOT(done(int)));

    auto funcToHook = funcs[funcWnd->exec()];
    qDebug() << funcToHook;
    if (funcToHook.isEmpty()) {
        log("Operation cancelled: no function choosen.");
    }

    log("Waiting for dll to connect...");

    char szFullPath[MAX_PATH] = {};
    GetCurrentDirectoryA(MAX_PATH, szFullPath);
    QString str = QString(szFullPath) + "\\injection\\ProcessAnalyserDll.dll";
    Injector::apply(m_curProcInfo.id, str.toStdString().c_str());
    log("Dll loaded.");


    // Préparer les données d'injection �  envoyer
    QString injectionData;

    QString funcToHook_name(m_curItem->parent()->text(1) + '\2' + m_curItem->text(0));
    if(hookerManager.isHooked(funcToHook_name)){
        QMessageBox message;
        message.setIcon(QMessageBox::Warning);
        message.setText("This function has already been hooked.\nYou are changing the payload function.");
        message.exec();

        HookedFunc hf = hookerManager.takeHookedFunc(funcToHook_name);
        injectionData = QString(m_curItem->parent()->text(1) + '\2' +
                                m_curItem->text(0) + '\2' +
                                hf.paylDll_name + '\2' +
                                hf.paylFunc_name + '\0');

        auto args = new ArgHookFunction{m_curProcInfo.id, injectionData, hf.rsrvBytes + '\0'};
        computer.apply(ActionType::UNHOOK_FUNCTION, args);
    }
    else{   // TODO: remove this block once multiple trampolines are available
        HookedFunc hf = hookerManager.popHookedFunc();
        if(hf.rsrvBytes != ""){
            injectionData = QString(m_curItem->parent()->text(1) + '\2' +
                                    m_curItem->text(0) + '\2' +
                                    hf.paylDll_name + '\2' +
                                    hf.paylFunc_name + '\0');

            auto args = new ArgHookFunction{m_curProcInfo.id, injectionData, hf.rsrvBytes + '\0'};
            computer.apply(ActionType::UNHOOK_FUNCTION, args);
        }
    }

    injectionData = QString(m_curItem->parent()->text(1) + '\2' +
                            m_curItem->text(0) + '\2' +
                            dllPath + '\2' +
                            funcToHook + '\0');

    auto args = new ArgHookFunction{m_curProcInfo.id, injectionData};
    computer.apply(ActionType::HOOK_FUNCTION, args);
}

void MainWindow::actionUnhook(){
    QString funcToUnhook_name(m_curItem->parent()->text(1) + '\2' + m_curItem->text(0));

    HookedFunc hf = hookerManager.takeHookedFunc(funcToUnhook_name);
    QString injectionData(funcToUnhook_name + '\2' +
                            hf.paylDll_name + '\2' +
                            hf.paylFunc_name + '\0');

    auto args = new ArgHookFunction{m_curProcInfo.id, injectionData, hf.rsrvBytes + '\0'};
    computer.apply(ActionType::UNHOOK_FUNCTION, args);
}

void MainWindow::actionPreview()
{
    ui->processIdLine->setCurrentText(m_curItem->text(1));
    on_previewButton_clicked();
}


void MainWindow::actionSpy()
{

}


void MainWindow::prepareMenu( const QPoint & pos )
{
    m_curItem = ui->dllViewer->itemAt(pos);
    if (!m_curItem)
        return;

    QAction *preview = new QAction("&Preview", this);
    connect(preview, SIGNAL(triggered()), this, SLOT(actionPreview()));
    QAction* spy = new QAction("&Spy", this);
    connect(spy, SIGNAL(triggered()), this, SLOT(actionSpy()));
    QAction *hook = new QAction("&Hook", this);
    connect(hook, SIGNAL(triggered()), this, SLOT(actionPreviewHook()));
    QAction *unhook = new QAction("&Unhook", this);
    connect(unhook, SIGNAL(triggered()), this, SLOT(actionUnhook()));

    QMenu menu(this);
    if (!m_curItem->parent()) {
        menu.addAction(preview);
        if (computer.isConnected()) menu.addAction(spy);
    } else {
        menu.addAction(hook);
        if(hookerManager.isHooked(m_curItem->parent()->text(1) + '\2' + m_curItem->text(0)))
            menu.addAction(unhook);
    }
    menu.exec(ui->dllViewer->mapToGlobal(pos));
}


void MainWindow::log(const QString& info)
{
    ui->logText->append(info);
    ui->statusbar->showMessage(info);
}


MainWindow::~MainWindow()
{
    delete ui;
}




void MainWindow::on_previewButton_clicked()
{
    log("");
    log("Loading module dependencies preview.");

    QString data = ui->processIdLine->currentText();
    if (data.isEmpty()) {
        log("Operation failed: No info given.");
        return;
    }

    ui->previewButton->setEnabled(false);
    ui->connectButton->setEnabled(false);

    m_curProcInfo = parseProcInfo(data);

    log("Module settles in " + m_curProcInfo.path);

    ui->dllViewer->clear();
    ui->dllViewer->header()->setStretchLastSection(false);

    m_threadProgress.setMinimum(0);
    m_threadProgress.setMaximum(0);
    ui->statusbar->addPermanentWidget(&m_threadProgress);

    // Prepairing input args
    auto args = new ArgPreviewTreeItems{ detective, m_curProcInfo.path };
    // Starting thread
    computer.apply(ActionType::PREVIEW_TREE_ITEMS, args);
}


void MainWindow::on_connectButton_clicked()
{
    log("");
    log("Loading runtime module dependencies.");

    QString data = ui->processIdLine->currentText();
    if (data.isEmpty()) {
        log("Operation failed: No info given.");
        return;
    }

    ui->previewButton->setEnabled(false);
    ui->connectButton->setEnabled(false);

    m_curProcInfo = parseProcInfo(data);

    log("Injecting ProcessAnalyserDll.dll into process " + QString::number(m_curProcInfo.id));

    ui->dllViewer->clear();
    ui->dllViewer->header()->setStretchLastSection(false);

    m_threadProgress.setMinimum(0);
    m_threadProgress.setMaximum(0);
    m_threadProgress.show();
    ui->statusbar->addPermanentWidget(&m_threadProgress);

    // Prepairing input args
    QList<QString> hookedList = hookerManager.getFuncsList();
    auto args = new ArgConnectTreeItems{ m_curProcInfo.id, hookedList };
    // Starting thread
    computer.apply(ActionType::CONNECT_TREE_ITEMS, args);
}


void MainWindow::on_refreshButton_clicked()
{
    ui->processIdLine->clear();

    QIcon ico;
    auto allProcs = detective.getRunningProcesses();
    for (const auto& proc : allProcs)
    {
        HINSTANCE hInstance = ::GetModuleHandle(NULL);
        HICON Icon = ExtractIconA(hInstance, proc.path.toStdString().c_str(), 0);
        if (Icon != NULL) {
            ico = QIcon(QPixmap::fromImage(QImage::fromHICON(Icon)));
        } else {
            ico = QIcon(":/Misc/Resources/question.png");
        }

        ui->processIdLine->addItem(ico, QString::number(proc.id) + " <" + proc.path + ">");
    }

    ui->processIdLine->setCurrentIndex(0);
}


void MainWindow::on_searchButton_clicked()
{
    auto textToSearch = ui->searchLine->text();

    QList<QTreeWidgetItem*> clist =
            ui->dllViewer->findItems(textToSearch,
                                     Qt::MatchContains |
                                     Qt::MatchRecursive);

    int funcsTotalCount(0);

    for(int i(0); i < ui->dllViewer->topLevelItemCount(); ++i)
    {
        auto item = ui->dllViewer->topLevelItem(i);

        for (int j(0); j < item->childCount(); ++j)
        {
            auto child = item->child(j);

            if (clist.contains(child)) {
                child->setHidden(false);
                funcsTotalCount += 1;
            } else {
                child->setHidden(true);
            }
        }
    }

    if (textToSearch.isEmpty()) {
        ui->dllViewer->collapseAll();
    } else {
        ui->dllViewer->expandAll();
    }

    log("");
    log("Found " + QString::number(funcsTotalCount) +
        " correspondences with \"" + textToSearch + "\"");
}


void MainWindow::on_processIdLine_currentIndexChanged(int index)
{
    ui->processIdLine->setCurrentText(ui->processIdLine->itemText(index).split(" ")[0]);
}


void MainWindow::progressUpdated(int newVal)
{
    m_threadProgress.setValue(newVal);
}
void MainWindow::progressMaxUpdated(int newVal)
{
    m_threadProgress.setMaximum(newVal);
}

void MainWindow::functionReady(ActionType type, void* args)
{
    switch (type)
    {
    case ActionType::PREVIEW_TREE_ITEMS:
    {
        ui->statusbar->removeWidget(&m_threadProgress);
        auto treeItems = (ArgPreviewTreeItems*)args;

        ui->dllViewer->addTopLevelItems(treeItems->ret_items);
        ui->dllViewer->resizeColumnToContents(0);

        log("Loaded " + QString::number(ui->dllViewer->topLevelItemCount()) + " dll dependencies.");
        delete treeItems;

        ui->previewButton->setEnabled(true);
        ui->connectButton->setEnabled(true);
    } break;

    case ActionType::CONNECT_TREE_ITEMS:
    {
        ui->statusbar->removeWidget(&m_threadProgress);
        auto treeItems = (ArgConnectTreeItems*)args;

        ui->dllViewer->addTopLevelItems(treeItems->ret_items);
        ui->dllViewer->resizeColumnToContents(0);

        log("Loaded " + QString::number(ui->dllViewer->topLevelItemCount()) + " dll dependencies.");
        delete treeItems;

        ui->previewButton->setEnabled(true);
        ui->connectButton->setEnabled(true);
    } break;

    case ActionType::HOOK_PREVIEW:
    {
        auto previewResult = (ArgHookPreview*)args;
        QMessageBox message;
        message.setIcon(QMessageBox::Warning);

        if(previewResult->success){
            message.setText("Following instructions of the target function will be overwritten:\n\n" + previewResult->overw_insts);
            auto acceptButt = message.addButton("OK", QMessageBox::AcceptRole);
            message.addButton("Cancel", QMessageBox::RejectRole);
            connect(acceptButt, SIGNAL(clicked(bool)), this, SLOT(actionHook()));
            message.exec();
        }
        else{
            message.setText("Failed to preview the function!");
            message.exec();
        }

    } break;

    case ActionType::HOOK_FUNCTION:
    {
        auto hookResult = (ArgHookFunction*)args;
        if(hookResult->success){
            qDebug() << "hook successfull" << hookResult->overw_bytes;

            QList<QString> hkInfo = hookResult->injection_data.split('\2');
            HookedFunc hkFunc = {hkInfo[2], hkInfo[3], hookResult->overw_bytes};
            hookerManager.putHookedFunc(hkInfo[0] + '\2' + hkInfo[1], hkFunc);

            on_connectButton_clicked();
        }
        else{
            qDebug() << "hook failed!";
        }
    } break;

    case ActionType::UNHOOK_FUNCTION:
    {
        auto unhookResult = (ArgHookFunction*)args;
        if(unhookResult->success){
            qDebug() << "unhooking successful!";
            on_connectButton_clicked();
        }
        else{
            throw("everything is broken now!");
        }
    } break;

    case ActionType::INVALID:
    {
        throw "Fuck your action";
    } break;
    }
}


void MainWindow::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event);
    computer.terminate();
}


ProcInfo MainWindow::parseProcInfo(QString info) const
{
    QString procPath;

    if (info.isEmpty()) {
        throw "Fuck your info";
    }

    // Obtenir un id de processus
    bool ok;
    int procId = info.toUInt(&ok);
    if (ok) {
        HANDLE handl = OpenProcess(PROCESS_ALL_ACCESS, TRUE, procId);
        procPath = Helper::getProcessPath(handl);
    } else {
        procId = -1;
        procPath = info;
    }

    return { procId, procPath };
}











