
#include <iostream>
#include <string>
#include <limits>
#include <vector>
#include <algorithm>

using namespace std;

// Terminal colors
const string RESET  = "\033[0m";
const string RED    = "\033[31m";
const string GREEN  = "\033[32m";
const string YELLOW = "\033[33m";
const string BLUE   = "\033[34m";
const string CYAN   = "\033[36m";
const string BOLD   = "\033[1m";

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
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }

    int findPatient(int id) {
        for (int i = 0; i < (int)patients.size(); i++) {
            if (patients[i].patientId == id) {
                return i;
            }
        }
        return -1;
    }

    void showPatient(const Patient& p) {
        cout << CYAN << "\nPatient ID: " << RESET
             << p.patientId;
        cout << CYAN << "\nName: " << RESET << p.name;
        cout << CYAN << "\nAge: " << RESET << p.age;
        cout << CYAN << "\nGender: " << RESET << p.gender;
        cout << CYAN << "\nCondition: " << RESET
             << p.medicalCondition;

        if (p.emergency) {
            cout << RED << BOLD
                 << "\nType: EMERGENCY"
                 << RESET;
            cout << YELLOW << "\nPriority: "
                 << p.priorityLevel << RESET;
        } else {
            cout << GREEN << "\nType: Regular"
                 << RESET;
        }

        cout << (p.waiting
            ? YELLOW + string("\nStatus: Waiting") + RESET
            : GREEN + string("\nStatus: Attended") + RESET);

        cout << "\n-----------------------------\n";
    }

public:
    void registerPatient() {
        Patient p;

        cout << BLUE << BOLD
             << "\n===== REGISTER PATIENT =====\n"
             << RESET;

        cout << "Enter Patient ID: ";
        if (!(cin >> p.patientId)) {
            clearInput();
            cout << RED << "Invalid ID!\n" << RESET;
            return;
        }

        if (findPatient(p.patientId) != -1) {
            clearInput();
            cout << RED
                 << "Patient ID already exists!\n"
                 << RESET;
            return;
        }

        clearInput();

        cout << "Enter Name: ";
        getline(cin, p.name);

        cout << "Enter Age: ";
        if (!(cin >> p.age) || p.age < 0) {
            clearInput();
            cout << RED << "Invalid age!\n" << RESET;
            return;
        }

        clearInput();

        cout << "Enter Gender: ";
        getline(cin, p.gender);

        cout << "Enter Medical Condition: ";
        getline(cin, p.medicalCondition);

        int choice;
        cout << "Is this an emergency patient? (1 = Yes, 0 = No): ";

        if (!(cin >> choice) || (choice != 0 && choice != 1)) {
            clearInput();
            cout << RED << "Enter 1 or 0 only!\n" << RESET;
            return;
        }

        p.emergency = (choice == 1);
        p.priorityLevel = 0;

        if (p.emergency) {
            cout << "Enter Priority (1 = Low, 5 = Highest): ";

            if (!(cin >> p.priorityLevel) ||
                p.priorityLevel < 1 ||
                p.priorityLevel > 5) {
                clearInput();
                cout << RED
                     << "Priority must be between 1 and 5!\n"
                     << RESET;
                return;
            }
        }

        p.waiting = true;
        patients.push_back(p);

        cout << GREEN << BOLD
             << "\nPatient registered successfully!\n"
             << RESET;
    }

    void displayPatients() {
        cout << BLUE << BOLD
             << "\n===== ALL PATIENTS =====\n"
             << RESET;

        if (patients.empty()) {
            cout << YELLOW << "No patient records found.\n"
                 << RESET;
            return;
        }

        for (const Patient& p : patients) {
            showPatient(p);
        }
    }

    void searchPatient() {
        cout << BLUE << BOLD
             << "\n===== SEARCH PATIENT =====\n"
             << RESET;

        cout << "Search by: 1. Patient ID  2. Name\n";
        cout << "Enter choice: ";

        int choice;
        if (!(cin >> choice) || (choice != 1 && choice != 2)) {
            clearInput();
            cout << RED << "Invalid choice!\n" << RESET;
            return;
        }

        if (choice == 1) {
            int id;
            cout << "Enter Patient ID: ";

            if (!(cin >> id)) {
                clearInput();
                cout << RED << "Invalid ID!\n" << RESET;
                return;
            }

            int index = findPatient(id);

            if (index != -1) {
                showPatient(patients[index]);
            } else {
                cout << RED << "Patient not found!\n" << RESET;
            }
        } else {
            clearInput();
            string name;

            cout << "Enter patient name: ";
            getline(cin, name);

            bool found = false;

            for (const Patient& p : patients) {
                if (p.name == name) {
                    showPatient(p);
                    found = true;
                }
            }

            if (!found) {
                cout << RED << "Patient not found!\n" << RESET;
            }
        }
    }

    void editPatient() {
        int id;
        cout << "\nEnter Patient ID to edit: ";

        if (!(cin >> id)) {
            clearInput();
            cout << RED << "Invalid ID!\n" << RESET;
            return;
        }

        int index = findPatient(id);

        if (index == -1) {
            cout << RED << "Patient not found!\n" << RESET;
            return;
        }

        Patient& p = patients[index];
        clearInput();

        cout << "Enter new name: ";
        getline(cin, p.name);

        cout << "Enter new age: ";
        if (!(cin >> p.age) || p.age < 0) {
            clearInput();
            cout << RED << "Invalid age!\n" << RESET;
            return;
        }

        clearInput();

        cout << "Enter new gender: ";
        getline(cin, p.gender);

        cout << "Enter new medical condition: ";
        getline(cin, p.medicalCondition);

        int emergencyChoice;
        cout << "Emergency patient? (1 = Yes, 0 = No): ";

        if (!(cin >> emergencyChoice) ||
            (emergencyChoice != 0 && emergencyChoice != 1)) {
            clearInput();
            cout << RED << "Enter 1 or 0 only!\n" << RESET;
            return;
        }

        p.emergency = (emergencyChoice == 1);
        p.priorityLevel = 0;

        if (p.emergency && p.waiting) {
            cout << "Enter Priority (1-5): ";

            if (!(cin >> p.priorityLevel) ||
                p.priorityLevel < 1 ||
                p.priorityLevel > 5) {
                clearInput();
                cout << RED
                     << "Priority must be between 1 and 5!\n"
                     << RESET;
                return;
            }
        }

        cout << GREEN << "Patient details updated!\n"
             << RESET;
    }

    void deletePatient() {
        int id;
        cout << "\nEnter Patient ID to delete: ";

        if (!(cin >> id)) {
            clearInput();
            cout << RED << "Invalid ID!\n" << RESET;
            return;
        }

        int index = findPatient(id);

        if (index == -1) {
            cout << RED << "Patient not found!\n" << RESET;
            return;
        }

        patients.erase(patients.begin() + index);

        cout << GREEN << "Patient deleted successfully!\n"
             << RESET;
    }

    void displayQueues() {
        vector<Patient> emergencyPatients;
        vector<Patient> regularPatients;

        for (const Patient& p : patients) {
            if (p.waiting) {
                if (p.emergency) {
                    emergencyPatients.push_back(p);
                } else {
                    regularPatients.push_back(p);
                }
            }
        }

        sort(emergencyPatients.begin(), emergencyPatients.end(),
            [](const Patient& a, const Patient& b) {
                return a.priorityLevel > b.priorityLevel;
            });

        cout << RED << BOLD
             << "\n===== EMERGENCY WAITING QUEUE =====\n"
             << RESET;

        if (emergencyPatients.empty()) {
            cout << "No emergency patients waiting.\n";
        }

        for (const Patient& p : emergencyPatients) {
            cout << RED << "ID: " << p.patientId
                 << " | Name: " << p.name
                 << " | Priority: " << p.priorityLevel
                 << RESET << "\n";
        }

        cout << GREEN << BOLD
             << "\n===== REGULAR WAITING QUEUE =====\n"
             << RESET;

        if (regularPatients.empty()) {
            cout << "No regular patients waiting.\n";
        }

        for (const Patient& p : regularPatients) {
            cout << GREEN << "ID: " << p.patientId
                 << " | Name: " << p.name
                 << RESET << "\n";
        }
    }

    void attendNextPatient() {
        int selected = -1;

        // Find the waiting emergency patient with highest priority.
        for (int i = 0; i < (int)patients.size(); i++) {
            if (patients[i].waiting && patients[i].emergency) {
                if (selected == -1 ||
                    patients[i].priorityLevel >
                    patients[selected].priorityLevel) {
                    selected = i;
                }
            }
        }

        // If no emergency patient is waiting, choose the
        // earliest registered regular patient.
        if (selected == -1) {
            for (int i = 0; i < (int)patients.size(); i++) {
                if (patients[i].waiting && !patients[i].emergency) {
                    selected = i;
                    break;
                }
            }
        }

        if (selected == -1) {
            cout << YELLOW
                 << "No patients are waiting.\n"
                 << RESET;
            return;
        }

        cout << GREEN << BOLD
             << "\nNext patient is being attended:\n"
             << RESET;

        showPatient(patients[selected]);
        patients[selected].waiting = false;

        cout << GREEN << BOLD
             << "Patient marked as attended.\n"
             << RESET;
    }

    void run() {
        int choice;

        do {
            cout << "\n";
            cout << CYAN << BOLD;
            cout << "========================================\n";
            cout << "   HOSPITAL PATIENT MANAGEMENT SYSTEM\n";
            cout << "========================================\n";
            cout << RESET;

            cout << BLUE   << "1. Register Patient\n" << RESET;
            cout << BLUE   << "2. Display All Patients\n" << RESET;
            cout << BLUE   << "3. Search Patient\n" << RESET;
            cout << BLUE   << "4. Edit Patient Details\n" << RESET;
            cout << BLUE   << "5. Delete Patient Details\n" << RESET;
            cout << GREEN  << "6. Attend Next Patient\n" << RESET;
            cout << YELLOW << "7. Display Waiting Queues\n" << RESET;
            cout << RED    << "8. Exit\n" << RESET;

            cout << CYAN << "\nEnter your choice: " << RESET;

            if (!(cin >> choice)) {
                clearInput();
                cout << RED << "Please enter a number from 1 to 8.\n"
                     << RESET;
                continue;
            }

            switch (choice) {
                case 1: registerPatient(); break;
                case 2: displayPatients(); break;
                case 3: searchPatient(); break;
                case 4: editPatient(); break;
                case 5: deletePatient(); break;
                case 6: attendNextPatient(); break;
                case 7: displayQueues(); break;
                case 8:
                    cout << GREEN << BOLD
                         << "Thank you for using the system!\n"
                         << RESET;
                    break;
                default:
                    cout << RED << "Invalid choice! Try again.\n"
                         << RESET;
            }

        } while (choice != 8);
    }
};

int main() {
    HospitalSystem hospital;
    hospital.run();
    return 0;
}
