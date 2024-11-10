#include "aimbot.h"
#include "../renderer/renderer.h"
#include <cmath>

namespace driver {
    namespace codes {
        constexpr ULONG attach = CTL_CODE(FILE_DEVICE_UNKNOWN, 0x696, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);
        constexpr ULONG read = CTL_CODE(FILE_DEVICE_UNKNOWN, 0x697, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);
        constexpr ULONG write = CTL_CODE(FILE_DEVICE_UNKNOWN, 0x698, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);
    }

    struct Request {
        HANDLE process_id;
        PVOID target;
        PVOID buffer;
        SIZE_T size;
        SIZE_T return_size;
    };

    template <class T>
    T read_memory(HANDLE driver_handle, const std::uintptr_t addr) {
        T temp = {};
        Request r;
        r.target = reinterpret_cast<PVOID>(addr);
        r.buffer = &temp;
        r.size = sizeof(T);
        DeviceIoControl(driver_handle, codes::read, &r, sizeof(r), &r, sizeof(r), nullptr, nullptr);
        return temp;
    }

    template <class T>
    void write_memory(HANDLE driver_handle, const std::uintptr_t addr, const T& value) {
        Request r;
        r.target = reinterpret_cast<PVOID>(addr);
        r.buffer = (PVOID)&value;
        r.size = sizeof(T);
        DeviceIoControl(driver_handle, codes::write, &r, sizeof(r), &r, sizeof(r), nullptr, nullptr);
    }
}

float aimbot::distance(vec3 p1, vec3 p2) {
    return sqrt(pow(p1.x - p2.x, 2) + pow(p1.y - p2.y, 2) + pow(p1.z - p2.z, 2));
}

// Calculate the angle between two vectors
float angleBetween(vec3 a, vec3 b) {
    float dotProduct = a.x * b.x + a.y * b.y + a.z * b.z;
    float magnitudeA = sqrt(a.x * a.x + a.y * a.y + a.z * a.z);
    float magnitudeB = sqrt(b.x * b.x + b.y * b.y + b.z * b.z);
    return acos(dotProduct / (magnitudeA * magnitudeB)) * (180.0f / 3.141592653589793f); // angle return value
}

// Determine if the enemy is seen by the local player
static bool Visible(HANDLE driver, uintptr_t localIndex, uintptr_t entityPawn) {
    uintptr_t state = driver::read_memory<uintptr_t>(driver, entityPawn + cs2_dumper::schemas::client_dll::C_CSPlayerPawn::m_entitySpottedState
        + cs2_dumper::schemas::client_dll::EntitySpottedState_t::m_bSpottedByMask);
    return state & (1 << (localIndex - 1));
}

void aimbot::frame(HANDLE driver, uintptr_t module_base) {
    renderer::draw::text(L"Aimbot: ON (F2 to toggle)", 10, 40, D3DCOLOR_XRGB(0, 255, 0));

    uintptr_t entityList = driver::read_memory<uintptr_t>(driver, module_base + cs2_dumper::offsets::client_dll::dwEntityList);
    if (!entityList) return;

    uintptr_t localPlayerPawn = driver::read_memory<uintptr_t>(driver, module_base + cs2_dumper::offsets::client_dll::dwLocalPlayerPawn);
    if (!localPlayerPawn) return;

    BYTE team = driver::read_memory<BYTE>(driver, localPlayerPawn + cs2_dumper::schemas::client_dll::C_BaseEntity::m_iTeamNum);
    vec3 localEyePos = driver::read_memory<vec3>(driver, localPlayerPawn + cs2_dumper::schemas::client_dll::C_BasePlayerPawn::m_vOldOrigin) +
        driver::read_memory<vec3>(driver, localPlayerPawn + cs2_dumper::schemas::client_dll::C_BaseModelEntity::m_vecViewOffset);

    float closest_distance = -1;
    vec3 enemyPos;
    bool targetFound = false;
    int localIndex = -1;

    for (int i = 0; i < 32; i++) {
        uintptr_t listEntry = driver::read_memory<uintptr_t>(driver, entityList + ((8 * (i & 0x7ff) >> 9) + 16));
        if (!listEntry) continue;

        uintptr_t entityController = driver::read_memory<uintptr_t>(driver, listEntry + 120 * (i & 0x1ff));
        if (!entityController) continue;

        uintptr_t entityControllerPawn = driver::read_memory<uintptr_t>(driver, entityController + cs2_dumper::schemas::client_dll::CCSPlayerController::m_hPlayerPawn);
        if (!entityControllerPawn) continue;

        uintptr_t entityPawn = driver::read_memory<uintptr_t>(driver, listEntry + 120 * (entityControllerPawn & 0x1ff));

        if (entityPawn == localPlayerPawn) {
            localIndex = i;
            continue;
        }
    }

    for (int i = 0; i < 32; i++) {
        uintptr_t listEntry = driver::read_memory<uintptr_t>(driver, entityList + ((8 * (i & 0x7ff) >> 9) + 16));
        if (!listEntry) continue;

        uintptr_t entityController = driver::read_memory<uintptr_t>(driver, listEntry + 120 * (i & 0x1ff));
        if (!entityController) continue;

        uintptr_t entityControllerPawn = driver::read_memory<uintptr_t>(driver, entityController + cs2_dumper::schemas::client_dll::CCSPlayerController::m_hPlayerPawn);
        if (!entityControllerPawn) continue;

        uintptr_t entityPawn = driver::read_memory<uintptr_t>(driver, listEntry + 120 * (entityControllerPawn & 0x1ff));

        if (team == driver::read_memory<BYTE>(driver, entityPawn + cs2_dumper::schemas::client_dll::C_BaseEntity::m_iTeamNum))
            continue;

        if (driver::read_memory<std::uint32_t>(driver, entityPawn + cs2_dumper::schemas::client_dll::C_BaseEntity::m_iHealth) <= 0)
            continue;

        if (!Visible(driver, localIndex, entityPawn)) continue;

        vec3 entityEyePos = driver::read_memory<vec3>(driver, entityPawn + cs2_dumper::schemas::client_dll::C_BasePlayerPawn::m_vOldOrigin) +
            driver::read_memory<vec3>(driver, entityPawn + cs2_dumper::schemas::client_dll::C_BaseModelEntity::m_vecViewOffset);

        float current_distance = aimbot::distance(localEyePos, entityEyePos);

        if (closest_distance < 0 || current_distance < closest_distance) {
            closest_distance = current_distance;
            enemyPos = entityEyePos;
            targetFound = true;
        }
    }

    if (targetFound) {
        vec3 relativeAngle = (enemyPos - localEyePos).RelativeAngle();
        vec3 currentViewAngle = driver::read_memory<vec3>(driver, module_base + cs2_dumper::offsets::client_dll::dwViewAngles);

        vec3 deltaAngle = (relativeAngle - currentViewAngle);

        float angleDiff = angleBetween(relativeAngle, currentViewAngle);

        if (angleDiff > 20.0f) {
            return;
        }
        //smooth aim
        vec3 smoothedAngle = currentViewAngle + deltaAngle * 0.8f;
        driver::write_memory<vec3>(driver, module_base + cs2_dumper::offsets::client_dll::dwViewAngles, smoothedAngle);
    }
}
