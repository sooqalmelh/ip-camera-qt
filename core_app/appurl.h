#ifndef APPURL_H
#define APPURL_H

#include "head.h"

class AppUrl
{
public:
    //获取url集合
    static QStringList getUrls(QStringList &newurls);
    static void appendUrl(QStringList &urls, const QString &url);
};

#endif // APPURL_H
