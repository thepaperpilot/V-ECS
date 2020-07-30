#include "NoiseBindings.h"

#include <hastyNoise/hastyNoise.h>

void vecs::NoiseBindings::setupState(sol::state& lua, size_t fastestSimd) {
	lua.new_enum("noiseType",
		"Cellular", HastyNoise::NoiseType::Cellular,
		"Cubic", HastyNoise::NoiseType::Cubic,
		"CubicFractal", HastyNoise::NoiseType::CubicFractal,
		"Perlin", HastyNoise::NoiseType::Perlin,
		"PerlinFractal", HastyNoise::NoiseType::PerlinFractal,
		"Simplex", HastyNoise::NoiseType::Simplex,
		"SimplexFractal", HastyNoise::NoiseType::SimplexFractal,
		"Value", HastyNoise::NoiseType::Value,
		"ValueFractal", HastyNoise::NoiseType::ValueFractal,
		"WhiteNoise", HastyNoise::NoiseType::WhiteNoise
	);
	lua.new_enum("cellularReturnType",
		"Distance2", HastyNoise::CellularReturnType::Distance2,
		"Distance2Add", HastyNoise::CellularReturnType::Distance2Add,
		"Distance2Cave", HastyNoise::CellularReturnType::Distance2Cave,
		"Distance2Div", HastyNoise::CellularReturnType::Distance2Div,
		"Distance2Mul", HastyNoise::CellularReturnType::Distance2Mul,
		"Distance2Sub", HastyNoise::CellularReturnType::Distance2Sub,
		"NoiseLookup", HastyNoise::CellularReturnType::NoiseLookup,
		"Value", HastyNoise::CellularReturnType::Value,
		"ValueDistance2", HastyNoise::CellularReturnType::ValueDistance2
	);
	lua.new_enum("perturbType",
		"Gradient", HastyNoise::PerturbType::Gradient,
		"GradientFractal", HastyNoise::PerturbType::GradientFractal,
		"GradientNormalise", HastyNoise::PerturbType::Gradient_Normalise,
		"GradientFractalNormalise", HastyNoise::PerturbType::GradientFractal_Normalise
	);

	lua.new_usertype<HastyNoise::NoiseSIMD>("noise",
		"new", sol::factories(
			[fastestSimd](HastyNoise::NoiseType noiseType, int seed, float frequency) -> HastyNoise::NoiseSIMD* {
				HastyNoise::NoiseSIMD* noise = HastyNoise::details::CreateNoise(seed, fastestSimd);
				noise->SetNoiseType(noiseType);
				noise->SetFrequency(frequency);
				return noise;
			}
		),
		// sol2 automatically deals with FloatBuffer and provides us the pointer
		"getNoiseSet", [](HastyNoise::NoiseSIMD& noise, float* buffer, int chunkX, int chunkY, int chunkZ, const int chunkSize) -> sol::as_table_t<std::vector<float>> {
			noise.FillSet(buffer, chunkX * chunkSize, chunkY * chunkSize, chunkZ * chunkSize, chunkSize, chunkSize, chunkSize);
			return sol::as_table(std::vector<float>(buffer, buffer + chunkSize * chunkSize * chunkSize));
		},
		"setAxisScales", &HastyNoise::NoiseSIMD::SetAxisScales,
		"setCellularReturnType", &HastyNoise::NoiseSIMD::SetCellularReturnType,
		"setCellularJitter", &HastyNoise::NoiseSIMD::SetCellularJitter,
		"setPerturbType", &HastyNoise::NoiseSIMD::SetPerturbType,
		"setPerturbAmp", &HastyNoise::NoiseSIMD::SetPerturbAmp,
		"setPerturbFrequency", &HastyNoise::NoiseSIMD::SetPerturbFrequency,
		"setPerturbFractalOctaves", &HastyNoise::NoiseSIMD::SetPerturbFractalOctaves,
		"setPerturbFractalLacunarity", &HastyNoise::NoiseSIMD::SetPerturbFractalLacunarity,
		"setPerturbFractalGain", &HastyNoise::NoiseSIMD::SetPerturbFractalGain
	);
	lua.new_usertype<HastyNoise::FloatBuffer>("noiseBuffer",
		"new", sol::factories([fastestSimd](int chunkSize) -> HastyNoise::FloatBuffer { return HastyNoise::GetEmptySet(chunkSize * chunkSize * chunkSize, fastestSimd); })
	);
}
