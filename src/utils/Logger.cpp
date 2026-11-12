#include "utils/Logger.hpp"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMutex>
#include <QMutexLocker>
#include <QTextStream>

#include <cstdio>
#include <cstdlib>

namespace chainx {
namespace {

QFile* g_logFile = nullptr;
QMutex g_mutex;
QString g_logPath;
QtMessageHandler g_previousHandler = nullptr;

const char* levelName(QtMsgType type)
{
    switch (type) {
    case QtDebugMsg:
        return "DEBUG";
    case QtInfoMsg:
        return "INFO";
    case QtWarningMsg:
        return "WARN";
    case QtCriticalMsg:
        return "ERROR";
    case QtFatalMsg:
        return "FATAL";
    }
    return "LOG";
}

} // namespace

void Logger::install(const QString& logFilePath)
{
    QMutexLocker locker(&g_mutex);

    g_logPath = logFilePath;
    if (!g_logPath.isEmpty()) {
        const QFileInfo info(g_logPath);
        QDir().mkpath(info.absolutePath());

        auto* file = new QFile(g_logPath);
        if (file->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
            g_logFile = file;
        } else {
            std::fprintf(stderr, "ChainX: unable to open log file %s\n",
                         qPrintable(g_logPath));
            delete file;
        }
    }

    g_previousHandler = qInstallMessageHandler(&Logger::handleMessage);
}

void Logger::shutdown()
{
    qInstallMessageHandler(g_previousHandler);
    g_previousHandler = nullptr;

    QMutexLocker locker(&g_mutex);
    if (g_logFile) {
        g_logFile->flush();
        g_logFile->close();
        delete g_logFile;
        g_logFile = nullptr;
    }
}

QString Logger::logFilePath()
{
    QMutexLocker locker(&g_mutex);
    return g_logPath;
}

void Logger::handleMessage(QtMsgType type,
                           const QMessageLogContext& context,
                           const QString& message)
{
    const QString timestamp =
        QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz"));

    QString line = QStringLiteral("%1 [%2] %3")
                       .arg(timestamp, QString::fromLatin1(levelName(type)), message);
    if (context.file && context.line > 0) {
        line += QStringLiteral(" (%1:%2)")
                    .arg(QString::fromUtf8(context.file))
                    .arg(context.line);
    }

    {
        QMutexLocker locker(&g_mutex);
        if (g_logFile) {
            QTextStream stream(g_logFile);
            stream << line << '\n';
            stream.flush();
        }
    }

    std::fprintf(stderr, "%s\n", qPrintable(line));

    if (type == QtFatalMsg) {
        std::abort();
    }
}

} // namespace chainx
