#pragma once

#include <QString>
#include <QStringList>
#include <QVariantMap>

namespace UiLocale {

QString normalize(const QString &language);
QStringList codes();
QVariantMap strings(const QString &language);
QString t(const QString &language, const QString &key);

} // namespace UiLocale
