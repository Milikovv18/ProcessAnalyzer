#ifndef METACOMPUTESH_H
#define METACOMPUTESH_H

#include <QTreeWidget>
#include <QThread>
#include <QBrush>

#include "network.h"
#include "injector.h"
#include "detective.h"
#include "moduleobject.h"


enum class ActionType
{
    INVALID,
    PREVIEW_TREE_ITEMS,
    CONNECT_TREE_ITEMS,
    HOOK_PREVIEW,
    HOOK_FUNCTION,
    UNHOOK_FUNCTION
};


struct ArgPreviewTreeItems
{
    Detective& detective;
    const QString& path;
    QList<QTreeWidgetItem*> ret_items;
};

struct ArgConnectTreeItems
{
    int id;
    QList<QString> hookedList;
    QList<QTreeWidgetItem*> ret_items;
};

struct ArgHookPreview
{
    int id;
    QString injection_data;

    bool success;
    QString overw_insts;
};

struct ArgHookFunction
{
    int id;
    QString injection_data;

    QString overw_bytes;
    bool success;
};


class MetaCompute : public QThread
{
    Q_OBJECT

public:
    void apply(ActionType type, void* args);
    static void setupConnection();

    bool isConnected() const;

signals:
    void progressUpdated(int newVal);
    void progressMaxUpdated(int newVal);
    void functionReady(ActionType type, void* args);

    void log(const QString& info);

private:
    static Network network; // *this is temporary, but connection must be continous
    ActionType m_type;
    void* m_args;

    void run() override;

    QList<QTreeWidgetItem*> getPreviewTreeItems(Detective& det, QString path);
    QList<QTreeWidgetItem*> getConnectTreeItems(int id, QList<QString>& hookedList);
    void connectNetwork(int id);
    //void hookFunction(QString tarLib, QString tarFunc, QString newLib, QString newFunc);
    bool hookPreview(ArgHookPreview& args);
    bool hookFunction(ArgHookFunction& args);
    bool unhookFunction(ArgHookFunction& args);
};

#endif // METACOMPUTESH_H
