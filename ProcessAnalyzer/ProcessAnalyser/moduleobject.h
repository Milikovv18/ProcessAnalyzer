#ifndef MODULEOBJECT_H
#define MODULEOBJECT_H

#include <QKeyEvent>
#include <QString>

#include "helper.h"


class ModuleObject
{
public:
    ModuleObject();
    ModuleObject(const QString& path, const QString& name);

    void setName(const QString& name);
    void setPath(const QString& path);
    void setVirtual(bool b);
    void setLocal(bool b);
    void setExports(bool b);

    const QString& getName() const;
    const QString& getPath() const;
    bool isVirtual() const;
    bool isLocal() const;
    bool exports() const;

private:
    QString m_name;
    QString m_path;

    bool m_virtuel = false;
    bool m_local = false;
    bool m_noExport = false;
};

#endif // MODULEOBJECT_H
