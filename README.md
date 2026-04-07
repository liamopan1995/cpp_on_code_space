环境准备： prepare/install_openspec.h 
.
├── README.md
├── openspec/                  # ← 不要动这里，放规格用
│   ├── config.yaml
│   ├── changes/
│   ├── specs/                 # 当前系统的「单一事实来源」
│   └── ... (archive 等)
├── prepare/
│   └── install_openspec.sh
│
├── src/                       # ← 你的主要源码
│   ├── main.cpp
│   ├── module1/
│   │   ├── foo.cpp
│   │   └── foo.h
│   └── ...
│
├── include/                   # ← 公开的头文件（可选，按需）
│   └── myproject/
│
├── tests/                     # ← 测试用例（强烈推荐）
│   ├── unit/
│   ├── integration/
│   └── CMakeLists.txt
│
├── CMakeLists.txt             # 构建系统
├── CMakePresets.json
├── .gitignore
└── ... (其他配置文件)