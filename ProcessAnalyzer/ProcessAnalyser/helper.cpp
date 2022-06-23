#include "helper.h"



Helper::Helper()
{

}


QString Helper::getProcessPath(HANDLE proc)
{
    unsigned long len = 256;
    auto nameArr = new wchar_t[len]{};
    QueryFullProcessImageName(proc, 0, nameArr, &len);
    auto name = QString::fromWCharArray(nameArr);

    delete[] nameArr;
    return name;
}


QString Helper::getNameFromPath(const QString& path)
{
    //qDebug()<< path;
    return path.right(path.size() - path.lastIndexOf('\\') - 1);
}
