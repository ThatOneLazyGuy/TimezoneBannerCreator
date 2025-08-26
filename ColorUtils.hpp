#pragma once

#include <cstdint>

inline uint8_t AddColorComponent(const uint8_t first, const uint8_t second)
{
	return static_cast<uint8_t>(std::min<uint32_t>(255u, first + second));
}

inline uint8_t MulColorComponent(const uint8_t first, const uint8_t second)
{
	return static_cast<uint8_t>(first * second / 255);
}

inline uint8_t DivColorComponent(const uint8_t first, const uint8_t second)
{
	if (second == 0) return 0;
	return static_cast<uint8_t>(255 * first / second);
}

inline uint32_t AddColors(const uint32_t first, const uint32_t second)
{
	const uint8_t alpha_first = first >> 24;
	const uint8_t alpha_second = second >> 24;

	uint8_t out_rgba[4];
	out_rgba[3] = AddColorComponent(alpha_first, MulColorComponent(alpha_second, 255 - alpha_first));

	for (size_t i = 0; i < 3; i++)
	{
		const size_t bits_to_shift = 8 * i;

		const uint8_t component_first = MulColorComponent((first >> bits_to_shift) & 0xFF, alpha_first);
		const uint8_t component_second = MulColorComponent(MulColorComponent((second >> bits_to_shift) & 0xFF, alpha_second), 255 - alpha_first);

		out_rgba[i] = DivColorComponent(AddColorComponent(component_first, component_second), out_rgba[3]);
	}

	return *reinterpret_cast<uint32_t*>(out_rgba);
}