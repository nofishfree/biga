#pragma once

#include "UserConfig.h"

#include <QObject>

#include <memory>

class ConfigManager final : public QObject {
public:
    explicit ConfigManager(QObject* parent = nullptr);
    ~ConfigManager() override;

    // Reads the user configuration at most once per process.
    void loadOnce();

    UserConfig& config();
    const UserConfig& config() const;

    // Serializes a snapshot on a dedicated worker thread. All errors are caught
    // and logged inside this API; nothing is thrown to the caller.
    void SaveUserConfigAsync();

    // Blocks until every queued save has completed.
    void waitForPendingSaves();

private:
    class Writer;

    UserConfig m_config = UserConfig::defaults();
    bool m_loaded = false;
    std::unique_ptr<Writer> m_writer;
};
