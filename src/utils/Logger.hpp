#pragma once

#include <QString>

namespace chainx {

/**
 * Minimal application logger.
 *
 * Messages are written to both stderr and a rotating-in-place log file in the
 * application data directory. Installed as the Qt message handler so that
 * qDebug/qWarning/qCritical output is captured as well.
 */
class Logger
{
public:
    Logger() = delete;

    /// Installs the message handler. Safe to call once at startup.
    static void install(const QString& logFilePath);

    /// Restores the previous Qt message handler.
    static void shutdown();

    static QString logFilePath();

private:
    static void handleMessage(QtMsgType type,
                              const QMessageLogContext& context,
                              const QString& message);
};

} // namespace chainx
