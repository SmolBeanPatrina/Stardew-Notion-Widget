#include "CacheStore.h"
#include <sstream>
#include <ctime>
#include <iomanip>

// ── Constructor ───────────────────────────────────────────────────────────────

CacheStore::CacheStore(const std::string& dbPath)
    : db_(dbPath, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE)
{
    db_.exec("PRAGMA journal_mode=WAL;");
    initSchema();
}

// ── Private helpers ───────────────────────────────────────────────────────────

void CacheStore::initSchema() {
    db_.exec(R"(
        CREATE TABLE IF NOT EXISTS pages (
            notion_id   TEXT PRIMARY KEY,
            type        TEXT NOT NULL,
            content     TEXT NOT NULL,
            last_synced TEXT NOT NULL
        );

        CREATE TABLE IF NOT EXISTS tasks (
            notion_id   TEXT PRIMARY KEY,
            title       TEXT NOT NULL,
            done        INTEGER NOT NULL DEFAULT 0,
            priority    INTEGER NOT NULL DEFAULT 0,
            reward      INTEGER NOT NULL DEFAULT 0,
            source      TEXT NOT NULL DEFAULT '',
            last_synced TEXT NOT NULL
        );

        CREATE TABLE IF NOT EXISTS tracker (
            key         TEXT PRIMARY KEY,
            value       TEXT NOT NULL,
            last_synced TEXT NOT NULL
        );

        CREATE TABLE IF NOT EXISTS pending_writes (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            notion_id   TEXT NOT NULL,
            operation   TEXT NOT NULL,
            payload     TEXT NOT NULL,
            created_at  TEXT NOT NULL,
            retry_count INTEGER NOT NULL DEFAULT 0
        );
    )");
}

std::string CacheStore::nowISO() {
    std::time_t now = std::time(nullptr);
    std::tm* utc    = std::gmtime(&now);
    std::ostringstream oss;
    oss << std::put_time(utc, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

// ── Tasks ─────────────────────────────────────────────────────────────────────

void CacheStore::upsertTask(const std::string& notionId,
                            const std::string& title,
                            bool               done,
                            int                priority,
                            int                reward,
                            const std::string& source) {
    SQLite::Statement stmt(db_, R"(
        INSERT INTO tasks (notion_id, title, done, priority, reward, source, last_synced)
        VALUES (?, ?, ?, ?, ?, ?, ?)
        ON CONFLICT(notion_id) DO UPDATE SET
            title       = excluded.title,
            done        = excluded.done,
            priority    = excluded.priority,
            reward      = excluded.reward,
            source      = excluded.source,
            last_synced = excluded.last_synced
    )");
    stmt.bind(1, notionId);
    stmt.bind(2, title);
    stmt.bind(3, done ? 1 : 0);
    stmt.bind(4, priority);
    stmt.bind(5, reward);
    stmt.bind(6, source);
    stmt.bind(7, nowISO());
    stmt.exec();
}

std::vector<Task> CacheStore::getTasks() {
    std::vector<Task> tasks;
    SQLite::Statement stmt(db_,
        "SELECT notion_id, title, done, priority, reward, source, last_synced "
        "FROM tasks ORDER BY priority DESC, title");

    while (stmt.executeStep()) {
        Task t;
        t.notionId   = stmt.getColumn(0).getString();
        t.title      = stmt.getColumn(1).getString();
        t.done       = stmt.getColumn(2).getInt() != 0;
        t.priority   = stmt.getColumn(3).getInt();
        t.reward     = stmt.getColumn(4).getInt();
        t.source     = stmt.getColumn(5).getString();
        t.lastSynced = stmt.getColumn(6).getString();
        tasks.push_back(t);
    }
    return tasks;
}

std::vector<Task> CacheStore::getTasksBySource(const std::string& source) {
    std::vector<Task> tasks;
    SQLite::Statement stmt(db_,
        "SELECT notion_id, title, done, priority, reward, source, last_synced "
        "FROM tasks WHERE source = ? ORDER BY priority DESC, title");
    stmt.bind(1, source);

    while (stmt.executeStep()) {
        Task t;
        t.notionId   = stmt.getColumn(0).getString();
        t.title      = stmt.getColumn(1).getString();
        t.done       = stmt.getColumn(2).getInt() != 0;
        t.priority   = stmt.getColumn(3).getInt();
        t.reward     = stmt.getColumn(4).getInt();
        t.source     = stmt.getColumn(5).getString();
        t.lastSynced = stmt.getColumn(6).getString();
        tasks.push_back(t);
    }
    return tasks;
}

void CacheStore::setTaskDone(const std::string& notionId, bool done) {
    SQLite::Statement stmt(db_,
        "UPDATE tasks SET done = ?, last_synced = ? WHERE notion_id = ?");
    stmt.bind(1, done ? 1 : 0);
    stmt.bind(2, nowISO());
    stmt.bind(3, notionId);
    stmt.exec();

    // Queue write back to Notion
    std::string payload = "{\"checked\":" + std::string(done ? "true" : "false") + "}";
    enqueueWrite(notionId, "update_checkbox", payload);
}

// ── Tracker ───────────────────────────────────────────────────────────────────

void CacheStore::setTrackerValue(const std::string& key, const std::string& value) {
    SQLite::Statement stmt(db_, R"(
        INSERT INTO tracker (key, value, last_synced)
        VALUES (?, ?, ?)
        ON CONFLICT(key) DO UPDATE SET
            value       = excluded.value,
            last_synced = excluded.last_synced
    )");
    stmt.bind(1, key);
    stmt.bind(2, value);
    stmt.bind(3, nowISO());
    stmt.exec();
}

std::string CacheStore::getTrackerValue(const std::string& key) {
    SQLite::Statement stmt(db_, "SELECT value FROM tracker WHERE key = ?");
    stmt.bind(1, key);
    if (stmt.executeStep()) {
        return stmt.getColumn(0).getString();
    }
    return "";
}

std::vector<TrackerEntry> CacheStore::getTrackerEntries() {
    std::vector<TrackerEntry> entries;
    SQLite::Statement stmt(db_, "SELECT key, value, last_synced FROM tracker");
    while (stmt.executeStep()) {
        TrackerEntry e;
        e.key        = stmt.getColumn(0).getString();
        e.value      = stmt.getColumn(1).getString();
        e.lastSynced = stmt.getColumn(2).getString();
        entries.push_back(e);
    }
    return entries;
}

// ── Raw page cache ────────────────────────────────────────────────────────────

void CacheStore::upsertPage(const std::string& notionId,
                            const std::string& type,
                            const std::string& contentJson) {
    SQLite::Statement stmt(db_, R"(
        INSERT INTO pages (notion_id, type, content, last_synced)
        VALUES (?, ?, ?, ?)
        ON CONFLICT(notion_id) DO UPDATE SET
            type        = excluded.type,
            content     = excluded.content,
            last_synced = excluded.last_synced
    )");
    stmt.bind(1, notionId);
    stmt.bind(2, type);
    stmt.bind(3, contentJson);
    stmt.bind(4, nowISO());
    stmt.exec();
}

// ── Write queue ───────────────────────────────────────────────────────────────

void CacheStore::enqueueWrite(const std::string& notionId,
                              const std::string& operation,
                              const std::string& payload) {
    SQLite::Statement stmt(db_, R"(
        INSERT INTO pending_writes (notion_id, operation, payload, created_at, retry_count)
        VALUES (?, ?, ?, ?, 0)
    )");
    stmt.bind(1, notionId);
    stmt.bind(2, operation);
    stmt.bind(3, payload);
    stmt.bind(4, nowISO());
    stmt.exec();
}

std::vector<PendingWrite> CacheStore::getPendingWrites() {
    std::vector<PendingWrite> writes;
    SQLite::Statement stmt(db_,
        "SELECT id, notion_id, operation, payload, created_at, retry_count "
        "FROM pending_writes ORDER BY created_at");
    while (stmt.executeStep()) {
        PendingWrite w;
        w.id         = stmt.getColumn(0).getInt();
        w.notionId   = stmt.getColumn(1).getString();
        w.operation  = stmt.getColumn(2).getString();
        w.payload    = stmt.getColumn(3).getString();
        w.createdAt  = stmt.getColumn(4).getString();
        w.retryCount = stmt.getColumn(5).getInt();
        writes.push_back(w);
    }
    return writes;
}

void CacheStore::deletePendingWrite(int id) {
    SQLite::Statement stmt(db_, "DELETE FROM pending_writes WHERE id = ?");
    stmt.bind(1, id);
    stmt.exec();
}

void CacheStore::incrementRetryCount(int id) {
    SQLite::Statement stmt(db_,
        "UPDATE pending_writes SET retry_count = retry_count + 1 WHERE id = ?");
    stmt.bind(1, id);
    stmt.exec();
}