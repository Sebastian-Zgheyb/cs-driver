#pragma once

#include <vector>

#include "../renderer/renderer.h"
#include "../offsets/client_dll.hpp"
#include "../offsets/offsets.hpp"

struct viewMatrix
{
	float m[16];
};

namespace wallhack
{
	inline std::vector<uintptr_t> entities;
	inline viewMatrix vm = {};

	inline uintptr_t pID;
	inline uintptr_t modBase;

	void loop(HANDLE driver_handle, uintptr_t modBase);
	void frame(HANDLE driver_handle, uintptr_t modBase);
	void render(HANDLE driver_handle, const viewMatrix& vm, const std::vector<uintptr_t>& entities);
	bool w2s(const vec3& world, vec2& screen, const float m[16]);
}