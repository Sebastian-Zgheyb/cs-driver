#include "wallhack.h"

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

        BOOL success = DeviceIoControl(driver_handle, codes::read, &r, sizeof(r), &r, sizeof(r), nullptr, nullptr);
      
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

void wallhack::loop(HANDLE driver_handle, uintptr_t modBase) {
    uintptr_t entity_list = driver::read_memory<uintptr_t>(driver_handle, modBase + cs2_dumper::offsets::client_dll::dwEntityList);
    uintptr_t localPlayerPawn = driver::read_memory<uintptr_t>(driver_handle, modBase + cs2_dumper::offsets::client_dll::dwLocalPlayerPawn);
    BYTE team = driver::read_memory<BYTE>(driver_handle, localPlayerPawn + cs2_dumper::schemas::client_dll::C_BaseEntity::m_iTeamNum);

    std::vector<uintptr_t> buffer;

    for (int i = 1; i < 32; i++) {
        uintptr_t listEntry = driver::read_memory<uintptr_t>(driver_handle, entity_list + ((8 * (i & 0x1ff) >> 9) + 16));
        if (!listEntry) continue;

        uintptr_t entityController = driver::read_memory<uintptr_t>(driver_handle, listEntry + 120 * (i & 0x1ff));
        if (!entityController) continue;

        uintptr_t entityControllerPawn = driver::read_memory<uintptr_t>(driver_handle, entityController + cs2_dumper::schemas::client_dll::CCSPlayerController::m_hPlayerPawn);
        if (!entityControllerPawn) continue;

        uintptr_t entity = driver::read_memory<uintptr_t>(driver_handle, listEntry + 120 * (entityControllerPawn & 0x1ff));
        if (entity) buffer.emplace_back(entity);
    }

    entities = buffer;
    Sleep(10);
}

void wallhack::frame(HANDLE driver_handle, uintptr_t modBase) { 
    renderer::pDevice->Clear(0, 0, D3DCLEAR_TARGET, NULL, 1.f, 0);
    renderer::pDevice->BeginScene();

    // Read and render view matrix
    viewMatrix vm = driver::read_memory<viewMatrix>(driver_handle, modBase + cs2_dumper::offsets::client_dll::dwViewMatrix);
    render(driver_handle, vm, entities);
    
    renderer::pDevice->EndScene();
    renderer::pDevice->Present(0, 0, 0, 0);
}

void wallhack::render(HANDLE driver_handle, const viewMatrix& vm, const std::vector<uintptr_t>& entities) {
    for (uintptr_t entity : entities) {
        vec3 absOrigin = driver::read_memory<vec3>(driver_handle, entity + cs2_dumper::schemas::client_dll::C_BasePlayerPawn::m_vOldOrigin);
        vec3 eyePos = absOrigin + driver::read_memory<vec3>(driver_handle, entity + cs2_dumper::schemas::client_dll::C_BaseModelEntity::m_vecViewOffset);

        vec2 head, feet;

        if (w2s(absOrigin, head, vm.m) && w2s(eyePos, feet, vm.m)) {
            float width = (head.y - feet.y);
            feet.x += width;
            feet.y -= width;

            renderer::draw::box(D3DXVECTOR2{ head.x, head.y }, D3DXVECTOR2{ feet.x, feet.y }, D3DCOLOR_XRGB(255, 255, 255));
        }
    }
}

bool wallhack::w2s(const vec3& world, vec2& screen, const float m[16]) {
    vec4 clipCoords;
    clipCoords.x = world.x * m[0] + world.y * m[1] + world.z * m[2] + m[3];
    clipCoords.y = world.x * m[4] + world.y * m[5] + world.z * m[6] + m[7];
    clipCoords.z = world.x * m[8] + world.y * m[9] + world.z * m[10] + m[11];
    clipCoords.w = world.x * m[12] + world.y * m[13] + world.z * m[14] + m[15];

    if (clipCoords.w < 0.1f) return false;

    vec3 ndc;
    ndc.x = clipCoords.x / clipCoords.w;
    ndc.y = clipCoords.y / clipCoords.w;

    screen.x = (WINDOW_W / 2 * ndc.x) + (ndc.x + WINDOW_W / 2);
    screen.y = -(WINDOW_H / 2 * ndc.y) + (ndc.y + WINDOW_H / 2);

    return true;
}