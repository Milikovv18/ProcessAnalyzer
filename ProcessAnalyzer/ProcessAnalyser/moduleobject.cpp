#include "moduleobject.h"


ModuleObject::ModuleObject()
{

}


ModuleObject::ModuleObject(const QString& path, const QString& name)
{
    m_name = name;
    m_path = path;
}




void ModuleObject::setName(const QString& name)
{
    m_name = name;
}


void ModuleObject::setPath(const QString& path)
{
    m_path = path;

    // Changer un nom aussi
    m_name = Helper::getNameFromPath(path);
}


void ModuleObject::setVirtual(bool b)
{
    m_virtuel = b;
}


void ModuleObject::setLocal(bool b)
{
    m_local = b;
}


void ModuleObject::setExports(bool b)
{
    m_noExport = !b;
}



const QString& ModuleObject::getName() const
{
    return m_name;
}


const QString& ModuleObject::getPath() const
{
    return m_path;
}


bool ModuleObject::isVirtual() const
{
    return m_virtuel;
}


bool ModuleObject::isLocal() const
{
    return m_local;
}


bool ModuleObject::exports() const
{
    return !m_noExport;
}





