/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright © 2017-2019 by The qTox Project Contributors
 * Copyright © 2024 The TokTok team.
 */

#pragma once

#include <QString>

namespace TextFormatter
{
    QString highlightURI(const QString& message);

    QString applyMarkdown(const QString& message, bool showFormattingSymbols);
} // namespace TextFormatter
