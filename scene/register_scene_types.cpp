/**************************************************************************/
/*  register_scene_types.cpp                                              */
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

#include "register_scene_types.h"

#include "scene/resources/compressed_texture.h"
#include "scene/resources/packed_scene.h"
#include "scene/resources/shader.h"
#include "scene/scene_string_names.h"

static Ref<ResourceFormatSaverText> resource_saver_text;
static Ref<ResourceFormatLoaderText> resource_loader_text;

static Ref<ResourceFormatLoaderCompressedTexture> resource_loader_compressed_texture;

static Ref<ResourceFormatSaverShader> resource_saver_shader;
static Ref<ResourceFormatLoaderShader> resource_loader_shader;

void unregister_scene_types() { SceneStrings::finalize(); }

void register_scene_types() { SceneStrings::initialize(); }

void register_scene_singletons() {}

// ==========================================================================
// Merged from scene/property_list_helper.cpp
// ==========================================================================

#include "property_list_helper.h"

HashMap<String, Vector<PropertyListHelper*>> PropertyListHelper::base_helpers; // static

void PropertyListHelper::clear_base_helpers()
{ // static
	for (KeyValue<String, Vector<PropertyListHelper*>>& E : base_helpers) {
		for (PropertyListHelper* helper : E.value) {
			helper->clear();
		}
	}
	base_helpers.clear();
}

void PropertyListHelper::register_base_helper(
	const String& p_class_name, PropertyListHelper* p_helper)
{ // static
	base_helpers[p_class_name].push_back(p_helper);
}

Vector<PropertyListHelper*> PropertyListHelper::get_helpers_for_class(
	const String& p_class_name)
{
	if (base_helpers.has(p_class_name)) {
		return base_helpers[p_class_name];
	}
	else {
		return Vector<PropertyListHelper*>();
	}
}

void PropertyListHelper::set_prefix(const String& p_prefix) { prefix = p_prefix; }

#ifdef TOOLS_ENABLED

#endif

bool PropertyListHelper::property_can_revert(const String& p_property) const
{
	return is_property_valid(p_property);
}

bool PropertyListHelper::is_property_valid(const String& p_property, int* r_index) const
{
	return false;
}

void PropertyListHelper::clear() {}

// ==========================================================================
// Merged from scene/property_utils.cpp
// ==========================================================================

#include "core/config/engine.h"
#include "core/io/resource_loader.h"
#include "core/templates/local_vector.h"
#include "property_utils.h"
#include "scene/resources/packed_scene.h"

// Like SceneState::PackState, but using a raw pointer to avoid the cost of
// updating the reference count during the internal work of the functions below
namespace
{
struct _FastPackState
{
	SceneState* state = nullptr;
	int node = -1;
};
} // namespace

static bool _collect_inheritance_chain(const Ref<SceneState>& p_state, const NodePath& p_path,
	LocalVector<_FastPackState>& r_states_stack)
{
	bool found = false;

	LocalVector<_FastPackState> inheritance_states;

	Ref<SceneState> state = p_state;
	while (state.is_valid()) {
		int node = state->find_node_by_path(p_path);
		if (node >= 0) {
			// This one has state for this node
			inheritance_states.push_back({state.ptr(), node});
			found = true;
		}
		state = state->get_base_scene_state();
	}

	if (inheritance_states.size() > 0) {
		for (int i = inheritance_states.size() - 1; i >= 0; --i) {
			r_states_stack.push_back(inheritance_states[i]);
		}
	}

	return found;
}

// ============================================================================
// Merged from scene/audio/audio_stream_player.cpp
// ============================================================================

#include "scene/animation/animation_mixer.h"
#include "servers/audio/audio_stream.h"
#include "servers/display/accessibility_server.h"

void AudioStreamPlayer::set_stream(Ref<AudioStream> p_stream) { internal->set_stream(p_stream); }

Ref<AudioStream> AudioStreamPlayer::get_stream() const { return internal->stream; }

void AudioStreamPlayer::set_volume_db(float p_volume)
{
	ERR_FAIL_COND_MSG(Math::is_nan(p_volume), "Volume can't be set to NaN.");
	internal->volume_db = p_volume;

	Vector<AudioFrame> volume_vector = _get_volume_vector();
	for (Ref<AudioStreamPlayback>& playback : internal->stream_playbacks) {
		AudioServer::set_playback_all_bus_volumes_linear(playback, volume_vector);
	}
}

float AudioStreamPlayer::get_volume_db() const { return internal->volume_db; }

void AudioStreamPlayer::set_volume_linear(float p_volume)
{
	set_volume_db(Math::linear_to_db(p_volume));
}

float AudioStreamPlayer::get_volume_linear() const { return Math::db_to_linear(get_volume_db()); }

void AudioStreamPlayer::set_pitch_scale(float p_pitch_scale)
{
	internal->set_pitch_scale(p_pitch_scale);
}

float AudioStreamPlayer::get_pitch_scale() const { return internal->pitch_scale; }

void AudioStreamPlayer::set_max_polyphony(int p_max_polyphony)
{
	internal->set_max_polyphony(p_max_polyphony);
}

int AudioStreamPlayer::get_max_polyphony() const { return internal->max_polyphony; }

void AudioStreamPlayer::play(float p_from_pos)
{
	Ref<AudioStreamPlayback> stream_playback = internal->play_basic();
	if (stream_playback.is_null()) {
		return;
	}
	AudioServer::start_playback_stream(
		stream_playback, internal->bus, _get_volume_vector(), p_from_pos, internal->pitch_scale);
	internal->ensure_playback_limit();

	// Sample handling.
	if (stream_playback->get_is_sample() && stream_playback->get_sample_playback().is_valid()) {
		Ref<AudioSamplePlayback> sample_playback = stream_playback->get_sample_playback();
		sample_playback->offset = p_from_pos;
		sample_playback->volume_vector = _get_volume_vector();
		sample_playback->bus = get_bus();

		AudioServer::start_sample_playback(sample_playback);
	}
}

void AudioStreamPlayer::seek(float p_seconds) { internal->seek(p_seconds); }

void AudioStreamPlayer::stop() { internal->stop_basic(); }

bool AudioStreamPlayer::is_playing() const { return internal->is_playing(); }

float AudioStreamPlayer::get_playback_position() { return internal->get_playback_position(); }

void AudioStreamPlayer::set_bus(const String& p_bus)
{
	internal->bus = p_bus;
	for (const Ref<AudioStreamPlayback>& playback : internal->stream_playbacks) {
		AudioServer::set_playback_bus_exclusive(
			playback, p_bus, _get_volume_vector());
	}
}

String AudioStreamPlayer::get_bus() const { return internal->get_bus(); }

void AudioStreamPlayer::set_autoplay(bool p_enable) { internal->autoplay = p_enable; }

bool AudioStreamPlayer::is_autoplay_enabled() const { return internal->autoplay; }

void AudioStreamPlayer::set_mix_target(MixTarget p_target) { mix_target = p_target; }

AudioStreamPlayer::MixTarget AudioStreamPlayer::get_mix_target() const { return mix_target; }

void AudioStreamPlayer::_set_playing(bool p_enable) { internal->set_playing(p_enable); }

void AudioStreamPlayer::set_stream_paused(bool p_pause) { internal->set_stream_paused(p_pause); }

bool AudioStreamPlayer::get_stream_paused() const { return internal->get_stream_paused(); }

Vector<AudioFrame> AudioStreamPlayer::_get_volume_vector()
{
	Vector<AudioFrame> volume_vector;
	// We need at most four stereo pairs (for 7.1 systems).
	volume_vector.resize(4);

	// Initialize the volume vector to zero.
	for (AudioFrame& channel_volume_db : volume_vector) {
		channel_volume_db = AudioFrame(0, 0);
	}

	float volume_linear = Math::db_to_linear(internal->volume_db);

	// Set the volume vector up according to the speaker mode and mix target.
	// TODO do we need to scale the volume down when we output to more channels?
	if (AudioServer::get_speaker_mode() == AudioServer::SPEAKER_MODE_STEREO) {
		volume_vector.write[0] = AudioFrame(volume_linear, volume_linear);
	}
	else {
		switch (mix_target) {
		case MIX_TARGET_STEREO: {
			volume_vector.write[0] = AudioFrame(volume_linear, volume_linear);
		} break;
		case MIX_TARGET_SURROUND: {
			// TODO Make sure this is right.
			volume_vector.write[0] = AudioFrame(volume_linear, volume_linear);
			volume_vector.write[1] = AudioFrame(volume_linear, /* LFE= */ 1.0f);
			volume_vector.write[2] = AudioFrame(volume_linear, volume_linear);
			volume_vector.write[3] = AudioFrame(volume_linear, volume_linear);
		} break;
		case MIX_TARGET_CENTER: {
			// TODO Make sure this is right.
			volume_vector.write[1] = AudioFrame(volume_linear, /* LFE= */ 1.0f);
		} break;
		}
	}
	return volume_vector;
}

bool AudioStreamPlayer::has_stream_playback() { return internal->has_stream_playback(); }

Ref<AudioStreamPlayback> AudioStreamPlayer::get_stream_playback()
{
	return internal->get_stream_playback();
}

AudioServer::PlaybackType AudioStreamPlayer::get_playback_type() const
{
	return internal->get_playback_type();
}

void AudioStreamPlayer::set_playback_type(AudioServer::PlaybackType p_playback_type)
{
	internal->set_playback_type(p_playback_type);
}

AudioStreamPlayer::~AudioStreamPlayer() { memdelete(internal); }

void AudioStreamPlayerInternal::set_stream(Ref<AudioStream>) {}

void AudioStreamPlayerInternal::set_playing(bool) {}

void AudioStreamPlayerInternal::seek(float) {}

Ref<AudioStreamPlayback> AudioStreamPlayerInternal::play_basic()
{
	Ref<AudioStreamPlayback> asp = memnew(AudioStreamPlayback);
	return asp;
}

Error AudioServer::set_input_device_active(bool) { return OK; }

int AudioDriver::_get_configured_mix_rate() { return 0; }

void AudioStreamPlayerInternal::process() {}

void AudioStreamPlayerInternal::notification(int) {}

// ==========================================================================
// Merged from scene/audio/audio_stream_player_internal.cpp
// ==========================================================================

#include "core/config/engine.h"
#include "scene/main/node.h"
#include "scene/main/scene_tree.h"
#include "servers/audio/audio_stream.h"

void AudioStreamPlayerInternal::_set_process(bool p_enabled)
{
	if (physical) {
		node->set_physics_process_internal(p_enabled);
	}
	else {
		node->set_process_internal(p_enabled);
	}
}

void AudioStreamPlayerInternal::ensure_playback_limit()
{
	while (stream_playbacks.size() > max_polyphony) {
		AudioServer::stop_playback_stream(stream_playbacks[0]);
		stream_playbacks.remove_at(0);
	}
}

void AudioStreamPlayerInternal::set_stream_paused(bool p_pause)
{
	// TODO this does not have perfect recall, fix that maybe? If there are zero playbacks
	// registered with the AudioServer, this bool isn't persisted.
	for (Ref<AudioStreamPlayback>& playback : stream_playbacks) {
		AudioServer::set_playback_paused(playback, p_pause);
		if (_is_sample() && playback->get_sample_playback().is_valid()) {
			AudioServer::set_sample_playback_pause(
				playback->get_sample_playback(), p_pause);
		}
	}
}

bool AudioStreamPlayerInternal::get_stream_paused() const
{
	// There's currently no way to pause some playback streams but not others. Check the first and
	// don't bother looking at the rest.
	if (!stream_playbacks.is_empty()) {
		return AudioServer::is_playback_paused(stream_playbacks[0]);
	}
	return false;
}

void AudioStreamPlayerInternal::stop_basic()
{
	for (Ref<AudioStreamPlayback>& playback : stream_playbacks) {
		AudioServer::stop_playback_stream(playback);
	}
	stream_playbacks.clear();

	active.clear();
	_set_process(false);
}

bool AudioStreamPlayerInternal::is_playing() const
{
	for (const Ref<AudioStreamPlayback>& playback : stream_playbacks) {
		if (AudioServer::is_playback_active(playback)) {
			return true;
		}
	}
	return false;
}

float AudioStreamPlayerInternal::get_playback_position()
{
	// Return the playback position of the most recently started playback stream.
	if (!stream_playbacks.is_empty()) {
		return AudioServer::get_playback_position(
			stream_playbacks[stream_playbacks.size() - 1]);
	}
	return 0;
}

bool AudioStreamPlayerInternal::is_active() const { return active.is_set(); }

void AudioStreamPlayerInternal::set_pitch_scale(float p_pitch_scale)
{
	ERR_FAIL_COND(p_pitch_scale <= 0.0);
	pitch_scale = p_pitch_scale;

	for (Ref<AudioStreamPlayback>& playback : stream_playbacks) {
		AudioServer::set_playback_pitch_scale(playback, pitch_scale);
	}
}

void AudioStreamPlayerInternal::set_max_polyphony(int p_max_polyphony)
{
	if (p_max_polyphony > 0) {
		max_polyphony = p_max_polyphony;
	}
}

bool AudioStreamPlayerInternal::has_stream_playback() { return !stream_playbacks.is_empty(); }

Ref<AudioStreamPlayback> AudioStreamPlayerInternal::get_stream_playback()
{
	ERR_FAIL_COND_V_MSG(stream_playbacks.is_empty(), Ref<AudioStreamPlayback>(),
		"Player is inactive. Call play() before requesting get_stream_playback().");
	return stream_playbacks[stream_playbacks.size() - 1];
}

void AudioStreamPlayerInternal::set_playback_type(AudioServer::PlaybackType p_playback_type)
{
	playback_type = p_playback_type;
}

AudioServer::PlaybackType AudioStreamPlayerInternal::get_playback_type() const
{
	return playback_type;
}

String AudioStreamPlayerInternal::get_bus() const
{
	const String bus_name = bus;
	for (int i = 0; i < AudioServer::get_bus_count(); i++) {
		if (AudioServer::get_bus_name(i) == bus_name) {
			return bus;
		}
	}
	return SceneString(Master);
}

// ============================================================================
// Merged from scene/theme/theme_db.cpp
// ============================================================================

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/io/resource_loader.h"
#include "scene/gui/control.h"
#include "scene/main/node.h"
#include "scene/main/window.h"
#include "scene/resources/font.h"
#include "scene/resources/style_box.h"
#include "scene/resources/texture.h"
// --- Inlined from scene/theme/default_theme.h ---
#include "scene/resources/theme.h"

void fill_default_theme(Ref<Theme> &theme, const Ref<Font> &default_font, const Ref<Font> &bold_font, const Ref<Font> &bold_italics_font, const Ref<Font> &italics_font, Ref<Texture2D> &default_icon, Ref<StyleBox> &default_style, float p_scale);
void make_default_theme(float p_scale, Ref<Font> p_font, TextServer::SubpixelPositioning p_font_subpixel = TextServer::SUBPIXEL_POSITIONING_AUTO, TextServer::Hinting p_font_hinting = TextServer::HINTING_LIGHT, TextServer::FontAntialiasing p_font_antialiased = TextServer::FONT_ANTIALIASING_GRAY, bool p_font_msdf = false, bool p_font_generate_mipmaps = false);

// --- End scene/theme/default_theme.h ---
#include "servers/rendering/renderer.h"
#include "servers/text/text_server.h"
#include "scene/theme/theme_db.h"

void ThemeDB::initialize()
{
	if (!data) {
		data = memnew(Data);
	}
}

void ThemeDB::finalize()
{
	if (!data) {
		return;
	}
	_finalize_theme_contexts();
	data->default_theme.unref();
	data->project_theme.unref();
	data->fallback_font.unref();
	data->fallback_icon.unref();
	data->fallback_stylebox.unref();
	memdelete(data);
	data = nullptr;
}

void ThemeDB::initialize_theme()
{
	if (!data) {
		return;
	}
	if (Renderer::data) {
		make_default_theme(1.0, Ref<Font>());
	}
	_init_default_theme_context();
}

void ThemeDB::initialize_theme_noproject()
{
	if (!data) {
		return;
	}
	if (Renderer::data) {
		make_default_theme(1.0, Ref<Font>());
	}
	_init_default_theme_context();
}

void ThemeDB::finalize_theme()
{
	if (!data) {
		return;
	}
	if (!Renderer::data) {
		WARN_PRINT("Finalizing theme when there is no Renderer is an error; check the order "
				   "of operations.");
	}
	_finalize_theme_contexts();
	data->default_theme.unref();
	data->fallback_font.unref();
	data->fallback_icon.unref();
	data->fallback_stylebox.unref();
}

// Global Theme resources.

void ThemeDB::set_default_theme(const Ref<Theme>& p_default)
{
	if (data) {
		data->default_theme = p_default;
	}
}

Ref<Theme> ThemeDB::get_default_theme() { return data ? data->default_theme : Ref<Theme>(); }

void ThemeDB::set_project_theme(const Ref<Theme>& p_project_default)
{
	if (data) {
		data->project_theme = p_project_default;
	}
}

Ref<Theme> ThemeDB::get_project_theme() { return data ? data->project_theme : Ref<Theme>(); }

void ThemeDB::set_fallback_base_scale(float p_base_scale)
{
	if (data) {
		data->fallback_base_scale = p_base_scale;
	}
}

float ThemeDB::get_fallback_base_scale() { return data ? data->fallback_base_scale : 1.0f; }

void ThemeDB::set_fallback_font(const Ref<Font>& p_font)
{
	if (data) {
		data->fallback_font = p_font;
	}
}

Ref<Font> ThemeDB::get_fallback_font() { return data ? data->fallback_font : Ref<Font>(); }

void ThemeDB::set_fallback_font_size(int p_font_size)
{
	if (data) {
		data->fallback_font_size = p_font_size;
	}
}

int ThemeDB::get_fallback_font_size() { return data ? data->fallback_font_size : 16; }

void ThemeDB::set_fallback_icon(const Ref<Texture2D>& p_icon)
{
	if (data) {
		data->fallback_icon = p_icon;
	}
}

Ref<Texture2D> ThemeDB::get_fallback_icon()
{
	return data ? data->fallback_icon : Ref<Texture2D>();
}

void ThemeDB::set_fallback_stylebox(const Ref<StyleBox>& p_stylebox)
{
	if (data) {
		data->fallback_stylebox = p_stylebox;
	}
}

Ref<StyleBox> ThemeDB::get_fallback_stylebox()
{
	return data ? data->fallback_stylebox : Ref<StyleBox>();
}

void ThemeDB::get_native_type_dependencies(const String& p_base_type, Vector<String>& r_result)
{
	(void)r_result;
	if (p_base_type == String()) {
		return;
	}
}

void ThemeDB::_init_default_theme_context()
{
	if (!data) {
		return;
	}
	if (data->default_theme_context) {
		memdelete(data->default_theme_context);
	}
	data->default_theme_context = memnew(ThemeContext);
	Vector<Ref<Theme>> themes;

#ifdef TOOLS_ENABLED
	if (!Engine::is_editor_hint()) {
		themes.push_back(data->project_theme);
	}
#else
	themes.push_back(data->project_theme);
#endif
	themes.push_back(data->default_theme);
	data->default_theme_context->set_themes(themes);
}

void ThemeDB::_finalize_theme_contexts()
{
	if (!data) {
		return;
	}
	if (data->default_theme_context) {
		memdelete(data->default_theme_context);
		data->default_theme_context = nullptr;
	}
	while (data->theme_contexts.size()) {
		HashMap<Node*, ThemeContext*>::Iterator E = data->theme_contexts.begin();
		memdelete(E->value);
		data->theme_contexts.remove(E);
	}
}

ThemeContext* ThemeDB::create_theme_context(Node* p_node, Vector<Ref<Theme>>& p_themes)
{
	(void)p_node;
	(void)p_themes;
	return nullptr;
}

void ThemeDB::destroy_theme_context(Node* p_node)
{
	if (!data || !data->theme_contexts.has(p_node)) {
		return;
	}
	memdelete(data->theme_contexts[p_node]);
	data->theme_contexts.erase(p_node);
}

ThemeContext* ThemeDB::get_theme_context(Node* p_node)
{
	if (!data || !data->theme_contexts.has(p_node)) {
		return nullptr;
	}
	return data->theme_contexts[p_node];
}

ThemeContext* ThemeDB::get_default_theme_context()
{
	return data ? data->default_theme_context : nullptr;
}

ThemeContext* ThemeDB::get_nearest_theme_context(Node* p_for_node)
{
	if (!data || !p_for_node) {
		return nullptr;
	}
	ERR_FAIL_COND_V(!p_for_node->is_inside_tree(), nullptr);
	Node* parent_node = p_for_node->get_parent();
	while (parent_node) {
		if (data->theme_contexts.has(parent_node)) {
			return data->theme_contexts[parent_node];
		}
		parent_node = parent_node->get_parent();
	}
	return nullptr;
}

// Theme item binding.

void ThemeDB::bind_class_item(Theme::DataType p_data_type, const String& p_class_name,
	const String& p_prop_name, const String& p_item_name, ThemeItemSetter p_setter)
{
	if (!data) {
		return;
	}
	ERR_FAIL_COND_MSG(data->theme_item_binds[p_class_name].has(p_prop_name),
		vformat("Failed to bind theme item '%s' in class '%s': already bound", p_prop_name,
			p_class_name));
	ThemeItemBind bind;
	bind.data_type = p_data_type;
	bind.class_name = p_class_name;
	bind.item_name = p_item_name;
	bind.setter = p_setter;
	data->theme_item_binds[p_class_name][p_prop_name] = bind;
	data->theme_item_binds_list[p_class_name].push_back(bind);
}

void ThemeDB::bind_class_external_item(Theme::DataType p_data_type, const String& p_class_name,
	const String& p_prop_name, const String& p_item_name, const String& p_type_name,
	ThemeItemSetter p_setter)
{
	if (!data) {
		return;
	}
	ERR_FAIL_COND_MSG(data->theme_item_binds[p_class_name].has(p_prop_name),
		vformat("Failed to bind theme item '%s' in class '%s': already bound", p_prop_name,
			p_class_name));
	ThemeItemBind bind;
	bind.data_type = p_data_type;
	bind.class_name = p_class_name;
	bind.item_name = p_item_name;
	bind.type_name = p_type_name;
	bind.external = true;
	bind.setter = p_setter;
	data->theme_item_binds[p_class_name][p_prop_name] = bind;
	data->theme_item_binds_list[p_class_name].push_back(bind);
}

void ThemeDB::update_class_instance_items(Node* p_instance) { (void)p_instance; }

void ThemeDB::get_class_items(const String& p_class_name, List<ThemeItemBind>* r_list,
	bool p_include_inherited, Theme::DataType p_filter_type)
{
	if (!data) {
		return;
	}
	List<String> class_hierarchy;
	String class_name = p_class_name;
	while (class_name != String()) {
		class_hierarchy.push_front(class_name);
	}
	HashSet<String> inherited_props;
	for (const String& theme_type : class_hierarchy) {
		HashMap<String, List<ThemeItemBind>>::Iterator E =
			data->theme_item_binds_list.find(theme_type);
		if (E) {
			for (const ThemeItemBind& F : E->value) {
				if (p_filter_type != Theme::DATA_TYPE_MAX && F.data_type != p_filter_type) {
					continue;
				}
				if (inherited_props.has(F.item_name)) {
					continue;
				}
				if (F.external || F.class_name != p_class_name) {
					inherited_props.insert(F.item_name);
					if (!p_include_inherited) {
						continue;
					}
				}
				r_list->push_back(F);
			}
		}
	}
}

void ThemeDB::_sort_theme_items()
{
	if (!data) {
		return;
	}
	for (KeyValue<String, List<ThemeDB::ThemeItemBind>>& E : data->theme_item_binds_list) {
		E.value.sort_custom<ThemeItemBind::SortByType>();
	}
}

const Vector<Ref<Theme>> ThemeContext::get_themes() const { return themes; }

Ref<Theme> ThemeContext::get_fallback_theme() const
{
	if (themes.is_empty()) {
		return ThemeDB::get_default_theme();
	}
	return themes[themes.size() - 1];
}

// --- theme_owner.cpp ---

#include "scene/gui/control.h"
#include "scene/main/window.h"
#include "scene/theme/theme_db.h"

Node* ThemeOwner::_get_next_owner_node(Node* p_from_node) const
{
	Node n = Node();
	return &n;
}

Ref<Theme> ThemeOwner::_get_owner_node_theme(Node* p_owner_node) const
{
	Ref<Theme> t = memnew(Theme);
	return t;
}

ThemeContext* ThemeOwner::_get_active_owner_context() const
{
	if (owner_context) {
		return owner_context;
	}

	return ThemeDB::get_default_theme_context();
}

bool ThemeOwner::has_theme_item_in_types(
	Theme::DataType p_data_type, const String& p_name, const Vector<String>& p_theme_types)
{
	ERR_FAIL_COND_V_MSG(
		p_theme_types.is_empty(), false, "At least one theme type must be specified.");

	// First, look through each control or window node in the branch, until no valid parent can be
	// found. Only nodes with a theme resource attached are considered.
	Node* current_owner = owner_node;

	while (current_owner) {
		// For each theme resource check the theme types provided and see if p_name exists with any
		// of them.
		for (const String& E : p_theme_types) {
			Ref<Theme> owner_theme = _get_owner_node_theme(current_owner);

			if (owner_theme.is_valid() && owner_theme->has_theme_item(p_data_type, p_name, E)) {
				return true;
			}
		}

		current_owner = _get_next_owner_node(current_owner);
	}

	// Second, check global themes from the appropriate context.
	ThemeContext* global_context = _get_active_owner_context();
	for (const Ref<Theme>& theme : global_context->get_themes()) {
		if (theme.is_valid()) {
			for (const String& E : p_theme_types) {
				if (theme->has_theme_item(p_data_type, p_name, E)) {
					return true;
				}
			}
		}
	}

	// Finally, if no match exists, return false.
	return false;
}

float ThemeOwner::get_theme_default_base_scale()
{
	// First, look through each control or window node in the branch, until no valid parent can be
	// found. Only nodes with a theme resource attached are considered. For each theme resource see
	// if their assigned theme has the default value defined and valid.
	Node* current_owner = owner_node;

	while (current_owner) {
		Ref<Theme> owner_theme = _get_owner_node_theme(current_owner);

		if (owner_theme.is_valid() && owner_theme->has_default_base_scale()) {
			return owner_theme->get_default_base_scale();
		}

		current_owner = _get_next_owner_node(current_owner);
	}

	// Second, check global themes from the appropriate context.
	ThemeContext* global_context = _get_active_owner_context();
	for (const Ref<Theme>& theme : global_context->get_themes()) {
		if (theme.is_valid()) {
			if (theme->has_default_base_scale()) {
				return theme->get_default_base_scale();
			}
		}
	}

	// Finally, if no match exists, return the universal default.
	return ThemeDB::get_fallback_base_scale();
}

Ref<Font> ThemeOwner::get_theme_default_font()
{
	// First, look through each control or window node in the branch, until no valid parent can be
	// found. Only nodes with a theme resource attached are considered. For each theme resource see
	// if their assigned theme has the default value defined and valid.
	Node* current_owner = owner_node;

	while (current_owner) {
		Ref<Theme> owner_theme = _get_owner_node_theme(current_owner);

		if (owner_theme.is_valid() && owner_theme->has_default_font()) {
			return owner_theme->get_default_font();
		}

		current_owner = _get_next_owner_node(current_owner);
	}

	// Second, check global themes from the appropriate context.
	ThemeContext* global_context = _get_active_owner_context();
	for (const Ref<Theme>& theme : global_context->get_themes()) {
		if (theme.is_valid()) {
			if (theme->has_default_font()) {
				return theme->get_default_font();
			}
		}
	}

	// Finally, if no match exists, return the universal default.
	return ThemeDB::get_fallback_font();
}

int ThemeOwner::get_theme_default_font_size()
{
	// First, look through each control or window node in the branch, until no valid parent can be
	// found. Only nodes with a theme resource attached are considered. For each theme resource see
	// if their assigned theme has the default value defined and valid.
	Node* current_owner = owner_node;

	while (current_owner) {
		Ref<Theme> owner_theme = _get_owner_node_theme(current_owner);

		if (owner_theme.is_valid() && owner_theme->has_default_font_size()) {
			return owner_theme->get_default_font_size();
		}

		current_owner = _get_next_owner_node(current_owner);
	}

	// Second, check global themes from the appropriate context.
	ThemeContext* global_context = _get_active_owner_context();
	for (const Ref<Theme>& theme : global_context->get_themes()) {
		if (theme.is_valid()) {
			if (theme->has_default_font_size()) {
				return theme->get_default_font_size();
			}
		}
	}

	// Finally, if no match exists, return the universal default.
	return ThemeDB::get_fallback_font_size();
}



void ThemeOwner::set_owner_node(Node*) {}

void ThemeOwner::set_owner_context(ThemeContext*, bool) {}

void ThemeOwner::propagate_theme_changed(Node*, Node*, bool, bool) {}

void ThemeContext::set_themes(Vector<Ref<Theme>>&) {}

// --- default_theme.cpp ---

#include "core/io/image.h"

#include "scene/resources/font.h"
#include "scene/resources/gradient_texture.h"
#include "scene/resources/image_texture.h"
#include "scene/resources/style_box.h"
#include "scene/resources/theme.h"
#include "scene/scene_string_names.h"
#include "scene/theme/default_theme_icons.gen.h"
#include "scene/theme/theme_db.h"
#include "servers/text/text_server.h"

#ifdef BROTLI_ENABLED
#include "scene/theme/default_font.gen.h"
#endif

static const int default_font_size = 16;

static float scale = 1.0;

static const int default_margin = 4;
static const int default_corner_radius = 3;

static Ref<StyleBox> make_flat_stylebox(Color p_color, float p_margin_left = default_margin,
	float p_margin_top = default_margin, float p_margin_right = default_margin,
	float p_margin_bottom = default_margin, int p_corner_radius = default_corner_radius,
	bool p_draw_center = true, int p_border_width = 0)
{
	Ref<StyleBox> style(memnew(StyleBox(StyleBox::STYLE_FLAT)));
	style->set_bg_color(p_color);
	style->set_content_margin_individual(Math::round(p_margin_left * scale),
		Math::round(p_margin_top * scale), Math::round(p_margin_right * scale),
		Math::round(p_margin_bottom * scale));

	style->set_corner_radius_all(Math::round(p_corner_radius * scale));
	style->set_anti_aliased(true);
	// Adjust level of detail based on the corners' effective sizes.
	style->set_corner_detail(MIN(Math::ceil(1.5 * p_corner_radius), 6) * scale);

	style->set_draw_center(p_draw_center);
	style->set_border_width_all(Math::round(p_border_width * scale));

	return style;
}

static Ref<StyleBox> sb_expand(
	Ref<StyleBox> p_sbox, float p_left, float p_top, float p_right, float p_bottom)
{
	p_sbox->set_expand_margin(SIDE_LEFT, Math::round(p_left * scale));
	p_sbox->set_expand_margin(SIDE_TOP, Math::round(p_top * scale));
	p_sbox->set_expand_margin(SIDE_RIGHT, Math::round(p_right * scale));
	p_sbox->set_expand_margin(SIDE_BOTTOM, Math::round(p_bottom * scale));
	return p_sbox;
}

static Ref<StyleBox> make_empty_stylebox(float p_margin_left = -1, float p_margin_top = -1,
	float p_margin_right = -1, float p_margin_bottom = -1)
{
	Ref<StyleBox> style(memnew(StyleBox(StyleBox::STYLE_EMPTY)));
	style->set_content_margin_individual(Math::round(p_margin_left * scale),
		Math::round(p_margin_top * scale), Math::round(p_margin_right * scale),
		Math::round(p_margin_bottom * scale));
	return style;
}

void make_default_theme(float p_scale, Ref<Font> p_font,
	TextServer::SubpixelPositioning p_font_subpixel, TextServer::Hinting p_font_hinting,
	TextServer::FontAntialiasing p_font_antialiasing, bool p_font_msdf,
	bool p_font_generate_mipmaps)
{
	Ref<Theme> t;
	t.instantiate();
	Ref<StyleBox> default_style;
	Ref<Texture2D> default_icon;
	Ref<Font> default_font;
	Ref<FontVariation> bold_font;
	Ref<FontVariation> bold_italics_font;
	Ref<FontVariation> italics_font;
	float default_scale = CLAMP(p_scale, 0.5, 8.0);
	if (p_font.is_valid()) {
		// Use the custom font defined in the Project Settings.
		default_font = p_font;
	}
	else {
		// Use the default DynamicFont (separate from the editor font).
		// The default DynamicFont is chosen to have a small file size since it's
		// embedded in both editor and export template binaries.
		Ref<FontFile> dynamic_font;
		dynamic_font.instantiate();
#ifdef BROTLI_ENABLED
		dynamic_font->set_data_ptr(_font_OpenSans_SemiBold, _font_OpenSans_SemiBold_size);
		dynamic_font->set_subpixel_positioning(p_font_subpixel);
		dynamic_font->set_hinting(p_font_hinting);
		dynamic_font->set_antialiasing(p_font_antialiasing);
		dynamic_font->set_multichannel_signed_distance_field(p_font_msdf);
		dynamic_font->set_generate_mipmaps(p_font_generate_mipmaps);
#endif
		default_font = dynamic_font;
	}
	if (default_font.is_valid()) {
		bold_font.instantiate();
		bold_font->set_base_font(default_font);
		bold_font->set_variation_embolden(1.2);
		bold_italics_font.instantiate();
		bold_italics_font->set_base_font(default_font);
		bold_italics_font->set_variation_embolden(1.2);
		bold_italics_font->set_variation_transform(Transform2D(1.0, 0.2, 0.0, 1.0, 0.0, 0.0));
		italics_font.instantiate();
		italics_font->set_base_font(default_font);
		italics_font->set_variation_transform(Transform2D(1.0, 0.2, 0.0, 1.0, 0.0, 0.0));
	}
	fill_default_theme(t, default_font, bold_font, bold_italics_font, italics_font, default_icon,
		default_style, default_scale);
	ThemeDB::set_default_theme(t);
	ThemeDB::set_fallback_base_scale(default_scale);
	ThemeDB::set_fallback_icon(default_icon);
	ThemeDB::set_fallback_stylebox(default_style);
	ThemeDB::set_fallback_font(default_font);
	ThemeDB::set_fallback_font_size(default_font_size * default_scale);
}

void fill_default_theme(Ref<Theme>& r_theme, const Ref<Font>& p_font, const Ref<Font>& p_bold_font,
	const Ref<Font>& p_bold_italics_font, const Ref<Font>& p_italics_font, Ref<Texture2D>& r_icons,
	Ref<StyleBox>& r_style, float p_scale)
{
}
