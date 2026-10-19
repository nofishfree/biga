#include "ConfigManager.h"

#include "AppPaths.h"
#include "Log.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonParseError>

#include <condition_variable>
#include <exception>
#include <mutex>
#include <optional>
#include <thread>
#include <utility>

#include <windows.h>

class ConfigManager::Writer {
public:
    explicit Writer(QString path)
        : m_path(std::move(path))
        , m_thread([this] { run(); })
    {
    }

    ~Writer()
    {
        {
            const std::lock_guard<std::mutex> lock(m_mutex);
            m_stop = true;
        }
        m_condition.notify_all();

        try {
            if (m_thread.joinable()) {
                m_thread.join();
            }
        } catch (...) {
            try {
                m_thread.detach();
            } catch (...) {
            }
        }
    }

    void enqueue(const UserConfig& config)
    {
        {
            const std::lock_guard<std::mutex> lock(m_mutex);
            if (m_stop) {
                return;
            }
            // A save that has not started yet is superseded by the newest
            // snapshot. If a write is already running, this newest snapshot is
            // written immediately afterwards.
            m_pending = config;
        }
        m_condition.notify_all();
    }

    void waitForIdle()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_condition.wait(lock, [this] {
            return !m_pending.has_value() && !m_busy;
        });
    }

private:
    void run()
    {
        for (;;) {
            UserConfig config;
            {
                std::unique_lock<std::mutex> lock(m_mutex);
                m_condition.wait(lock, [this] {
                    return m_stop || m_pending.has_value();
                });

                if (!m_pending.has_value()) {
                    if (m_stop) {
                        return;
                    }
                    continue;
                }

                config = std::move(*m_pending);
                m_pending.reset();
                m_busy = true;
            }

            try {
                writeConfig(config);
            } catch (const std::exception& exception) {
                Log::error(QStringLiteral("保存用户配置时发生异常: %1")
                               .arg(QString::fromUtf8(exception.what())));
            } catch (...) {
                Log::error(QStringLiteral("保存用户配置时发生未知异常"));
            }

            {
                const std::lock_guard<std::mutex> lock(m_mutex);
                m_busy = false;
            }
            m_condition.notify_all();
        }
    }

    bool writeConfig(const UserConfig& config)
    {
        if (!AppPaths::ensureDataDir()) {
            Log::error(QStringLiteral("无法创建应用数据目录: %1")
                           .arg(AppPaths::dataDir()));
            return false;
        }

        const QByteArray bytes =
            QJsonDocument(config.toJson()).toJson(QJsonDocument::Indented) + '\n';

        const QString finalPath = m_path;
        const QString temporaryPath = finalPath + QStringLiteral(".tmp");

        QFile file(temporaryPath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            Log::error(QStringLiteral("无法写入配置临时文件 %1: %2")
                           .arg(temporaryPath, file.errorString()));
            return false;
        }

        const qint64 written = file.write(bytes);
        if (written != bytes.size()) {
            const QString error = file.errorString();
            file.close();
            QFile::remove(temporaryPath);
            Log::error(QStringLiteral("写入配置临时文件不完整 %1: %2")
                           .arg(temporaryPath, error));
            return false;
        }

        if (!file.flush()) {
            const QString error = file.errorString();
            file.close();
            QFile::remove(temporaryPath);
            Log::error(QStringLiteral("刷新配置临时文件失败 %1: %2")
                           .arg(temporaryPath, error));
            return false;
        }
        file.close();

        const std::wstring nativeTemporary =
            QDir::toNativeSeparators(temporaryPath).toStdWString();
        const std::wstring nativeFinal =
            QDir::toNativeSeparators(finalPath).toStdWString();

        if (!MoveFileExW(nativeTemporary.c_str(), nativeFinal.c_str(),
                         MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
            const DWORD error = GetLastError();
            QFile::remove(temporaryPath);
            Log::error(QStringLiteral("原子替换用户配置失败 %1 (Win32 %2)")
                           .arg(finalPath)
                           .arg(static_cast<qulonglong>(error)));
            return false;
        }

        return true;
    }

    QString m_path;
    std::mutex m_mutex;
    std::condition_variable m_condition;
    std::optional<UserConfig> m_pending;
    bool m_busy = false;
    bool m_stop = false;
    std::thread m_thread;
};

ConfigManager::ConfigManager(QObject* parent)
    : QObject(parent)
    , m_config(UserConfig::defaults())
    , m_writer(std::make_unique<Writer>(AppPaths::configFilePath()))
{
}

ConfigManager::~ConfigManager() = default;

void ConfigManager::loadOnce()
{
    if (m_loaded) {
        return;
    }
    m_loaded = true;

    const QString path = AppPaths::configFilePath();
    QFile file(path);
    if (!file.exists()) {
        Log::info(QStringLiteral("用户配置不存在，使用默认配置: %1").arg(path));
        return;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        Log::warning(QStringLiteral("无法读取用户配置，使用默认配置: %1 (%2)")
                         .arg(path, file.errorString()));
        return;
    }

    QJsonParseError parseError{};
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(),
                                                           &parseError);
    file.close();

    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        Log::warning(QStringLiteral("用户配置解析失败，使用默认配置: %1 (%2)")
                         .arg(path, parseError.errorString()));
        return;
    }

    m_config = UserConfig::fromJson(document.object());
    Log::info(QStringLiteral("已读取用户配置: %1").arg(path));
}

UserConfig& ConfigManager::config()
{
    return m_config;
}

const UserConfig& ConfigManager::config() const
{
    return m_config;
}

void ConfigManager::SaveUserConfigAsync()
{
    try {
        m_writer->enqueue(m_config);
    } catch (const std::exception& exception) {
        Log::error(QStringLiteral("排入用户配置保存任务失败: %1")
                       .arg(QString::fromUtf8(exception.what())));
    } catch (...) {
        Log::error(QStringLiteral("排入用户配置保存任务失败: 未知异常"));
    }
}

void ConfigManager::waitForPendingSaves()
{
    try {
        m_writer->waitForIdle();
    } catch (const std::exception& exception) {
        Log::error(QStringLiteral("等待用户配置保存结束失败: %1")
                       .arg(QString::fromUtf8(exception.what())));
    } catch (...) {
        Log::error(QStringLiteral("等待用户配置保存结束失败: 未知异常"));
    }
}
