/**************************************************************************/
/*  range.cpp                                                             */
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

#include <cmath>
#include <thirdparty/misc/r128.h>
#include "core/string/translation_server.h"
#include "range.h"

double NumericRange::snapped_r128(double p_value, double p_step)
{
	if (p_step == 0.0) {
		return p_value;
	}
	if (p_value > (1e18 * p_step) || p_value < -(1e18 * p_step)) {
		return Math::snapped(p_value, p_step);
	}
	const int decimals = MIN(18, 14 - Math::floor(std::log10(MAX(Math::abs(p_value), p_step))));
	const double scale = Math::pow(10.0, decimals);
	p_value *= scale;
	p_step *= scale;
	String step_str = String::num(p_step);
	String value_str = String::num(p_value);
	R128 step_r128;
	R128 value_r128;
	const R128 half_r128 = R128(0.5);
	r128FromString(&step_r128, step_str.ascii().get_data(), nullptr);
	r128FromString(&value_r128, value_str.ascii().get_data(), nullptr);
	r128Div(&value_r128, &value_r128, &step_r128);
	r128Add(&value_r128, &value_r128, &half_r128);
	r128Floor(&value_r128, &value_r128);
	r128Mul(&value_r128, &value_r128, &step_r128);
	if (scale != 1.0) {
		const R128 scale_r128 = R128(scale);
		r128Div(&value_r128, &value_r128, &scale_r128);
	}
	return (double)value_r128;
}

double NumericRange::calc_value(double p_val, double p_step) const
{
	if (Math::is_nan(p_val)) {
		return p_val;
	}

	if (p_step > 0) {
		if (Math::abs(min) > p_step * 1e14) {
			p_val = snapped_r128(p_val, p_step);
		}
		else {
			p_val = snapped_r128(p_val - min, p_step) + min;
		}
	}

	if (rounded_values) {
		p_val = Math::round(p_val);
	}

	if (!allow_greater && p_val > max - page) {
		p_val = max - page;
	}

	if (!allow_lesser && p_val < min) {
		p_val = min;
	}
	return p_val;
}

bool NumericRange::set_value(double p_val)
{
	double prev_val = value;
	value = calc_value(p_val, step);
	return value != prev_val && !(Math::is_nan(value) && Math::is_nan(prev_val));
}

bool NumericRange::set_min(double p_min)
{
	if (min == p_min) {
		return false;
	}
	min = p_min;
	max = MAX(max, min);
	page = CLAMP(page, 0.0, max - min);
	value = calc_value(value, step);
	return true;
}

bool NumericRange::set_max(double p_max)
{
	double max_validated = MAX(p_max, min);
	if (max == max_validated) {
		return false;
	}
	max = max_validated;
	page = CLAMP(page, 0.0, max - min);
	value = calc_value(value, step);
	return true;
}

bool NumericRange::set_step(double p_step)
{
	if (step == p_step) {
		return false;
	}
	step = p_step;
	return true;
}

bool NumericRange::set_page(double p_page)
{
	double page_validated = CLAMP(p_page, 0.0, max - min);
	if (page == page_validated) {
		return false;
	}
	page = page_validated;
	value = calc_value(value, step);
	return true;
}

bool NumericRange::set_as_ratio(double p_ratio)
{
	double v;
	if (exp_ratio && min >= 0) {
		double exp_min = min == 0 ? 0.0 : Math::log(min) / Math::log(2.0);
		double exp_max = Math::log(max) / Math::log(2.0);
		v = Math::pow(2.0, exp_min + (exp_max - exp_min) * p_ratio);
	}
	else {
		double percent = (max - min) * p_ratio;
		if (step > 0) {
			double steps = std::round(percent / step);
			v = steps * step + min;
		}
		else {
			v = percent + min;
		}
	}
	v = CLAMP(v, min, max);
	return set_value(v);
}

double NumericRange::get_as_ratio() const
{
	if (Math::is_equal_approx(max, min)) {
		return 1.0;
	}

	if (exp_ratio && min >= 0) {
		double exp_min = min == 0 ? 0.0 : Math::log(min) / Math::log(2.0);
		double exp_max = Math::log(max) / Math::log(2.0);
		double clamped = CLAMP(value, min, max);
		double v = Math::log(clamped) / Math::log(2.0);
		return CLAMP((v - exp_min) / (exp_max - exp_min), 0.0, 1.0);
	}
	else {
		double clamped = CLAMP(value, min, max);
		return CLAMP((clamped - min) / (max - min), 0.0, 1.0);
	}
}

void NumericRange::append_configuration_warnings(PackedStringArray& r_warnings) const
{
	if (exp_ratio && min < 0) {
		r_warnings.push_back(
			RTR("If \"Exp Edit\" is enabled, \"Min Value\" must be greater or equal to 0."));
	}
}
