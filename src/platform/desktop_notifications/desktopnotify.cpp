/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright © 2019 by The qTox Project Contributors
 * Copyright © 2024 The TokTok team.
 */

#include "desktopnotify.h"

#include "src/persistence/settings.h"

#include <QSystemTrayIcon>

DesktopNotify::DesktopNotify(Settings& settings, QSystemTrayIcon* icon)
    : settings_{settings}
    , icon_{icon}
{
    if (icon_) {
        connect(icon_, &QSystemTrayIcon::messageClicked, this, &DesktopNotify::notificationClosed);
    }
}

DesktopNotify::~DesktopNotify() = default;

void DesktopNotify::notifyMessage(const NotificationData& notificationData)
{
    if (!(settings_.getNotify() && settings_.getDesktopNotify())) {
        return;
    }

    if (icon_) {
        icon_->showMessage(notificationData.title, notificationData.message, notificationData.pixmap);
    }
}
