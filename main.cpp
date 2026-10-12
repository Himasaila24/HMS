
#include <iostream>
#include <string>
#include <limits>
#include <queue>
#include <vector>
#include <algorithm>

using namespace std;

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

class HospitalSystem {
private:
    vector<Patient> patients;

    void clearInput() {
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }

    int findPatient(int id) {
        for (int i = 0; i < (int)patients.size(); i++) {
            if (patients[i].patientId == id)
                return i;
        }
        return -1;
    }

    void showPatient(const Patient& p) {
        cout << "\nPatient ID: " << p.patientId
             << "\nName: " << p.name
             << "\nAge: " << p.age
             << "\nGender: " << p.gender
             << "\nCondition: " << p.medicalCondition
             << "\nType: " << (p.emergency ? "Emergency" : "Regular")
             << "\nPriority: " << p.priorityLevel
             << "\nWaiting: " << (p.waiting ? "Yes" : "No")
             << "\n";
    }

public:
    void registerPatient() {
        Patient p{};

        cout << "Enter Patient ID: ";
        if (!(cin >> p.patientId)) {
            cout << "Invalid ID.\n";
            cin.clear();
            clearInput();
            return;
        }

        if (findPatient(p.patientId) != -1) {
            cout << "Patient ID already exists.\n";
            clearInput();
            return;
        }

        clearInput();

        cout << "Enter Name: ";
        getline(cin, p.name);

        cout << "Enter Age: ";
        if (!(cin >> p.age) || p.age <= 0) {
            cout << "Invalid age.\n";
            cin.clear();
            clearInput();
            return;
        }

        clearInput();

        cout << "Enter Gender: ";
        getline(cin, p.gender);

        cout << "Enter Medical Condition: ";
        getline(cin, p.medicalCondition);

        int emergency;
        cout << "Emergency? (1=Yes, 0=No): ";
        if (!(cin >> emergency) ||
            (emergency != 0 && emergency != 1)) {
            cout << "Invalid choice.\n";
            cin.clear();
            clearInput();
            return;
        }

        p.emergency = (emergency == 1);
        p.priorityLevel = 0;

        if (p.emergency) {
            cout << "Priority (1-5, 5=highest): ";
            if (!(cin >> p.priorityLevel) ||
                p.priorityLevel < 1 || p.priorityLevel > 5) {
                cout << "Invalid priority.\n";
                cin.clear();
                clearInput();
                return;
            }
        }

        p.waiting = true;
        patients.push_back(p);

        cout << "Patient registered successfully!\n";
    }

    void displayPatients() {
        if (patients.empty()) {
            cout << "No patient records found.\n";
            return;
        }

        cout << "\n--- All Patient Records ---\n";
        for (const Patient& p : patients)
            showPatient(p);
    }

    void searchPatient() {
        if (patients.empty()) {
            cout << "No patient records found.\n";
            return;
        }

        int choice;
        cout << "1. Search by ID\n2. Search by Name\nChoice: ";
        if (!(cin >> choice)) {
            cout << "Invalid input.\n";
            cin.clear();
            clearInput();
            return;
        }

        bool found = false;

        if (choice == 1) {
            int id;
            cout << "Enter Patient ID: ";
            if (!(cin >> id)) {
                cout << "Invalid ID.\n";
                cin.clear();
                clearInput();
                return;
            }

            int index = findPatient(id);
            if (index != -1) {
                showPatient(patients[index]);
                found = true;
            }
        } else if (choice == 2) {
            string name;
            clearInput();
            cout << "Enter Patient Name: ";
            getline(cin, name);

            for (const Patient& p : patients) {
                if (p.name == name) {
                    showPatient(p);
                    found = true;
                }
            }
        } else {
            cout << "Invalid choice.\n";
            return;
        }

        if (!found)
            cout << "Patient not found.\n";
    }

    void editPatient() {
        int id;
        cout << "Enter Patient ID to edit: ";
        if (!(cin >> id)) {
            cout << "Invalid ID.\n";
            cin.clear();
            clearInput();
            return;
        }

        int index = findPatient(id);
        if (index == -1) {
            cout << "Patient not found.\n";
            return;
        }

        Patient& p = patients[index];
        clearInput();

        cout << "Enter new Name: ";
        getline(cin, p.name);

        cout << "Enter new Age: ";
        if (!(cin >> p.age) || p.age <= 0) {
            cout << "Invalid age. Edit cancelled.\n";
            cin.clear();
            clearInput();
            return;
        }

        clearInput();

        cout << "Enter new Gender: ";
        getline(cin, p.gender);

        cout << "Enter new Medical Condition: ";
        getline(cin, p.medicalCondition);

        if (p.emergency && p.waiting) {
            cout << "Enter new Priority (1-5): ";
            int priority;
            if (!(cin >> priority) || priority < 1 || priority > 5) {
                cout << "Invalid priority.\n";
                cin.clear();
                clearInput();
                return;
            }
            p.priorityLevel = priority;
        }

        cout << "Patient details updated successfully!\n";
    }

    void deletePatient() {
        int id;
        cout << "Enter Patient ID to delete: ";
        if (!(cin >> id)) {
            cout << "Invalid ID.\n";
            cin.clear();
            clearInput();
            return;
        }

        int index = findPatient(id);
        if (index == -1) {
            cout << "Patient not found.\n";
            return;
        }

        patients.erase(patients.begin() + index);
        cout << "Patient record deleted successfully.\n";
    }

    void displayQueues() {
        bool found = false;

        cout << "\n--- Emergency Queue ---\n";
        vector<Patient> emergencies;

        for (const Patient& p : patients) {
            if (p.waiting && p.emergency)
                emergencies.push_back(p);
        }

        sort(emergencies.begin(), emergencies.end(),
             [](const Patient& a, const Patient& b) {
                 return a.priorityLevel > b.priorityLevel;
             });

        if (emergencies.empty())
            cout << "No emergency patients waiting.\n";

        for (const Patient& p : emergencies) {
            cout << "ID: " << p.patientId
                 << " | Name: " << p.name
                 << " | Priority: " << p.priorityLevel << "\n";
            found = true;
        }

        cout << "\n--- Regular Queue (FIFO) ---\n";
        for (const Patient& p : patients) {
            if (p.waiting && !p.emergency) {
                cout << "ID: " << p.patientId
                     << " | Name: " << p.name << "\n";
                found = true;
            }
        }

        if (!found)
            cout << "No patients waiting.\n";
    }

    void attendNextPatient() {
        int selected = -1;

        // Emergency patients are attended first.
        for (int i = 0; i < (int)patients.size(); i++) {
            if (patients[i].waiting && patients[i].emergency) {
                if (selected == -1 ||
                    patients[i].priorityLevel >
                    patients[selected].priorityLevel) {
                    selected = i;
                }
            }
        }

        // Otherwise, attend the earliest waiting regular patient.
        if (selected == -1) {
            for (int i = 0; i < (int)patients.size(); i++) {
                if (patients[i].waiting && !patients[i].emergency) {
                    selected = i;
                    break;
                }
            }
        }

        if (selected == -1) {
            cout << "No patients waiting.\n";
            return;
        }

        patients[selected].waiting = false;

        cout << "Patient attended: "
             << patients[selected].name
             << " (ID: " << patients[selected].patientId << ")\n";
    }
};

int main() {
    HospitalSystem system;
    int choice;

    do {
        cout << "\n===== HOSPITAL PATIENT MANAGEMENT SYSTEM =====\n"
             << "1. Register Patient\n"
             << "2. Display All Patients\n"
             << "3. Search Patient\n"
             << "4. Edit Patient Details\n"
             << "5. Delete Patient Details\n"
             << "6. Attend Next Patient\n"
             << "7. Display Waiting Queues\n"
             << "8. Exit\n"
             << "Enter your choice: ";

        if (!(cin >> choice)) {
            cout << "Please enter a number.\n";
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            continue;
        }

        switch (choice) {
            case 1: system.registerPatient(); break;
            case 2: system.displayPatients(); break;
            case 3: system.searchPatient(); break;
            case 4: system.editPatient(); break;
            case 5: system.deletePatient(); break;
            case 6: system.attendNextPatient(); break;
            case 7: system.displayQueues(); break;
            case 8: cout << "Thank you for using the system!\n"; break;
            default: cout << "Invalid choice. Try again.\n";
        }

    } while (choice != 8);

    return 0;
}
