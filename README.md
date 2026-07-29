# COMP315 Final Phase — Group Twelve Software Solutions
# Group Memebers:
Member Name	- Student ID
Yahya Bobat	- 223020664
Jawad Kachbal -	223037045
Yusuf Sheikh -	224068106
Mahomed Valli Mahomed -	224005878
Muhammad Varachia	- 224004509
Kiyan Krishna -	224115592
Andiswa Hlengwa - 221002430

**Project Name:** Nexus Logistics Inventory Management System
**Module:** COMP315
**Group:** Group Twelve Software Solutions

---

## ⚠️ Important Notices

### Primary Submission — CLI Application
> **The Command Line Interface (CLI) application is the main and official submission for this project.** All core functionality, features, and grading criteria are implemented and demonstrated through the CLI. Please refer to the CLI when evaluating the submission.

### Bonus — GUI Application
> **The Graphical User Interface (GUI) is a bonus component and is incomplete.** It was developed as an additional effort beyond the project requirements. It may contain missing features, visual bugs, or incomplete functionality. It is provided as-is and should **not** be used as the basis for project assessment.
> <img width="2556" height="1367" alt="Screenshot 2026-07-29 184027" src="https://github.com/user-attachments/assets/930bba7f-fdd5-4e2d-9db5-416a0a87b896" />
<img width="1459" height="929" alt="Screenshot 2026-07-29 184105" src="https://github.com/user-attachments/assets/89d24f6a-e24d-4a19-9f08-818467f71e01" />
<img width="2546" height="1366" alt="Screenshot 2026-07-29 184119" src="https://github.com/user-attachments/assets/651f600e-a20b-45be-bb48-30952063f498" />
<img width="2545" height="1360" alt="Screenshot 2026-07-29 184129" src="https://github.com/user-attachments/assets/cf3f4eea-62bc-4c43-8ac7-c7855c120037" />
<img width="2554" height="1366" alt="Screenshot 2026-07-29 184158" src="https://github.com/user-attachments/assets/2a35cc6d-2530-41f6-94da-e4db83519c2d" />
<img width="2556" height="1357" alt="Screenshot 2026-07-29 184206" src="https://github.com/user-attachments/assets/f41b7d4c-c62a-4a10-935e-8a92b2c56b5c" />
<img width="2559" height="1394" alt="Screenshot 2026-07-29 184219" src="https://github.com/user-attachments/assets/4a0f61f7-3ab6-4fd5-9606-b91372c2021b" />
<img width="2550" height="1362" alt="Screenshot 2026-07-29 184237" src="https://github.com/user-attachments/assets/7064b95a-c687-4460-bcc9-07d1e106d4ad" />
<img width="2555" height="1357" alt="Screenshot 2026-07-29 184246" src="https://github.com/user-attachments/assets/8e55d266-282f-4e35-9cf9-f308afaf16bf" />
<img width="2559" height="1380" alt="Screenshot 2026-07-29 184259" src="https://github.com/user-attachments/assets/25b6c022-a28d-4d0c-8573-d78489775a3d" />


### AI Usage Disclaimer
> Portions of this project made use of AI-assisted tools (such as GitHub Copilot or similar) during development, primarily for code suggestions, boilerplate generation, and debugging assistance. All code was reviewed, understood, and integrated by the group members. The design decisions, logic, and overall architecture remain the work of Group Twelve Software Solutions.

---

## Folder Structure

```
Group Twelve Software Solutions COMP315 Final Phase/
└── COMP315 Final Submission/
    ├── 1. FinalReport/
    │   └── Group Twelve Software Solutions LTD Final Report.pdf
    │
    ├── 2. Final Source Code/
    │   ├── CLI Source Code (Main Submission)/
    │   │   └── COMP315 Project Final/          ← CodeBlocks project
    │   │       ├── COMP315 Project.cbp          ← CodeBlocks project file
    │   │       ├── main.cpp
    │   │       ├── InventorySystem.cpp / .h
    │   │       ├── AnalyticsManager.cpp / .h
    │   │       ├── AnalyticsRecord.cpp / .h
    │   │       ├── Product.cpp / .h
    │   │       ├── StandardProduct.cpp / .h
    │   │       ├── DiscountedProduct.cpp / .h
    │   │       ├── TaxableProduct.cpp / .h
    │   │       ├── Warehouse.cpp / .h
    │   │       ├── WorkerThread.cpp / .h
    │   │       ├── Order.cpp / .h
    │   │       ├── OrderPool.cpp / .h
    │   │       ├── Logger.h
    │   │       ├── Catalog/
    │   │       │   ├── Products.csv
    │   │       │   └── Updated_Products.csv
    │   │       ├── Orders/
    │   │       │   ├── Warehouse1_Orders.csv
    │   │       │   ├── Warehouse2_Orders.csv
    │   │       │   ├── Warehouse3_Orders.csv
    │   │       │   ├── Warehouse4_Orders.csv
    │   │       │   └── Warehouse5_Orders.csv
    │   │       ├── Analytics/
    │   │       │   ├── Global/GlobalItemRecords.csv
    │   │       │   └── Warehouses/
    │   │       │       ├── Warehouse 1/Warehouse1ItemRecords.csv
    │   │       │       ├── Warehouse 2/Warehouse2ItemRecords.csv
    │   │       │       ├── Warehouse 3/Warehouse3ItemRecords.csv
    │   │       │       ├── Warehouse 4/Warehouse4ItemRecords.csv
    │   │       │       └── Warehouse 5/Warehouse5ItemRecords.csv
    │   │       ├── bin/
    │   │       │   ├── Debug/COMP315 Project.exe
    │   │       │   └── Release/COMP315 Project.exe  ← Compiled output
    │   │       └── obj/                             ← Intermediate build objects
    │   │
    │   └── BONUS - GUI/                             ← ⚠️ BONUS — INCOMPLETE
    │       └── NexusLogisticsGUI Final Source Code/
    │           └── SourceCode/                      ← Qt Creator project
    │               ├── CMakeLists.txt               ← Qt CMake project file
    │               ├── main.cpp
    │               ├── final_project_v2/            ← Shared CLI backend code
    │               ├── src/
    │               │   ├── integration/             ← Service adapters / app controller
    │               │   ├── models/                  ← DTO model headers
    │               │   └── ui/                      ← Qt UI pages (.cpp / .h / .ui)
    │               │       ├── mainwindow
    │               │       ├── dashboardpage
    │               │       ├── inventorypage
    │               │       ├── analyticspage
    │               │       ├── orderspage
    │               │       ├── logspage
    │               │       ├── settingspage
    │               │       └── navbutton
    │               ├── assets/
    │               │   ├── icons/                   ← Navigation & product icons
    │               │   ├── card_patterns/           ← Dashboard card background images
    │               │   └── logo/                    ← Application logo (.png / .ico)
    │               └── resources/
    │                   ├── resources.qrc            ← Qt resource file
    │                   ├── style.qss                ← Application stylesheet
    │                   └── windows_app_icon.rc
    │
    └── 3. Final Compiled Project exes/
        ├── 1. COMP315 Project Compiled (Main Submission)/   ← ✅ RUN THIS
        │   ├── RUN ME COMP315 Project.exe           ← Double-click to launch
        │   ├── libgcc_s_seh-1.dll
        │   ├── libstdc++-6.dll
        │   ├── libwinpthread-1.dll
        │   ├── Catalog/
        │   ├── Orders/
        │   └── Analytics/
        │
        └── 2. BONUS - GUI/                          ← ⚠️ BONUS — INCOMPLETE
            └── NexusLogistics GUI Final/
                ├── Run NexusLogisticsGUI Final.bat  ← Double-click to launch GUI
                └── Build/
                    ├── NexusLogistics.exe
                    ├── Qt6Core.dll / Qt6Gui.dll / Qt6Widgets.dll / Qt6Charts.dll
                    ├── libgcc_s_seh-1.dll / libstdc++-6.dll / libwinpthread-1.dll
                    ├── platforms/qwindows.dll
                    ├── imageformats/ / tls/ / styles/ / translations/
                    └── final_project_v2/            ← Data files used at runtime
```

---

## How to Run

### Option 1 — Pre-Compiled CLI (Recommended, Main Submission)

This is the easiest way to run the project and is the **primary submission**.

1. Navigate to:
   ```
   3. Final Compiled Project exes/
   └── 1. COMP315 Project Compiled (Main Submission)/
   ```
2. Double-click **`RUN ME COMP315 Project.exe`**.
3. The CLI will launch in a terminal window. Follow the on-screen menu prompts to interact with the system.

> **Important:** Do not move or rename the `Catalog/`, `Orders/`, or `Analytics/` folders that sit alongside the `.exe`. The application reads from and writes to these CSV data files at runtime.

---

### Option 2 — Build CLI from Source (CodeBlocks)

Use this if you want to compile the CLI project yourself.

**Requirements:** [Code::Blocks](https://www.codeblocks.org/) with a MinGW GCC compiler (C++17 or later).

1. Navigate to:
   ```
   2. Final Source Code/CLI Source Code (Main Submission)/COMP315 Project Final/
   ```
2. Open **`COMP315 Project.cbp`** in Code::Blocks.
3. Select your build target (**Debug** or **Release**) from the toolbar.
4. Click **Build → Build** (or press `Ctrl+F9`).
5. The compiled executable will appear in `bin/Debug/` or `bin/Release/`.
6. Run the executable from within Code::Blocks or by navigating to the `bin/` output folder. Ensure the `Catalog/`, `Orders/`, and `Analytics/` folders are in the **same directory as the executable** when running outside the IDE.

---

### Option 3 — Run Pre-Compiled GUI ⚠️ BONUS ONLY

> **Note:** The GUI is a bonus component and is incomplete. It is provided for demonstration purposes only and is not the primary submission.

1. Navigate to:
   ```
   3. Final Compiled Project exes/
   └── 2. BONUS - GUI/NexusLogistics GUI Final/
   ```
2. Double-click **`Run NexusLogisticsGUI Final.bat`**.
   - This batch file launches `Build\NexusLogistics.exe` with all required Qt `.dll` files already bundled in the `Build/` folder.
3. The NexusLogistics GUI window will open.

> **Do not** move the `.exe` out of the `Build/` folder — it depends on all the Qt `.dll` files present there.

---

### Option 4 — Build GUI from Source (Qt Creator) ⚠️ BONUS ONLY

> **Note:** The GUI is a bonus component and is incomplete.

**Requirements:** [Qt Creator](https://www.qt.io/product/development-tools) with Qt 6 (including Qt Charts) and a MinGW or MSVC compiler.

1. Navigate to:
   ```
   2. Final Source Code/BONUS - GUI/NexusLogisticsGUI Final Source Code/SourceCode/
   ```
2. Open **`CMakeLists.txt`** in Qt Creator (`File → Open File or Project`).
3. Configure the project by selecting your installed Qt 6 kit when prompted.
4. Click **Build → Build All** (or press `Ctrl+B`).
5. Run the project from Qt Creator using the green play button.

---

## Source Code Overview

### CLI — Core Classes

| File | Description |
|---|---|
| `main.cpp` | Application entry point and main menu loop |
| `InventorySystem.cpp/.h` | Central system managing warehouses, products, and operations |
| `Warehouse.cpp/.h` | Individual warehouse data and stock management |
| `Product.cpp/.h` | Abstract base class for all product types |
| `StandardProduct.cpp/.h` | Concrete class for standard (non-discounted, non-taxed) products |
| `DiscountedProduct.cpp/.h` | Concrete class for products with a discount applied |
| `TaxableProduct.cpp/.h` | Concrete class for products subject to tax |
| `Order.cpp/.h` | Represents an individual order |
| `OrderPool.cpp/.h` | Manages a pool of pending/completed orders |
| `WorkerThread.cpp/.h` | Multi-threaded worker for processing order batches |
| `AnalyticsManager.cpp/.h` | Manages reading and writing analytics records |
| `AnalyticsRecord.cpp/.h` | Data model for an analytics log entry |
| `Logger.h` | Utility header for logging output |

### CLI — Data Files

| Folder | Description |
|---|---|
| `Catalog/Products.csv` | Master product catalogue |
| `Catalog/Updated_Products.csv` | Product catalogue after system modifications |
| `Orders/Warehouse[N]_Orders.csv` | Pending orders per warehouse (5 warehouses) |
| `Analytics/Global/GlobalItemRecords.csv` | Global analytics across all warehouses |
| `Analytics/Warehouses/Warehouse [N]/` | Per-warehouse analytics records |

### GUI — Additional Layers (Bonus)

| Folder | Description |
|---|---|
| `src/ui/` | Qt widget pages (Dashboard, Inventory, Analytics, Orders, Logs, Settings) |
| `src/integration/` | Adapter layer connecting the CLI backend to the GUI frontend |
| `src/models/` | Data transfer objects (DTOs) used between layers |
| `assets/` | Icons, card patterns, and application logo |
| `resources/style.qss` | Application-wide Qt stylesheet |

---

## System Requirements

**CLI:**
- Windows 10/11 (64-bit)
- No additional software required for the pre-compiled version (DLLs are bundled)
- Code::Blocks + MinGW required only if building from source

**GUI (Bonus):**
- Windows 10/11 (64-bit)
- No additional software required for the pre-compiled version (all Qt DLLs are bundled)
- Qt Creator + Qt 6 (with Charts module) required only if building from source

---

*Group Twelve Software Solutions — COMP315 Final Submission*
