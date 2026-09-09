#pragma once
#include "CacheStore.h"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

// Describes one of your child databases on the main page
struct DatabaseInfo {
    std::string id;    // Notion ID with dashes
    std::string name;  // e.g. "Miscellaneous Tasks"
};

// Parses a Notion database query response and returns a list of Tasks
std::vector<Task> parseTaskDatabase(const nlohmann::json& queryResponse,
                                    const std::string& sourceName);

// Extracts the list of child_database blocks from a page's block children response
std::vector<DatabaseInfo> parseChildDatabases(const nlohmann::json& blocksResponse);

// Helper: strips dashes from a Notion ID for use in API URLs
std::string stripDashes(const std::string& id);