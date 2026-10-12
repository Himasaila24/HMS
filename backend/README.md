# Hospital Patient Management System — C++ REST API Backend

This folder contains the C++ HTTP & JSON backend server (`server.cpp`) that powers the **Hospital Patient Management System** web dashboard.

---

## 1. How Your Existing `main.cpp` Logic Was Preserved

Your original `main.cpp` (and `clo.cpp`) remains completely untouched in the root folder so you can still compile and run the console version at any time.

In `backend/server.cpp`, we preserved your core C++ design:
- **`struct Patient`**: Identical fields (`patientId`, `name`, `age`, `gender`, `medicalCondition`, `priorityLevel`, `emergency`, `waiting`).
- **`class HospitalSystem`**:
  - Uses `std::vector<Patient> patients` for in-memory storage.
  - Uses linear search (`findPatient(int id)`) to find records and prevent duplicate Patient IDs.
  - Uses **Emergency Priority Queue logic** (Priority `5` highest down to `1` lowest, with `std::stable_sort` / strict `>` comparison so equal-priority emergencies are attended in FIFO order).
  - Uses **First-In, First-Out (FIFO)** queue logic for regular patients when no emergency patient is waiting.
- **What Changed for the Web API**:
  - Replaced terminal `cin`/`cout` prompts with parameterized methods that accept parsed JSON inputs and return structured responses and HTTP status codes (`200`, `201`, `400`, `404`, `409`).
  - Added a `std::mutex` lock so concurrent HTTP requests from the browser are thread-safe.

---

## 2. C++ Libraries Used (Included in `backend/`)

To keep compilation simple on Windows without requiring CMake or external package managers, both single-header libraries are already included in the `backend/` folder:

1. **`httplib.h` (`cpp-httplib` v0.18.3)**
   - Lightweight single-header C++17 HTTP server library.
   - Source: `https://github.com/yhirose/cpp-httplib`
2. **`json.hpp` (`nlohmann/json` v3.11.3)**
   - Single-header JSON parser and serializer for Modern C++.
   - Source: `https://github.com/nlohmann/json`

### How to Re-Download These Headers on Windows (If Needed)
Open the terminal in VS Code (`Ctrl + ~`) from the `HPM` root directory and run:
```powershell
curl.exe -sL "https://raw.githubusercontent.com/yhirose/cpp-httplib/v0.18.3/httplib.h" -o "backend/httplib.h"
curl.exe -sL "https://github.com/nlohmann/json/releases/download/v3.11.3/json.hpp" -o "backend/json.hpp"
```

---

## 3. How to Compile & Run on Windows (VS Code)

Make sure you have **MinGW-w64 / MSYS2 `g++`** installed and added to your Windows `PATH` (for example, `C:\msys64\ucrt64\bin`).

### Step 1: Compile `server.cpp`
From the `HPM` project root folder in the VS Code terminal, run:
```powershell
g++ -std=c++17 -O2 backend/server.cpp -o backend/server.exe -lws2_32
```
*(Note: `-lws2_32` links the Windows Winsock2 networking library required by `cpp-httplib` on Windows.)*

### Step 2: Run the Server
```powershell
.\backend\server.exe
```

You will see:
```text
=========================================================
  HOSPITAL PATIENT MANAGEMENT SYSTEM - C++ WEB SERVER
=========================================================
  Server running at:   http://localhost:8080
  REST API base URL:   http://localhost:8080/api
  Storage mode:        In-Memory (resets on restart)
  Press Ctrl+C to stop the server.
=========================================================
```

---

## 4. REST API Endpoints Reference

| Method | Endpoint | Description |
| :--- | :--- | :--- |
| `GET` | `/api/health` | Health check confirming the C++ backend is online. |
| `GET` | `/api/stats` | Returns total, waiting, emergency, and attended counts, recent patients, and next patient in queue. |
| `GET` | `/api/patients` | Returns all registered patients in registration order. |
| `GET` | `/api/patients/search?q=<term>&mode=all\|id\|name` | Searches patients by ID or Name. |
| `POST` | `/api/patients` | Registers a new patient (validates fields and prevents duplicate IDs with `409 Conflict`). |
| `PUT` | `/api/patients/:id` | Updates an existing patient's name, age, gender, condition, type, and priority. |
| `DELETE` | `/api/patients/:id` | Deletes a patient record by ID. |
| `GET` | `/api/queue` | Returns separated `emergencyQueue` (Priority 5→1) and `regularQueue` (FIFO). |
| `POST` | `/api/attend` | Attends the highest-priority emergency patient first, or the earliest FIFO regular patient. |

---

## 5. Data Storage Note
Patient records are stored in memory inside `std::vector<Patient> patients`. When you stop or restart `server.exe`, all records are cleared. Because all operations are encapsulated inside `class HospitalSystem`, you can easily extend the class later with CSV/JSON file persistence or SQLite without changing any frontend code.
