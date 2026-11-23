#include "modules/modules_enabled.gen.h"

#if defined(MODULE_MOBILE_VR_ENABLED)
#include "core/input/input.h"
#include "modules/register_module_types.h"
#include "core/os/os.h"
// --- Inlined from modules/mobile_vr/mobile_vr_interface.h ---
#include "servers/xr/xr_interface.h"
#include "servers/xr/xr_server.h"

/**
	The mobile interface is a native VR interface that can be used on Android and iOS phones.
	It contains a basic implementation supporting 3DOF tracking if a gyroscope and accelerometer are
	present and sets up the proper projection matrices based on the values provided.

	We're planning to eventually do separate interfaces towards mobile SDKs that have far more
   capabilities and do not rely on the user providing most of these settings (though enhancing this
   with auto detection features based on the device we're running on would be cool). I'm mostly
   adding this as an example or base plate for more advanced interfaces.
*/

class MobileVRInterface : public XRInterface
{
private:
	bool initialized = false;
	XRInterface::TrackingStatus tracking_state;
	XRPose::TrackingConfidence tracking_confidence = XRPose::XR_TRACKING_CONFIDENCE_NONE;

	// Just set some defaults for these. At some point we need to look at adding a lookup table for
	// common device + headset combos and/or support reading cardboard QR codes
	double eye_height = 1.85;
	uint64_t last_ticks = 0;

	double intraocular_dist = 6.0;
	double display_width = 14.5;
	double display_to_lens = 4.0;
	double oversample = 1.5;

	Rect2 offset_rect = Rect2(0, 0, 1, 1); // Full screen rect.

	double k1 = 0.215;
	double k2 = 0.215;
	double aspect = 1.0;

	// at a minimum we need a tracker for our head
	Ref<XRTracker> head;
	Transform3D head_transform;

	XRVRS xr_vrs;

	/*
		logic for processing our sensor data, this was originally in our positional tracker logic
	   but I think that doesn't make sense in hindsight. It only makes marginally more sense to park
	   it here for now, this probably deserves an object of its own
	*/
	Vector3 scale_magneto(const Vector3& p_magnetometer);
	Basis combine_acc_mag(const Vector3& p_grav, const Vector3& p_magneto);

	int mag_count = 0;
	bool has_gyro = false;
	bool sensor_first = false;
	Vector3 last_accerometer_data;
	Vector3 last_magnetometer_data;
	Vector3 mag_current_min;
	Vector3 mag_current_max;
	Vector3 mag_next_min;
	Vector3 mag_next_max;

	///@TODO a few support functions for trackers, most are math related and should likely be moved
	///elsewhere
	float floor_decimals(const float p_value, const float p_decimals)
	{
		float power_of_10 = std::pow(10.0f, p_decimals);
		return std::floor(p_value * power_of_10) / power_of_10;
	}

	Vector3 floor_decimals(const Vector3& p_vector, const float p_decimals)
	{
		return Vector3(floor_decimals(p_vector.x, p_decimals),
			floor_decimals(p_vector.y, p_decimals), floor_decimals(p_vector.z, p_decimals));
	}

	Vector3 low_pass(const Vector3& p_vector, const Vector3& p_last_vector, const float p_factor)
	{
		return p_vector + (p_factor * (p_last_vector - p_vector));
	}

	Vector3 scrub(const Vector3& p_vector, const Vector3& p_last_vector, const float p_decimals,
		const float p_factor)
	{
		return low_pass(floor_decimals(p_vector, p_decimals), p_last_vector, p_factor);
	}

	void set_position_from_sensors();

protected:

public:
	void set_eye_height(const double p_eye_height);
	double get_eye_height() const;

	void set_iod(const double p_iod);
	double get_iod() const;

	void set_display_width(const double p_display_width);
	double get_display_width() const;

	void set_offset_rect(const Rect2& p_offset_rect);
	Rect2 get_offset_rect() const;

	void set_display_to_lens(const double p_display_to_lens);
	double get_display_to_lens() const;

	void set_oversample(const double p_oversample);
	double get_oversample() const;

	void set_k1(const double p_k1);
	double get_k1() const;

	void set_k2(const double p_k2);
	double get_k2() const;

	float get_vrs_min_radius() const;
	void set_vrs_min_radius(float p_vrs_min_radius);

	float get_vrs_strength() const;
	void set_vrs_strength(float p_vrs_strength);

	virtual String get_name() const override;
	virtual uint32_t get_capabilities() const override;

	virtual TrackingStatus get_tracking_status() const override;

	virtual bool is_initialized() const override;
	virtual bool initialize() override;
	virtual void uninitialize() override;

	virtual bool supports_play_area_mode(XRInterface::PlayAreaMode p_mode) override;
	virtual XRInterface::PlayAreaMode get_play_area_mode() const override;
	virtual bool set_play_area_mode(XRInterface::PlayAreaMode p_mode) override;

	virtual Size2 get_render_target_size() override;
	virtual uint32_t get_view_count() override;
	virtual Transform3D get_camera_transform() override;
	virtual Transform3D get_transform_for_view(
		uint32_t p_view, const Transform3D& p_cam_transform) override;
	virtual Projection get_projection_for_view(
		uint32_t p_view, double p_aspect, double p_z_near, double p_z_far) override;
	virtual Vector<RenderingServerTypes::BlitToScreen> post_draw_viewport(
		RID p_render_target, const Rect2& p_screen_rect) override;

	virtual void process() override;

	virtual RID get_vrs_texture() override;

	MobileVRInterface();
	~MobileVRInterface();
};



// --- End modules/mobile_vr/mobile_vr_interface.h ---
#include "servers/display/display_server.h"
#include "servers/rendering/rendering_server_types.h"

String MobileVRInterface::get_name() const { return "Native mobile"; }

uint32_t MobileVRInterface::get_capabilities() const { return XRInterface::XR_STEREO; }

Vector3 MobileVRInterface::scale_magneto(const Vector3& p_magnetometer)
{
	// Our magnetometer doesn't give us nice clean data.
	// Well it may on macOS because we're getting a calibrated value in the current implementation
	// but Android we're getting raw data. This is a fairly simple adjustment we can do to correct
	// for the magnetometer data being elliptical

	Vector3 mag_raw = p_magnetometer;
	Vector3 mag_scaled = p_magnetometer;

	// update our variables every x frames
	if (mag_count > 20) {
		mag_current_min = mag_next_min;
		mag_current_max = mag_next_max;
		mag_count = 0;
	}
	else {
		mag_count++;
	};

	// adjust our min and max
	if (mag_raw.x > mag_next_max.x) {
		mag_next_max.x = mag_raw.x;
	}
	if (mag_raw.y > mag_next_max.y) {
		mag_next_max.y = mag_raw.y;
	}
	if (mag_raw.z > mag_next_max.z) {
		mag_next_max.z = mag_raw.z;
	}

	if (mag_raw.x < mag_next_min.x) {
		mag_next_min.x = mag_raw.x;
	}
	if (mag_raw.y < mag_next_min.y) {
		mag_next_min.y = mag_raw.y;
	}
	if (mag_raw.z < mag_next_min.z) {
		mag_next_min.z = mag_raw.z;
	}

	// scale our x, y and z
	if (!(mag_current_max.x - mag_current_min.x)) {
		mag_raw.x -= (mag_current_min.x + mag_current_max.x) / 2.0;
		mag_scaled.x =
			(mag_raw.x - mag_current_min.x) / ((mag_current_max.x - mag_current_min.x) * 2.0 - 1.0);
	};

	if (!(mag_current_max.y - mag_current_min.y)) {
		mag_raw.y -= (mag_current_min.y + mag_current_max.y) / 2.0;
		mag_scaled.y =
			(mag_raw.y - mag_current_min.y) / ((mag_current_max.y - mag_current_min.y) * 2.0 - 1.0);
	};

	if (!(mag_current_max.z - mag_current_min.z)) {
		mag_raw.z -= (mag_current_min.z + mag_current_max.z) / 2.0;
		mag_scaled.z =
			(mag_raw.z - mag_current_min.z) / ((mag_current_max.z - mag_current_min.z) * 2.0 - 1.0);
	};

	return mag_scaled;
}

Basis MobileVRInterface::combine_acc_mag(const Vector3& p_grav, const Vector3& p_magneto)
{
	// yup, stock standard cross product solution...
	Vector3 up = -p_grav.normalized();

	Vector3 magneto_east =
		up.cross(p_magneto.normalized()); // or is this west?, but should be horizon aligned now
	magneto_east.normalize();

	Vector3 magneto = up.cross(magneto_east); // and now we have a horizon aligned north
	magneto.normalize();

	// We use our gravity and magnetometer vectors to construct our matrix
	Basis acc_mag_m3;
	acc_mag_m3.rows[0] = -magneto_east;
	acc_mag_m3.rows[1] = up;
	acc_mag_m3.rows[2] = magneto;

	return acc_mag_m3;
}

void MobileVRInterface::set_position_from_sensors()
{
	_THREAD_SAFE_METHOD_

	// this is a helper function that attempts to adjust our transform using our 9dof sensors
	// 9dof is a misleading marketing term coming from 3 accelerometer axis + 3 gyro axis + 3
	// magnetometer axis = 9 axis but in reality this only offers 3 dof (yaw, pitch, roll)
	// orientation

	Basis orientation = head_transform.basis;

	uint64_t ticks = OS::get_ticks_usec();
	uint64_t ticks_elapsed = ticks - last_ticks;
	float delta_time = (double)ticks_elapsed / 1000000.0;

	// few things we need
	Vector3 down(0.0, -1.0, 0.0); // Down is Y negative

	// make copies of our inputs
	bool has_grav = false;
	Vector3 acc = Input::get_accelerometer();
	Vector3 gyro = Input::get_gyroscope();
	Vector3 grav = Input::get_gravity();
	Vector3 magneto = scale_magneto(
		Input::get_magnetometer()); // this may be overkill on iOS because we're already getting a
									// calibrated magnetometer reading

	if (sensor_first) {
		sensor_first = false;
	}
	else {
		acc = scrub(acc, last_accerometer_data, 2, 0.2f);
		magneto = scrub(magneto, last_magnetometer_data, 3, 0.3f);
	};

	last_accerometer_data = acc;
	last_magnetometer_data = magneto;

	if (grav.length() < 0.1f) {
		// not ideal but use our accelerometer, this will contain shaky user behavior
		// maybe look into some math but I'm guessing that if this isn't available, it's because we
		// lack the gyro sensor to actually work out what a stable gravity vector is
		grav = acc;
		if (grav.length() > 0.1f) {
			has_grav = true;
		};
	}
	else {
		has_grav = true;
	};

	bool has_magneto = magneto.length() > 0.1f;
	if (gyro.length() > 0.1f) {
		/* this can return to 0.0 if the user doesn't move the phone, so once on, it's on */
		has_gyro = true;
	};

	if (has_gyro) {
		// start with applying our gyro (do NOT smooth our gyro!)
		Basis rotate;
		rotate.rotate(orientation.get_column(0), gyro.x * delta_time);
		rotate.rotate(orientation.get_column(1), gyro.y * delta_time);
		rotate.rotate(orientation.get_column(2), gyro.z * delta_time);
		orientation = rotate * orientation;

		tracking_state = XRInterface::XR_NORMAL_TRACKING;
		tracking_confidence = XRPose::XR_TRACKING_CONFIDENCE_HIGH;
	};

	///@TODO improve this, the magnetometer is very fidgety sometimes flipping the axis for no
	///apparent reason (probably a bug on my part)
	// if you have a gyro + accelerometer that combo tends to be better than combining all three but
	// without a gyro you need the magnetometer..
	if (has_magneto && has_grav && !has_gyro) {
		// convert to quaternions, easier to smooth those out
		Quaternion transform_quat(orientation);
		Quaternion acc_mag_quat(combine_acc_mag(grav, magneto));
		transform_quat = transform_quat.slerp(acc_mag_quat, 0.1);
		orientation = Basis(transform_quat);

		tracking_state = XRInterface::XR_NORMAL_TRACKING;
		tracking_confidence = XRPose::XR_TRACKING_CONFIDENCE_HIGH;
	}
	else if (has_grav) {
		// use gravity vector to make sure down is down...
		// transform gravity into our world space
		grav.normalize();
		Vector3 grav_adj = orientation.xform(grav);
		float dot = grav_adj.dot(down);
		if ((dot > -1.0) && (dot < 1.0)) {
			// axis around which we have this rotation
			Vector3 axis = grav_adj.cross(down);
			axis.normalize();

			Basis drift_compensation(axis, std::acos(dot) * delta_time * 10);
			orientation = drift_compensation * orientation;
		};
	};

	// and copy to our head transform
	head_transform.basis = orientation.orthonormalized();

	last_ticks = ticks;
}


void MobileVRInterface::set_eye_height(const double p_eye_height) { eye_height = p_eye_height; }

double MobileVRInterface::get_eye_height() const { return eye_height; }

void MobileVRInterface::set_offset_rect(const Rect2& p_offset_rect) { offset_rect = p_offset_rect; }

Rect2 MobileVRInterface::get_offset_rect() const { return offset_rect; }

void MobileVRInterface::set_iod(const double p_iod) { intraocular_dist = p_iod; }

double MobileVRInterface::get_iod() const { return intraocular_dist; }

void MobileVRInterface::set_display_width(const double p_display_width)
{
	display_width = p_display_width;
}

double MobileVRInterface::get_display_width() const { return display_width; }

void MobileVRInterface::set_display_to_lens(const double p_display_to_lens)
{
	display_to_lens = p_display_to_lens;
}

double MobileVRInterface::get_display_to_lens() const { return display_to_lens; }

void MobileVRInterface::set_oversample(const double p_oversample) { oversample = p_oversample; }

double MobileVRInterface::get_oversample() const { return oversample; }

void MobileVRInterface::set_k1(const double p_k1) { k1 = p_k1; }

double MobileVRInterface::get_k1() const { return k1; }

void MobileVRInterface::set_k2(const double p_k2) { k2 = p_k2; }

double MobileVRInterface::get_k2() const { return k2; }

float MobileVRInterface::get_vrs_min_radius() const { return xr_vrs.get_vrs_min_radius(); }

void MobileVRInterface::set_vrs_min_radius(float p_vrs_min_radius)
{
	xr_vrs.set_vrs_min_radius(p_vrs_min_radius);
}

float MobileVRInterface::get_vrs_strength() const { return xr_vrs.get_vrs_strength(); }

void MobileVRInterface::set_vrs_strength(float p_vrs_strength)
{
	xr_vrs.set_vrs_strength(p_vrs_strength);
}

uint32_t MobileVRInterface::get_view_count()
{
	// needs stereo...
	return 2;
}

XRInterface::TrackingStatus MobileVRInterface::get_tracking_status() const
{
	return tracking_state;
}

bool MobileVRInterface::is_initialized() const { return (initialized); }

bool MobileVRInterface::initialize()
{
	ERR_FAIL_COND_V(!XRServer::is_initialized(), false);

	if (!initialized) {
		// reset our sensor data
		mag_count = 0;
		has_gyro = false;
		sensor_first = true;
		mag_next_min = Vector3(10000, 10000, 10000);
		mag_next_max = Vector3(-10000, -10000, -10000);
		mag_current_min = Vector3(0, 0, 0);
		mag_current_max = Vector3(0, 0, 0);
		head_transform.basis = Basis();
		head_transform.origin = Vector3(0.0, eye_height, 0.0);

		// we must create a tracker for our head
		head.instantiate();
		head->set_tracker_type(XRServer::TRACKER_HEAD);
		head->set_tracker_name("head");
		head->set_tracker_desc("Players head");
		XRServer::add_tracker(head);

		// make this our primary interface
		XRServer::set_primary_interface(this);

		last_ticks = OS::get_ticks_usec();

		initialized = true;
	};

	return true;
}

void MobileVRInterface::uninitialize()
{
	if (initialized) {
		// do any cleanup here...
		if (XRServer::is_initialized()) {
			if (head.is_valid()) {
				XRServer::remove_tracker(head);

				head.unref();
			}

			if (XRServer::get_primary_interface() == this) {
				// no longer our primary interface
				XRServer::set_primary_interface(nullptr);
			}
		}

		initialized = false;
	};
}

bool MobileVRInterface::supports_play_area_mode(XRInterface::PlayAreaMode p_mode)
{
	// This interface has no positional tracking so fix this to 3DOF
	return p_mode == XR_PLAY_AREA_3DOF;
}

XRInterface::PlayAreaMode MobileVRInterface::get_play_area_mode() const
{
	return XR_PLAY_AREA_3DOF;
}

bool MobileVRInterface::set_play_area_mode(XRInterface::PlayAreaMode p_mode)
{
	return p_mode == XR_PLAY_AREA_3DOF;
}

Size2 MobileVRInterface::get_render_target_size()
{
	_THREAD_SAFE_METHOD_

	// we use half our window size
	Size2 target_size = DisplayServer::window_get_size();

	target_size.x *= 0.5 * oversample;
	target_size.y *= oversample;

	return target_size;
}

Transform3D MobileVRInterface::get_camera_transform()
{
	_THREAD_SAFE_METHOD_

	Transform3D transform_for_eye;

	ERR_FAIL_COND_V(!XRServer::is_initialized(), transform_for_eye);

	if (initialized) {
		float world_scale = XRServer::get_world_scale();

		// just scale our origin point of our transform
		Transform3D _head_transform = head_transform;
		_head_transform.origin *= world_scale;

		transform_for_eye = (XRServer::get_reference_frame()) * _head_transform;
	}

	return transform_for_eye;
}

Transform3D MobileVRInterface::get_transform_for_view(
	uint32_t p_view, const Transform3D& p_cam_transform)
{
	_THREAD_SAFE_METHOD_

	Transform3D transform_for_eye;

	ERR_FAIL_COND_V(!XRServer::is_initialized(), transform_for_eye);

	if (initialized) {
		float world_scale = XRServer::get_world_scale();

		// we don't need to check for the existence of our HMD, doesn't affect our values...
		// note * 0.01 to convert cm to m and * 0.5 as we're moving half in each direction...
		if (p_view == 0) {
			transform_for_eye.origin.x = -(intraocular_dist * 0.01 * 0.5 * world_scale);
		}
		else if (p_view == 1) {
			transform_for_eye.origin.x = intraocular_dist * 0.01 * 0.5 * world_scale;
		}
		else {
			// should not have any other values..
		};

		// just scale our origin point of our transform
		Transform3D _head_transform = head_transform;
		_head_transform.origin *= world_scale;

		transform_for_eye = p_cam_transform * (XRServer::get_reference_frame()) * _head_transform *
							transform_for_eye;
	}
	else {
		// huh? well just return what we got....
		transform_for_eye = p_cam_transform;
	};

	return transform_for_eye;
}

Projection MobileVRInterface::get_projection_for_view(
	uint32_t p_view, double p_aspect, double p_z_near, double p_z_far)
{
	_THREAD_SAFE_METHOD_

	Projection eye;

	aspect = p_aspect;
	eye.set_for_hmd(p_view + 1, p_aspect, intraocular_dist, display_width, display_to_lens,
		oversample, p_z_near, p_z_far);

	return eye;
}

Vector<RenderingServerTypes::BlitToScreen> MobileVRInterface::post_draw_viewport(
	RID p_render_target, const Rect2& p_screen_rect)
{
	_THREAD_SAFE_METHOD_

	Vector<RenderingServerTypes::BlitToScreen> blit_to_screen;

	// We must have a valid render target.
	ERR_FAIL_COND_V(!p_render_target.is_valid(), blit_to_screen);

	// We will only output to screen if this is our main viewport.
	if (p_screen_rect == Rect2()) {
		// Warn the developer once, it's up to the developer to output to screen.
		WARN_PRINT_ONCE("SubViewport used with MobileVRInterface, no output to screen");

		return blit_to_screen;
	}

	Rect2 modified_screen_rect =
		Rect2(p_screen_rect.position + offset_rect.position * p_screen_rect.size,
			p_screen_rect.size * offset_rect.size);

	// and add our blits
	RenderingServerTypes::BlitToScreen blit;
	blit.render_target = p_render_target;
	blit.multi_view.use_layer = true;
	blit.lens_distortion.apply = true;
	blit.lens_distortion.k1 = k1;
	blit.lens_distortion.k2 = k2;
	blit.lens_distortion.upscale = oversample;
	blit.lens_distortion.aspect_ratio = aspect;

	// left eye
	blit.dst_rect = modified_screen_rect;
	blit.dst_rect.size.width *= 0.5;
	blit.multi_view.layer = 0;
	blit.lens_distortion.eye_center.x =
		((-intraocular_dist / 2.0) + (display_width / 4.0)) / (display_width / 2.0);
	blit_to_screen.push_back(blit);

	// right eye
	blit.dst_rect = modified_screen_rect;
	blit.dst_rect.size.width *= 0.5;
	blit.dst_rect.position.x += blit.dst_rect.size.width;
	blit.multi_view.layer = 1;
	blit.lens_distortion.eye_center.x =
		((intraocular_dist / 2.0) - (display_width / 4.0)) / (display_width / 2.0);
	blit_to_screen.push_back(blit);

	return blit_to_screen;
}

void MobileVRInterface::process()
{
	_THREAD_SAFE_METHOD_

	if (initialized) {
		// update our head transform orientation
		set_position_from_sensors();

		// update our head transform position (should be constant)
		head_transform.origin = Vector3(0.0, eye_height, 0.0);

		if (head.is_valid()) {
			// Set our head position, note in real space, reference frame and world scale is applied
			// later
			head->set_pose("default", head_transform, Vector3(), Vector3(), tracking_confidence);
		}
	};
}

RID MobileVRInterface::get_vrs_texture()
{
	PackedVector2Array eye_foci;

	Size2 target_size = get_render_target_size();
	real_t aspect_ratio = target_size.x / target_size.y;
	uint32_t view_count = get_view_count();

	for (uint32_t v = 0; v < view_count; v++) {
		Projection cm = get_projection_for_view(v, aspect_ratio, 0.1, 1000.0);
		Vector3 center = cm.xform(Vector3(0.0, 0.0, 999.0));

		eye_foci.push_back(Vector2(center.x, center.y));
	}

	return xr_vrs.make_vrs_texture(target_size, eye_foci);
}

MobileVRInterface::MobileVRInterface() {}

MobileVRInterface::~MobileVRInterface()
{
	// and make sure we cleanup if we haven't already
	if (is_initialized()) {
		uninitialize();
	};
}

static Ref<MobileVRInterface> mobile_vr;

void initialize_mobile_vr_module(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
	if (XRServer::is_initialized()) {
		mobile_vr.instantiate();
		XRServer::add_interface(mobile_vr);
	}
}

void uninitialize_mobile_vr_module(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	if (mobile_vr.is_valid()) {
		// uninitialize our interface if it is initialized
		if (mobile_vr->is_initialized()) {
			mobile_vr->uninitialize();
		}

		// unregister our interface from the XR server
		if (XRServer::is_initialized()) {
			XRServer::remove_interface(mobile_vr);
		}

		// and release
		mobile_vr.unref();
	}
}
#endif // defined(MODULE_MOBILE_VR_ENABLED)


// --- Merged from modules/webxr/webxr_interface.cpp ---
#if defined(MODULE_WEBXR_ENABLED)
// --- Inlined from modules/webxr/webxr_interface.h ---
#include "core/templates/hash_map.h"
#include "core/templates/vector.h"
#include "servers/xr/xr_interface.h"
#include "servers/xr/xr_server.h"

#ifdef WEB_ENABLED
#include "core/templates/rb_map.h"

namespace GLES3 {
class TextureStorage;
}
#endif // WEB_ENABLED

/**
	The WebXR interface is a VR/AR interface that can be used on the web.
*/

class WebXRInterface final : public XRInterface {
public:
	enum TargetRayMode {
		TARGET_RAY_MODE_UNKNOWN,
		TARGET_RAY_MODE_GAZE,
		TARGET_RAY_MODE_TRACKED_POINTER,
		TARGET_RAY_MODE_SCREEN,
	};

protected:
#ifndef DISABLE_DEPRECATED
	static void _bind_compatibility_methods();
	Ref<XRTracker> _get_input_source_tracker_bind_compat_90645(int p_input_source_id) const;
#endif

public:
	void is_session_supported(const String &p_session_mode);
	void set_session_mode(const String &p_session_mode);
	String get_session_mode() const;
	void set_required_features(const String &p_required_features);
	String get_required_features() const;
	void set_optional_features(const String &p_optional_features);
	String get_optional_features() const;
	void set_requested_reference_space_types(const String &p_requested_reference_space_types);
	String get_requested_reference_space_types() const;
	String get_reference_space_type() const;
	String get_enabled_features() const;
	bool is_input_source_active(int p_input_source_id) const;
	Ref<XRTracker> get_input_source_tracker(int p_input_source_id) const;
	TargetRayMode get_input_source_target_ray_mode(int p_input_source_id) const;
	String get_visibility_state() const;
	float get_display_refresh_rate() const;
	void set_display_refresh_rate(float p_refresh_rate);
	Vector<float> get_available_display_refresh_rates() const;
	Vector<EnvironmentBlendMode> get_supported_environment_blend_modes();
	HashMap<String, String> get_system_info();

	virtual String get_name() const override;
	virtual uint32_t get_capabilities() const override;
	virtual bool is_initialized() const override;
	virtual bool initialize() override;
	virtual void uninitialize() override;

#ifdef WEB_ENABLED
private:
	bool initialized;
	Ref<XRTracker> head_tracker;
	Transform3D head_transform;

	String session_mode;
	String required_features;
	String optional_features;
	String requested_reference_space_types;
	String reference_space_type;
	String enabled_features;

	XRInterface::EnvironmentBlendMode environment_blend_mode = XRInterface::XR_ENV_BLEND_MODE_OPAQUE;

	Size2 render_targetsize;
	RBMap<unsigned int, RID> texture_cache;
	struct Touch {
		bool is_touching = false;
		Vector2 position;
	} touches[5];

	static constexpr uint8_t input_source_count = 16;

	struct InputSource {
		Ref<XRTracker> tracker;
		bool active = false;
		TargetRayMode target_ray_mode;
		int touch_index = -1;
	} input_sources[input_source_count];

	static const int WEBXR_HAND_JOINT_MAX = 25;
	static const int HAND_MAX = 2;
	Ref<XRTracker> hand_trackers[HAND_MAX];

	RID color_texture;
	RID depth_texture;

	RID _get_color_texture();
	RID _get_depth_texture();
	RID _get_texture(unsigned int p_texture_id);
	Transform3D _js_matrix_to_transform(float *p_js_matrix);
	void _update_input_source(int p_input_source_id);

	Vector2 _get_screen_position_from_joy_vector(const Vector2 &p_joy_vector);

public:
	virtual PackedVector3Array get_play_area() const override;
	virtual XRInterface::EnvironmentBlendMode get_environment_blend_mode() const override;
	virtual bool set_environment_blend_mode(EnvironmentBlendMode p_new_environment_blend_mode) override;

	virtual Size2 get_render_target_size() override;
	virtual uint32_t get_view_count() override;
	virtual Transform3D get_camera_transform() override;
	virtual Transform3D get_transform_for_view(uint32_t p_view, const Transform3D &p_cam_transform) override;
	virtual Projection get_projection_for_view(uint32_t p_view, double p_aspect, double p_z_near, double p_z_far) override;
	virtual bool pre_draw_viewport(RID p_render_target) override;
	virtual Vector<RenderingServerTypes::BlitToScreen> post_draw_viewport(RID p_render_target, const Rect2 &p_screen_rect) override;
	virtual RID get_color_texture() override;
	virtual RID get_depth_texture() override;
	virtual RID get_velocity_texture() override;

	virtual void process() override;

	void _on_input_event(int p_event_type, int p_input_source_id);

	// Internal setters used by callbacks from Emscripten.
	inline void _set_reference_space_type(const String &p_reference_space_type) { reference_space_type = p_reference_space_type; }
	inline void _set_enabled_features(const String &p_enabled_features) { enabled_features = p_enabled_features; }
	void _set_environment_blend_mode(const String &p_blend_mode_string);

	WebXRInterface();
	~WebXRInterface();

private:
	String tracker_names[16] = {
		String("left_hand"),
		String("right_hand"),
		String("tracker_2"),
		String("tracker_3"),
		String("tracker_4"),
		String("tracker_5"),
		String("tracker_6"),
		String("tracker_7"),
		String("tracker_8"),
		String("tracker_9"),
		String("tracker_10"),
		String("tracker_11"),
		String("tracker_12"),
		String("tracker_13"),
		String("tracker_14"),
		String("tracker_15"),
	};

	String touch_names[5] = {
		String("touch_0"),
		String("touch_1"),
		String("touch_2"),
		String("touch_3"),
		String("touch_4"),
	};

	String standard_axis_names[10] = {
		String("touchpad_x"),
		String("touchpad_y"),
		String("thumbstick_x"),
		String("thumbstick_y"),
		String("axis_4"),
		String("axis_5"),
		String("axis_6"),
		String("axis_7"),
		String("axis_8"),
		String("axis_9"),
	};

	String standard_vector_names[2] = {
		String("touchpad"),
		String("thumbstick"),
	};

	String standard_button_names[10] = {
		String("trigger_click"),
		String("grip_click"),
		String("touchpad_click"),
		String("thumbstick_click"),
		String("ax_button"),
		String("by_button"),
		String("button_6"),
		String("button_7"),
		String("button_8"),
		String("button_9"),
	};

	String standard_button_pressure_names[10] = {
		String("trigger"),
		String("grip"),
		String("touchpad_click_pressure"),
		String("thumbstick_click_pressure"),
		String("ax_button_pressure"),
		String("by_button_pressure"),
		String("button_pressure_6"),
		String("button_pressure_7"),
		String("button_pressure_8"),
		String("button_pressure_9"),
	};

	String unknown_button_names[10] = {
		String("button_0"),
		String("button_1"),
		String("button_2"),
		String("button_3"),
		String("button_4"),
		String("button_5"),
		String("button_6"),
		String("button_7"),
		String("button_8"),
		String("button_9"),
	};

	String unknown_axis_names[10] = {
		String("axis_0"),
		String("axis_1"),
		String("axis_2"),
		String("axis_3"),
		String("axis_4"),
		String("axis_5"),
		String("axis_6"),
		String("axis_7"),
		String("axis_8"),
		String("axis_9"),
	};

	String unknown_button_pressure_names[10] = {
		String("button_pressure_0"),
		String("button_pressure_1"),
		String("button_pressure_2"),
		String("button_pressure_3"),
		String("button_pressure_4"),
		String("button_pressure_5"),
		String("button_pressure_6"),
		String("button_pressure_7"),
		String("button_pressure_8"),
		String("button_pressure_9"),
	};
#else // !WEB_ENABLED
public:
	WebXRInterface() = default;
	~WebXRInterface() = default;
#endif // WEB_ENABLED
};

// --- End modules/webxr/webxr_interface.h ---
#include "modules/register_module_types.h"

#ifndef DISABLE_DEPRECATED


Ref<XRTracker> WebXRInterface::_get_input_source_tracker_bind_compat_90645(
	int p_input_source_id) const
{
	return get_input_source_tracker(p_input_source_id);
}

void WebXRInterface::_bind_compatibility_methods() {}

#endif // DISABLE_DEPRECATED


#ifdef WEB_ENABLED
// --- Inlined from modules/webxr/godot_webxr.h ---
#ifdef __cplusplus
extern "C" {
#endif

enum WebXRInputEvent {
	WEBXR_INPUT_EVENT_SELECTSTART,
	WEBXR_INPUT_EVENT_SELECTEND,
	WEBXR_INPUT_EVENT_SQUEEZESTART,
	WEBXR_INPUT_EVENT_SQUEEZEEND,
};

typedef void (*GodotWebXRSupportedCallback)(char *p_session_mode, int p_supported);
typedef void (*GodotWebXRStartedCallback)(char *p_reference_space_type, char *p_enabled_features, char *p_environment_blend_mode);
typedef void (*GodotWebXREndedCallback)();
typedef void (*GodotWebXRFailedCallback)(char *p_message);
typedef void (*GodotWebXRInputEventCallback)(int p_event_type, int p_input_source_id);
typedef void (*GodotWebXRSimpleEventCallback)(char *p_signal_name);

extern int godot_webxr_is_supported();
extern void godot_webxr_is_session_supported(const char *p_session_mode, GodotWebXRSupportedCallback p_callback);

extern void godot_webxr_initialize(
		const char *p_session_mode,
		const char *p_required_features,
		const char *p_optional_features,
		const char *p_requested_reference_space_types,
		GodotWebXRStartedCallback p_on_session_started,
		GodotWebXREndedCallback p_on_session_ended,
		GodotWebXRFailedCallback p_on_session_failed,
		GodotWebXRInputEventCallback p_on_input_event,
		GodotWebXRSimpleEventCallback p_on_simple_event);
extern void godot_webxr_uninitialize();

extern int godot_webxr_get_view_count();
extern bool godot_webxr_get_render_target_size(int *r_size);
extern bool godot_webxr_get_transform_for_view(int p_view, float *r_transform);
extern bool godot_webxr_get_projection_for_view(int p_view, float *r_transform);
extern unsigned int godot_webxr_get_color_texture();
extern unsigned int godot_webxr_get_depth_texture();
extern unsigned int godot_webxr_get_velocity_texture();

extern bool godot_webxr_update_input_source(
		int p_input_source_id,
		float *r_target_pose,
		int *r_target_ray_mode,
		int *r_touch_index,
		int *r_has_grip_pose,
		float *r_grip_pose,
		int *r_has_standard_mapping,
		int *r_button_count,
		float *r_buttons,
		int *r_axes_count,
		float *r_axes,
		int *r_has_hand_data,
		float *r_hand_joints,
		float *r_hand_radii);

extern char *godot_webxr_get_visibility_state();
extern int godot_webxr_get_bounds_geometry(float **r_points);

extern float godot_webxr_get_frame_rate();
extern void godot_webxr_update_target_frame_rate(float p_frame_rate);
extern int godot_webxr_get_supported_frame_rates(float **r_frame_rates);

#ifdef __cplusplus
}
#endif

// --- End modules/webxr/godot_webxr.h ---

#include "core/input/input.h"
#include "core/os/os.h"
#include "drivers/gles3/storage/texture_storage.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "servers/display/display_server.h"
#include "servers/xr/xr_server.h"

#include <emscripten.h>

#include <cstdlib>

void _emwebxr_on_session_supported(char *p_session_mode, int p_supported) {
	ERR_FAIL_COND(!XRServer::is_initialized());

	Ref<WebXRInterface> interface = XRServer::find_interface("WebXR");
	ERR_FAIL_COND(interface.is_null());

	(void)p_session_mode;
	(void)p_supported;
}

void _emwebxr_on_session_started(char *p_reference_space_type, char *p_enabled_features, char *p_environment_blend_mode) {
	ERR_FAIL_COND(!XRServer::is_initialized());

	Ref<WebXRInterface> interface = XRServer::find_interface("WebXR");
	ERR_FAIL_COND(interface.is_null());

	String reference_space_type = String(p_reference_space_type);
	interface->_set_reference_space_type(reference_space_type);
	interface->_set_enabled_features(p_enabled_features);
	interface->_set_environment_blend_mode(p_environment_blend_mode);
}

void _emwebxr_on_session_ended() {
	ERR_FAIL_COND(!XRServer::is_initialized());

	Ref<WebXRInterface> interface = XRServer::find_interface("WebXR");
	ERR_FAIL_COND(interface.is_null());

	interface->uninitialize();
}

void _emwebxr_on_session_failed(char *p_message) {
	ERR_FAIL_COND(!XRServer::is_initialized());

	Ref<WebXRInterface> interface = XRServer::find_interface("WebXR");
	ERR_FAIL_COND(interface.is_null());

	interface->uninitialize();

	ERR_PRINT(String(p_message));
}

extern "C" EMSCRIPTEN_KEEPALIVE void _emwebxr_on_input_event(int p_event_type, int p_input_source_id) {
	ERR_FAIL_COND(!XRServer::is_initialized());

	Ref<WebXRInterface> interface = XRServer::find_interface("WebXR");
	ERR_FAIL_COND(interface.is_null());

	interface->_on_input_event(p_event_type, p_input_source_id);
}

extern "C" EMSCRIPTEN_KEEPALIVE void _emwebxr_on_simple_event(char *p_signal_name) {
	ERR_FAIL_COND(!XRServer::is_initialized());

	Ref<WebXRInterface> interface = XRServer::find_interface("WebXR");
	ERR_FAIL_COND(interface.is_null());

	String signal_name = String(p_signal_name);
	(void)signal_name;
}

void WebXRInterface::is_session_supported(const String &p_session_mode) {
	godot_webxr_is_session_supported(p_session_mode.utf8().get_data(), &_emwebxr_on_session_supported);
}

void WebXRInterface::set_session_mode(const String &p_session_mode) {
	session_mode = p_session_mode;
}

String WebXRInterface::get_session_mode() const {
	return session_mode;
}

void WebXRInterface::set_required_features(const String &p_required_features) {
	required_features = p_required_features;
}

String WebXRInterface::get_required_features() const {
	return required_features;
}

void WebXRInterface::set_optional_features(const String &p_optional_features) {
	optional_features = p_optional_features;
}

String WebXRInterface::get_optional_features() const {
	return optional_features;
}

void WebXRInterface::set_requested_reference_space_types(const String &p_requested_reference_space_types) {
	requested_reference_space_types = p_requested_reference_space_types;
}

String WebXRInterface::get_requested_reference_space_types() const {
	return requested_reference_space_types;
}

String WebXRInterface::get_reference_space_type() const {
	return reference_space_type;
}

String WebXRInterface::get_enabled_features() const {
	return enabled_features;
}

bool WebXRInterface::is_input_source_active(int p_input_source_id) const {
	ERR_FAIL_INDEX_V(p_input_source_id, input_source_count, false);
	return input_sources[p_input_source_id].active;
}

Ref<XRTracker> WebXRInterface::get_input_source_tracker(int p_input_source_id) const {
	ERR_FAIL_INDEX_V(p_input_source_id, input_source_count, Ref<XRTracker>());
	return input_sources[p_input_source_id].tracker;
}

WebXRInterface::TargetRayMode WebXRInterface::get_input_source_target_ray_mode(int p_input_source_id) const {
	ERR_FAIL_INDEX_V(p_input_source_id, input_source_count, WebXRInterface::TARGET_RAY_MODE_UNKNOWN);
	if (!input_sources[p_input_source_id].active) {
		return WebXRInterface::TARGET_RAY_MODE_UNKNOWN;
	}
	return input_sources[p_input_source_id].target_ray_mode;
}

String WebXRInterface::get_visibility_state() const {
	char *c_str = godot_webxr_get_visibility_state();
	if (c_str) {
		String visibility_state = String(c_str);
		free(c_str);

		return visibility_state;
	}
	return String();
}

PackedVector3Array WebXRInterface::get_play_area() const {
	PackedVector3Array ret;

	float *points;
	int point_count = godot_webxr_get_bounds_geometry(&points);
	if (point_count > 0) {
		ret.resize(point_count);
		for (int i = 0; i < point_count; i++) {
			float *js_vector3 = points + (i * 3);
			ret.set(i, Vector3(js_vector3[0], js_vector3[1], js_vector3[2]));
		}
		free(points);
	}

	return ret;
}

float WebXRInterface::get_display_refresh_rate() const {
	return godot_webxr_get_frame_rate();
}

void WebXRInterface::set_display_refresh_rate(float p_refresh_rate) {
	godot_webxr_update_target_frame_rate(p_refresh_rate);
}

Vector<float> WebXRInterface::get_available_display_refresh_rates() const {
	Vector<float> ret;

	float *rates;
	int rate_count = godot_webxr_get_supported_frame_rates(&rates);
	if (rate_count > 0) {
		ret.resize(rate_count);
		for (int i = 0; i < rate_count; i++) {
			ret.write[i] = rates[i];
		}
		free(rates);
	}

	return ret;
}

Vector<XRInterface::EnvironmentBlendMode> WebXRInterface::get_supported_environment_blend_modes() {
	Vector<XRInterface::EnvironmentBlendMode> blend_modes;
	// The blend mode can't be changed, so return the current blend mode as the only supported one.
	blend_modes.push_back(environment_blend_mode);
	return blend_modes;
}

XRInterface::EnvironmentBlendMode WebXRInterface::get_environment_blend_mode() const {
	return environment_blend_mode;
}

bool WebXRInterface::set_environment_blend_mode(EnvironmentBlendMode p_new_environment_blend_mode) {
	if (environment_blend_mode == p_new_environment_blend_mode) {
		// Environment blend mode can't be changed, but we'll consider it a success to set it
		// to what it already is.
		return true;
	}
	return false;
}

void WebXRInterface::_set_environment_blend_mode(const String &p_blend_mode_string) {
	if (p_blend_mode_string == "opaque") {
		environment_blend_mode = XRInterface::XR_ENV_BLEND_MODE_OPAQUE;
	} else if (p_blend_mode_string == "additive") {
		environment_blend_mode = XRInterface::XR_ENV_BLEND_MODE_ADDITIVE;
	} else if (p_blend_mode_string == "alpha-blend") {
		environment_blend_mode = XRInterface::XR_ENV_BLEND_MODE_ALPHA_BLEND;
	} else {
		// Not all browsers can give us this information, so as a fallback,
		// we'll make some guesses about the blend mode.
		if (session_mode == "immersive-ar") {
			environment_blend_mode = XRInterface::XR_ENV_BLEND_MODE_ALPHA_BLEND;
		} else {
			environment_blend_mode = XRInterface::XR_ENV_BLEND_MODE_OPAQUE;
		}
	}
}

String WebXRInterface::get_name() const {
	return "WebXR";
}

uint32_t WebXRInterface::get_capabilities() const {
	return XRInterface::XR_STEREO | XRInterface::XR_MONO | XRInterface::XR_VR | XRInterface::XR_AR;
}

uint32_t WebXRInterface::get_view_count() {
	return godot_webxr_get_view_count();
}

bool WebXRInterface::is_initialized() const {
	return (initialized);
}

bool WebXRInterface::initialize() {
	ERR_FAIL_COND_V(!XRServer::is_initialized(), false);

	if (!initialized) {
		if (!godot_webxr_is_supported()) {
			ERR_PRINT("WebXR is unsupported by this web browser.");
			return false;
		}

		if (session_mode == "immersive-vr" && !GLES3::Config::multiview_supported) {
			ERR_PRINT("Stereo rendering in Godot requires multiview, but this web browser doesn't support it.");
			return false;
		}

		if (requested_reference_space_types.is_empty()) {
			ERR_PRINT("No reference spaces were requested.");
			return false;
		}

		enabled_features.clear();

		// We must create a tracker for our head.
		head_transform.basis = Basis();
		head_transform.origin = Vector3();
		head_tracker.instantiate();
		head_tracker->set_tracker_type(XRServer::TRACKER_HEAD);
		head_tracker->set_tracker_name("head");
		head_tracker->set_tracker_desc("Players head");
		XRServer::add_tracker(head_tracker);

		// Make this our primary interface.
		XRServer::set_primary_interface(this);

		// Clear render_targetsize to make sure it gets reset to the new size.
		// Clearing in uninitialize() doesn't work because a frame can still be
		// rendered after it's called, which will fill render_targetsize again.
		render_targetsize.width = 0;
		render_targetsize.height = 0;

		initialized = true;

		godot_webxr_initialize(
				session_mode.utf8().get_data(),
				required_features.utf8().get_data(),
				optional_features.utf8().get_data(),
				requested_reference_space_types.utf8().get_data(),
				&_emwebxr_on_session_started,
				&_emwebxr_on_session_ended,
				&_emwebxr_on_session_failed,
				&_emwebxr_on_input_event,
				&_emwebxr_on_simple_event);
	};

	return true;
}

void WebXRInterface::uninitialize() {
	if (initialized) {
		if (XRServer::is_initialized()) {
			if (head_tracker.is_valid()) {
				XRServer::remove_tracker(head_tracker);

				head_tracker.unref();
			}

			for (int i = 0; i < HAND_MAX; i++) {
				if (hand_trackers[i].is_valid()) {
					XRServer::remove_tracker(hand_trackers[i]);

					hand_trackers[i].unref();
				}
			}

			if (XRServer::get_primary_interface() == this) {
				// no longer our primary interface
				XRServer::set_primary_interface(nullptr);
			}
		}

		godot_webxr_uninitialize();

		if (GLES3::TextureStorage::is_initialized()) {
			for (KeyValue<unsigned int, RID> &E : texture_cache) {
				// Forcibly mark as not part of a render target so we can free it.
				GLES3::Texture *texture = GLES3::TextureStorage::get_texture(E.value);
				texture->is_render_target = false;

				GLES3::TextureStorage::texture_free(E.value);
			}
		}

		texture_cache.clear();
		reference_space_type.clear();
		enabled_features.clear();
		environment_blend_mode = XRInterface::XR_ENV_BLEND_MODE_OPAQUE;
		initialized = false;
	};
}

HashMap<String, String> WebXRInterface::get_system_info() {
	HashMap<String, String> dict;

	// TODO get actual information from WebXR to return here
	dict[String("XRRuntimeName")] = String("WebXR");
	dict[String("XRRuntimeVersion")] = String("");

	return dict;
}

Transform3D WebXRInterface::_js_matrix_to_transform(float *p_js_matrix) {
	Transform3D transform;

	transform.basis.rows[0].x = p_js_matrix[0];
	transform.basis.rows[1].x = p_js_matrix[1];
	transform.basis.rows[2].x = p_js_matrix[2];
	transform.basis.rows[0].y = p_js_matrix[4];
	transform.basis.rows[1].y = p_js_matrix[5];
	transform.basis.rows[2].y = p_js_matrix[6];
	transform.basis.rows[0].z = p_js_matrix[8];
	transform.basis.rows[1].z = p_js_matrix[9];
	transform.basis.rows[2].z = p_js_matrix[10];
	transform.origin.x = p_js_matrix[12];
	transform.origin.y = p_js_matrix[13];
	transform.origin.z = p_js_matrix[14];

	return transform;
}

Size2 WebXRInterface::get_render_target_size() {
	if (render_targetsize.width != 0 && render_targetsize.height != 0) {
		return render_targetsize;
	}

	int js_size[2];
	bool has_size = godot_webxr_get_render_target_size(js_size);

	if (!initialized || !has_size) {
		// As a temporary default (until WebXR is fully initialized), use the
		// window size.
		return DisplayServer::window_get_size();
	}

	render_targetsize.width = (float)js_size[0];
	render_targetsize.height = (float)js_size[1];

	return render_targetsize;
}

Transform3D WebXRInterface::get_camera_transform() {
	Transform3D camera_transform;

	ERR_FAIL_COND_V(!XRServer::is_initialized(), camera_transform);

	if (initialized) {
		double world_scale = XRServer::get_world_scale();

		Transform3D _head_transform = head_transform;
		_head_transform.origin *= world_scale;

		camera_transform = (XRServer::get_reference_frame()) * _head_transform;
	}

	return camera_transform;
}

Transform3D WebXRInterface::get_transform_for_view(uint32_t p_view, const Transform3D &p_cam_transform) {
	ERR_FAIL_COND_V(!XRServer::is_initialized(), p_cam_transform);
	ERR_FAIL_COND_V(!initialized, p_cam_transform);

	float js_matrix[16];
	bool has_transform = godot_webxr_get_transform_for_view(p_view, js_matrix);
	if (!has_transform) {
		return p_cam_transform;
	}

	Transform3D transform_for_view = _js_matrix_to_transform(js_matrix);

	double world_scale = XRServer::get_world_scale();
	transform_for_view.origin *= world_scale;

	return p_cam_transform * XRServer::get_reference_frame() * transform_for_view;
}

Projection WebXRInterface::get_projection_for_view(uint32_t p_view, double p_aspect, double p_z_near, double p_z_far) {
	Projection view;

	ERR_FAIL_COND_V(!initialized, view);

	float js_matrix[16];
	bool has_projection = godot_webxr_get_projection_for_view(p_view, js_matrix);
	if (!has_projection) {
		return view;
	}

	int k = 0;
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			view.columns[i][j] = js_matrix[k++];
		}
	}

	// Copied from godot_oculus_mobile's ovr_mobile_session.cpp
	view.columns[2][2] = -(p_z_far + p_z_near) / (p_z_far - p_z_near);
	view.columns[3][2] = -(2.0f * p_z_far * p_z_near) / (p_z_far - p_z_near);

	return view;
}

bool WebXRInterface::pre_draw_viewport(RID p_render_target) {
	if (!GLES3::TextureStorage::is_initialized()) {
		return false;
	}

	// Cache the resources so we don't have to get them from JS twice.
	color_texture = _get_color_texture();
	depth_texture = _get_depth_texture();

	// Per the WebXR spec, it returns "opaque textures" to us, which may be the
	// same WebGLTexture object (which would be the same GLuint in C++) but
	// represent a different underlying resource (probably the next texture in
	// the XR device's swap chain). In order to render to this texture, we need
	// to re-attach it to the FBO, otherwise we get an "incomplete FBO" error.
	//
	// See: https://immersive-web.github.io/layers/#xropaquetextures
	//
	// So, even if the color and depth textures have the same GLuint as the last
	// frame, we need to re-attach them again.
	GLES3::TextureStorage::render_target_set_reattach_textures(p_render_target, true);

	return true;
}

Vector<RenderingServerTypes::BlitToScreen> WebXRInterface::post_draw_viewport(RID p_render_target, const Rect2 &p_screen_rect) {
	Vector<RenderingServerTypes::BlitToScreen> blit_to_screen;

	if (!GLES3::TextureStorage::is_initialized()) {
		return blit_to_screen;
	}

	GLES3::TextureStorage::render_target_set_reattach_textures(p_render_target, false);

	return blit_to_screen;
}

RID WebXRInterface::_get_color_texture() {
	unsigned int texture_id = godot_webxr_get_color_texture();
	if (texture_id == 0) {
		return RID();
	}

	return _get_texture(texture_id);
}

RID WebXRInterface::_get_depth_texture() {
	unsigned int texture_id = godot_webxr_get_depth_texture();
	if (texture_id == 0) {
		return RID();
	}

	return _get_texture(texture_id);
}

RID WebXRInterface::_get_texture(unsigned int p_texture_id) {
	RBMap<unsigned int, RID>::Element *cache = texture_cache.find(p_texture_id);
	if (cache != nullptr) {
		return cache->get();
	}

	if (!GLES3::TextureStorage::is_initialized()) {
		return RID();
	}

	uint32_t view_count = godot_webxr_get_view_count();
	Size2 texture_size = get_render_target_size();

	RID texture = GLES3::TextureStorage::texture_create_from_native_handle(
			view_count == 1 ? RSE::TEXTURE_TYPE_2D : RSE::TEXTURE_TYPE_LAYERED,
			Image::FORMAT_RGBA8,
			p_texture_id,
			(int)texture_size.width,
			(int)texture_size.height,
			1,
			view_count);

	texture_cache.insert(p_texture_id, texture);

	return texture;
}

RID WebXRInterface::get_color_texture() {
	return color_texture;
}

RID WebXRInterface::get_depth_texture() {
	return depth_texture;
}

RID WebXRInterface::get_velocity_texture() {
	unsigned int texture_id = godot_webxr_get_velocity_texture();
	if (texture_id == 0) {
		return RID();
	}

	return _get_texture(texture_id);
}

void WebXRInterface::process() {
	if (initialized) {
		// Get the "head" position.
		float js_matrix[16];
		if (godot_webxr_get_transform_for_view(-1, js_matrix)) {
			head_transform = _js_matrix_to_transform(js_matrix);
		}
		if (head_tracker.is_valid()) {
			head_tracker->set_pose("default", head_transform, Vector3(), Vector3());
		}

		// Update all input sources.
		for (int i = 0; i < input_source_count; i++) {
			_update_input_source(i);
		}
	};
}

void WebXRInterface::_update_input_source(int p_input_source_id) {
	ERR_FAIL_COND(!XRServer::is_initialized());

	InputSource &input_source = input_sources[p_input_source_id];

	float target_pose[16];
	int tmp_target_ray_mode;
	int touch_index;
	int has_grip_pose;
	float grip_pose[16];
	int has_standard_mapping;
	int button_count;
	float buttons[10];
	int axes_count;
	float axes[10];
	int has_hand_data;
	float hand_joints[WEBXR_HAND_JOINT_MAX * 16];
	float hand_radii[WEBXR_HAND_JOINT_MAX];

	input_source.active = godot_webxr_update_input_source(
			p_input_source_id,
			target_pose,
			&tmp_target_ray_mode,
			&touch_index,
			&has_grip_pose,
			grip_pose,
			&has_standard_mapping,
			&button_count,
			buttons,
			&axes_count,
			axes,
			&has_hand_data,
			hand_joints,
			hand_radii);

	if (!input_source.active) {
		if (input_source.tracker.is_valid()) {
			XRServer::remove_tracker(input_source.tracker);
			input_source.tracker.unref();
		}
		return;
	}

	input_source.target_ray_mode = (WebXRInterface::TargetRayMode)tmp_target_ray_mode;
	input_source.touch_index = touch_index;

	Ref<XRTracker> &tracker = input_source.tracker;

	if (tracker.is_null()) {
		tracker.instantiate();
		tracker->set_tracker_type(XRServer::TRACKER_CONTROLLER);

		String tracker_name;
		if (input_source.target_ray_mode == WebXRInterface::TargetRayMode::TARGET_RAY_MODE_SCREEN) {
			tracker_name = touch_names[touch_index];
		} else {
			tracker_name = tracker_names[p_input_source_id];
		}

		// Input source id's 0 and 1 are always the left and right hands.
		if (p_input_source_id < 2) {
			tracker->set_tracker_name(tracker_name);
			tracker->set_tracker_desc(p_input_source_id == 0 ? "Left hand controller" : "Right hand controller");
			tracker->set_tracker_hand(p_input_source_id == 0 ? XRTracker::TRACKER_HAND_LEFT : XRTracker::TRACKER_HAND_RIGHT);
		} else {
			tracker->set_tracker_name(tracker_name);
			tracker->set_tracker_desc(tracker_name);
		}
		XRServer::add_tracker(tracker);
	}

	Transform3D aim_transform = _js_matrix_to_transform(target_pose);
	tracker->set_pose(SceneString(default_), aim_transform, Vector3(), Vector3());
	tracker->set_pose(String("aim"), aim_transform, Vector3(), Vector3());
	if (has_grip_pose) {
		tracker->set_pose(String("grip"), _js_matrix_to_transform(grip_pose), Vector3(), Vector3());
	}

	for (int i = 0; i < button_count; i++) {
		String button_name = has_standard_mapping ? standard_button_names[i] : unknown_button_names[i];
		String button_pressure_name = has_standard_mapping ? standard_button_pressure_names[i] : unknown_button_pressure_names[i];
		float value = buttons[i];
		bool state = value > 0.0;
		tracker->set_input(button_name, state);
		tracker->set_input(button_pressure_name, value);
	}

	for (int i = 0; i < axes_count; i++) {
		String axis_name = has_standard_mapping ? standard_axis_names[i] : unknown_axis_names[i];
		float value = axes[i];
		if (has_standard_mapping && (i == 1 || i == 3)) {
			// Invert the Y-axis on thumbsticks and trackpads, in order to
			// match OpenXR and other XR platform SDKs.
			value = -value;
		}
		tracker->set_input(axis_name, value);
	}

	// Also create Vector2's for the thumbstick and trackpad when we have the
	// standard mapping.
	if (has_standard_mapping) {
		if (axes_count >= 2) {
			tracker->set_input(standard_vector_names[0], Vector2(axes[0], -axes[1]));
		}
		if (axes_count >= 4) {
			tracker->set_input(standard_vector_names[1], Vector2(axes[2], -axes[3]));
		}
	}

	if (input_source.target_ray_mode == WebXRInterface::TARGET_RAY_MODE_SCREEN) {
		if (touch_index < 5 && axes_count >= 2) {
			Vector2 joy_vector = Vector2(axes[0], axes[1]);
			Vector2 position = _get_screen_position_from_joy_vector(joy_vector);

			if (touches[touch_index].is_touching) {
				Vector2 delta = position - touches[touch_index].position;

				// If position has changed by at least 1 pixel, generate a drag event.
				if (std::abs(delta.x) >= 1.0 || std::abs(delta.y) >= 1.0) {
					InputEvent event = InputEvent::make_screen_drag();
					event.screen_drag.index = touch_index;
					event.screen_drag.position = position;
					event.screen_drag.relative = delta;
					event.screen_drag.screen_relative = delta;
					Input::parse_input_event(event);
				}
			}

			touches[touch_index].position = position;
		}
	}

	if (p_input_source_id < 2) {
		Ref<XRTracker> hand_tracker = hand_trackers[p_input_source_id];
		if (has_hand_data) {
			// Transform orientations to match Godot Humanoid skeleton.
			const Basis bone_adjustment(
					Vector3(-1.0, 0.0, 0.0),
					Vector3(0.0, 0.0, -1.0),
					Vector3(0.0, -1.0, 0.0));

			if (unlikely(hand_tracker.is_null())) {
				hand_tracker.instantiate();
				hand_tracker->set_tracker_type(XRServer::TRACKER_HAND);
				hand_tracker->set_tracker_hand(p_input_source_id == 0 ? XRTracker::TRACKER_HAND_LEFT : XRTracker::TRACKER_HAND_RIGHT);
				hand_tracker->set_tracker_name(p_input_source_id == 0 ? "/user/hand_tracker/left" : "/user/hand_tracker/right");

				// These flags always apply, since WebXR doesn't give us enough insight to be more fine grained.
				uint32_t joint_flags(XRTracker::HAND_JOINT_FLAG_POSITION_VALID | XRTracker::HAND_JOINT_FLAG_ORIENTATION_VALID | XRTracker::HAND_JOINT_FLAG_POSITION_TRACKED | XRTracker::HAND_JOINT_FLAG_ORIENTATION_TRACKED);
				for (int godot_joint = 0; godot_joint < XRTracker::HAND_JOINT_MAX; godot_joint++) {
					hand_tracker->set_hand_joint_flags((XRTracker::HandJoint)godot_joint, joint_flags);
				}

				hand_trackers[p_input_source_id] = hand_tracker;
				XRServer::add_tracker(hand_tracker);
			}

			hand_tracker->set_has_tracking_data(true);
			for (int webxr_joint = 0; webxr_joint < WEBXR_HAND_JOINT_MAX; webxr_joint++) {
				XRTracker::HandJoint godot_joint = (XRTracker::HandJoint)(webxr_joint + 1);

				Transform3D joint_transform = _js_matrix_to_transform(hand_joints + (16 * webxr_joint));
				joint_transform.basis *= bone_adjustment;
				hand_tracker->set_hand_joint_transform(godot_joint, joint_transform);

				hand_tracker->set_hand_joint_radius(godot_joint, hand_radii[webxr_joint]);
			}

			// WebXR doesn't have a palm joint, so we calculate it by finding the middle of the middle finger metacarpal bone.
			{
				// Start by getting the middle finger metacarpal joint.
				// Note: 10 is the WebXR middle finger metacarpal joint.
				Transform3D palm_transform = _js_matrix_to_transform(hand_joints + (10 * 16));
				palm_transform.basis *= bone_adjustment;

				// Get the middle finger phalanx position.
				// Note: 11 is the WebXR middle finger phalanx proximal joint and 12 is the origin offset.
				const float *phalanx_pos = hand_joints + (11 * 16) + 12;
				Vector3 phalanx(phalanx_pos[0], phalanx_pos[1], phalanx_pos[2]);

				// Offset the palm half-way towards the phalanx joint.
				palm_transform.origin = (palm_transform.origin + phalanx) / 2.0;

				// Set the palm joint and the pose.
				hand_tracker->set_hand_joint_transform(XRTracker::HAND_JOINT_PALM, palm_transform);
				hand_tracker->set_pose("default", palm_transform, Vector3(), Vector3());
			}

		} else if (hand_tracker.is_valid()) {
			hand_tracker->set_has_tracking_data(false);
			hand_tracker->invalidate_pose("default");
		}
	}
}

void WebXRInterface::_on_input_event(int p_event_type, int p_input_source_id) {
	// Get the latest data for this input source. For transient input sources,
	// we may not have any data at all yet!
	_update_input_source(p_input_source_id);

	if (p_event_type == WEBXR_INPUT_EVENT_SELECTSTART || p_event_type == WEBXR_INPUT_EVENT_SELECTEND) {
		const InputSource &input_source = input_sources[p_input_source_id];
		if (input_source.target_ray_mode == WebXRInterface::TARGET_RAY_MODE_SCREEN) {
			int touch_index = input_source.touch_index;
			if (touch_index >= 0 && touch_index < 5) {
				touches[touch_index].is_touching = (p_event_type == WEBXR_INPUT_EVENT_SELECTSTART);

				InputEvent event = InputEvent::make_screen_touch();
				event.touch.index = touch_index;
				event.touch.position = touches[touch_index].position;
				event.pressed = (p_event_type == WEBXR_INPUT_EVENT_SELECTSTART);

				Input::parse_input_event(event);
			}
		}
	}

	(void)p_event_type;
	(void)p_input_source_id;
}

Vector2 WebXRInterface::_get_screen_position_from_joy_vector(const Vector2 &p_joy_vector) {
	if (!SceneTree::is_initialized()) {
		return Vector2();
	}

	Window *viewport = SceneTree::get_root();
	if (!viewport) {
		return Vector2();
	}

	Vector2 position_percentage((p_joy_vector.x + 1.0f) / 2.0f, ((p_joy_vector.y) + 1.0f) / 2.0f);
	Vector2 position = (Size2)viewport->get_size() * position_percentage;

	return position;
}

WebXRInterface::WebXRInterface() {
	initialized = false;
	session_mode = "inline";
	requested_reference_space_types = "local";
}

WebXRInterface::~WebXRInterface() {
	// and make sure we cleanup if we haven't already
	if (initialized) {
		uninitialize();
	};
}

#else // !WEB_ENABLED

void WebXRInterface::is_session_supported(const String &p_session_mode) {}
void WebXRInterface::set_session_mode(const String &p_session_mode) {}
String WebXRInterface::get_session_mode() const { return String(); }
void WebXRInterface::set_required_features(const String &p_required_features) {}
String WebXRInterface::get_required_features() const { return String(); }
void WebXRInterface::set_optional_features(const String &p_optional_features) {}
String WebXRInterface::get_optional_features() const { return String(); }
void WebXRInterface::set_requested_reference_space_types(const String &p_requested_reference_space_types) {}
String WebXRInterface::get_requested_reference_space_types() const { return String(); }
String WebXRInterface::get_reference_space_type() const { return String(); }
String WebXRInterface::get_enabled_features() const { return String(); }
bool WebXRInterface::is_input_source_active(int p_input_source_id) const { return false; }
Ref<XRTracker> WebXRInterface::get_input_source_tracker(int p_input_source_id) const { return Ref<XRTracker>(); }
WebXRInterface::TargetRayMode WebXRInterface::get_input_source_target_ray_mode(int p_input_source_id) const { return TARGET_RAY_MODE_UNKNOWN; }
String WebXRInterface::get_visibility_state() const { return String(); }
float WebXRInterface::get_display_refresh_rate() const { return 0.0f; }
void WebXRInterface::set_display_refresh_rate(float p_refresh_rate) {}
Vector<float> WebXRInterface::get_available_display_refresh_rates() const { return Vector<float>(); }
Vector<XRInterface::EnvironmentBlendMode> WebXRInterface::get_supported_environment_blend_modes() { return Vector<XRInterface::EnvironmentBlendMode>(); }
HashMap<String, String> WebXRInterface::get_system_info() { return HashMap<String, String>(); }

String WebXRInterface::get_name() const { return "WebXR"; }
uint32_t WebXRInterface::get_capabilities() const { return 0; }
bool WebXRInterface::is_initialized() const { return false; }
bool WebXRInterface::initialize() { return false; }
void WebXRInterface::uninitialize() {}

#endif // WEB_ENABLED

#ifdef WEB_ENABLED
#endif


#ifdef WEB_ENABLED
static Ref<WebXRInterface> webxr;
#endif

void initialize_webxr_module(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
#ifdef WEB_ENABLED
	if (XRServer::is_initialized()) {
		webxr.instantiate();
		XRServer::add_interface(webxr);
	}
#endif
}

void uninitialize_webxr_module(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

#ifdef WEB_ENABLED
	if (webxr.is_valid()) {
		// uninitialize our interface if it is initialized
		if (webxr->is_initialized()) {
			webxr->uninitialize();
		}

		// unregister our interface from the XR server
		if (XRServer::is_initialized()) {
			XRServer::remove_interface(webxr);
		}

		// and release
		webxr.unref();
	}
#endif
}
#endif // defined(MODULE_WEBXR_ENABLED)
