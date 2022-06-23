#include "hookermanager.h"

HookerManager::HookerManager(){}

HookedFunc HookerManager::takeHookedFunc(const QString& key){
    return hookedDict.take(key);
}

HookedFunc HookerManager::popHookedFunc(){
    if(!hookedDict.isEmpty()){
        return hookedDict.take(hookedDict.keys().back());
    }
    return {};
}

bool HookerManager::isHooked(const QString &key) const{
    return hookedDict.contains(key);
}

QList<QString> HookerManager::getFuncsList() const{
    //return QList<const QString>();
    QList<QString> fList;
    Helper h;

    for(auto &elem : hookedDict.keys()){
        auto parts = elem.split('\2');
        fList.push_back(h.getNameFromPath(parts[0]) + '\2' + parts[1]);
    }
    return fList;
}

void HookerManager::putHookedFunc(const QString &key, const HookedFunc &val){
    hookedDict.insert(key, val);
}

void HookerManager::ejectPayloadDll(const QString &dllName){
    // to be continued
}
