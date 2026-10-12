#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <mutex>
#include <cctype>
#include <cstdlib>
#include <filesystem>

#include "httplib.h"
#include "json.hpp"

using namespace std;
using json = nlohmann::json;

// ============================================================================
// Core Data Structure (Preserved from main.cpp)
// ============================================================================
struct Patient {
    int patientId;
    string name;
    int age;
    string gender;
    string medicalCondition;
    int priorityLevel;
    bool emergency;
    bool waiting;
};

// Helper function to convert a Patient struct to JSON
json patientToJson(const Patient& p) {
    return json{
        {"patientId", p.patientId},
        {"name", p.name},
        {"age", p.age},
        {"gender", p.gender},
        {"medicalCondition", p.medicalCondition},
        {"priorityLevel", p.priorityLevel},
        {"emergency", p.emergency},
        {"type", p.emergency ? "Emergency" : "Regular"},
        {"waiting", p.waiting},
        {"status", p.waiting ? "Waiting" : "Attended"}
    };
}

// String utility helpers for validation and search
string trimString(const string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

string toLowerString(string str) {
    transform(str.begin(), str.end(), str.begin(),
              [](unsigned char c) { return static_cast<char>(tolower(c)); });
    return str;
}

// ============================================================================
// Core Hospital Management Logic (Preserved and adapted from main.cpp)
// ============================================================================
class HospitalSystem {
private:
    vector<Patient> patients;
    mutex mtx;

    // Linear search by Patient ID (Preserved from main.cpp)
    int findPatient(int id) const {
        for (int i = 0; i < static_cast<int>(patients.size()); i++) {
            if (patients[i].patientId == id) {
                return i;
            }
        }
        return -1;
    }

    // Internal helper to find index of next patient to be attended
    int findNextPatientIndex() const {
        int selected = -1;

        // 1. Emergency patients are attended first (highest priority 5 down to 1, FIFO tie-breaker)
        for (int i = 0; i < static_cast<int>(patients.size()); i++) {
            if (patients[i].waiting && patients[i].emergency) {
                if (selected == -1 ||
                    patients[i].priorityLevel > patients[selected].priorityLevel) {
                    selected = i;
                }
            }
        }

        // 2. Otherwise, attend the earliest waiting regular patient (FIFO order)
        if (selected == -1) {
            for (int i = 0; i < static_cast<int>(patients.size()); i++) {
                if (patients[i].waiting && !patients[i].emergency) {
                    selected = i;
                    break;
                }
            }
        }

        return selected;
    }

public:
    HospitalSystem() {
        seedSamplePatients();
    }

    // Load initial sample patients in C++ memory so the dashboard is immediately populated
    void seedSamplePatients() {
        lock_guard<mutex> lock(mtx);
        patients.clear();
        patients.push_back(Patient{101, "Aarav Mehta", 34, "Male", "Seasonal Viral Fever", 0, false, true});
        patients.push_back(Patient{102, "Priya Sharma", 28, "Female", "Migraine & Fatigue", 0, false, true});
        patients.push_back(Patient{201, "Rohan Verma", 49, "Male", "Compound Leg Fracture", 3, true, true});
        patients.push_back(Patient{202, "Ananya Reddy", 62, "Female", "Acute Chest Pain & Arrhythmia", 5, true, true});
        patients.push_back(Patient{100, "Sneha Kapoor", 41, "Female", "General Health Checkup", 0, false, false});
    }

    void clearAllPatients() {
        lock_guard<mutex> lock(mtx);
        patients.clear();
    }

    // 1. Register a new patient
    bool registerPatient(int patientId,
                         const string& rawName,
                         int age,
                         const string& rawGender,
                         const string& rawCondition,
                         bool emergency,
                         int priorityLevel,
                         Patient& createdPatient,
                         string& errorMsg,
                         int& httpStatus) {
        lock_guard<mutex> lock(mtx);

        if (patientId <= 0) {
            errorMsg = "Invalid Patient ID. ID must be a positive number.";
            httpStatus = 400;
            return false;
        }

        if (findPatient(patientId) != -1) {
            errorMsg = "Patient ID " + to_string(patientId) + " already exists. Please use a unique ID.";
            httpStatus = 409;
            return false;
        }

        string name = trimString(rawName);
        if (name.empty()) {
            errorMsg = "Patient full name is required.";
            httpStatus = 400;
            return false;
        }

        if (age <= 0 || age > 150) {
            errorMsg = "Invalid age. Age must be between 1 and 150.";
            httpStatus = 400;
            return false;
        }

        string gender = trimString(rawGender);
        if (gender.empty()) {
            errorMsg = "Gender is required.";
            httpStatus = 400;
            return false;
        }

        string medicalCondition = trimString(rawCondition);
        if (medicalCondition.empty()) {
            errorMsg = "Medical condition is required.";
            httpStatus = 400;
            return false;
        }

        int finalPriority = 0;
        if (emergency) {
            if (priorityLevel < 1 || priorityLevel > 5) {
                errorMsg = "Emergency priority must be between 1 (lowest) and 5 (highest).";
                httpStatus = 400;
                return false;
            }
            finalPriority = priorityLevel;
        }

        Patient p{};
        p.patientId = patientId;
        p.name = name;
        p.age = age;
        p.gender = gender;
        p.medicalCondition = medicalCondition;
        p.emergency = emergency;
        p.priorityLevel = finalPriority;
        p.waiting = true;

        patients.push_back(p);
        createdPatient = p;
        httpStatus = 201;
        return true;
    }

    // 2. Display / Retrieve all patients
    vector<Patient> getAllPatients() {
        lock_guard<mutex> lock(mtx);
        return patients;
    }

    // 3. Search patients by ID or Name
    vector<Patient> searchPatients(const string& rawQuery, const string& mode = "all") {
        lock_guard<mutex> lock(mtx);
        vector<Patient> results;
        string query = trimString(rawQuery);

        if (query.empty()) {
            return patients;
        }

        string lowerQuery = toLowerString(query);

        // Check if query is a pure integer for ID matching
        bool isNumber = !query.empty() && all_of(query.begin(), query.end(), ::isdigit);
        int queryId = -1;
        if (isNumber) {
            try {
                queryId = stoi(query);
            } catch (...) {
                queryId = -1;
            }
        }

        if (mode == "id") {
            if (queryId != -1) {
                int index = findPatient(queryId);
                if (index != -1) {
                    results.push_back(patients[index]);
                }
            }
            return results;
        }

        for (const Patient& p : patients) {
            bool matchId = (mode != "name") &&
                           ((queryId != -1 && p.patientId == queryId) ||
                            to_string(p.patientId).find(query) != string::npos);
            bool matchName = (mode != "id") &&
                             (toLowerString(p.name).find(lowerQuery) != string::npos);

            if (matchId || matchName) {
                results.push_back(p);
            }
        }

        return results;
    }

    // 4. Edit patient details
    bool editPatient(int id,
                     const string& rawName,
                     int age,
                     const string& rawGender,
                     const string& rawCondition,
                     bool emergency,
                     int priorityLevel,
                     Patient& updatedPatient,
                     string& errorMsg,
                     int& httpStatus) {
        lock_guard<mutex> lock(mtx);

        int index = findPatient(id);
        if (index == -1) {
            errorMsg = "Patient with ID " + to_string(id) + " not found.";
            httpStatus = 404;
            return false;
        }

        string name = trimString(rawName);
        if (name.empty()) {
            errorMsg = "Patient full name cannot be empty.";
            httpStatus = 400;
            return false;
        }

        if (age <= 0 || age > 150) {
            errorMsg = "Invalid age. Age must be between 1 and 150.";
            httpStatus = 400;
            return false;
        }

        string gender = trimString(rawGender);
        if (gender.empty()) {
            errorMsg = "Gender cannot be empty.";
            httpStatus = 400;
            return false;
        }

        string condition = trimString(rawCondition);
        if (condition.empty()) {
            errorMsg = "Medical condition cannot be empty.";
            httpStatus = 400;
            return false;
        }

        int finalPriority = 0;
        if (emergency) {
            if (priorityLevel < 1 || priorityLevel > 5) {
                errorMsg = "Emergency priority must be between 1 and 5.";
                httpStatus = 400;
                return false;
            }
            finalPriority = priorityLevel;
        }

        Patient& p = patients[index];
        p.name = name;
        p.age = age;
        p.gender = gender;
        p.medicalCondition = condition;
        p.emergency = emergency;
        p.priorityLevel = finalPriority;

        updatedPatient = p;
        httpStatus = 200;
        return true;
    }

    // 5. Delete patient record
    bool deletePatient(int id, Patient& deletedPatient, string& errorMsg, int& httpStatus) {
        lock_guard<mutex> lock(mtx);

        int index = findPatient(id);
        if (index == -1) {
            errorMsg = "Patient with ID " + to_string(id) + " not found.";
            httpStatus = 404;
            return false;
        }

        deletedPatient = patients[index];
        patients.erase(patients.begin() + index);
        httpStatus = 200;
        return true;
    }

    // 6. Retrieve Waiting Queues (Emergency sorted by priority 5->1, Regular FIFO)
    void getQueues(vector<Patient>& emergencyQueue,
                   vector<Patient>& regularQueue,
                   Patient& nextToAttend,
                   bool& hasNext) {
        lock_guard<mutex> lock(mtx);
        emergencyQueue.clear();
        regularQueue.clear();

        for (const Patient& p : patients) {
            if (p.waiting) {
                if (p.emergency) {
                    emergencyQueue.push_back(p);
                } else {
                    regularQueue.push_back(p);
                }
            }
        }

        // Preserve FIFO order among emergency patients with identical priority levels
        stable_sort(emergencyQueue.begin(), emergencyQueue.end(),
                    [](const Patient& a, const Patient& b) {
                        return a.priorityLevel > b.priorityLevel;
                    });

        int nextIdx = findNextPatientIndex();
        if (nextIdx != -1) {
            nextToAttend = patients[nextIdx];
            hasNext = true;
        } else {
            hasNext = false;
        }
    }

    // 7. Attend Next Patient (Highest-priority emergency first, then FIFO regular)
    bool attendNextPatient(Patient& attendedPatient, string& errorMsg, int& httpStatus) {
        lock_guard<mutex> lock(mtx);

        int selected = findNextPatientIndex();
        if (selected == -1) {
            errorMsg = "No patients are currently waiting in the queue.";
            httpStatus = 404;
            return false;
        }

        patients[selected].waiting = false;
        attendedPatient = patients[selected];
        httpStatus = 200;
        return true;
    }

    // 8. Get Dashboard Statistics & Recently Registered Patients
    json getDashboardStats() {
        lock_guard<mutex> lock(mtx);

        int totalPatients = static_cast<int>(patients.size());
        int waitingPatients = 0;
        int waitingEmergency = 0;
        int totalEmergency = 0;
        int waitingRegular = 0;
        int attendedPatients = 0;

        for (const Patient& p : patients) {
            if (p.emergency) {
                totalEmergency++;
            }
            if (p.waiting) {
                waitingPatients++;
                if (p.emergency) {
                    waitingEmergency++;
                } else {
                    waitingRegular++;
                }
            } else {
                attendedPatients++;
            }
        }

        // Recently registered patients (most recent first, up to 6)
        json recentList = json::array();
        int count = 0;
        for (int i = static_cast<int>(patients.size()) - 1; i >= 0 && count < 6; i--, count++) {
            recentList.push_back(patientToJson(patients[i]));
        }

        int nextIdx = findNextPatientIndex();
        json nextPatientJson = nullptr;
        if (nextIdx != -1) {
            nextPatientJson = patientToJson(patients[nextIdx]);
        }

        return json{
            {"totalPatients", totalPatients},
            {"waitingPatients", waitingPatients},
            {"emergencyPatients", waitingEmergency},
            {"totalEmergencyPatients", totalEmergency},
            {"regularWaitingPatients", waitingRegular},
            {"attendedPatients", attendedPatients},
            {"recentPatients", recentList},
            {"nextPatient", nextPatientJson}
        };
    }
};

// ============================================================================
// HTTP & JSON Helper Utilities
// ============================================================================
void setCorsHeaders(httplib::Response& res) {
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "Content-Type, Accept");
}

void sendJsonResponse(httplib::Response& res, int status, const json& payload) {
    setCorsHeaders(res);
    res.status = status;
    res.set_content(payload.dump(2), "application/json; charset=utf-8");
}

void sendErrorResponse(httplib::Response& res, int status, const string& message) {
    sendJsonResponse(res, status, json{
        {"success", false},
        {"error", message}
    });
}

// ============================================================================
// Main Server Entry Point
// ============================================================================
int main(int argc, char* argv[]) {
    HospitalSystem hospital;
    httplib::Server svr;

    int port = 8080;
    bool openBrowser = true;

    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        if (arg == "--no-browser") {
            openBrowser = false;
        } else {
            try {
                port = stoi(arg);
            } catch (...) {}
        }
    }

    // Handle CORS preflight requests for all routes
    svr.Options(R"(.*)", [](const httplib::Request&, httplib::Response& res) {
        setCorsHeaders(res);
        res.status = 204;
    });

    // Health check endpoint
    svr.Get("/api/health", [](const httplib::Request&, httplib::Response& res) {
        sendJsonResponse(res, 200, json{
            {"success", true},
            {"status", "ok"},
            {"service", "Hospital Patient Management System C++ Backend"}
        });
    });

    // Reset sample patients endpoint
    svr.Post("/api/reset", [&hospital](const httplib::Request&, httplib::Response& res) {
        hospital.seedSamplePatients();
        sendJsonResponse(res, 200, json{
            {"success", true},
            {"message", "Sample hospital patient records restored."}
        });
    });

    // Clear all patients endpoint
    svr.Post("/api/clear", [&hospital](const httplib::Request&, httplib::Response& res) {
        hospital.clearAllPatients();
        sendJsonResponse(res, 200, json{
            {"success", true},
            {"message", "All patient records cleared."}
        });
    });

    // Dashboard Statistics endpoint
    svr.Get("/api/stats", [&hospital](const httplib::Request&, httplib::Response& res) {
        json stats = hospital.getDashboardStats();
        stats["success"] = true;
        sendJsonResponse(res, 200, stats);
    });

    // Get all patients (or filter with query parameters)
    svr.Get("/api/patients", [&hospital](const httplib::Request& req, httplib::Response& res) {
        vector<Patient> list;
        if (req.has_param("q") || req.has_param("search")) {
            string q = req.has_param("q") ? req.get_param_value("q") : req.get_param_value("search");
            string mode = req.has_param("mode") ? req.get_param_value("mode") : "all";
            list = hospital.searchPatients(q, mode);
        } else {
            list = hospital.getAllPatients();
        }

        json patientsJson = json::array();
        for (const Patient& p : list) {
            patientsJson.push_back(patientToJson(p));
        }

        sendJsonResponse(res, 200, json{
            {"success", true},
            {"count", patientsJson.size()},
            {"patients", patientsJson}
        });
    });

    // Dedicated Search endpoint: /api/patients/search?q=...&mode=all|id|name
    svr.Get("/api/patients/search", [&hospital](const httplib::Request& req, httplib::Response& res) {
        string query = req.has_param("q") ? req.get_param_value("q") : "";
        string mode = req.has_param("mode") ? req.get_param_value("mode") : "all";

        vector<Patient> results = hospital.searchPatients(query, mode);
        json patientsJson = json::array();
        for (const Patient& p : results) {
            patientsJson.push_back(patientToJson(p));
        }

        sendJsonResponse(res, 200, json{
            {"success", true},
            {"query", query},
            {"mode", mode},
            {"count", patientsJson.size()},
            {"patients", patientsJson}
        });
    });

    // Register Patient endpoint: POST /api/patients
    svr.Post("/api/patients", [&hospital](const httplib::Request& req, httplib::Response& res) {
        json body;
        try {
            body = json::parse(req.body);
        } catch (...) {
            sendErrorResponse(res, 400, "Invalid JSON format in request body.");
            return;
        }

        if (!body.contains("patientId") || !body.contains("name") ||
            !body.contains("age") || !body.contains("gender") ||
            !body.contains("medicalCondition")) {
            sendErrorResponse(res, 400, "Missing required patient fields (patientId, name, age, gender, medicalCondition).");
            return;
        }

        int patientId = 0;
        int age = 0;
        bool emergency = false;
        int priorityLevel = 0;

        try {
            if (body["patientId"].is_string()) {
                patientId = stoi(body["patientId"].get<string>());
            } else if (body["patientId"].is_number_integer()) {
                patientId = body["patientId"].get<int>();
            } else {
                sendErrorResponse(res, 400, "Patient ID must be a valid integer.");
                return;
            }

            if (body["age"].is_string()) {
                age = stoi(body["age"].get<string>());
            } else if (body["age"].is_number_integer()) {
                age = body["age"].get<int>();
            } else {
                sendErrorResponse(res, 400, "Age must be a valid integer.");
                return;
            }

            if (body.contains("emergency")) {
                if (body["emergency"].is_boolean()) {
                    emergency = body["emergency"].get<bool>();
                } else if (body["emergency"].is_number_integer()) {
                    emergency = (body["emergency"].get<int>() == 1);
                } else if (body["emergency"].is_string()) {
                    string val = toLowerString(body["emergency"].get<string>());
                    emergency = (val == "true" || val == "1" || val == "emergency" || val == "yes");
                }
            }

            if (emergency && body.contains("priorityLevel")) {
                if (body["priorityLevel"].is_string()) {
                    priorityLevel = stoi(body["priorityLevel"].get<string>());
                } else if (body["priorityLevel"].is_number_integer()) {
                    priorityLevel = body["priorityLevel"].get<int>();
                }
            }
        } catch (...) {
            sendErrorResponse(res, 400, "Invalid numeric value for Patient ID, Age, or Priority Level.");
            return;
        }

        string name = body["name"].is_string() ? body["name"].get<string>() : "";
        string gender = body["gender"].is_string() ? body["gender"].get<string>() : "";
        string condition = body["medicalCondition"].is_string() ? body["medicalCondition"].get<string>() : "";

        Patient created{};
        string errorMsg;
        int statusCode = 201;

        if (!hospital.registerPatient(patientId, name, age, gender, condition,
                                      emergency, priorityLevel, created, errorMsg, statusCode)) {
            sendErrorResponse(res, statusCode, errorMsg);
            return;
        }

        sendJsonResponse(res, 201, json{
            {"success", true},
            {"message", "Patient registered successfully!"},
            {"patient", patientToJson(created)}
        });
    });

    // Edit Patient endpoint: PUT /api/patients/:id
    svr.Put(R"(/api/patients/(\d+))", [&hospital](const httplib::Request& req, httplib::Response& res) {
        int id = 0;
        try {
            id = stoi(req.matches[1]);
        } catch (...) {
            sendErrorResponse(res, 400, "Invalid Patient ID in URL.");
            return;
        }

        json body;
        try {
            body = json::parse(req.body);
        } catch (...) {
            sendErrorResponse(res, 400, "Invalid JSON format in request body.");
            return;
        }

        if (!body.contains("name") || !body.contains("age") ||
            !body.contains("gender") || !body.contains("medicalCondition")) {
            sendErrorResponse(res, 400, "Missing required fields (name, age, gender, medicalCondition).");
            return;
        }

        int age = 0;
        bool emergency = false;
        int priorityLevel = 0;

        try {
            if (body["age"].is_string()) {
                age = stoi(body["age"].get<string>());
            } else if (body["age"].is_number_integer()) {
                age = body["age"].get<int>();
            } else {
                sendErrorResponse(res, 400, "Age must be a valid integer.");
                return;
            }

            if (body.contains("emergency")) {
                if (body["emergency"].is_boolean()) {
                    emergency = body["emergency"].get<bool>();
                } else if (body["emergency"].is_number_integer()) {
                    emergency = (body["emergency"].get<int>() == 1);
                } else if (body["emergency"].is_string()) {
                    string val = toLowerString(body["emergency"].get<string>());
                    emergency = (val == "true" || val == "1" || val == "emergency" || val == "yes");
                }
            }

            if (emergency && body.contains("priorityLevel")) {
                if (body["priorityLevel"].is_string()) {
                    priorityLevel = stoi(body["priorityLevel"].get<string>());
                } else if (body["priorityLevel"].is_number_integer()) {
                    priorityLevel = body["priorityLevel"].get<int>();
                }
            }
        } catch (...) {
            sendErrorResponse(res, 400, "Invalid numeric value for Age or Priority Level.");
            return;
        }

        string name = body["name"].is_string() ? body["name"].get<string>() : "";
        string gender = body["gender"].is_string() ? body["gender"].get<string>() : "";
        string condition = body["medicalCondition"].is_string() ? body["medicalCondition"].get<string>() : "";

        Patient updated{};
        string errorMsg;
        int statusCode = 200;

        if (!hospital.editPatient(id, name, age, gender, condition,
                                  emergency, priorityLevel, updated, errorMsg, statusCode)) {
            sendErrorResponse(res, statusCode, errorMsg);
            return;
        }

        sendJsonResponse(res, 200, json{
            {"success", true},
            {"message", "Patient details updated successfully!"},
            {"patient", patientToJson(updated)}
        });
    });

    // Delete Patient endpoint: DELETE /api/patients/:id
    svr.Delete(R"(/api/patients/(\d+))", [&hospital](const httplib::Request& req, httplib::Response& res) {
        int id = 0;
        try {
            id = stoi(req.matches[1]);
        } catch (...) {
            sendErrorResponse(res, 400, "Invalid Patient ID in URL.");
            return;
        }

        Patient deleted{};
        string errorMsg;
        int statusCode = 200;

        if (!hospital.deletePatient(id, deleted, errorMsg, statusCode)) {
            sendErrorResponse(res, statusCode, errorMsg);
            return;
        }

        sendJsonResponse(res, 200, json{
            {"success", true},
            {"message", "Patient record deleted successfully."},
            {"patient", patientToJson(deleted)}
        });
    });

    // Waiting Queues endpoint: GET /api/queue
    svr.Get("/api/queue", [&hospital](const httplib::Request&, httplib::Response& res) {
        vector<Patient> emergencyQueue;
        vector<Patient> regularQueue;
        Patient nextToAttend{};
        bool hasNext = false;

        hospital.getQueues(emergencyQueue, regularQueue, nextToAttend, hasNext);

        json emergencyJson = json::array();
        for (const Patient& p : emergencyQueue) {
            emergencyJson.push_back(patientToJson(p));
        }

        json regularJson = json::array();
        for (const Patient& p : regularQueue) {
            regularJson.push_back(patientToJson(p));
        }

        json nextJson = hasNext ? patientToJson(nextToAttend) : nullptr;

        sendJsonResponse(res, 200, json{
            {"success", true},
            {"emergencyCount", emergencyJson.size()},
            {"regularCount", regularJson.size()},
            {"totalWaiting", emergencyJson.size() + regularJson.size()},
            {"emergencyQueue", emergencyJson},
            {"regularQueue", regularJson},
            {"nextPatient", nextJson}
        });
    });

    // Attend Next Patient endpoint: POST /api/attend
    svr.Post("/api/attend", [&hospital](const httplib::Request&, httplib::Response& res) {
        Patient attended{};
        string errorMsg;
        int statusCode = 200;

        if (!hospital.attendNextPatient(attended, errorMsg, statusCode)) {
            sendErrorResponse(res, statusCode, errorMsg);
            return;
        }

        sendJsonResponse(res, 200, json{
            {"success", true},
            {"message", "Patient attended: " + attended.name + " (ID: " + to_string(attended.patientId) + ")"},
            {"patient", patientToJson(attended)}
        });
    });

    // Locate and mount the frontend directory reliably regardless of current working directory
    vector<filesystem::path> candidateDirs = {
        filesystem::path("./frontend"),
        filesystem::path("../frontend")
    };
    if (argc > 0 && argv[0] != nullptr) {
        try {
            filesystem::path exeDir = filesystem::absolute(filesystem::path(argv[0])).parent_path();
            candidateDirs.push_back(exeDir / "frontend");
            candidateDirs.push_back(exeDir.parent_path() / "frontend");
        } catch (...) {}
    }

    string mountedFrontend = "";
    for (const auto& dir : candidateDirs) {
        if (filesystem::exists(dir / "index.html")) {
            mountedFrontend = filesystem::canonical(dir).string();
            svr.set_mount_point("/", mountedFrontend);
            break;
        }
    }

    if (!mountedFrontend.empty()) {
        cout << "[INFO] Serving frontend files from: " << mountedFrontend << "\n";
    } else {
        cout << "[WARN] Frontend directory not found. API endpoints are still active.\n";
    }

    cout << "=========================================================\n";
    cout << "  HOSPITAL PATIENT MANAGEMENT SYSTEM - C++ WEB SERVER\n";
    cout << "=========================================================\n";
    cout << "  Open in Browser:     http://localhost:" << port << "\n";
    cout << "  REST API Base URL:   http://localhost:" << port << "/api\n";
    cout << "  Storage Mode:        In-Memory (resets on restart)\n";
    cout << "  Press Ctrl+C to stop the server.\n";
    cout << "=========================================================\n";
    cout.flush();

    if (openBrowser) {
        string url = "http://localhost:" + to_string(port);
        string cmd = "start \"\" \"" + url + "\"";
        system(cmd.c_str());
    }

    if (!svr.listen("0.0.0.0", port)) {
        cerr << "[ERROR] Failed to start server on port " << port << ". The port may already be in use.\n";
        return 1;
    }

    return 0;
}
