#include <iostream>
#include <nlohmann/json.hpp>
#ifdef CPPHTTPLIB_USE_NON_BLOCKING_GETADDRINFO
#undef CPPHTTPLIB_USE_NON_BLOCKING_GETADDRINFO
#endif
#include <httplib.h>
#include <SQLiteCpp/SQLiteCpp.h>
#include "env.hpp"

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
    return 0;
}