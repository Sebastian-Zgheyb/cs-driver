#include "aimbot.h"

namespace driver {
	namespace codes {
		// Used to setup the driver.
		constexpr ULONG attach =
			CTL_CODE(FILE_DEVICE_UNKNOWN, 0x696, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);

		// Read process memory.
		constexpr ULONG read =
			CTL_CODE(FILE_DEVICE_UNKNOWN, 0x697, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);

		// Write process memory
		constexpr ULONG write =
			CTL_CODE(FILE_DEVICE_UNKNOWN, 0x698, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);
	} // namespace codes

	// Shared between user mode & kernel mode.
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

static bool Visible(HANDLE driver, uintptr_t localIndex, uintptr_t entityPawn)
{
    uintptr_t state = driver::read_memory<uintptr_t>(driver, entityPawn + cs2_dumper::schemas::client_dll::C_CSPlayerPawn::m_entitySpottedState 
        + cs2_dumper::schemas::client_dll::EntitySpottedState_t::m_bSpottedByMask);
    return state & (1 << (localIndex - 1));
}


void aimbot::frame(HANDLE driver, uintptr_t module_base) {
    // Read from the game using the driver
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

    // First pass to find local index
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

    // Calculate the relative angle and write to the game's memory using the driver
    if (targetFound) {
        vec3 relativeAngle = (enemyPos - localEyePos).RelativeAngle();
        // Read current view angles
        vec3 currentViewAngle = driver::read_memory<vec3>(driver, module_base + cs2_dumper::offsets::client_dll::dwViewAngles);

        // Calculate delta angle
        vec3 deltaAngle = (relativeAngle - currentViewAngle);

        // Scale delta angle by the smoothing factor
        vec3 smoothedAngle = currentViewAngle + deltaAngle * 0.8;

        // Write the smoothed angle back to the game
        driver::write_memory<vec3>(driver, module_base + cs2_dumper::offsets::client_dll::dwViewAngles, smoothedAngle);

    }
}

/*
before aimbot smoothing:
// Calculate the relative angle and write to the game's memory using the driver
    if (targetFound) {
        vec3 relativeAngle = (enemyPos - localEyePos).RelativeAngle();
        driver::write_memory<vec3>(driver, module_base + cs2_dumper::offsets::client_dll::dwViewAngles, relativeAngle);
    }

*/


/*
aimbot smoothing attempt
if (targetFound) {
        vec3 relativeAngle = (enemyPos - localEyePos).RelativeAngle();

        // Read the aim punch (recoil) angle
        vec3 aimPunch = driver::read_memory<vec3>(driver, localPlayerPawn + cs2_dumper::schemas::client_dll::C_CSPlayerPawn::m_aimPunchAngle);

        // Adjust relativeAngle by subtracting recoil
        relativeAngle = relativeAngle - (aimPunch * 2);

        // Read current view angles
        vec3 currentViewAngle = driver::read_memory<vec3>(driver, module_base + cs2_dumper::offsets::client_dll::dwViewAngles);

        // Calculate delta angle
        vec3 deltaAngle = (relativeAngle - currentViewAngle);

        // Scale delta angle by the smoothing factor
        vec3 smoothedAngle = currentViewAngle + deltaAngle * 0.9;

        // Write the smoothed angle back to the game
        driver::write_memory<vec3>(driver, module_base + cs2_dumper::offsets::client_dll::dwViewAngles, smoothedAngle);
    }
*/