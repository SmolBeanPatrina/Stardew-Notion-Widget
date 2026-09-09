#include "NotionParser.h"
#include <algorithm>
#include <iostream>

// ── Helpers ───────────────────────────────────────────────────────────────────

std::string stripDashes(const std::string& id) {
    std::string result;
    result.reserve(id.size());
    for (char c : id) {
        if (c != '-') result += c;
    }
    return result;
}

// Safe JSON navigation — returns default if path doesn't exist
static bool safeGetBool(const nlohmann::json& j,
                        std::initializer_list<std::string> path,
                        bool defaultVal = false) {
    const nlohmann::json* cur = &j;
    for (auto& key : path) {
        if (!cur->is_object() || !cur->contains(key)) return defaultVal;
        cur = &(*cur)[key];
    }
    return cur->is_boolean() ? cur->get<bool>() : defaultVal;
}

static int safeGetInt(const nlohmann::json& j,
                      std::initializer_list<std::string> path,
                      int defaultVal = 0) {
    const nlohmann::json* cur = &j;
    for (auto& key : path) {
        if (!cur->is_object() || !cur->contains(key)) return defaultVal;
        cur = &(*cur)[key];
    }
    if (cur->is_null())   return defaultVal;
    if (cur->is_number()) return cur->get<int>();
    return defaultVal;
}

static std::string safeGetString(const nlohmann::json& j,
                                 std::initializer_list<std::string> path,
                                 const std::string& defaultVal = "") {
    const nlohmann::json* cur = &j;
    for (auto& key : path) {
        if (!cur->is_object() || !cur->contains(key)) return defaultVal;
        cur = &(*cur)[key];
    }
    return cur->is_string() ? cur->get<std::string>() : defaultVal;
}

// ── Child database parser ─────────────────────────────────────────────────────

std::vector<DatabaseInfo> parseChildDatabases(const nlohmann::json& blocksResponse) {
    std::vector<DatabaseInfo> databases;

    if (!blocksResponse.contains("results")) return databases;

    for (auto& block : blocksResponse["results"]) {
        if (!block.contains("type")) continue;
        if (block["type"] != "child_database") continue;

        DatabaseInfo info;
        info.id   = safeGetString(block, {"id"});
        info.name = safeGetString(block, {"child_database", "title"});

        if (!info.id.empty()) {
            databases.push_back(info);
        }
    }

    return databases;
}

// ── Task database parser ──────────────────────────────────────────────────────

std::vector<Task> parseTaskDatabase(const nlohmann::json& queryResponse,
                                    const std::string& sourceName) {
    std::vector<Task> tasks;

    if (!queryResponse.contains("results")) return tasks;

    for (auto& page : queryResponse["results"]) {
        // Skip archived or trashed rows
        if (safeGetBool(page, {"in_trash"}))   continue;
        if (safeGetBool(page, {"is_archived"})) continue;

        if (!page.contains("properties")) continue;
        auto& props = page["properties"];

        Task t;
        t.notionId = safeGetString(page, {"id"});
        t.source   = sourceName;
        t.done     = false; // Notion doesn't have a checkbox here; completion is tracked locally

        // ── Task Name (title property) ────────────────────────────────────────
        if (props.contains("Task Name")) {
            auto& titleProp = props["Task Name"];
            if (titleProp.contains("title") && titleProp["title"].is_array()
                && !titleProp["title"].empty()) {
                t.title = safeGetString(titleProp["title"][0], {"plain_text"});
            }
        }
        if (t.title.empty()) t.title = "(Untitled)";

        // ── Priority (1-10) ───────────────────────────────────────────────────
        if (props.contains("Priority (1-10)")) {
            t.priority = safeGetInt(props, {"Priority (1-10)", "number"}, 0);
        }

        // ── Reward ────────────────────────────────────────────────────────────
        if (props.contains("Reward")) {
            t.reward = safeGetInt(props, {"Reward", "number"}, 0);
        }

        t.lastSynced = ""; // filled in by CacheStore on upsert

        if (!t.notionId.empty()) {
            tasks.push_back(t);
        }
    }

    return tasks;
}