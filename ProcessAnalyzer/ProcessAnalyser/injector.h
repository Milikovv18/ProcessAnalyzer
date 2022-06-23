#ifndef INJECTOR_H
#define INJECTOR_H

#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <QDebug>

class Injector
{
public:
    Injector();

    static void apply(int procId, const char* dllName);

private:

};

#endif // INJECTOR_H
