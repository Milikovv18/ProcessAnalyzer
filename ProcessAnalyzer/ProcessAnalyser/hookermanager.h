#ifndef HOOKERMANAGER_H
#define HOOKERMANAGER_H

#include <QString>
#include <QHash>

#include "helper.h"

struct HookedFunc{
    QString paylDll_name;
    QString paylFunc_name;

    QString rsrvBytes;
};

class HookerManager
{
public:
    HookerManager();
    HookedFunc takeHookedFunc(const QString& key);
    HookedFunc popHookedFunc();
    bool isHooked(const QString& key) const;
    QList<QString> getFuncsList() const;

    void putHookedFunc(const QString& key, const HookedFunc& val);
    void ejectPayloadDll(const QString& dllName);

private:
    QHash<QString, HookedFunc> hookedDict;
};

#endif // HOOKERMANAGER_H
