/**************************************************************************/
/*  test_fastnoise_lite.h                                                 */
/**************************************************************************/
/*                         This file is part of:                          */
/*                            VOLTAIRE ENGINE                             */
/*                https://github.com/BendyLand/Voltaire                   */
/**************************************************************************/
/* Copyright (c) 2026-present Bland Logic LLC.                            */
/* Copyright (c) 2014-2026 Godot Engine contributors (see AUTHORS.md).    */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#pragma once

#include "scene/resources/image_texture.h"

#include "tests/test_macros.h"

namespace TestFastNoiseLite {

// Uitility functions for finding differences in noise generation

bool all_equal_approx(const Vector<real_t> &p_values_1, const Vector<real_t> &p_values_2) {
	ERR_FAIL_COND_V_MSG(p_values_1.size() != p_values_2.size(), false, "Arrays must be the same size. This is a error in the test code.");

	for (int i = 0; i < p_values_1.size(); i++) {
		if (!Math::is_equal_approx(p_values_1[i], p_values_2[i])) {
			return false;
		}
	}
	return true;
}

Vector<Pair<size_t, size_t>> find_approx_equal_vec_pairs(std::initializer_list<Vector<real_t>> inputs) {
	Vector<Vector<real_t>> p_array = Vector<Vector<real_t>>(inputs);

	Vector<Pair<size_t, size_t>> result;
	for (int i = 0; i < p_array.size(); i++) {
		for (int j = i + 1; j < p_array.size(); j++) {
			if (all_equal_approx(p_array[i], p_array[j])) {
				result.push_back(Pair<size_t, size_t>(i, j));
			}
		}
	}
	return result;
}

#define CHECK_ARGS_APPROX_PAIRWISE_DISTINCT_VECS(...) \
	{ \
		Vector<Pair<size_t, size_t>> equal_pairs = find_approx_equal_vec_pairs({ __VA_ARGS__ }); \
		for (Pair<size_t, size_t> p : equal_pairs) { \
			MESSAGE("Argument with index ", p.first, " is approximately equal to argument with index ", p.second); \
		} \
		CHECK_MESSAGE(equal_pairs.size() == 0, "All arguments should be pairwise distinct."); \
	}

Vector<real_t> get_noise_samples_1d(const FastNoiseLite &p_noise, size_t p_count = 32) {
	Vector<real_t> result;
	result.resize(p_count);
	for (size_t i = 0; i < p_count; i++) {
		result.write[i] = p_noise.get_noise_1d(i);
	}
	return result;
}

Vector<real_t> get_noise_samples_2d(const FastNoiseLite &p_noise, size_t p_count = 32) {
	Vector<real_t> result;
	result.resize(p_count);
	for (size_t i = 0; i < p_count; i++) {
		result.write[i] = p_noise.get_noise_2d(i, i);
	}
	return result;
}

Vector<real_t> get_noise_samples_3d(const FastNoiseLite &p_noise, size_t p_count = 32) {
	Vector<real_t> result;
	result.resize(p_count);
	for (size_t i = 0; i < p_count; i++) {
		result.write[i] = p_noise.get_noise_3d(i, i, i);
	}
	return result;
}

// The following test suite is rather for testing the wrapper code than the actual noise generation.

TEST_CASE("[FastNoiseLite] Getter and setter") {
	FastNoiseLite noise;

	noise.set_noise_type(FastNoiseLite::NoiseType::TYPE_SIMPLEX_SMOOTH);
	CHECK(noise.get_noise_type() == FastNoiseLite::NoiseType::TYPE_SIMPLEX_SMOOTH);

	noise.set_seed(123);
	CHECK(noise.get_seed() == 123);

	noise.set_frequency(0.123);
	CHECK(noise.get_frequency() == doctest::Approx(0.123));

	noise.set_offset(Vector3(1, 2, 3));
	CHECK(noise.get_offset() == Vector3(1, 2, 3));

	noise.set_fractal_type(FastNoiseLite::FractalType::FRACTAL_PING_PONG);
	CHECK(noise.get_fractal_type() == FastNoiseLite::FractalType::FRACTAL_PING_PONG);

	noise.set_fractal_octaves(2);
	CHECK(noise.get_fractal_octaves() == 2);

	noise.set_fractal_lacunarity(1.123);
	CHECK(noise.get_fractal_lacunarity() == doctest::Approx(1.123));

	noise.set_fractal_gain(0.123);
	CHECK(noise.get_fractal_gain() == doctest::Approx(0.123));

	noise.set_fractal_weighted_strength(0.123);
	CHECK(noise.get_fractal_weighted_strength() == doctest::Approx(0.123));

	noise.set_fractal_ping_pong_strength(0.123);
	CHECK(noise.get_fractal_ping_pong_strength() == doctest::Approx(0.123));

	noise.set_cellular_distance_function(FastNoiseLite::CellularDistanceFunction::DISTANCE_MANHATTAN);
	CHECK(noise.get_cellular_distance_function() == FastNoiseLite::CellularDistanceFunction::DISTANCE_MANHATTAN);

	noise.set_cellular_return_type(FastNoiseLite::CellularReturnType::RETURN_DISTANCE2_SUB);
	CHECK(noise.get_cellular_return_type() == FastNoiseLite::CellularReturnType::RETURN_DISTANCE2_SUB);

	noise.set_cellular_jitter(0.123);
	CHECK(noise.get_cellular_jitter() == doctest::Approx(0.123));

	noise.set_domain_warp_enabled(true);
	CHECK(noise.is_domain_warp_enabled() == true);
	noise.set_domain_warp_enabled(false);
	CHECK(noise.is_domain_warp_enabled() == false);

	noise.set_domain_warp_type(FastNoiseLite::DomainWarpType::DOMAIN_WARP_SIMPLEX_REDUCED);
	CHECK(noise.get_domain_warp_type() == FastNoiseLite::DomainWarpType::DOMAIN_WARP_SIMPLEX_REDUCED);

	noise.set_domain_warp_amplitude(0.123);
	CHECK(noise.get_domain_warp_amplitude() == doctest::Approx(0.123));

	noise.set_domain_warp_frequency(0.123);
	CHECK(noise.get_domain_warp_frequency() == doctest::Approx(0.123));

	noise.set_domain_warp_fractal_type(FastNoiseLite::DomainWarpFractalType::DOMAIN_WARP_FRACTAL_INDEPENDENT);
	CHECK(noise.get_domain_warp_fractal_type() == FastNoiseLite::DomainWarpFractalType::DOMAIN_WARP_FRACTAL_INDEPENDENT);

	noise.set_domain_warp_fractal_octaves(2);
	CHECK(noise.get_domain_warp_fractal_octaves() == 2);

	noise.set_domain_warp_fractal_lacunarity(1.123);
	CHECK(noise.get_domain_warp_fractal_lacunarity() == doctest::Approx(1.123));

	noise.set_domain_warp_fractal_gain(0.123);
	CHECK(noise.get_domain_warp_fractal_gain() == doctest::Approx(0.123));
}

TEST_CASE("[FastNoiseLite] Basic noise generation") {
	FastNoiseLite noise;
	noise.set_noise_type(FastNoiseLite::NoiseType::TYPE_SIMPLEX);
	noise.set_fractal_type(FastNoiseLite::FractalType::FRACTAL_NONE);
	noise.set_seed(123);
	noise.set_offset(Vector3(10, 10, 10));

	// 1D noise will be checked just in the cases where there's the possibility of
	// finding a bug/regression in the wrapper function.
	// (since it uses FastNoise's 2D noise generator with the Y coordinate set to 0).

	SUBCASE("Determinacy of noise generation (all noise types)") {
		noise.set_noise_type(FastNoiseLite::NoiseType::TYPE_SIMPLEX);
		CHECK(noise.get_noise_2d(0, 0) == doctest::Approx(noise.get_noise_2d(0, 0)));
		CHECK(noise.get_noise_3d(0, 0, 0) == doctest::Approx(noise.get_noise_3d(0, 0, 0)));
		noise.set_noise_type(FastNoiseLite::NoiseType::TYPE_SIMPLEX_SMOOTH);
		CHECK(noise.get_noise_2d(0, 0) == doctest::Approx(noise.get_noise_2d(0, 0)));
		CHECK(noise.get_noise_3d(0, 0, 0) == doctest::Approx(noise.get_noise_3d(0, 0, 0)));
		noise.set_noise_type(FastNoiseLite::NoiseType::TYPE_CELLULAR);
		CHECK(noise.get_noise_2d(0, 0) == doctest::Approx(noise.get_noise_2d(0, 0)));
		CHECK(noise.get_noise_3d(0, 0, 0) == doctest::Approx(noise.get_noise_3d(0, 0, 0)));
		noise.set_noise_type(FastNoiseLite::NoiseType::TYPE_PERLIN);
		CHECK(noise.get_noise_2d(0, 0) == doctest::Approx(noise.get_noise_2d(0, 0)));
		CHECK(noise.get_noise_3d(0, 0, 0) == doctest::Approx(noise.get_noise_3d(0, 0, 0)));
		noise.set_noise_type(FastNoiseLite::NoiseType::TYPE_VALUE);
		CHECK(noise.get_noise_2d(0, 0) == doctest::Approx(noise.get_noise_2d(0, 0)));
		CHECK(noise.get_noise_3d(0, 0, 0) == doctest::Approx(noise.get_noise_3d(0, 0, 0)));
		noise.set_noise_type(FastNoiseLite::NoiseType::TYPE_VALUE_CUBIC);
		CHECK(noise.get_noise_2d(0, 0) == doctest::Approx(noise.get_noise_2d(0, 0)));
		CHECK(noise.get_noise_3d(0, 0, 0) == doctest::Approx(noise.get_noise_3d(0, 0, 0)));
	}

	SUBCASE("Different seeds should produce different noise") {
		noise.set_seed(456);
		Vector<real_t> noise_seed_1_1d = get_noise_samples_1d(noise);
		Vector<real_t> noise_seed_1_2d = get_noise_samples_2d(noise);
		Vector<real_t> noise_seed_1_3d = get_noise_samples_3d(noise);
		noise.set_seed(123);
		Vector<real_t> noise_seed_2_1d = get_noise_samples_1d(noise);
		Vector<real_t> noise_seed_2_2d = get_noise_samples_2d(noise);
		Vector<real_t> noise_seed_2_3d = get_noise_samples_3d(noise);

		CHECK_FALSE(all_equal_approx(noise_seed_1_1d, noise_seed_2_1d));
		CHECK_FALSE(all_equal_approx(noise_seed_1_2d, noise_seed_2_2d));
		CHECK_FALSE(all_equal_approx(noise_seed_1_3d, noise_seed_2_3d));
	}

	SUBCASE("Different frequencies should produce different noise") {
		noise.set_frequency(0.1);
		Vector<real_t> noise_frequency_1_1d = get_noise_samples_1d(noise);
		Vector<real_t> noise_frequency_1_2d = get_noise_samples_2d(noise);
		Vector<real_t> noise_frequency_1_3d = get_noise_samples_3d(noise);
		noise.set_frequency(1.0);
		Vector<real_t> noise_frequency_2_1d = get_noise_samples_1d(noise);
		Vector<real_t> noise_frequency_2_2d = get_noise_samples_2d(noise);
		Vector<real_t> noise_frequency_2_3d = get_noise_samples_3d(noise);

		CHECK_FALSE(all_equal_approx(noise_frequency_1_1d, noise_frequency_2_1d));
		CHECK_FALSE(all_equal_approx(noise_frequency_1_2d, noise_frequency_2_2d));
		CHECK_FALSE(all_equal_approx(noise_frequency_1_3d, noise_frequency_2_3d));
	}

	SUBCASE("Noise should be offset by the offset parameter") {
		noise.set_offset(Vector3(1, 2, 3));
		Vector<real_t> noise_offset_1_1d = get_noise_samples_1d(noise);
		Vector<real_t> noise_offset_1_2d = get_noise_samples_2d(noise);
		Vector<real_t> noise_offset_1_3d = get_noise_samples_3d(noise);
		noise.set_offset(Vector3(4, 5, 6));
		Vector<real_t> noise_offset_2_1d = get_noise_samples_1d(noise);
		Vector<real_t> noise_offset_2_2d = get_noise_samples_2d(noise);
		Vector<real_t> noise_offset_2_3d = get_noise_samples_3d(noise);

		CHECK_FALSE(all_equal_approx(noise_offset_1_1d, noise_offset_2_1d));
		CHECK_FALSE(all_equal_approx(noise_offset_1_2d, noise_offset_2_2d));
		CHECK_FALSE(all_equal_approx(noise_offset_1_3d, noise_offset_2_3d));
	}

	SUBCASE("Different noise types should produce different noise") {
		noise.set_noise_type(FastNoiseLite::NoiseType::TYPE_SIMPLEX);
		Vector<real_t> noise_type_simplex_2d = get_noise_samples_2d(noise);
		Vector<real_t> noise_type_simplex_3d = get_noise_samples_3d(noise);
		noise.set_noise_type(FastNoiseLite::NoiseType::TYPE_SIMPLEX_SMOOTH);
		Vector<real_t> noise_type_simplex_smooth_2d = get_noise_samples_2d(noise);
		Vector<real_t> noise_type_simplex_smooth_3d = get_noise_samples_3d(noise);
		noise.set_noise_type(FastNoiseLite::NoiseType::TYPE_CELLULAR);
		Vector<real_t> noise_type_cellular_2d = get_noise_samples_2d(noise);
		Vector<real_t> noise_type_cellular_3d = get_noise_samples_3d(noise);
		noise.set_noise_type(FastNoiseLite::NoiseType::TYPE_PERLIN);
		Vector<real_t> noise_type_perlin_2d = get_noise_samples_2d(noise);
		Vector<real_t> noise_type_perlin_3d = get_noise_samples_3d(noise);
		noise.set_noise_type(FastNoiseLite::NoiseType::TYPE_VALUE);
		Vector<real_t> noise_type_value_2d = get_noise_samples_2d(noise);
		Vector<real_t> noise_type_value_3d = get_noise_samples_3d(noise);
		noise.set_noise_type(FastNoiseLite::NoiseType::TYPE_VALUE_CUBIC);
		Vector<real_t> noise_type_value_cubic_2d = get_noise_samples_2d(noise);
		Vector<real_t> noise_type_value_cubic_3d = get_noise_samples_3d(noise);

		CHECK_ARGS_APPROX_PAIRWISE_DISTINCT_VECS(noise_type_simplex_2d,
				noise_type_simplex_smooth_2d,
				noise_type_cellular_2d,
				noise_type_perlin_2d,
				noise_type_value_2d,
				noise_type_value_cubic_2d);

		CHECK_ARGS_APPROX_PAIRWISE_DISTINCT_VECS(noise_type_simplex_3d,
				noise_type_simplex_smooth_3d,
				noise_type_cellular_3d,
				noise_type_perlin_3d,
				noise_type_value_3d,
				noise_type_value_cubic_3d);
	}
}

TEST_CASE("[FastNoiseLite] Fractal noise") {
	FastNoiseLite noise;
	noise.set_noise_type(FastNoiseLite::NoiseType::TYPE_SIMPLEX);
	noise.set_offset(Vector3(10, 10, 10));
	noise.set_frequency(0.01);
	noise.set_fractal_type(FastNoiseLite::FractalType::FRACTAL_FBM);
	noise.set_fractal_octaves(4);
	noise.set_fractal_lacunarity(2.0);
	noise.set_fractal_gain(0.5);
	noise.set_fractal_weighted_strength(0.5);
	noise.set_fractal_ping_pong_strength(2.0);

	SUBCASE("Different fractal types should produce different results") {
		noise.set_fractal_type(FastNoiseLite::FractalType::FRACTAL_NONE);
		Vector<real_t> fractal_type_none_2d = get_noise_samples_2d(noise);
		Vector<real_t> fractal_type_none_3d = get_noise_samples_3d(noise);
		noise.set_fractal_type(FastNoiseLite::FractalType::FRACTAL_FBM);
		Vector<real_t> fractal_type_fbm_2d = get_noise_samples_2d(noise);
		Vector<real_t> fractal_type_fbm_3d = get_noise_samples_3d(noise);
		noise.set_fractal_type(FastNoiseLite::FractalType::FRACTAL_RIDGED);
		Vector<real_t> fractal_type_ridged_2d = get_noise_samples_2d(noise);
		Vector<real_t> fractal_type_ridged_3d = get_noise_samples_3d(noise);
		noise.set_fractal_type(FastNoiseLite::FractalType::FRACTAL_PING_PONG);
		Vector<real_t> fractal_type_ping_pong_2d = get_noise_samples_2d(noise);
		Vector<real_t> fractal_type_ping_pong_3d = get_noise_samples_3d(noise);

		CHECK_ARGS_APPROX_PAIRWISE_DISTINCT_VECS(fractal_type_none_2d,
				fractal_type_fbm_2d,
				fractal_type_ridged_2d,
				fractal_type_ping_pong_2d);

		CHECK_ARGS_APPROX_PAIRWISE_DISTINCT_VECS(fractal_type_none_3d,
				fractal_type_fbm_3d,
				fractal_type_ridged_3d,
				fractal_type_ping_pong_3d);
	}

	SUBCASE("Different octaves should produce different results") {
		noise.set_fractal_octaves(1.0);
		Vector<real_t> fractal_octaves_1_2d = get_noise_samples_2d(noise);
		Vector<real_t> fractal_octaves_1_3d = get_noise_samples_3d(noise);
		noise.set_fractal_octaves(8.0);
		Vector<real_t> fractal_octaves_2_2d = get_noise_samples_2d(noise);
		Vector<real_t> fractal_octaves_2_3d = get_noise_samples_3d(noise);

		CHECK_FALSE(all_equal_approx(fractal_octaves_1_2d, fractal_octaves_2_2d));
		CHECK_FALSE(all_equal_approx(fractal_octaves_1_3d, fractal_octaves_2_3d));
	}

	SUBCASE("Different lacunarity should produce different results") {
		noise.set_fractal_lacunarity(1.0);
		Vector<real_t> fractal_lacunarity_1_2d = get_noise_samples_2d(noise);
		Vector<real_t> fractal_lacunarity_1_3d = get_noise_samples_3d(noise);
		noise.set_fractal_lacunarity(2.0);
		Vector<real_t> fractal_lacunarity_2_2d = get_noise_samples_2d(noise);
		Vector<real_t> fractal_lacunarity_2_3d = get_noise_samples_3d(noise);

		CHECK_FALSE(all_equal_approx(fractal_lacunarity_1_2d, fractal_lacunarity_2_2d));
		CHECK_FALSE(all_equal_approx(fractal_lacunarity_1_3d, fractal_lacunarity_2_3d));
	}

	SUBCASE("Different gain should produce different results") {
		noise.set_fractal_gain(0.5);
		Vector<real_t> fractal_gain_1_2d = get_noise_samples_2d(noise);
		Vector<real_t> fractal_gain_1_3d = get_noise_samples_3d(noise);
		noise.set_fractal_gain(0.75);
		Vector<real_t> fractal_gain_2_2d = get_noise_samples_2d(noise);
		Vector<real_t> fractal_gain_2_3d = get_noise_samples_3d(noise);

		CHECK_FALSE(all_equal_approx(fractal_gain_1_2d, fractal_gain_2_2d));
		CHECK_FALSE(all_equal_approx(fractal_gain_1_3d, fractal_gain_2_3d));
	}

	SUBCASE("Different weights should produce different results") {
		noise.set_fractal_weighted_strength(0.5);
		Vector<real_t> fractal_weighted_strength_1_2d = get_noise_samples_2d(noise);
		Vector<real_t> fractal_weighted_strength_1_3d = get_noise_samples_3d(noise);
		noise.set_fractal_weighted_strength(0.75);
		Vector<real_t> fractal_weighted_strength_2_2d = get_noise_samples_2d(noise);
		Vector<real_t> fractal_weighted_strength_2_3d = get_noise_samples_3d(noise);

		CHECK_FALSE(all_equal_approx(fractal_weighted_strength_1_2d, fractal_weighted_strength_2_2d));
		CHECK_FALSE(all_equal_approx(fractal_weighted_strength_1_3d, fractal_weighted_strength_2_3d));
	}

	SUBCASE("Different ping pong strength should produce different results") {
		noise.set_fractal_type(FastNoiseLite::FractalType::FRACTAL_PING_PONG);
		noise.set_fractal_ping_pong_strength(0.5);
		Vector<real_t> fractal_ping_pong_strength_1_2d = get_noise_samples_2d(noise);
		Vector<real_t> fractal_ping_pong_strength_1_3d = get_noise_samples_3d(noise);
		noise.set_fractal_ping_pong_strength(0.75);
		Vector<real_t> fractal_ping_pong_strength_2_2d = get_noise_samples_2d(noise);
		Vector<real_t> fractal_ping_pong_strength_2_3d = get_noise_samples_3d(noise);

		CHECK_FALSE(all_equal_approx(fractal_ping_pong_strength_1_2d, fractal_ping_pong_strength_2_2d));
		CHECK_FALSE(all_equal_approx(fractal_ping_pong_strength_1_3d, fractal_ping_pong_strength_2_3d));
	}
}

TEST_CASE("[FastNoiseLite] Cellular noise") {
	FastNoiseLite noise;
	noise.set_fractal_type(FastNoiseLite::FractalType::FRACTAL_NONE);
	noise.set_noise_type(FastNoiseLite::NoiseType::TYPE_CELLULAR);
	noise.set_cellular_distance_function(FastNoiseLite::CellularDistanceFunction::DISTANCE_EUCLIDEAN);
	noise.set_cellular_return_type(FastNoiseLite::CellularReturnType::RETURN_DISTANCE);
	noise.set_frequency(1.0);

	SUBCASE("Different distance functions should produce different results") {
		noise.set_cellular_distance_function(FastNoiseLite::CellularDistanceFunction::DISTANCE_EUCLIDEAN);
		Vector<real_t> cellular_distance_function_euclidean_2d = get_noise_samples_2d(noise);
		Vector<real_t> cellular_distance_function_euclidean_3d = get_noise_samples_3d(noise);
		noise.set_cellular_distance_function(FastNoiseLite::CellularDistanceFunction::DISTANCE_EUCLIDEAN_SQUARED);
		Vector<real_t> cellular_distance_function_euclidean_squared_2d = get_noise_samples_2d(noise);
		Vector<real_t> cellular_distance_function_euclidean_squared_3d = get_noise_samples_3d(noise);
		noise.set_cellular_distance_function(FastNoiseLite::CellularDistanceFunction::DISTANCE_MANHATTAN);
		Vector<real_t> cellular_distance_function_manhattan_2d = get_noise_samples_2d(noise);
		Vector<real_t> cellular_distance_function_manhattan_3d = get_noise_samples_3d(noise);
		noise.set_cellular_distance_function(FastNoiseLite::CellularDistanceFunction::DISTANCE_HYBRID);
		Vector<real_t> cellular_distance_function_hybrid_2d = get_noise_samples_2d(noise);
		Vector<real_t> cellular_distance_function_hybrid_3d = get_noise_samples_3d(noise);

		CHECK_ARGS_APPROX_PAIRWISE_DISTINCT_VECS(cellular_distance_function_euclidean_2d,
				cellular_distance_function_euclidean_squared_2d,
				cellular_distance_function_manhattan_2d,
				cellular_distance_function_hybrid_2d);

		CHECK_ARGS_APPROX_PAIRWISE_DISTINCT_VECS(cellular_distance_function_euclidean_3d,
				cellular_distance_function_euclidean_squared_3d,
				cellular_distance_function_manhattan_3d,
				cellular_distance_function_hybrid_3d);
	}

	SUBCASE("Different return function types should produce different results") {
		noise.set_cellular_return_type(FastNoiseLite::CellularReturnType::RETURN_CELL_VALUE);
		Vector<real_t> cellular_return_type_cell_value_2d = get_noise_samples_2d(noise);
		Vector<real_t> cellular_return_type_cell_value_3d = get_noise_samples_3d(noise);
		noise.set_cellular_return_type(FastNoiseLite::CellularReturnType::RETURN_DISTANCE);
		Vector<real_t> cellular_return_type_distance_2d = get_noise_samples_2d(noise);
		Vector<real_t> cellular_return_type_distance_3d = get_noise_samples_3d(noise);
		noise.set_cellular_return_type(FastNoiseLite::CellularReturnType::RETURN_DISTANCE2);
		Vector<real_t> cellular_return_type_distance2_2d = get_noise_samples_2d(noise);
		Vector<real_t> cellular_return_type_distance2_3d = get_noise_samples_3d(noise);
		noise.set_cellular_return_type(FastNoiseLite::CellularReturnType::RETURN_DISTANCE2_ADD);
		Vector<real_t> cellular_return_type_distance2_add_2d = get_noise_samples_2d(noise);
		Vector<real_t> cellular_return_type_distance2_add_3d = get_noise_samples_3d(noise);
		noise.set_cellular_return_type(FastNoiseLite::CellularReturnType::RETURN_DISTANCE2_SUB);
		Vector<real_t> cellular_return_type_distance2_sub_2d = get_noise_samples_2d(noise);
		Vector<real_t> cellular_return_type_distance2_sub_3d = get_noise_samples_3d(noise);
		noise.set_cellular_return_type(FastNoiseLite::CellularReturnType::RETURN_DISTANCE2_MUL);
		Vector<real_t> cellular_return_type_distance2_mul_2d = get_noise_samples_2d(noise);
		Vector<real_t> cellular_return_type_distance2_mul_3d = get_noise_samples_3d(noise);
		noise.set_cellular_return_type(FastNoiseLite::CellularReturnType::RETURN_DISTANCE2_DIV);
		Vector<real_t> cellular_return_type_distance2_div_2d = get_noise_samples_2d(noise);
		Vector<real_t> cellular_return_type_distance2_div_3d = get_noise_samples_3d(noise);

		CHECK_ARGS_APPROX_PAIRWISE_DISTINCT_VECS(cellular_return_type_cell_value_2d,
				cellular_return_type_distance_2d,
				cellular_return_type_distance2_2d,
				cellular_return_type_distance2_add_2d,
				cellular_return_type_distance2_sub_2d,
				cellular_return_type_distance2_mul_2d,
				cellular_return_type_distance2_div_2d);

		CHECK_ARGS_APPROX_PAIRWISE_DISTINCT_VECS(cellular_return_type_cell_value_3d,
				cellular_return_type_distance_3d,
				cellular_return_type_distance2_3d,
				cellular_return_type_distance2_add_3d,
				cellular_return_type_distance2_sub_3d,
				cellular_return_type_distance2_mul_3d,
				cellular_return_type_distance2_div_3d);
	}

	SUBCASE("Different cellular jitter should produce different results") {
		noise.set_cellular_jitter(0.0);
		Vector<real_t> cellular_jitter_1_2d = get_noise_samples_2d(noise);
		Vector<real_t> cellular_jitter_1_3d = get_noise_samples_3d(noise);
		noise.set_cellular_jitter(0.5);
		Vector<real_t> cellular_jitter_2_2d = get_noise_samples_2d(noise);
		Vector<real_t> cellular_jitter_2_3d = get_noise_samples_3d(noise);

		CHECK_FALSE(all_equal_approx(cellular_jitter_1_2d, cellular_jitter_2_2d));
		CHECK_FALSE(all_equal_approx(cellular_jitter_1_3d, cellular_jitter_2_3d));
	}
}

TEST_CASE("[FastNoiseLite] Domain warp") {
	FastNoiseLite noise;
	noise.set_frequency(1.0);
	noise.set_domain_warp_amplitude(200.0);
	noise.set_noise_type(FastNoiseLite::NoiseType::TYPE_SIMPLEX);
	noise.set_domain_warp_enabled(true);

	SUBCASE("Different domain warp types should produce different results") {
		noise.set_domain_warp_type(FastNoiseLite::DomainWarpType::DOMAIN_WARP_SIMPLEX);
		Vector<real_t> domain_warp_type_simplex_2d = get_noise_samples_2d(noise);
		Vector<real_t> domain_warp_type_simplex_3d = get_noise_samples_3d(noise);
		noise.set_domain_warp_type(FastNoiseLite::DomainWarpType::DOMAIN_WARP_SIMPLEX_REDUCED);
		Vector<real_t> domain_warp_type_simplex_reduced_2d = get_noise_samples_2d(noise);
		Vector<real_t> domain_warp_type_simplex_reduced_3d = get_noise_samples_3d(noise);
		noise.set_domain_warp_type(FastNoiseLite::DomainWarpType::DOMAIN_WARP_BASIC_GRID);
		Vector<real_t> domain_warp_type_basic_grid_2d = get_noise_samples_2d(noise);
		Vector<real_t> domain_warp_type_basic_grid_3d = get_noise_samples_3d(noise);

		CHECK_ARGS_APPROX_PAIRWISE_DISTINCT_VECS(domain_warp_type_simplex_2d,
				domain_warp_type_simplex_reduced_2d,
				domain_warp_type_basic_grid_2d);

		CHECK_ARGS_APPROX_PAIRWISE_DISTINCT_VECS(domain_warp_type_simplex_3d,
				domain_warp_type_simplex_reduced_3d,
				domain_warp_type_basic_grid_3d);
	}

	SUBCASE("Different domain warp amplitude should produce different results") {
		noise.set_domain_warp_amplitude(0.0);
		Vector<real_t> domain_warp_amplitude_1_2d = get_noise_samples_2d(noise);
		Vector<real_t> domain_warp_amplitude_1_3d = get_noise_samples_3d(noise);
		noise.set_domain_warp_amplitude(100.0);
		Vector<real_t> domain_warp_amplitude_2_2d = get_noise_samples_2d(noise);
		Vector<real_t> domain_warp_amplitude_2_3d = get_noise_samples_3d(noise);

		CHECK_FALSE(all_equal_approx(domain_warp_amplitude_1_2d, domain_warp_amplitude_2_2d));
		CHECK_FALSE(all_equal_approx(domain_warp_amplitude_1_3d, domain_warp_amplitude_2_3d));
	}

	SUBCASE("Different domain warp frequency should produce different results") {
		noise.set_domain_warp_frequency(0.1);
		Vector<real_t> domain_warp_frequency_1_2d = get_noise_samples_2d(noise);
		Vector<real_t> domain_warp_frequency_1_3d = get_noise_samples_3d(noise);
		noise.set_domain_warp_frequency(2.0);
		Vector<real_t> domain_warp_frequency_2_2d = get_noise_samples_2d(noise);
		Vector<real_t> domain_warp_frequency_2_3d = get_noise_samples_3d(noise);

		CHECK_FALSE(all_equal_approx(domain_warp_frequency_1_2d, domain_warp_frequency_2_2d));
		CHECK_FALSE(all_equal_approx(domain_warp_frequency_1_3d, domain_warp_frequency_2_3d));
	}

	SUBCASE("Different domain warp fractal type should produce different results") {
		noise.set_domain_warp_fractal_type(FastNoiseLite::DomainWarpFractalType::DOMAIN_WARP_FRACTAL_NONE);
		Vector<real_t> domain_warp_fractal_type_none_2d = get_noise_samples_2d(noise);
		Vector<real_t> domain_warp_fractal_type_none_3d = get_noise_samples_3d(noise);
		noise.set_domain_warp_fractal_type(FastNoiseLite::DomainWarpFractalType::DOMAIN_WARP_FRACTAL_PROGRESSIVE);
		Vector<real_t> domain_warp_fractal_type_progressive_2d = get_noise_samples_2d(noise);
		Vector<real_t> domain_warp_fractal_type_progressive_3d = get_noise_samples_3d(noise);
		noise.set_domain_warp_fractal_type(FastNoiseLite::DomainWarpFractalType::DOMAIN_WARP_FRACTAL_INDEPENDENT);
		Vector<real_t> domain_warp_fractal_type_independent_2d = get_noise_samples_2d(noise);
		Vector<real_t> domain_warp_fractal_type_independent_3d = get_noise_samples_3d(noise);

		CHECK_ARGS_APPROX_PAIRWISE_DISTINCT_VECS(domain_warp_fractal_type_none_2d,
				domain_warp_fractal_type_progressive_2d,
				domain_warp_fractal_type_independent_2d);

		CHECK_ARGS_APPROX_PAIRWISE_DISTINCT_VECS(domain_warp_fractal_type_none_3d,
				domain_warp_fractal_type_progressive_3d,
				domain_warp_fractal_type_independent_3d);
	}

	SUBCASE("Different domain warp fractal octaves should produce different results") {
		noise.set_domain_warp_fractal_octaves(1);
		Vector<real_t> domain_warp_fractal_octaves_1_2d = get_noise_samples_2d(noise);
		Vector<real_t> domain_warp_fractal_octaves_1_3d = get_noise_samples_3d(noise);
		noise.set_domain_warp_fractal_octaves(6);
		Vector<real_t> domain_warp_fractal_octaves_2_2d = get_noise_samples_2d(noise);
		Vector<real_t> domain_warp_fractal_octaves_2_3d = get_noise_samples_3d(noise);

		CHECK_FALSE(all_equal_approx(domain_warp_fractal_octaves_1_2d, domain_warp_fractal_octaves_2_2d));
		CHECK_FALSE(all_equal_approx(domain_warp_fractal_octaves_1_3d, domain_warp_fractal_octaves_2_3d));
	}

	SUBCASE("Different domain warp fractal lacunarity should produce different results") {
		noise.set_domain_warp_fractal_lacunarity(0.5);
		Vector<real_t> domain_warp_fractal_lacunarity_1_2d = get_noise_samples_2d(noise);
		Vector<real_t> domain_warp_fractal_lacunarity_1_3d = get_noise_samples_3d(noise);
		noise.set_domain_warp_fractal_lacunarity(5.0);
		Vector<real_t> domain_warp_fractal_lacunarity_2_2d = get_noise_samples_2d(noise);
		Vector<real_t> domain_warp_fractal_lacunarity_2_3d = get_noise_samples_3d(noise);

		CHECK_FALSE(all_equal_approx(domain_warp_fractal_lacunarity_1_2d, domain_warp_fractal_lacunarity_2_2d));
		CHECK_FALSE(all_equal_approx(domain_warp_fractal_lacunarity_1_3d, domain_warp_fractal_lacunarity_2_3d));
	}

	SUBCASE("Different domain warp fractal gain should produce different results") {
		noise.set_domain_warp_fractal_gain(0.1);
		Vector<real_t> domain_warp_fractal_gain_1_2d = get_noise_samples_2d(noise);
		Vector<real_t> domain_warp_fractal_gain_1_3d = get_noise_samples_3d(noise);
		noise.set_domain_warp_fractal_gain(0.9);
		Vector<real_t> domain_warp_fractal_gain_2_2d = get_noise_samples_2d(noise);
		Vector<real_t> domain_warp_fractal_gain_2_3d = get_noise_samples_3d(noise);

		CHECK_FALSE(all_equal_approx(domain_warp_fractal_gain_1_2d, domain_warp_fractal_gain_2_2d));
		CHECK_FALSE(all_equal_approx(domain_warp_fractal_gain_1_3d, domain_warp_fractal_gain_2_3d));
	}
}

// Raw image data for the reference images used in the regression tests.
// Generated with the following code:
//     for (int y = 0; y < img->get_data().size(); y++) {
//         printf("0x%x,", img->get_data()[y]);
//     }

const Vector<uint8_t> ref_img_1_data = { 0xff, 0xe6, 0xd2, 0xc2, 0xb7, 0xb4, 0xb4, 0xb7, 0xc2, 0xd2, 0xe6, 0xe6, 0xcb, 0xb4, 0xa1, 0x94, 0x90, 0x90, 0x94, 0xa1, 0xb4, 0xcb, 0xd2, 0xb4, 0x99, 0x82, 0x72, 0x6c, 0x6c, 0x72, 0x82, 0x99, 0xb4, 0xc2, 0xa1, 0x82, 0x65, 0x50, 0x48, 0x48, 0x50, 0x65, 0x82, 0xa1, 0xb7, 0x94, 0x72, 0x50, 0x32, 0x24, 0x24, 0x32, 0x50, 0x72, 0x94, 0xb4, 0x90, 0x6c, 0x48, 0x24, 0x0, 0x0, 0x24, 0x48, 0x6c, 0x90, 0xb4, 0x90, 0x6c, 0x48, 0x24, 0x0, 0x0, 0x24, 0x48, 0x6c, 0x90, 0xb7, 0x94, 0x72, 0x50, 0x32, 0x24, 0x24, 0x33, 0x50, 0x72, 0x94, 0xc2, 0xa1, 0x82, 0x65, 0x50, 0x48, 0x48, 0x50, 0x66, 0x82, 0xa1, 0xd2, 0xb4, 0x99, 0x82, 0x72, 0x6c, 0x6c, 0x72, 0x82, 0x99, 0xb4, 0xe6, 0xcb, 0xb4, 0xa1, 0x94, 0x90, 0x90, 0x94, 0xa1, 0xb4, 0xcc };
const Vector<uint8_t> ref_img_2_data = { 0xff, 0xe6, 0xd2, 0xc2, 0xb7, 0xb4, 0xb4, 0xb7, 0xc2, 0xd2, 0xe6, 0xe6, 0xcb, 0xb4, 0xa1, 0x94, 0x90, 0x90, 0x94, 0xa1, 0xb4, 0xcb, 0xd2, 0xb4, 0x99, 0x82, 0x72, 0x6c, 0x6c, 0x72, 0x82, 0x99, 0xb4, 0xc2, 0xa1, 0x82, 0x65, 0x50, 0x48, 0x48, 0x50, 0x65, 0x82, 0xa1, 0xb7, 0x94, 0x72, 0x50, 0x32, 0x24, 0x24, 0x32, 0x50, 0x72, 0x94, 0xb4, 0x90, 0x6c, 0x48, 0x24, 0x0, 0x0, 0x24, 0x48, 0x6c, 0x90, 0xb4, 0x90, 0x6c, 0x48, 0x24, 0x0, 0x0, 0x24, 0x48, 0x6c, 0x90, 0xb7, 0x94, 0x72, 0x50, 0x32, 0x24, 0x24, 0x33, 0x50, 0x72, 0x94, 0xc2, 0xa1, 0x82, 0x65, 0x50, 0x48, 0x48, 0x50, 0x66, 0x82, 0xa1, 0xd2, 0xb4, 0x99, 0x82, 0x72, 0x6c, 0x6c, 0x72, 0x82, 0x99, 0xb4, 0xe6, 0xcb, 0xb4, 0xa1, 0x94, 0x90, 0x90, 0x94, 0xa1, 0xb4, 0xcc };
const Vector<uint8_t> ref_img_3_data = { 0xff, 0xe6, 0xd2, 0xc2, 0xb7, 0xb4, 0xb4, 0xb7, 0xc2, 0xd2, 0xe6, 0xe6, 0xcb, 0xb4, 0xa1, 0x94, 0x90, 0x90, 0x94, 0xa1, 0xb4, 0xcb, 0xd2, 0xb4, 0x99, 0x82, 0x72, 0x6c, 0x6c, 0x72, 0x82, 0x99, 0xb4, 0xc2, 0xa1, 0x82, 0x65, 0x50, 0x48, 0x48, 0x50, 0x65, 0x82, 0xa1, 0xb7, 0x94, 0x72, 0x50, 0x32, 0x24, 0x24, 0x32, 0x50, 0x72, 0x94, 0xb4, 0x90, 0x6c, 0x48, 0x24, 0x0, 0x0, 0x24, 0x48, 0x6c, 0x90, 0xb4, 0x90, 0x6c, 0x48, 0x24, 0x0, 0x0, 0x24, 0x48, 0x6c, 0x90, 0xb7, 0x94, 0x72, 0x50, 0x32, 0x24, 0x24, 0x33, 0x50, 0x72, 0x94, 0xc2, 0xa1, 0x82, 0x65, 0x50, 0x48, 0x48, 0x50, 0x66, 0x82, 0xa1, 0xd2, 0xb4, 0x99, 0x82, 0x72, 0x6c, 0x6c, 0x72, 0x82, 0x99, 0xb4, 0xe6, 0xcb, 0xb4, 0xa1, 0x94, 0x90, 0x90, 0x94, 0xa1, 0xb4, 0xcc };

// Utility function to compare two images pixel by pixel (for easy debugging of regressions)
void compare_image_with_reference(const Ref<Image> &p_img, const Ref<Image> &p_reference_img) {
	for (int y = 0; y < p_img->get_height(); y++) {
		for (int x = 0; x < p_img->get_width(); x++) {
			CHECK(p_img->get_pixel(x, y) == p_reference_img->get_pixel(x, y));
		}
	}
}

TEST_CASE("[FastNoiseLite] Generating seamless 2D images (11x11px) and compare to reference images") {
	FastNoiseLite noise;
	noise.set_noise_type(FastNoiseLite::NoiseType::TYPE_CELLULAR);
	noise.set_fractal_type(FastNoiseLite::FractalType::FRACTAL_NONE);
	noise.set_cellular_distance_function(FastNoiseLite::CellularDistanceFunction::DISTANCE_EUCLIDEAN);
	noise.set_frequency(0.1);
	noise.set_cellular_jitter(0.0);

	SUBCASE("Blend skirt 0.0") {
		Ref<Image> img = noise.get_seamless_image(11, 11, false, false, 0.0);

		Ref<Image> ref_img_1 = memnew(Image);
		ref_img_1->set_data(11, 11, false, Image::FORMAT_L8, ref_img_1_data);

		compare_image_with_reference(img, ref_img_1);
	}

	SUBCASE("Blend skirt 0.1") {
		Ref<Image> img = noise.get_seamless_image(11, 11, false, false, 0.1);

		Ref<Image> ref_img_2 = memnew(Image);
		ref_img_2->set_data(11, 11, false, Image::FORMAT_L8, ref_img_2_data);

		compare_image_with_reference(img, ref_img_2);
	}

	SUBCASE("Blend skirt 1.0") {
		Ref<Image> img = noise.get_seamless_image(11, 11, false, false, 0.1);

		Ref<Image> ref_img_3 = memnew(Image);
		ref_img_3->set_data(11, 11, false, Image::FORMAT_L8, ref_img_3_data);

		compare_image_with_reference(img, ref_img_3);
	}
}

} //namespace TestFastNoiseLite

// --- Folded from modules/noise/tests/test_noise_texture_2d.h ---
#include "scene/resources/image_texture.h"

#include "tests/test_macros.h"

namespace TestNoiseTexture2D {

class NoiseTextureTester : public RefCounted {

	const NoiseTexture2D *const texture;

public:
	NoiseTextureTester(const NoiseTexture2D *const p_texture) :
			texture{ p_texture } {}

	Color compute_average_color(const Ref<Image> &p_noise_image) {
		Color r_avg_color{};

		for (int i = 0; i < p_noise_image->get_width(); ++i) {
			for (int j = 0; j < p_noise_image->get_height(); ++j) {
				const Color pixel = p_noise_image->get_pixel(i, j);
				r_avg_color += pixel;
			}
		}

		int pixel_count = p_noise_image->get_width() * p_noise_image->get_height();
		r_avg_color /= pixel_count;
		return r_avg_color;
	}

	void check_mip_and_color_ramp() {
		const Ref<Image> noise_image = texture->get_image();
		CHECK(noise_image.is_valid());
		CHECK(noise_image->get_width() == texture->get_width());
		CHECK(noise_image->get_height() == texture->get_height());

		CHECK(noise_image->get_format() == Image::FORMAT_RGBA8);
		CHECK(noise_image->has_mipmaps());

		Color avg_color = compute_average_color(noise_image);

		// Check that the noise texture is modulated correctly by the color ramp (Gradient).
		CHECK_FALSE_MESSAGE((avg_color.r + avg_color.g + avg_color.b) == doctest::Approx(0.0), "The noise texture should not be all black");
		CHECK_FALSE_MESSAGE((avg_color.r + avg_color.g + avg_color.b) == doctest::Approx(noise_image->get_width() * noise_image->get_height() * 3.0), "The noise texture should not be all white");
		CHECK_MESSAGE(avg_color.g == doctest::Approx(0.0), "The noise texture should not have any green when modulated correctly by the color ramp");
	}

	void check_normal_map() {
		const Ref<Image> noise_image = texture->get_image();
		CHECK(noise_image.is_valid());
		CHECK(noise_image->get_width() == texture->get_width());
		CHECK(noise_image->get_height() == texture->get_height());

		CHECK(noise_image->get_format() == Image::FORMAT_RGBA8);
		CHECK_FALSE(noise_image->has_mipmaps());

		Color avg_color = compute_average_color(noise_image);

		// Check for the characteristic color distribution (for tangent space) of a normal map.
		CHECK(avg_color.r == doctest::Approx(0.5).epsilon(0.05));
		CHECK(avg_color.g == doctest::Approx(0.5).epsilon(0.05));
		CHECK(avg_color.b == doctest::Approx(1.0).epsilon(0.05));
	}

	void check_seamless_texture_grayscale() {
		const Ref<Image> noise_image = texture->get_image();
		CHECK(noise_image.is_valid());
		CHECK(noise_image->get_width() == texture->get_width());
		CHECK(noise_image->get_height() == texture->get_height());

		CHECK(noise_image->get_format() == Image::FORMAT_L8);

		Color avg_color = compute_average_color(noise_image);

		// Since it's a grayscale image and every channel except the alpha channel has the
		// same values (conversion happens in Image::get_pixel) we only need to test one channel.
		CHECK(avg_color.r == doctest::Approx(0.5).epsilon(0.05));
	}

	void check_seamless_texture_rgba() {
		const Ref<Image> noise_image = texture->get_image();
		CHECK(noise_image.is_valid());
		CHECK(noise_image->get_width() == texture->get_width());
		CHECK(noise_image->get_height() == texture->get_height());

		CHECK(noise_image->get_format() == Image::FORMAT_RGBA8);

		// Check that the noise texture is modulated correctly by the color ramp (Gradient).
		Color avg_color = compute_average_color(noise_image);

		// We use a default (black to white) gradient, so the average of the red, green and blue channels should be the same.
		CHECK(avg_color.r == doctest::Approx(0.5).epsilon(0.05));
		CHECK(avg_color.g == doctest::Approx(0.5).epsilon(0.05));
		CHECK(avg_color.b == doctest::Approx(0.5).epsilon(0.05));
	}
};

TEST_CASE("[NoiseTexture][SceneTree] Getter and setter") {
	Ref<NoiseTexture2D> noise_texture = memnew(NoiseTexture2D);

	Ref<FastNoiseLite> noise = memnew(FastNoiseLite);
	noise_texture->set_noise(noise);
	CHECK(noise_texture->get_noise() == noise);
	noise_texture->set_noise(nullptr);
	CHECK(noise_texture->get_noise().is_null());

	noise_texture->set_width(8);
	noise_texture->set_height(4);
	CHECK(noise_texture->get_width() == 8);
	CHECK(noise_texture->get_height() == 4);

	ERR_PRINT_OFF;
	noise_texture->set_width(-1);
	noise_texture->set_height(-1);
	ERR_PRINT_ON;
	CHECK(noise_texture->get_width() == 8);
	CHECK(noise_texture->get_height() == 4);

	noise_texture->set_invert(true);
	CHECK(noise_texture->get_invert() == true);
	noise_texture->set_invert(false);
	CHECK(noise_texture->get_invert() == false);

	noise_texture->set_in_3d_space(true);
	CHECK(noise_texture->is_in_3d_space() == true);
	noise_texture->set_in_3d_space(false);
	CHECK(noise_texture->is_in_3d_space() == false);

	noise_texture->set_generate_mipmaps(true);
	CHECK(noise_texture->is_generating_mipmaps() == true);
	noise_texture->set_generate_mipmaps(false);
	CHECK(noise_texture->is_generating_mipmaps() == false);

	noise_texture->set_seamless(true);
	CHECK(noise_texture->get_seamless() == true);
	noise_texture->set_seamless(false);
	CHECK(noise_texture->get_seamless() == false);

	noise_texture->set_seamless_blend_skirt(0.45);
	CHECK(noise_texture->get_seamless_blend_skirt() == doctest::Approx(0.45));

	ERR_PRINT_OFF;
	noise_texture->set_seamless_blend_skirt(-1.0);
	noise_texture->set_seamless_blend_skirt(2.0);
	CHECK(noise_texture->get_seamless_blend_skirt() == doctest::Approx(0.45));
	ERR_PRINT_ON;

	noise_texture->set_as_normal_map(true);
	CHECK(noise_texture->is_normal_map() == true);
	noise_texture->set_as_normal_map(false);
	CHECK(noise_texture->is_normal_map() == false);

	noise_texture->set_bump_strength(0.168);
	CHECK(noise_texture->get_bump_strength() == doctest::Approx(0.168));

	Ref<Gradient> gradient = memnew(Gradient);
	noise_texture->set_color_ramp(gradient);
	CHECK(noise_texture->get_color_ramp() == gradient);
	noise_texture->set_color_ramp(nullptr);
	CHECK(noise_texture->get_color_ramp().is_null());
}

TEST_CASE("[NoiseTexture2D][SceneTree] Generating a basic noise texture with mipmaps and color ramp modulation") {
	Ref<NoiseTexture2D> noise_texture = memnew(NoiseTexture2D);

	Ref<FastNoiseLite> noise = memnew(FastNoiseLite);
	noise_texture->set_noise(noise);

	Ref<Gradient> gradient = memnew(Gradient);
	Vector<float> offsets = { 0.0, 1.0 };
	Vector<Color> colors = { Color(1, 0, 0), Color(0, 0, 1) };
	gradient->set_offsets(offsets);
	gradient->set_colors(colors);

	noise_texture->set_color_ramp(gradient);
	noise_texture->set_width(16);
	noise_texture->set_height(16);
	noise_texture->set_generate_mipmaps(true);

	Ref<NoiseTextureTester> tester = memnew(NoiseTextureTester(noise_texture.ptr()));
	noise_texture->connect_changed(callable_mp(tester.ptr(), &NoiseTextureTester::check_mip_and_color_ramp));
}

TEST_CASE("[NoiseTexture2D][SceneTree] Generating a normal map without mipmaps") {
	Ref<NoiseTexture2D> noise_texture = memnew(NoiseTexture2D);

	Ref<FastNoiseLite> noise = memnew(FastNoiseLite);
	noise->set_frequency(0.5);
	noise_texture->set_noise(noise);
	noise_texture->set_width(16);
	noise_texture->set_height(16);
	noise_texture->set_as_normal_map(true);
	noise_texture->set_bump_strength(0.5);
	noise_texture->set_generate_mipmaps(false);

	Ref<NoiseTextureTester> tester = memnew(NoiseTextureTester(noise_texture.ptr()));
	noise_texture->connect_changed(callable_mp(tester.ptr(), &NoiseTextureTester::check_normal_map));
}

TEST_CASE("[NoiseTexture2D][SceneTree] Generating a seamless noise texture") {
	Ref<NoiseTexture2D> noise_texture = memnew(NoiseTexture2D);

	Ref<FastNoiseLite> noise = memnew(FastNoiseLite);
	noise->set_frequency(0.5);
	noise_texture->set_noise(noise);
	noise_texture->set_width(16);
	noise_texture->set_height(16);
	noise_texture->set_seamless(true);

	Ref<NoiseTextureTester> tester = memnew(NoiseTextureTester(noise_texture.ptr()));

	SUBCASE("Grayscale(L8) 16x16, with seamless blend skirt of 0.05") {
		noise_texture->set_seamless_blend_skirt(0.05);
		noise_texture->connect_changed(callable_mp(tester.ptr(), &NoiseTextureTester::check_seamless_texture_grayscale));
	}

	SUBCASE("16x16 modulated with default (transparent)black and white gradient (RGBA8), with seamless blend skirt of 1.0") {
		Ref<Gradient> gradient = memnew(Gradient);

		Vector<float> offsets = { 0.0, 1.0 };
		Vector<Color> colors = { Color(0, 0, 0, 0), Color(1, 1, 1, 1) };
		gradient->set_offsets(offsets);
		gradient->set_colors(colors);

		noise_texture->set_color_ramp(gradient);
		noise_texture->set_seamless_blend_skirt(1.0);
		noise_texture->connect_changed(callable_mp(tester.ptr(), &NoiseTextureTester::check_seamless_texture_rgba));
	}
}

} //namespace TestNoiseTexture2D
// --- End modules/noise/tests/test_noise_texture_2d.h ---

// --- Folded from modules/noise/tests/test_noise_texture_3d.h ---
#include "scene/resources/image_texture.h"

#include "tests/test_macros.h"

namespace TestNoiseTexture3D {

class NoiseTexture3DTester : public RefCounted {

	const NoiseTexture3D *const texture;

public:
	NoiseTexture3DTester(const NoiseTexture3D *const p_texture) :
			texture{ p_texture } {}

	Color compute_average_color(const Ref<Image> &p_noise_image) {
		Color r_avg_color{};

		for (int i = 0; i < p_noise_image->get_width(); ++i) {
			for (int j = 0; j < p_noise_image->get_height(); ++j) {
				const Color pixel = p_noise_image->get_pixel(i, j);
				r_avg_color += pixel;
			}
		}

		int pixel_count = p_noise_image->get_width() * p_noise_image->get_height();
		r_avg_color /= pixel_count;
		return r_avg_color;
	}

	void check_mip_and_color_ramp() {
		const Vector<Ref<Image>> noise_data = texture->get_data();

		for (int i = 0; i < noise_data.size(); i++) {
			const Ref<Image> noise_image = noise_data[i];

			CHECK(noise_image.is_valid());
			CHECK(noise_image->get_width() == texture->get_width());
			CHECK(noise_image->get_height() == texture->get_height());

			CHECK(noise_image->get_format() == Image::FORMAT_RGBA8);
			CHECK(!noise_image->has_mipmaps());

			Color avg_color = compute_average_color(noise_image);

			// Check that the noise texture is modulated correctly by the color ramp (Gradient).
			CHECK_FALSE_MESSAGE((avg_color.r + avg_color.g + avg_color.b) == doctest::Approx(0.0), "The noise texture should not be all black");
			CHECK_FALSE_MESSAGE((avg_color.r + avg_color.g + avg_color.b) == doctest::Approx(noise_image->get_width() * noise_image->get_height() * 3.0), "The noise texture should not be all white");
			CHECK_MESSAGE(avg_color.g == doctest::Approx(0.0), "The noise texture should not have any green when modulated correctly by the color ramp");
		}
	}

	void check_seamless_texture_grayscale() {
		const Vector<Ref<Image>> noise_data = texture->get_data();

		for (int i = 0; i < noise_data.size(); i++) {
			const Ref<Image> noise_image = noise_data[i];

			CHECK(noise_image.is_valid());
			CHECK(noise_image->get_width() == texture->get_width());
			CHECK(noise_image->get_height() == texture->get_height());

			CHECK(noise_image->get_format() == Image::FORMAT_L8);

			Color avg_color = compute_average_color(noise_image);

			// Since it's a grayscale image and every channel except the alpha channel has the
			// same values (conversion happens in Image::get_pixel) we only need to test one channel.
			CHECK(avg_color.r == doctest::Approx(0.5).epsilon(0.05));
		}
	}

	void check_seamless_texture_rgba() {
		const Vector<Ref<Image>> noise_data = texture->get_data();

		for (int i = 0; i < noise_data.size(); i++) {
			const Ref<Image> noise_image = noise_data[i];

			CHECK(noise_image.is_valid());
			CHECK(noise_image->get_width() == texture->get_width());
			CHECK(noise_image->get_height() == texture->get_height());

			CHECK(noise_image->get_format() == Image::FORMAT_RGBA8);

			// Check that the noise texture is modulated correctly by the color ramp (Gradient).
			Color avg_color = compute_average_color(noise_image);

			// We use a default (black to white) gradient, so the average of the red, green and blue channels should be the same.
			CHECK(avg_color.r == doctest::Approx(0.5).epsilon(0.05));
			CHECK(avg_color.g == doctest::Approx(0.5).epsilon(0.05));
			CHECK(avg_color.b == doctest::Approx(0.5).epsilon(0.05));
		}
	}
};

TEST_CASE("[NoiseTexture][SceneTree] Getter and setter") {
	Ref<NoiseTexture3D> noise_texture = memnew(NoiseTexture3D);

	Ref<FastNoiseLite> noise = memnew(FastNoiseLite);
	noise_texture->set_noise(noise);
	CHECK(noise_texture->get_noise() == noise);
	noise_texture->set_noise(nullptr);
	CHECK(noise_texture->get_noise().is_null());

	noise_texture->set_width(8);
	noise_texture->set_height(4);
	noise_texture->set_depth(2);
	CHECK(noise_texture->get_width() == 8);
	CHECK(noise_texture->get_height() == 4);
	CHECK(noise_texture->get_depth() == 2);

	ERR_PRINT_OFF;
	noise_texture->set_width(-1);
	noise_texture->set_height(-1);
	noise_texture->set_depth(-1);
	ERR_PRINT_ON;
	CHECK(noise_texture->get_width() == 8);
	CHECK(noise_texture->get_height() == 4);
	CHECK(noise_texture->get_depth() == 2);

	noise_texture->set_invert(true);
	CHECK(noise_texture->get_invert() == true);
	noise_texture->set_invert(false);
	CHECK(noise_texture->get_invert() == false);

	noise_texture->set_seamless(true);
	CHECK(noise_texture->get_seamless() == true);
	noise_texture->set_seamless(false);
	CHECK(noise_texture->get_seamless() == false);

	noise_texture->set_seamless_blend_skirt(0.45);
	CHECK(noise_texture->get_seamless_blend_skirt() == doctest::Approx(0.45));

	ERR_PRINT_OFF;
	noise_texture->set_seamless_blend_skirt(-1.0);
	noise_texture->set_seamless_blend_skirt(2.0);
	CHECK(noise_texture->get_seamless_blend_skirt() == doctest::Approx(0.45));
	ERR_PRINT_ON;

	Ref<Gradient> gradient = memnew(Gradient);
	noise_texture->set_color_ramp(gradient);
	CHECK(noise_texture->get_color_ramp() == gradient);
	noise_texture->set_color_ramp(nullptr);
	CHECK(noise_texture->get_color_ramp().is_null());
}

TEST_CASE("[NoiseTexture3D][SceneTree] Generating a basic noise texture with mipmaps and color ramp modulation") {
	Ref<NoiseTexture3D> noise_texture = memnew(NoiseTexture3D);

	Ref<FastNoiseLite> noise = memnew(FastNoiseLite);
	noise_texture->set_noise(noise);

	Ref<Gradient> gradient = memnew(Gradient);
	Vector<float> offsets = { 0.0, 1.0 };
	Vector<Color> colors = { Color(1, 0, 0), Color(0, 0, 1) };
	gradient->set_offsets(offsets);
	gradient->set_colors(colors);

	noise_texture->set_color_ramp(gradient);
	noise_texture->set_width(16);
	noise_texture->set_height(16);
	noise_texture->set_depth(16);

	Ref<NoiseTexture3DTester> tester = memnew(NoiseTexture3DTester(noise_texture.ptr()));
	noise_texture->connect_changed(callable_mp(tester.ptr(), &NoiseTexture3DTester::check_mip_and_color_ramp));
}

TEST_CASE("[NoiseTexture3D][SceneTree] Generating a seamless noise texture") {
	Ref<NoiseTexture3D> noise_texture = memnew(NoiseTexture3D);

	Ref<FastNoiseLite> noise = memnew(FastNoiseLite);
	noise->set_frequency(0.5);
	noise_texture->set_noise(noise);
	noise_texture->set_width(16);
	noise_texture->set_height(16);
	noise_texture->set_depth(16);
	noise_texture->set_seamless(true);

	Ref<NoiseTexture3DTester> tester = memnew(NoiseTexture3DTester(noise_texture.ptr()));

	SUBCASE("Grayscale(L8) 16x16x16, with seamless blend skirt of 0.05") {
		noise_texture->set_seamless_blend_skirt(0.05);
		noise_texture->connect_changed(callable_mp(tester.ptr(), &NoiseTexture3DTester::check_seamless_texture_grayscale));
	}

	SUBCASE("16x16x16 modulated with default (transparent)black and white gradient (RGBA8), with seamless blend skirt of 1.0") {
		Ref<Gradient> gradient = memnew(Gradient);

		Vector<float> offsets = { 0.0, 1.0 };
		Vector<Color> colors = { Color(0, 0, 0, 0), Color(1, 1, 1, 1) };
		gradient->set_offsets(offsets);
		gradient->set_colors(colors);

		noise_texture->set_color_ramp(gradient);
		noise_texture->set_seamless_blend_skirt(1.0);
		noise_texture->connect_changed(callable_mp(tester.ptr(), &NoiseTexture3DTester::check_seamless_texture_rgba));
	}
}

} //namespace TestNoiseTexture3D
// --- End modules/noise/tests/test_noise_texture_3d.h ---
