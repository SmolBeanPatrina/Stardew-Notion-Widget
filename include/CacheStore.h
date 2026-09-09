#pragma once
#include <string>
#include <vector>
#include <SQLiteCpp/SQLiteCpp.h>

// ── Data structs ──────────────────────────────────────────────────────────────

struct Task {
    std::string notionId;
    std::string title;
    bool        done;
    int         priority;   // 1-10, 0 if not set
    int         reward;     // time value, 0 if not set
    std::string source;     // database name e.g. "Miscellaneous Tasks"
    std::string lastSynced;
};

struct TrackerEntry {
    std::string key;
    std::string value;
    std::string lastSynced;
};

struct PendingWrite {
    int         id;
    std::string notionId;
    std::string operation;  // "update_checkbox", "update_property"
    std::string payload;    // JSON string
    std::string createdAt;
    int         retryCount;
};

// ── CacheStore ────────────────────────────────────────────────────────────────

class CacheStore {
public:
    explicit CacheStore(const std::string& dbPath);

    // ── Tasks ─────────────────────────────────────────────────────────────────
    void upsertTask(const std::string& notionId,
                    const std::string& title,
                    bool               done,
                    int                priority,
                    int                reward,
                    const std::string& source);

    std::vector<Task> getTasks();
    std::vector<Task> getTasksBySource(const std::string& source);
    void setTaskDone(const std::string& notionId, bool done);

    // ── Tracker ───────────────────────────────────────────────────────────────
    void setTrackerValue(const std::string& key, const std::string& value);
    std::string getTrackerValue(const std::string& key);
    std::vector<TrackerEntry> getTrackerEntries();

    // ── Raw page cache ────────────────────────────────────────────────────────
    void upsertPage(const std::string& notionId,
                    const std::string& type,
                    const std::string& contentJson);

    // ── Write queue ───────────────────────────────────────────────────────────
    void enqueueWrite(const std::string& notionId,
                      const std::string& operation,
                      const std::string& payload);

    std::vector<PendingWrite> getPendingWrites();
    void deletePendingWrite(int id);
    void incrementRetryCount(int id);

private:
    SQLite::Database db_;
    void initSchema();
    std::string nowISO();
};