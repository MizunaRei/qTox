/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright © 2014-2019 by The qTox Project Contributors
 * Copyright © 2024 The TokTok team.
 */
#include "src/net/updatecheck.h"

#include "src/persistence/settings.h"
#ifndef CMAKE_BUILD
#include "src/version.h"
#endif

#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QRegularExpression>
#include <QTimer>
#include <cassert>

#ifdef UPDATE_CHECK_ENABLED
namespace {
const QString versionUrl{
    QStringLiteral("https://api.github.com/repos/TokTok/qTox/releases/latest")};
// Release candidates are ignored, as they are prereleases and don't appear in
// the response to the releases/latest API call.
const QString versionRegexString{QStringLiteral(R"(v([0-9]+)\.([0-9]+)\.([0-9]+))")};

struct Version
{
    int major;
    int minor;
    int patch;
};

Version tagToVersion(const QString& tagName)
{
    // capture tag name to avoid showing update available on dev builds which include hash as part of describe
    QRegularExpression versionFormat(versionRegexString);
    auto matches = versionFormat.match(tagName);
    assert(matches.lastCapturedIndex() == 3);

    bool ok;
    const auto major = matches.captured(1).toInt(&ok);
    assert(ok);
    const auto minor = matches.captured(2).toInt(&ok);
    assert(ok);
    const auto patch = matches.captured(3).toInt(&ok);
    assert(ok);

    return {major, minor, patch};
}

bool isUpdateAvailable(const Version& current, const Version& available)
{
    // A user may have a version greater than our latest release in the time between a tag being
    // pushed and the release being published. Don't notify about an update in that case.

    if (current.major < available.major) {
        return true;
    }
    if (current.major > available.major) {
        return false;
    }

    if (current.minor < available.minor) {
        return true;
    }
    if (current.minor > available.minor) {
        return false;
    }

    if (current.patch < available.patch) {
        return true;
    }
    if (current.patch > available.patch) {
        return false;
    }

    return false;
}

bool isCurrentVersionStable()
{
    const QRegularExpression versionRegex(versionRegexString);
    auto currentVer = versionRegex.match(GIT_DESCRIBE_EXACT);
    if (currentVer.hasMatch()) {
        return true;
    } else {
        return false;
    }
}

} // namespace
#endif

UpdateCheck::UpdateCheck(const Settings& settings_)
    : settings(settings_)
{
    qInfo() << "qTox is running version:" << GIT_DESCRIBE;
#ifdef UPDATE_CHECK_ENABLED
    updateTimer.start(1000 * 60 * 60 * 24 /* 1 day */);
    connect(&updateTimer, &QTimer::timeout, this, &UpdateCheck::checkForUpdate);
    connect(&manager, &QNetworkAccessManager::finished, this, &UpdateCheck::handleResponse);
#endif
}

void UpdateCheck::checkForUpdate()
{
#ifdef UPDATE_CHECK_ENABLED
    if (!settings.getCheckUpdates()) {
        // still run the timer to check periodically incase setting changes
        return;
    }

    if (isCurrentVersionStable() == false) {
        qWarning() << "Currently running an untested/unstable version of qTox";
        emit versionIsUnstable();
        return;
    }

    manager.setProxy(settings.getProxy());
    QNetworkRequest request{versionUrl};
    manager.get(request);
#endif
}

void UpdateCheck::handleResponse(QNetworkReply* reply)
{
    assert(reply != nullptr);
    if (reply == nullptr) {
        qWarning() << "Update check returned null reply, ignoring";
        return;
    }

#ifdef UPDATE_CHECK_ENABLED
    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "Failed to check for update:" << reply->error();
        emit updateCheckFailed();
        reply->deleteLater();
        return;
    }
    const QByteArray result = reply->readAll();
    const QJsonDocument doc = QJsonDocument::fromJson(result);
    const QJsonObject jObject = doc.object();
    const QVariantMap mainMap = jObject.toVariantMap();
    const QString latestVersion = mainMap["tag_name"].toString();
    if (latestVersion.isEmpty()) {
        qWarning() << "No tag name found in response:";
        emit updateCheckFailed();
        reply->deleteLater();
        return;
    }

    const auto currentVer = tagToVersion(GIT_DESCRIBE);
    const auto availableVer = tagToVersion(latestVersion);

    if (isUpdateAvailable(currentVer, availableVer)) {
        qInfo() << "Update available to version" << latestVersion;
        const QUrl link{mainMap["html_url"].toString()};
        emit updateAvailable(latestVersion, link);
    } else {
        qInfo() << "qTox is up to date";
        emit upToDate();
    }

    reply->deleteLater();
#endif
}
