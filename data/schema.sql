-- Stores the raw JSON snapshot of any Notion object we've fetched
CREATE TABLE IF NOT EXISTS pages (
    notion_id     TEXT PRIMARY KEY,
    type          TEXT NOT NULL,  -- 'page' or 'database'
    content       TEXT NOT NULL,  -- raw JSON blob from Notion API
    last_synced   TEXT NOT NULL   -- ISO 8601 timestamp
);

-- Individual tasks parsed out of the database rows
CREATE TABLE IF NOT EXISTS tasks (
    notion_id     TEXT PRIMARY KEY,
    title         TEXT NOT NULL,
    done          INTEGER NOT NULL DEFAULT 0,  -- 0 = false, 1 = true
    due_date      TEXT,                         -- nullable
    last_synced   TEXT NOT NULL
);

-- Key/value store for simple tracked values (season, date, etc.)
CREATE TABLE IF NOT EXISTS tracker (
    key           TEXT PRIMARY KEY,  -- e.g. 'current_season', 'current_day'
    value         TEXT NOT NULL,
    last_synced   TEXT NOT NULL
);

-- Queued writes to push back to Notion when online
CREATE TABLE IF NOT EXISTS pending_writes (
    id            INTEGER PRIMARY KEY AUTOINCREMENT,
    notion_id     TEXT NOT NULL,   -- which page/block to update
    operation     TEXT NOT NULL,   -- 'update_checkbox', 'update_property', etc.
    payload       TEXT NOT NULL,   -- JSON of what to send
    created_at    TEXT NOT NULL,
    retry_count   INTEGER NOT NULL DEFAULT 0
);