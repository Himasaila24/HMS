/**
 * CarePulse HPM — Hospital Patient Management System (Standalone + Hybrid UI)
 * Mirrors the exact data model and triage rules of main.cpp & clo.cpp:
 *   - Patient { patientId, name, age, gender, medicalCondition, priorityLevel, emergency, waiting }
 *   - Emergency Queue: Waiting emergency patients sorted by priorityLevel (5 down to 1, FIFO tie-breaker)
 *   - Normal Queue: Waiting regular patients in First-In, First-Out (FIFO) registration order
 *   - Attend Next Patient: Highest-priority emergency patient first; otherwise earliest waiting regular patient
 *   - Works immediately when opened by double-clicking index.html (via localStorage)
 */

const STORAGE_KEY = 'hpm_patients_v1';

const DEFAULT_SAMPLE_PATIENTS = [
    {
        patientId: 101,
        name: 'Aarav Mehta',
        age: 34,
        gender: 'Male',
        medicalCondition: 'Seasonal Viral Fever',
        priorityLevel: 0,
        emergency: false,
        waiting: true
    },
    {
        patientId: 102,
        name: 'Priya Sharma',
        age: 28,
        gender: 'Female',
        medicalCondition: 'Migraine & Severe Fatigue',
        priorityLevel: 0,
        emergency: false,
        waiting: true
    },
    {
        patientId: 201,
        name: 'Rohan Verma',
        age: 49,
        gender: 'Male',
        medicalCondition: 'Compound Leg Fracture',
        priorityLevel: 3,
        emergency: true,
        waiting: true
    },
    {
        patientId: 202,
        name: 'Sneha Nair',
        age: 62,
        gender: 'Female',
        medicalCondition: 'Acute Chest Pain & Arrhythmia',
        priorityLevel: 5,
        emergency: true,
        waiting: true
    },
    {
        patientId: 103,
        name: 'Vikram Singh',
        age: 41,
        gender: 'Male',
        medicalCondition: 'Routine Blood Pressure Check',
        priorityLevel: 0,
        emergency: false,
        waiting: false
    },
    {
        patientId: 203,
        name: 'Kavya Iyer',
        age: 19,
        gender: 'Female',
        medicalCondition: 'Severe Asthma Attack',
        priorityLevel: 4,
        emergency: true,
        waiting: true
    }
];

let patients = [];
let pendingDeleteId = null;

// ============================================================================
// Storage & C++ Logic Helpers
// ============================================================================
function loadPatients() {
    try {
        const raw = localStorage.getItem(STORAGE_KEY);
        if (raw) {
            const parsed = JSON.parse(raw);
            if (Array.isArray(parsed)) {
                patients = parsed;
                return;
            }
        }
    } catch (_) {
        // Fallback to in-memory default sample data
    }
    patients = DEFAULT_SAMPLE_PATIENTS.map((p) => ({ ...p }));
    savePatients();
}

function savePatients() {
    try {
        localStorage.setItem(STORAGE_KEY, JSON.stringify(patients));
    } catch (_) {
        // Ignore storage errors in restricted environments
    }
}

function findPatientIndex(id) {
    for (let i = 0; i < patients.length; i++) {
        if (patients[i].patientId === id) {
            return i;
        }
    }
    return -1;
}

// Mirrors main.cpp / clo.cpp emergency queue sorting (Priority 5 -> 1, preserving registration order on ties)
function getEmergencyWaitingQueue() {
    return patients
        .map((p, idx) => ({ ...p, _regOrder: idx }))
        .filter((p) => p.waiting && p.emergency)
        .sort((a, b) => {
            if (b.priorityLevel !== a.priorityLevel) {
                return b.priorityLevel - a.priorityLevel;
            }
            return a._regOrder - b._regOrder;
        });
}

// Mirrors main.cpp / clo.cpp regular FIFO queue
function getRegularWaitingQueue() {
    return patients.filter((p) => p.waiting && !p.emergency);
}

// Mirrors main.cpp / clo.cpp attendNextPatient selection logic
function findNextPatientIndexToAttend() {
    let selected = -1;

    // 1. Highest-priority waiting emergency patient first
    for (let i = 0; i < patients.length; i++) {
        if (patients[i].waiting && patients[i].emergency) {
            if (selected === -1 || patients[i].priorityLevel > patients[selected].priorityLevel) {
                selected = i;
            }
        }
    }

    // 2. Otherwise earliest registered waiting regular patient (FIFO)
    if (selected === -1) {
        for (let i = 0; i < patients.length; i++) {
            if (patients[i].waiting && !patients[i].emergency) {
                selected = i;
                break;
            }
        }
    }

    return selected;
}

// ============================================================================
// UI Formatting & Toast Helpers
// ============================================================================
function escapeHtml(value) {
    if (value === null || value === undefined) return '';
    return String(value)
        .replace(/&/g, '&amp;')
        .replace(/</g, '&lt;')
        .replace(/>/g, '&gt;')
        .replace(/"/g, '&quot;')
        .replace(/'/g, '&#039;');
}

function showToast(message, type = 'success') {
    const container = document.getElementById('toastContainer');
    const toast = document.createElement('div');
    toast.className = `toast toast-${type}`;
    toast.innerHTML = `<span>${escapeHtml(message)}</span>`;
    container.appendChild(toast);

    setTimeout(() => {
        if (toast.parentElement) {
            toast.remove();
        }
    }, 3600);
}

function renderTypeBadge(p) {
    if (p.emergency) {
        return `<span class="badge badge-emergency">Emergency</span>`;
    }
    return `<span class="badge badge-regular">Normal (FIFO)</span>`;
}

function renderPriorityBadge(p) {
    if (!p.emergency) {
        return `<span class="badge badge-regular">Standard</span>`;
    }
    const level = Math.min(5, Math.max(1, Number(p.priorityLevel) || 1));
    return `<span class="badge prio-${level}">Priority ${level}</span>`;
}

function renderStatusBadge(p) {
    if (p.waiting) {
        return `<span class="badge badge-waiting">Waiting</span>`;
    }
    return `<span class="badge badge-attended">Attended</span>`;
}

// ============================================================================
// Rendering Functions
// ============================================================================
function renderAll() {
    const total = patients.length;
    const emergencyWaiting = getEmergencyWaitingQueue();
    const regularWaiting = getRegularWaitingQueue();
    const totalWaiting = emergencyWaiting.length + regularWaiting.length;
    const totalAttended = patients.filter((p) => !p.waiting).length;

    // Navigation pills
    document.getElementById('navTotalCount').textContent = total;
    document.getElementById('navWaitingCount').textContent = totalWaiting;

    // Summary Cards
    document.getElementById('cardTotalPatients').textContent = total;
    document.getElementById('cardWaitingPatients').textContent = totalWaiting;
    document.getElementById('cardWaitingSub').textContent =
        `${emergencyWaiting.length} emergency • ${regularWaiting.length} normal`;
    document.getElementById('cardEmergencyPatients').textContent = emergencyWaiting.length;
    document.getElementById('cardAttendedPatients').textContent = totalAttended;

    // Next Patient Spotlight
    const nextIdx = findNextPatientIndexToAttend();
    const nextDisplay = document.getElementById('nextPatientDisplay');
    const queueBannerNextText = document.getElementById('queueBannerNextText');

    if (nextIdx !== -1) {
        const nextP = patients[nextIdx];
        const queueTag = nextP.emergency
            ? `EMERGENCY (Priority ${nextP.priorityLevel})`
            : 'NORMAL QUEUE (FIFO)';
        nextDisplay.innerHTML = `
            <strong>${escapeHtml(nextP.name)}</strong> (ID: #${nextP.patientId}, ${nextP.age}y ${escapeHtml(nextP.gender)})
            &mdash; <span>${escapeHtml(nextP.medicalCondition)}</span>
            <span class="badge ${nextP.emergency ? 'badge-emergency' : 'badge-regular'}" style="margin-left:8px;">${queueTag}</span>
        `;
        queueBannerNextText.textContent =
            `Next to be attended: ${nextP.name} (ID #${nextP.patientId}) — ${queueTag}`;
    } else {
        nextDisplay.textContent = 'All registered patients have been attended! No patients waiting.';
        queueBannerNextText.textContent = 'No patients currently waiting in either queue.';
    }

    renderDashboardMiniQueues(emergencyWaiting, regularWaiting);
    renderRecentPatientsTable();
    renderManagePatientsTable();
    renderWaitingQueueTables(emergencyWaiting, regularWaiting);
}

function renderDashboardMiniQueues(emergencyWaiting, regularWaiting) {
    document.getElementById('dashEmergencyBadge').textContent = `${emergencyWaiting.length} Waiting`;
    document.getElementById('dashRegularBadge').textContent = `${regularWaiting.length} Waiting`;

    const emList = document.getElementById('dashEmergencyList');
    if (emergencyWaiting.length === 0) {
        emList.innerHTML = `<li class="mini-queue-item"><span>No emergency patients waiting</span></li>`;
    } else {
        emList.innerHTML = emergencyWaiting
            .slice(0, 4)
            .map(
                (p, i) => `
                <li class="mini-queue-item">
                    <span><strong>#${i + 1} ${escapeHtml(p.name)}</strong> (ID: ${p.patientId}) — ${escapeHtml(p.medicalCondition)}</span>
                    ${renderPriorityBadge(p)}
                </li>`
            )
            .join('');
    }

    const regList = document.getElementById('dashRegularList');
    if (regularWaiting.length === 0) {
        regList.innerHTML = `<li class="mini-queue-item"><span>No normal queue patients waiting</span></li>`;
    } else {
        regList.innerHTML = regularWaiting
            .slice(0, 4)
            .map(
                (p, i) => `
                <li class="mini-queue-item">
                    <span><strong>#${i + 1} ${escapeHtml(p.name)}</strong> (ID: ${p.patientId}) — ${escapeHtml(p.medicalCondition)}</span>
                    <span class="badge badge-regular">FIFO #${i + 1}</span>
                </li>`
            )
            .join('');
    }
}

function renderRecentPatientsTable() {
    const tbody = document.getElementById('recentPatientsTableBody');
    if (patients.length === 0) {
        tbody.innerHTML = `<tr><td colspan="5" class="empty-row">No patient records found.</td></tr>`;
        return;
    }

    const recent = [...patients].reverse().slice(0, 6);
    tbody.innerHTML = recent
        .map(
            (p) => `
            <tr>
                <td><strong>#${p.patientId}</strong></td>
                <td>
                    <div><strong>${escapeHtml(p.name)}</strong></div>
                    <small style="color:#64748b">${p.age} yrs • ${escapeHtml(p.gender)}</small>
                </td>
                <td>${escapeHtml(p.medicalCondition)}</td>
                <td>${p.emergency ? renderPriorityBadge(p) : renderTypeBadge(p)}</td>
                <td>${renderStatusBadge(p)}</td>
            </tr>`
        )
        .join('');
}

function renderManagePatientsTable() {
    const tbody = document.getElementById('allPatientsTableBody');
    const query = document.getElementById('searchQuery').value.trim().toLowerCase();
    const typeFilter = document.getElementById('filterType').value;
    const statusFilter = document.getElementById('filterStatus').value;

    const filtered = patients.filter((p) => {
        const matchesQuery =
            !query ||
            String(p.patientId).includes(query) ||
            p.name.toLowerCase().includes(query) ||
            p.medicalCondition.toLowerCase().includes(query);

        const matchesType =
            typeFilter === 'all' ||
            (typeFilter === 'emergency' && p.emergency) ||
            (typeFilter === 'regular' && !p.emergency);

        const matchesStatus =
            statusFilter === 'all' ||
            (statusFilter === 'waiting' && p.waiting) ||
            (statusFilter === 'attended' && !p.waiting);

        return matchesQuery && matchesType && matchesStatus;
    });

    if (filtered.length === 0) {
        tbody.innerHTML = `<tr><td colspan="8" class="empty-row">No matching patient records found.</td></tr>`;
        return;
    }

    tbody.innerHTML = filtered
        .map(
            (p) => `
            <tr>
                <td><strong>#${p.patientId}</strong></td>
                <td><strong>${escapeHtml(p.name)}</strong></td>
                <td>${p.age} yrs • ${escapeHtml(p.gender)}</td>
                <td>${escapeHtml(p.medicalCondition)}</td>
                <td>${renderTypeBadge(p)}</td>
                <td>${renderPriorityBadge(p)}</td>
                <td>${renderStatusBadge(p)}</td>
                <td class="text-right">
                    <div class="action-group">
                        ${
                            p.waiting
                                ? `<button class="btn btn-sm btn-emerald" onclick="attendSpecificPatient(${p.patientId})">Attend</button>`
                                : ''
                        }
                        <button class="btn btn-sm btn-ghost" onclick="openEditModal(${p.patientId})">Edit</button>
                        <button class="btn btn-sm btn-danger" onclick="openDeleteModal(${p.patientId})">Delete</button>
                    </div>
                </td>
            </tr>`
        )
        .join('');
}

function renderWaitingQueueTables(emergencyWaiting, regularWaiting) {
    document.getElementById('emergencyCountBadge').textContent = `${emergencyWaiting.length} Waiting`;
    document.getElementById('regularCountBadge').textContent = `${regularWaiting.length} Waiting`;

    const emBody = document.getElementById('emergencyQueueTableBody');
    if (emergencyWaiting.length === 0) {
        emBody.innerHTML = `<tr><td colspan="6" class="empty-row">No emergency patients waiting in queue.</td></tr>`;
    } else {
        emBody.innerHTML = emergencyWaiting
            .map(
                (p, idx) => `
                <tr>
                    <td><strong>#${idx + 1}</strong></td>
                    <td>#${p.patientId}</td>
                    <td>
                        <strong>${escapeHtml(p.name)}</strong>
                        <div style="font-size:0.76rem;color:#64748b">${p.age}y • ${escapeHtml(p.gender)}</div>
                    </td>
                    <td>${escapeHtml(p.medicalCondition)}</td>
                    <td>${renderPriorityBadge(p)}</td>
                    <td>
                        <button class="btn btn-sm btn-emerald" onclick="attendSpecificPatient(${p.patientId})">Attend</button>
                    </td>
                </tr>`
            )
            .join('');
    }

    const regBody = document.getElementById('regularQueueTableBody');
    if (regularWaiting.length === 0) {
        regBody.innerHTML = `<tr><td colspan="6" class="empty-row">No regular patients waiting in FIFO queue.</td></tr>`;
    } else {
        regBody.innerHTML = regularWaiting
            .map(
                (p, idx) => `
                <tr>
                    <td><strong>#${idx + 1}</strong></td>
                    <td>#${p.patientId}</td>
                    <td>
                        <strong>${escapeHtml(p.name)}</strong>
                        <div style="font-size:0.76rem;color:#64748b">${p.age}y • ${escapeHtml(p.gender)}</div>
                    </td>
                    <td>${escapeHtml(p.medicalCondition)}</td>
                    <td><span class="badge badge-regular">FIFO #${idx + 1}</span></td>
                    <td>
                        <button class="btn btn-sm btn-emerald" onclick="attendSpecificPatient(${p.patientId})">Attend</button>
                    </td>
                </tr>`
            )
            .join('');
    }
}

// ============================================================================
// Core Actions (Register, Attend, Edit, Delete)
// ============================================================================
function handleRegisterPatient(event) {
    event.preventDefault();

    const idVal = parseInt(document.getElementById('regId').value, 10);
    const nameVal = document.getElementById('regName').value.trim();
    const ageVal = parseInt(document.getElementById('regAge').value, 10);
    const genderVal = document.getElementById('regGender').value;
    const conditionVal = document.getElementById('regCondition').value.trim();
    const selectedType = document.querySelector('input[name="regType"]:checked').value;
    const isEmergency = selectedType === 'emergency';
    const priorityVal = isEmergency ? parseInt(document.getElementById('regPriority').value, 10) : 0;

    if (!Number.isInteger(idVal) || idVal <= 0) {
        showToast('Please enter a valid positive Patient ID.', 'error');
        return;
    }

    if (findPatientIndex(idVal) !== -1) {
        showToast(`Patient ID #${idVal} already exists!`, 'error');
        return;
    }

    if (!nameVal) {
        showToast('Please enter the patient name.', 'error');
        return;
    }

    if (!Number.isInteger(ageVal) || ageVal <= 0 || ageVal > 130) {
        showToast('Please enter a valid patient age (1–130).', 'error');
        return;
    }

    if (!genderVal) {
        showToast('Please select a gender.', 'error');
        return;
    }

    if (!conditionVal) {
        showToast('Please enter the medical condition.', 'error');
        return;
    }

    if (isEmergency && (priorityVal < 1 || priorityVal > 5)) {
        showToast('Emergency priority must be between 1 and 5.', 'error');
        return;
    }

    patients.push({
        patientId: idVal,
        name: nameVal,
        age: ageVal,
        gender: genderVal,
        medicalCondition: conditionVal,
        priorityLevel: priorityVal,
        emergency: isEmergency,
        waiting: true
    });

    savePatients();
    renderAll();
    resetRegisterForm();
    showToast(`Patient ${nameVal} (ID #${idVal}) registered successfully!`, 'success');
    switchView('dashboard');
}

function resetRegisterForm() {
    document.getElementById('registerForm').reset();
    document.getElementById('optRegularCard').classList.add('selected');
    document.getElementById('optEmergencyCard').classList.remove('selected');
    document.getElementById('regPriorityWrapper').classList.add('hidden');
}

function attendNextPatient() {
    const idx = findNextPatientIndexToAttend();
    if (idx === -1) {
        showToast('No patients are currently waiting in queue.', 'info');
        return;
    }

    patients[idx].waiting = false;
    const attended = patients[idx];
    savePatients();
    renderAll();

    const label = attended.emergency
        ? `Emergency Priority ${attended.priorityLevel}`
        : 'Normal Queue';
    showToast(`Attended: ${attended.name} (ID #${attended.patientId} — ${label})`, 'success');
}

window.attendSpecificPatient = function (patientId) {
    const idx = findPatientIndex(patientId);
    if (idx === -1) return;
    patients[idx].waiting = false;
    savePatients();
    renderAll();
    showToast(`Marked ${patients[idx].name} (ID #${patientId}) as Attended.`, 'success');
};

window.openEditModal = function (patientId) {
    const idx = findPatientIndex(patientId);
    if (idx === -1) return;
    const p = patients[idx];

    document.getElementById('editId').value = p.patientId;
    document.getElementById('editName').value = p.name;
    document.getElementById('editAge').value = p.age;
    document.getElementById('editGender').value = p.gender;
    document.getElementById('editCondition').value = p.medicalCondition;
    document.getElementById('editEmergency').value = p.emergency ? 'true' : 'false';
    document.getElementById('editWaiting').value = p.waiting ? 'true' : 'false';
    document.getElementById('editPriority').value = p.priorityLevel >= 1 ? p.priorityLevel : 3;

    const prioGroup = document.getElementById('editPriorityGroup');
    if (p.emergency) {
        prioGroup.classList.remove('hidden');
    } else {
        prioGroup.classList.add('hidden');
    }

    document.getElementById('editModal').classList.remove('hidden');
};

function closeEditModal() {
    document.getElementById('editModal').classList.add('hidden');
}

function handleSaveEdit(event) {
    event.preventDefault();
    const idVal = parseInt(document.getElementById('editId').value, 10);
    const idx = findPatientIndex(idVal);
    if (idx === -1) return;

    const nameVal = document.getElementById('editName').value.trim();
    const ageVal = parseInt(document.getElementById('editAge').value, 10);
    const genderVal = document.getElementById('editGender').value;
    const conditionVal = document.getElementById('editCondition').value.trim();
    const isEmergency = document.getElementById('editEmergency').value === 'true';
    const isWaiting = document.getElementById('editWaiting').value === 'true';
    const priorityVal = isEmergency ? parseInt(document.getElementById('editPriority').value, 10) : 0;

    if (!nameVal || !conditionVal || !Number.isInteger(ageVal) || ageVal <= 0) {
        showToast('Please fill out all fields with valid values.', 'error');
        return;
    }

    patients[idx] = {
        ...patients[idx],
        name: nameVal,
        age: ageVal,
        gender: genderVal,
        medicalCondition: conditionVal,
        emergency: isEmergency,
        waiting: isWaiting,
        priorityLevel: priorityVal
    };

    savePatients();
    renderAll();
    closeEditModal();
    showToast(`Updated patient record #${idVal} (${nameVal}).`, 'success');
}

window.openDeleteModal = function (patientId) {
    const idx = findPatientIndex(patientId);
    if (idx === -1) return;
    pendingDeleteId = patientId;
    const p = patients[idx];
    document.getElementById('deleteModalText').textContent =
        `Are you sure you want to delete ${p.name} (Patient ID #${p.patientId})? This action cannot be undone.`;
    document.getElementById('deleteModal').classList.remove('hidden');
};

function closeDeleteModal() {
    pendingDeleteId = null;
    document.getElementById('deleteModal').classList.add('hidden');
}

function confirmDeletePatient() {
    if (pendingDeleteId === null) return;
    const idx = findPatientIndex(pendingDeleteId);
    if (idx !== -1) {
        const removed = patients.splice(idx, 1)[0];
        savePatients();
        renderAll();
        showToast(`Deleted patient record: ${removed.name} (ID #${removed.patientId}).`, 'info');
    }
    closeDeleteModal();
}

// ============================================================================
// Navigation & Event Listeners
// ============================================================================
function switchView(viewName) {
    document.querySelectorAll('.nav-btn').forEach((btn) => {
        btn.classList.toggle('active', btn.dataset.view === viewName);
    });
    document.querySelectorAll('.view-panel').forEach((panel) => {
        panel.classList.toggle('active', panel.id === `view-${viewName}`);
    });
    closeMobileSidebar();
}

function openMobileSidebar() {
    document.getElementById('sidebar').classList.add('open');
    document.getElementById('sidebarBackdrop').classList.add('open');
}

function closeMobileSidebar() {
    document.getElementById('sidebar').classList.remove('open');
    document.getElementById('sidebarBackdrop').classList.remove('open');
}

document.addEventListener('DOMContentLoaded', () => {
    loadPatients();
    renderAll();

    // Sidebar view navigation
    document.querySelectorAll('.nav-btn').forEach((btn) => {
        btn.addEventListener('click', () => switchView(btn.dataset.view));
    });

    // Quick view jump buttons
    document.querySelectorAll('[data-goto]').forEach((btn) => {
        btn.addEventListener('click', () => switchView(btn.dataset.goto));
    });

    // Mobile sidebar controls
    document.getElementById('openSidebarBtn').addEventListener('click', openMobileSidebar);
    document.getElementById('closeSidebarBtn').addEventListener('click', closeMobileSidebar);
    document.getElementById('sidebarBackdrop').addEventListener('click', closeMobileSidebar);

    // Topbar & Spotlight buttons
    document.getElementById('topAttendNextBtn').addEventListener('click', attendNextPatient);
    document.getElementById('dashAttendBtn').addEventListener('click', attendNextPatient);
    document.getElementById('queueAttendNextBtn').addEventListener('click', attendNextPatient);
    document.getElementById('topNewPatientBtn').addEventListener('click', () => switchView('register'));

    // Reset sample data
    document.getElementById('resetSampleDataBtn').addEventListener('click', () => {
        patients = DEFAULT_SAMPLE_PATIENTS.map((p) => ({ ...p }));
        savePatients();
        renderAll();
        showToast('Demo patient records have been reset!', 'info');
    });

    // Register form type toggle
    document.querySelectorAll('input[name="regType"]').forEach((radio) => {
        radio.addEventListener('change', (e) => {
            const isEm = e.target.value === 'emergency';
            document.getElementById('optRegularCard').classList.toggle('selected', !isEm);
            document.getElementById('optEmergencyCard').classList.toggle('selected', isEm);
            document.getElementById('regPriorityWrapper').classList.toggle('hidden', !isEm);
        });
    });

    document.getElementById('registerForm').addEventListener('submit', handleRegisterPatient);
    document.getElementById('clearRegisterBtn').addEventListener('click', resetRegisterForm);

    // Search & Filters
    document.getElementById('searchQuery').addEventListener('input', renderManagePatientsTable);
    document.getElementById('filterType').addEventListener('change', renderManagePatientsTable);
    document.getElementById('filterStatus').addEventListener('change', renderManagePatientsTable);
    document.getElementById('resetSearchBtn').addEventListener('click', () => {
        document.getElementById('searchQuery').value = '';
        document.getElementById('filterType').value = 'all';
        document.getElementById('filterStatus').value = 'all';
        renderManagePatientsTable();
    });

    // Edit Modal
    document.getElementById('editEmergency').addEventListener('change', (e) => {
        const isEm = e.target.value === 'true';
        document.getElementById('editPriorityGroup').classList.toggle('hidden', !isEm);
    });
    document.getElementById('editForm').addEventListener('submit', handleSaveEdit);
    document.getElementById('closeEditModalBtn').addEventListener('click', closeEditModal);
    document.getElementById('cancelEditBtn').addEventListener('click', closeEditModal);

    // Delete Modal
    document.getElementById('closeDeleteModalBtn').addEventListener('click', closeDeleteModal);
    document.getElementById('cancelDeleteBtn').addEventListener('click', closeDeleteModal);
    document.getElementById('confirmDeleteBtn').addEventListener('click', confirmDeletePatient);
});
