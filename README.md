# Hospital Patient Management System (C++ Backend + Web Frontend)

A complete, responsive, full-stack **Hospital Patient Management System** featuring a **C++17 REST API Backend** (`cpp-httplib` + `nlohmann/json`) and a **Modern Hospital Dashboard Frontend** (`HTML5`, `CSS3`, and `Vanilla JavaScript`).

---

## 1. Project Structure

```text
HPM/
├── main.cpp               # Original C++ console application (preserved untouched)
├── clo.cpp                # Original colored C++ console application (preserved untouched)
├── backend/
│   ├── server.cpp         # C++ HTTP & JSON REST API server (also serves frontend/)
│   ├── server.exe         # Compiled Windows executable
│   ├── httplib.h          # Single-header cpp-httplib library (HTTP server)
│   ├── json.hpp           # Single-header nlohmann/json library (JSON parser)
│   └── README.md          # Detailed C++ backend & API documentation
├── frontend/
│   ├── index.html         # Hospital dashboard UI (Dashboard, Register, Manage, Queue)
│   ├── style.css          # Professional blue-and-white clinical theme & responsive layout
│   └── script.js          # Frontend REST API client & interactive logic
└── README.md              # Main project setup and usage guide
```

---

## 2. Key Features

### Dashboard
- **Summary Cards**: Live counts of **Total Patients**, **Waiting Patients**, **Emergency Patients**, and **Attended Patients**.
- **Next in Triage Spotlight**: Highlights the exact patient who will be attended next based on hospital priority rules.
- **Recently Registered Patients**: Displays the most recently registered patient records and their live status.
- **Waiting Queue Overview**: Shows separate live counts for the Emergency Priority Queue and Regular FIFO Queue.

### Register Patient
- Captures **Patient ID**, **Full Name**, **Age**, **Gender**, **Medical Condition**, **Patient Type** (`Regular` or `Emergency`), and **Emergency Priority (`1` to `5`)**.
- Validates all inputs on both the frontend and C++ backend.
- Prevents duplicate Patient IDs using linear search (`HTTP 409 Conflict`).

### Manage Patients
- Displays all patients in a responsive table with color-coded badges (`Emergency`, `Regular`, `Priority 1–5`, `Waiting`, `Attended`).
- **Search**: Search patients by **Patient ID** or **Full Name**.
- **Edit**: Update a patient's name, age, gender, medical condition, patient type, and emergency priority via a modal form.
- **Delete**: Safely remove a patient record after a confirmation dialog.

### Patient Queue & Triage Rules
- **Emergency Waiting Queue**: Sorted by priority from **5 (Highest)** down to **1 (Lowest)**. Equal-priority emergency patients preserve their registration order.
- **Regular Waiting Queue**: Strictly ordered by **First-In, First-Out (FIFO)** registration order.
- **Attend Next Patient**:
  1. If any emergency patient is waiting, the **highest-priority emergency patient** is attended first.
  2. If no emergency patient is waiting, the **earliest registered regular patient (FIFO)** is attended.
  3. Automatically updates dashboard statistics, queue tables, and the patient's status from `Waiting` to `Attended`.

---

## 3. Windows & VS Code Setup Instructions (Beginner-Friendly)

### Prerequisites
1. **Visual Studio Code (VS Code)** installed on Windows.
2. **MinGW-w64 / MSYS2 C++ Compiler (`g++`)** installed on Windows (already installed at `C:\msys64\ucrt64\bin\g++.exe`).
   - To check if `g++` is available, open the VS Code Terminal (`Ctrl + ~` or **Terminal → New Terminal**) and run:
     ```powershell
     g++ --version
     ```
3. **C++ Header Dependencies (`httplib.h` and `json.hpp`)**:
   - Both single-header libraries are already downloaded inside the `backend/` folder (`backend/httplib.h` and `backend/json.hpp`), so you do **not** need to install any external package manager to compile.
   - If you ever need to download them again on a new computer, run these two commands in the VS Code PowerShell terminal:
     ```powershell
     curl.exe -sL "https://raw.githubusercontent.com/yhirose/cpp-httplib/v0.18.3/httplib.h" -o "backend/httplib.h"
     curl.exe -sL "https://github.com/nlohmann/json/releases/download/v3.11.3/json.hpp" -o "backend/json.hpp"
     ```

---

## 4. How to Build and Run the Project

### Quickest Way: One-Click Launcher (`start_hospital_system.bat`)
Simply double-click **`start_hospital_system.bat`** in the `HPM` folder (or run `.\start_hospital_system.bat` in the VS Code terminal). It will start the C++ server and **automatically open `http://localhost:8080` in your browser**!

---

### Manual Steps in VS Code

#### Step 1: Open the Project in VS Code
1. Open **VS Code**.
2. Click **File → Open Folder...** and select `C:\Users\hnagireddy\Desktop\HPM`.
3. Open a new terminal in VS Code by pressing `` Ctrl + ` `` (or **Terminal → New Terminal**).

#### Step 2: Compile the C++ Backend Server
Press **`Ctrl + Shift + B`** in VS Code (which uses `.vscode/tasks.json`), or run the following command in the VS Code terminal:
```powershell
g++ -std=c++17 -O2 backend/server.cpp -o backend/server.exe -lws2_32 -static -static-libgcc -static-libstdc++
```
*(Note: `-lws2_32` links the Windows networking library, and `-static` bundles the C++ runtime so `server.exe` runs anywhere on Windows without missing DLL errors.)*

#### Step 3: Start the C++ Server
Run the compiled executable:
```powershell
.\backend\server.exe
```
When `server.exe` starts, it will **automatically open `http://localhost:8080`** in your default web browser.

#### Step 4: Open the Website in Your Browser
- **Option A — Directly from the C++ Server (Recommended)**:
  Visit **[http://localhost:8080](http://localhost:8080)** in Chrome, Edge, or Firefox.
- **Option B — Using the VS Code "Live Server" Extension**:
  Keep `.\backend\server.exe` running, then right-click `frontend/index.html` in VS Code and select **Open with Live Server**.

---

## 5. Data Storage Behavior
- All patient records are stored in the C++ backend's memory (`std::vector<Patient> patients` inside `HospitalSystem`).
- The frontend never stores a separate copy of patient data; every action queries and updates the C++ backend directly.
- **Important**: Because records are stored in RAM for this initial version, **all data will be reset when you stop or restart `server.exe`**. The `HospitalSystem` class is structured cleanly so file storage (CSV/JSON) or a database (SQLite) can be added easily in the future.
