#include <iostream>
#include <nlohmann/json.hpp>
#ifdef CPPHTTPLIB_USE_NON_BLOCKING_GETADDRINFO
#undef CPPHTTPLIB_USE_NON_BLOCKING_GETADDRINFO
#endif
#include <httplib.h>
#include <SQLiteCpp/SQLiteCpp.h>
#include "env.hpp"
#include "CacheStore.h"
#include "NotionParser.h"

void createOverlayWindow();


int main() {

    const std::string TOKEN = getToken("NOTION_TOKEN");
    const std::string PAGE_ID = getToken("TEST_PAGE_32ID");

    httplib::SSLClient cli("api.notion.com");
    cli.set_connection_timeout(5);

    httplib::Headers headers = {
        {"Authorization",  "Bearer " + TOKEN},
        {"Notion-Version", "2022-06-28"},
        {"Content-Type",   "application/json"}
    };

    auto res = cli.Get("/v1/pages/" + PAGE_ID, headers);

    if (!res) {
        std::cerr << "Request failed: " << res.error() << "\n";
        return 1;
    }

    std::cout << "Status: " << res->status << "\n";

    auto json = nlohmann::json::parse(res->body);
    std::cout << json.dump(2) << "\n"; // pretty-print

    //Initialize db?
    // SQLite::Database db("data/stardew.db", SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
    // db.exec(/* contents of schema.sql */);

    //createOverlayWindow();

    //TestCacheStore
    // CacheStore cache("data/stardew.db");
    // cache.upsertTask("test-id-001", "Water crops", false, "");
    // cache.upsertTask("test-id-002", "Check mail", false, "");
    // cache.setTrackerValue("current_season", "Spring");
    // cache.setTrackerValue("current_day", "Day 1");

    // auto tasks = cache.getTasks();
    // for (auto& t: tasks) {
    //     std::cout << "[" << (t.done ? "x" : " ") << "] " << t.title << "\n";
    // }

    // std::cout << "Season: " << cache.getTrackerValue("current_season") << "\n";

    //See Notion Blocks
    // std::string pageId = getToken("TEST_PAGE_32ID");

    auto blocks = cli.Get(("/v1/blocks/" + PAGE_ID + "/children").c_str(), headers);
    if (blocks && blocks->status == 200) {
        auto blockJson = nlohmann::json::parse(blocks->body);
        std::cout << blockJson.dump(2) << "\n";
    }

    auto dbQuery = cli.Post(
        "/v1/databases/38b8261266038060b3a4c848170f1cdd/query",
        headers,
        "{}", "application/json"
    );
    if (dbQuery && dbQuery->status == 200) {
        std::cout << nlohmann::json::parse(dbQuery->body).dump(2) << "\n";
    } else {
        std::cerr << "DB query failed: " << (dbQuery ? dbQuery->status : -1) << "\n";
        if (dbQuery) std::cerr << dbQuery->body << "\n";
    }

    auto databases = parseChildDatabases(nlohmann::json::parse(blocks->body));

    CacheStore cache("data/stardew.db");

    for (auto& db : databases) {
        auto queryRes = cli.Post(
            ("/v1/databases/" + stripDashes(db.id) + "/query").c_str(),
            headers, "{}", "application/json"
        );
        if (!queryRes || queryRes->status != 200) continue;

        auto tasks = parseTaskDatabase(
            nlohmann::json::parse(queryRes->body), db.name
        );

        for (auto& t : tasks) {
            cache.upsertTask(t.notionId, t.title, t.done,
                            t.priority, t.reward, t.source);
        }
    }

    // Print what's in the cache
    for (auto& t : cache.getTasks()) {
        std::cout << "[" << t.source << "] "
                << t.title
                << " | Priority: " << t.priority
                << " | Reward: " << t.reward << "\n";
    }

    return 0;
}