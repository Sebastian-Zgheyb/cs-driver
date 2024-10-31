#pragma once

#include "../offsets/client_dll.hpp"
#include "../offsets/offsets.hpp"

#include "../math/vector.h"
#include "../memory/memory.h"

namespace aimbot
{
	inline uintptr_t module_base;
	inline uintptr_t procID;

	float distance(vec3 p1, vec3 p2);
	void frame(HANDLE driver, uintptr_t module_base);
}