#ifndef DETECTIVE_H
#define DETECTIVE_H

#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <Psapi.h>
#include <ImageHlp.h>
#include <tlhelp32.h>

#include <QFile>
#include <QDebug>

#include "helper.h"
#include "moduleobject.h"

#define clear_buf ZeroMemory(m_uniBuffer, m_uniBufLen)


struct ProcInfo
{
    int id = -1;
    QString path;
};


class Detective
{
public:
    Detective(quint64 bufLen);

    QList<ProcInfo> getRunningProcesses();

    QList<ModuleObject> getStaticImport(QString fileName);
    QStringList getStaticExport(ModuleObject& obj);

    QList<ModuleObject> getRuntimeDeps(int procId);

    QString getLibPath(QString libName, bool system = false);

    ~Detective();

private:
    // Single-threaded buffer to avoid more memory allocations
    char* m_uniBuffer = nullptr;
    quint64 m_uniBufLen = 0;

    DWORD Rva2Offset(DWORD rva, PIMAGE_SECTION_HEADER psh, unsigned numOfSections) const;

    QString getLibPathRough(QString libName);
    QString getLibPathExact(QString libName);
};


#endif // DETECTIVE_H
