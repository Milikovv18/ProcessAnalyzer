#ifndef HELPER_H
#define HELPER_H

#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include "psapi.h"

#include <QString>
#include <QDebug>


class Helper
{
public:
    Helper();

    static QString getProcessPath(HANDLE proc);
    static QString getNameFromPath(const QString& path);
};

#endif // HELPER_H
