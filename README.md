# Stardew-Notion-Widget

Current Progress:
- CMake build system is fully configured
- Notion API queries live data from all five embedded databases
- SQLite cache stores and retrieves parsed tasks with priority and reward fields
- Win32 overlay window renders on screen

In progress: 
- the background sync engine (polling thread and write queue flush) and the overlay UI rendering real task data with Stardew-style theming using Direct2D

Updated September 15, 2026

<!--
Folder Layout:

notion-overlay/
├── CMakeLists.txt
├── src/
│   ├── main.cpp
│   ├── overlay/       # Win32 window code
│   ├── sync/          # Notion API + sync engine (Phase 3)
│   └── cache/         # SQLite wrapper (Phase 2)
├── include/           # your own headers
├── third_party/       # vendored libs go here
└── data/              # SQLite DB file at runtime
-->

