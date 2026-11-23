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

#include "main/main.h"

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


// --- Merged from scene/animation/animation_tree.cpp ---
// --- animation_tree.compat.inc ---
#ifndef DISABLE_DEPRECATED

#include "scene/animation/animation_mixer.h"

void AnimationTree::_set_process_callback_bind_compat_80813(
	AnimationTree::AnimationProcessCallback p_mode)
{
	set_callback_mode_process(
		static_cast<AnimationMixer::AnimationCallbackModeProcess>(static_cast<int>(p_mode)));
}

AnimationTree::AnimationProcessCallback
AnimationTree::_get_process_callback_bind_compat_80813() const
{
	return static_cast<AnimationProcessCallback>(static_cast<int>(get_callback_mode_process()));
}

Ref<AnimationNode> AnimationTree::_get_tree_root_bind_compat_80813() const
{
	return get_root_animation_node();
}

void AnimationTree::_bind_compatibility_methods() {}

#endif // DISABLE_DEPRECATED
#include "scene/animation/animation_mixer.h"
#include "core/config/engine.h"
#include "scene/animation/animation_mixer.h"
#include "scene/animation/animation_mixer.h"

thread_local AnimationNode::ProcessState* AnimationNode::tls_process_state = nullptr;
thread_local AnimationNodeInstance* AnimationNode::current_instance = nullptr;

bool AnimationNode::is_parameter_read_only(const String& p_parameter) const
{
	if (p_parameter == current_length || p_parameter == current_position ||
		p_parameter == current_delta) {
		return true;
	}

	return false;
}

void AnimationNode::blend_animation(ProcessState& p_process_state,
	AnimationNodeInstance& p_instance, const String& p_animation,
	AnimationMixer::PlaybackInfo& p_playback_info)
{
	p_playback_info.track_weights = &p_instance.track_weights;
	p_process_state.tree->make_animation_instance(p_animation, p_playback_info);
}

void AnimationNode::add_validation_error(const AnimationTree* p_tree, const String& p_path,
	const String& p_error, int p_input_index) const
{
	p_tree->_add_validation_error(p_path, p_error, p_input_index);
}

void AnimationNode::make_invalid(
	ProcessState& p_process_state, AnimationNodeInstance& p_instance, const String& p_reason)
{
	p_process_state.valid = false;

	// TODO:
	// Currently, if the AnimationTree stops due to an error as valid state at runtime, there is no
	// way for the user to be aware of the error. I assume we should output an error when there is a
	// change in p_process_state.valid between frames. Caching invalid_instance and checking the
	// difference every frame is accurate, but it is somewhat costly and therefore likely not
	// preferred. If we were to implement it, we should add an option in the project settings'
	// animation tab, like `enum PrintErrorMode: {NONE, LOOSE, STRICT}`.
	InvalidInstance& invalid_instance = p_process_state.invalid_instances[p_instance.path];
	invalid_instance.errors.push_back(p_reason);
}

AnimationNode::NodeTimeInfo AnimationNode::blend_input(ProcessState& p_process_state,
	AnimationNodeInstance& p_instance, int p_input,
	const AnimationMixer::PlaybackInfo& p_playback_info, FilterAction p_filter, bool p_sync,
	bool p_test_only)
{
	ERR_FAIL_INDEX_V(p_input, (int64_t)inputs.size(), NodeTimeInfo());

	AnimationNodeInstance* node_instance = nullptr;
	if (likely(p_instance.connection_instances.size() > 0)) {
		node_instance = p_instance.connection_instances[p_input];
	}
	if (unlikely(!node_instance)) {
		if (!p_test_only && p_instance.is_blended()) {
			make_invalid(p_process_state, p_instance,
				vformat(RTR("Nothing connected to input %d."), p_input));
		}
		return NodeTimeInfo();
	}

	real_t activity = 0.0;

	NodeTimeInfo nti = _blend_node(p_process_state, p_instance, *node_instance, p_playback_info,
		p_filter, p_sync, p_test_only, &activity);

#ifdef ENABLE_ACTIVITY_TRACKING
	LocalVector<AnimationNodeInstance::Activity>& input_activity = p_instance.input_activity;
	input_activity[p_input].last_pass = p_process_state.last_pass;
	input_activity[p_input].activity = activity;
#endif

	return nti;
}

AnimationNode::NodeTimeInfo AnimationNode::blend_node(ProcessState& p_process_state,
	AnimationNodeInstance& p_instance, AnimationNodeInstance* p_other,
	const AnimationMixer::PlaybackInfo& p_playback_info, FilterAction p_filter, bool p_sync,
	bool p_test_only)
{
	ERR_FAIL_NULL_V(p_other, NodeTimeInfo());
	return _blend_node(p_process_state, p_instance, *p_other, p_playback_info, p_filter, p_sync,
		p_test_only, nullptr);
}

AnimationNode::NodeTimeInfo AnimationNode::_blend_node(ProcessState& p_process_state,
	AnimationNodeInstance& p_instance, AnimationNodeInstance& p_other,
	AnimationMixer::PlaybackInfo p_playback_info, FilterAction p_filter, bool p_sync,
	bool p_test_only, real_t* r_activity)
{
	int blend_count = p_instance.track_weights.size();

	if ((int64_t)p_other.track_weights.size() != blend_count) {
		p_other.track_weights.resize(blend_count);
	}

	real_t* blendw = p_other.track_weights.ptr();
	const real_t* blendr = p_instance.track_weights.ptr();

	bool any_valid = false;

	if (has_filter() && is_filter_enabled() && p_filter != FILTER_IGNORE) {
		_update_filter_cache(p_process_state, p_instance);
		// All to zero by default.
		memset(blendw, 0, sizeof(real_t) * blend_count);

		for (const int idx : p_instance.filtered_track_indices_cache) {
			blendw[idx] = 1.0; // Filtered goes to one.
		}

		switch (p_filter) {
		case FILTER_IGNORE:
			break; // Will not happen anyway.
		case FILTER_PASS: {
			// Values filtered pass, the rest don't.
			for (int i = 0; i < blend_count; i++) {
				if (blendw[i] == 0) { // Not filtered, does not pass.
					continue;
				}

				blendw[i] = blendr[i] * p_playback_info.weight;
				if (!Math::is_zero_approx(blendw[i])) {
					any_valid = true;
				}
			}

		} break;
		case FILTER_STOP: {
			// Values filtered don't pass, the rest are blended.

			for (int i = 0; i < blend_count; i++) {
				if (blendw[i] > 0) { // Filtered, does not pass.
					continue;
				}

				blendw[i] = blendr[i] * p_playback_info.weight;
				if (!Math::is_zero_approx(blendw[i])) {
					any_valid = true;
				}
			}

		} break;
		case FILTER_BLEND: {
			// Filtered values are blended, the rest are passed without blending.

			for (int i = 0; i < blend_count; i++) {
				if (blendw[i] == 1.0) {
					blendw[i] = blendr[i] * p_playback_info.weight; // Filtered, blend.
				}
				else {
					blendw[i] = blendr[i]; // Not filtered, do not blend.
				}

				if (!Math::is_zero_approx(blendw[i])) {
					any_valid = true;
				}
			}

		} break;
		}
	}
	else {
		for (int i = 0; i < blend_count; i++) {
			// Regular blend.
			blendw[i] = blendr[i] * p_playback_info.weight;
			if (!Math::is_zero_approx(blendw[i])) {
				any_valid = true;
			}
		}
	}

	if (r_activity) {
		*r_activity = 0;
#ifdef ENABLE_ACTIVITY_TRACKING
		for (int i = 0; i < blend_count; i++) {
			*r_activity = MAX(*r_activity, Math::abs(blendw[i]));
		}
#endif
	}

	// This process, which depends on p_sync is needed to process sync correctly in the case of
	// that a synced AnimationNode exists under an un-synced AnimationNode.
	if (!p_playback_info.seeked && !p_sync && !any_valid) {
		p_playback_info.delta = 0.0;
	}
	p_other.blended = any_valid;
	return p_other.resource->_pre_process(p_process_state, p_other, p_playback_info, p_test_only);
}

void AnimationNode::get_child_nodes(LocalVector<ChildNode>* r_child_nodes) {}

String AnimationNode::get_caption() const
{
	if (endpoint_type == ENDPOINT_START) {
		return "Start";
	}
	if (endpoint_type == ENDPOINT_END) {
		return "End";
	}
	return "Node";
}

void AnimationNode::set_use_sync(bool p_sync) { sync = p_sync; }

bool AnimationNode::is_using_sync() const { return sync; }

void AnimationNode::remove_input(int p_index)
{
	ERR_FAIL_INDEX(p_index, (int64_t)inputs.size());
	inputs.remove_at(p_index);
	emit_changed();
}

bool AnimationNode::set_input_name(int p_input, const String& p_name)
{
	ERR_FAIL_INDEX_V(p_input, (int64_t)inputs.size(), false);
	ERR_FAIL_COND_V(p_name.contains_char('.') || p_name.contains_char('/'), false);
	inputs[p_input].name = p_name;
	emit_changed();
	return true;
}

String AnimationNode::get_input_name(int p_input) const
{
	ERR_FAIL_INDEX_V(p_input, (int64_t)inputs.size(), String());
	return inputs[p_input].name;
}

int AnimationNode::get_input_count() const { return inputs.size(); }

int AnimationNode::find_input(const String& p_name) const
{
	int idx = -1;
	for (int i = 0; i < (int64_t)inputs.size(); i++) {
		if (inputs[i].name == p_name) {
			idx = i;
			break;
		}
	}
	return idx;
}

AnimationNode::NodeTimeInfo AnimationNode::_process(ProcessState& p_process_state,
	AnimationNodeInstance& p_instance, const AnimationMixer::PlaybackInfo& p_playback_info,
	bool p_test_only)
{
	double r_ret = 0.0;
	NodeTimeInfo nti;
	nti.delta = r_ret;
	return nti;
}

void AnimationNode::set_filter_path(const NodePath& p_path, bool p_enable)
{
	if (p_enable) {
		(void)p_path.hash(); // Make sure the cache is valid.
		filter.insert(p_path);
	}
	else {
		filter.erase(p_path);
	}
	_mark_filters_dirty();
}

void AnimationNode::set_filter_enabled(bool p_enable)
{
	filter_enabled = p_enable;
	_mark_filters_dirty();
}

bool AnimationNode::is_filter_enabled() const { return filter_enabled; }

void AnimationNode::_mark_filters_dirty()
{
	filters_version++;
	if (unlikely(filters_version == 0)) {
		filters_version = 1;
	}
}

void AnimationNode::set_deletable(bool p_closable) { closable = p_closable; }

bool AnimationNode::is_deletable() const { return closable; }

bool AnimationNode::is_process_testing() const
{
	ERR_FAIL_NULL_V(tls_process_state, false);
	return tls_process_state->is_testing;
}

bool AnimationNode::is_path_filtered(const NodePath& p_path) const { return filter.has(p_path); }

void AnimationNode::_update_filter_cache(
	const ProcessState& p_process_state, const AnimationNodeInstance& p_instance)
{
	bool filters_dirty = p_instance.filters_version != filters_version;
	if (!p_process_state.track_map_updated && !filters_dirty) {
		return; // Cache is valid.
	}

	p_instance.filtered_track_indices_cache.clear();
	if (p_instance.filtered_track_indices_cache.size() < filter.size()) {
		p_instance.filtered_track_indices_cache.reserve(filter.size());
	}

	for (const NodePath& path : filter) {
		if (const int* p = p_process_state.track_map->getptr(path)) {
			p_instance.filtered_track_indices_cache.push_back(*p);
		}
	}
	p_instance.filters_version = filters_version;
}

Ref<AnimationNode> AnimationNode::find_node_by_path(const String& p_name) const
{
	Vector<String> split = p_name.split("/");
	Ref<AnimationNode> ret = const_cast<AnimationNode*>(this);
	for (int i = 0; i < split.size(); i++) {
		ret = ret->get_child_by_name(split[i]);
		if (ret.is_null()) {
			break;
		}
	}
	return ret;
}

void AnimationNode::blend_animation_ex(const String& p_animation, double p_time, double p_delta,
	bool p_seeked, bool p_is_external_seeking, real_t p_blend, Animation::LoopedFlag p_looped_flag)
{
	ERR_FAIL_NULL(tls_process_state);
	ERR_FAIL_NULL(current_instance);

	AnimationMixer::PlaybackInfo info;
	info.time = p_time;
	info.delta = p_delta;
	info.seeked = p_seeked;
	info.is_external_seeking = p_is_external_seeking;
	info.weight = p_blend;
	info.looped_flag = p_looped_flag;

	blend_animation(*tls_process_state, *current_instance, p_animation, info);
}

double AnimationNode::blend_node_ex(const String& p_sub_path, const Ref<AnimationNode>& p_node,
	double p_time, bool p_seek, bool p_is_external_seeking, real_t p_blend, FilterAction p_filter,
	bool p_sync, bool p_test_only)
{
	ERR_FAIL_NULL_V(tls_process_state, 0.0);
	ERR_FAIL_NULL_V(current_instance, 0.0);

	AnimationNodeInstance* other_instance =
		current_instance->get_child_instance_by_path_or_null(p_sub_path);
	ERR_FAIL_NULL_V_MSG(other_instance, 0.0,
		vformat("The sub-path '%s' does not exist under the current node instance.",
			String(p_sub_path)));

	AnimationMixer::PlaybackInfo info;
	info.time = p_time;
	info.seeked = p_seek;
	info.is_external_seeking = p_is_external_seeking;
	info.weight = p_blend;

	NodeTimeInfo nti = blend_node(
		*tls_process_state, *current_instance, other_instance, info, p_filter, p_sync, p_test_only);
	return nti.length - nti.position;
}

double AnimationNode::blend_input_ex(int p_input, double p_time, bool p_seek,
	bool p_is_external_seeking, real_t p_blend, FilterAction p_filter, bool p_sync,
	bool p_test_only)
{
	ERR_FAIL_NULL_V(tls_process_state, 0.0);
	ERR_FAIL_NULL_V(current_instance, 0.0);

	AnimationMixer::PlaybackInfo info;
	info.time = p_time;
	info.seeked = p_seek;
	info.is_external_seeking = p_is_external_seeking;
	info.weight = p_blend;

	NodeTimeInfo nti = blend_input(
		*tls_process_state, *current_instance, p_input, info, p_filter, p_sync, p_test_only);
	return nti.length - nti.position;
}

#ifdef TOOLS_ENABLED

#endif

AnimationNode::AnimationNode() {}

void AnimationTree::set_root_animation_node(const Ref<AnimationNode>& p_animation_node)
{
	root_animation_node = p_animation_node;
	validation_dirty = true;
	properties_dirty = true;
}

Ref<AnimationNode> AnimationTree::get_root_animation_node() const
{
	return root_animation_node;
}

bool AnimationTree::_blend_pre_process(
	double p_delta, int p_track_count, const AHashMap<NodePath, int>& p_track_map)
{
	_update_properties(); // If properties need updating, update them.

	if (root_animation_node.is_null()) {
		process_state = AnimationNode::ProcessState();
		return false; // Abort after _update_properties() and init process_state.
	}

	if (validation_dirty) {
		_update_connections();
		validation_dirty = false;
	}

	AnimationNodeInstance& instance =
		get_node_instance_by_path(SNAME(Animation::PARAMETERS_BASE_PATH.ascii().get_data()));

	{ // Setup.
		process_pass++;
		if (unlikely(process_pass == 0)) {
			process_pass = 1;
		}

		// Init process state.
		process_state = AnimationNode::ProcessState();
		process_state.tree = this;
		process_state.valid = true;
		process_state.invalid_instances.clear();
		process_state.last_pass = process_pass;
		process_state.track_map = &p_track_map;
		process_state.track_map_updated = track_map_version != last_track_map_version;
		process_state.is_testing = false;

		last_track_map_version = track_map_version;

		// Init node state for root AnimationNode.
		instance.track_weights.resize(p_track_count);
		real_t* src_blendsw = instance.track_weights.ptr();
		for (int i = 0; i < p_track_count; i++) {
			src_blendsw[i] = 1.0; // By default all go to 1 for the root input.
		}
		instance.blended = true;
		instance.path = SNAME(Animation::PARAMETERS_BASE_PATH.ascii().get_data());
	}

	// Process.
	{
		PlaybackInfo pi;
		pi.delta = p_delta;

		if (started) {
			started = false;
			// If started, seek.
			pi.seeked = true;
		}
		else {
			pi.seeked = false;
		}

		AnimationNode::tls_process_state = &process_state;
		root_animation_node->_pre_process(process_state, instance, pi, false);
		AnimationNode::tls_process_state = nullptr;
	}

	if (!process_state.valid) {
		return false; // State is not valid, abort process.
	}

	return true;
}

void AnimationTree::_set_active(bool p_active)
{
	_set_process(p_active);
	started = p_active;
}

void AnimationTree::set_advance_expression_base_node(const NodePath& p_path)
{
	advance_expression_base_node = p_path;
}

NodePath AnimationTree::get_advance_expression_base_node() const
{
	return advance_expression_base_node;
}

bool AnimationTree::is_state_invalid() const { return !process_state.valid; }

const AHashMap<String, AnimationNode::InvalidInstance>&
AnimationTree::get_invalid_instances() const
{
	return process_state.invalid_instances;
}

PackedStringArray AnimationTree::get_configuration_warnings() const
{
	PackedStringArray warnings = AnimationMixer::get_configuration_warnings();
	if (root_animation_node.is_null()) {
		warnings.push_back(RTR("No root AnimationNode for the graph is set."));
	}
	return warnings;
}

void AnimationTree::_add_validation_error(
	const String& p_path, const String& p_error, int p_input_index) const
{
	AnimationNode::InvalidInstance& invalid_instance = process_state.invalid_instances[p_path];

	if (p_input_index == -1) {
		if (invalid_instance.errors.find(p_error) == -1) {
			invalid_instance.errors.push_back(p_error);
		}
	}
	else {
		for (const AnimationNode::InvalidInstance::InputError& E : invalid_instance.input_errors) {
			if (E.index == p_input_index) {
				return;
			}
		}
		invalid_instance.input_errors.push_back({p_input_index, p_error});
	}
}

void AnimationTree::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_ENTER_TREE: {
		_setup_animation_player();
		if (active) {
			_set_process(true);
		}
	} break;
	}
}

NodePath AnimationTree::get_animation_player() const { return animation_player; }

#ifdef ENABLE_ACTIVITY_TRACKING
real_t AnimationTree::get_connection_activity(const String& p_path, int p_connection) const
{
	const AnimationNodeInstance* a = get_node_instance_by_path_or_null(p_path);
	if (!a) {
		return 0;
	}

	const LocalVector<AnimationNodeInstance::Activity>& activity = a->input_activity;
	if (p_connection < 0 || p_connection >= (int64_t)activity.size() ||
		activity[p_connection].last_pass != process_pass) {
		return 0;
	}

	return activity[p_connection].activity;
}
#endif

#ifdef TOOLS_ENABLED
String AnimationTree::get_editor_error_message() const
{
	if (!is_active()) {
		return TTR("The AnimationTree is inactive.\nActivate it in the inspector to enable "
				   "playback; check node warnings if activation fails.");
	}
	else if (!is_enabled()) {
		return TTR("The AnimationTree node (or one of its ancestors) has its process mode set to "
				   "Disabled.\nChange the process mode in the inspector to allow playback.");
	}

	return "";
}
#endif

AnimationTree::AnimationTree()
{
	deterministic = true;
	callback_mode_discrete = ANIMATION_CALLBACK_MODE_DISCRETE_FORCE_CONTINUOUS;
}

AnimationTree::~AnimationTree() {}

bool AnimationNode::has_filter() const { return false; }

Ref<AnimationNode> AnimationNode::get_child_by_name(const String& p_name) const
{
	return Ref<AnimationNode>();
}

void AnimationNode::_tree_changed() {}

void AnimationNode::_add_node(const Ref<AnimationNode>& p_node) {}

void AnimationNode::_remove_node(const Ref<AnimationNode>& p_node) {}

bool AnimationNode::add_input(const String& p_name) { return true; }

uint32_t AnimationTree::_get_libraries_property_usage() const { return 0; }

void AnimationTree::_update_properties() const {}

void AnimationTree::_update_connections() {}

void AnimationTree::_setup_animation_player() {}

// --- AnimationNode base methods (moved from animation_node_extension.cpp) ---

AnimationNode::NodeTimeInfo AnimationNode::_pre_process(
	AnimationNode::ProcessState&, AnimationNodeInstance&, AnimationMixer::PlaybackInfo const&, bool)
{
	return AnimationNode::NodeTimeInfo();
}

void AnimationNode::get_argument_options(
	String const&, int, List<String, DefaultAllocator>*) const
{
}

// ==========================================================================
// Merged from scene/animation/animation_blend_space.cpp
// ==========================================================================
// --- Inlined from scene/animation/animation_blend_space.h ---
#include "scene/animation/animation_mixer.h"

class AnimationNodeBlendSpace final : public AnimationNode
{
	const String ERR_NO_BLEND_POINT = "No blend points exist, so blending cannot take place.";
	const String ERR_NO_TRIANGLE = "No triangles exist, so blending cannot take place.";
	const String ERR_INVALID_POINT =
		"Cyclic sync modes require that all blend points in BlendSpace use non-nested Animation "
		"nodes with a finite, immutable length.";

public:
	enum Dimension
	{
		DIMENSION_1D,
		DIMENSION_2D,
	};

	enum BlendMode
	{
		BLEND_MODE_INTERPOLATED,
		BLEND_MODE_DISCRETE,
		BLEND_MODE_DISCRETE_CARRY,
	};

	enum SyncMode
	{
		SYNC_MODE_NONE,			   // Inactive animations are frozen (not advanced).
		SYNC_MODE_INDEPENDENT,	   // Inactive animations advance with weight=0 (previous "sync"
								   // behavior).
		SYNC_MODE_CYCLIC_MUTABLE,  // Time-scaled with blend-weight-dependent cycle length
								   // (self-normalizing).
		SYNC_MODE_CYCLIC_CONSTANT, // Time-scaled to complete one cycle in cyclic_length seconds.
	};

private:
	enum
	{
		MAX_BLEND_POINTS = 64
	};

	struct BlendPoint
	{
		String name;
		Ref<AnimationNode> node;
		Vector2 position;

		void reset()
		{
			name = String();
			node = Ref<AnimationNode>();
			position = Vector2();
		}
	};

	struct BlendTriangle
	{
		int points[3] = {};
	};

	Dimension dimension = DIMENSION_1D;
	BlendPoint blend_points[MAX_BLEND_POINTS];
	int blend_points_used = 0;

	Vector<BlendTriangle> triangles;

	String blend_position = "blend_position";
	String closest = "closest";
	Vector2 max_space = Vector2(1, 1);
	Vector2 min_space = Vector2(-1, -1);
	Vector2 snap = Vector2(0.1f, 0.1f);
	String value_label = "value";
	String x_label = "x";
	String y_label = "y";
	BlendMode blend_mode = BLEND_MODE_INTERPOLATED;

	bool auto_triangles = true;
	bool triangles_dirty = false;

	SyncMode sync_mode = SYNC_MODE_NONE;
	double cyclic_length = 0.0;
	double inverted_cycle_length = 0.0; // Cached 1/cyclic_length.
	LocalVector<double> cached_lengths;
	bool lengths_dirty = true;
	bool is_contain_invalid_point = false;

	void _set_triangles(const Vector<int>& p_triangles);
	Vector<int> _get_triangles() const;

#ifndef DISABLE_DEPRECATED
	void _retry_set_triangles(const Vector<int>& p_triangles);
	void _add_blend_point_bind_compat_110369(
		const Ref<AnimationNode>& p_node, float p_position, int p_at_index = -1);
	void _add_blend_point_bind_compat_110369(
		const Ref<AnimationNode>& p_node, const Vector2& p_position, int p_at_index = -1);
	static void _bind_compatibility_methods();
#endif

	void _blend_triangle(
		const Vector2& p_pos, const LocalVector<Vector2>& p_points, LocalVector<float>& r_weights);
	void _update_triangles();
	void _queue_auto_triangles();
	void _check_can_sync();

protected:
	virtual void _tree_changed() override;

public:
	Dimension get_dimension() const { return dimension; }

	virtual void validate_node(
		const AnimationTree* p_tree, const String& p_path) const override;
	virtual void get_child_nodes(LocalVector<ChildNode>* r_child_nodes) override;

	void add_blend_point(const Ref<AnimationNode>& p_node, float p_position,
		int p_at_index = -1, const String& p_name = String());
	void add_blend_point(const Ref<AnimationNode>& p_node, const Vector2& p_position,
		int p_at_index = -1, const String& p_name = String());

	void set_blend_point_position(int p_point, float p_position);
	void set_blend_point_position(int p_point, const Vector2& p_position);
	Vector2 get_blend_point_position(int p_point) const;
	float get_blend_point_position_1d(int p_point) const;

	void set_blend_point_node(int p_point, const Ref<AnimationNode>& p_node);
	Ref<AnimationNode> get_blend_point_node(int p_point) const;
	void set_blend_point_name(int p_point, const String& p_name);
	const String& get_blend_point_name(int p_point) const;
	int find_blend_point_by_name(const String& p_name) const;
	void remove_blend_point(int p_point);
	int get_blend_point_count() const;
	void reorder_blend_point(int p_from_index, int p_to_index);

	// 2D triangle methods
	bool has_triangle(int p_x, int p_y, int p_z) const;
	void add_triangle(int p_x, int p_y, int p_z, int p_at_index = -1);
	int get_triangle_point(int p_triangle, int p_point);
	void remove_triangle(int p_triangle);
	int get_triangle_count() const;
	Vector2 get_closest_point(const Vector2& p_point);
	void set_auto_triangles(bool p_enable);
	bool get_auto_triangles() const;

	// Space & snap (1D and 2D)
	void set_min_space(float p_min);
	void set_min_space(const Vector2& p_min);
	Vector2 get_min_space() const;
	float get_min_space_1d() const;

	void set_max_space(float p_max);
	void set_max_space(const Vector2& p_max);
	Vector2 get_max_space() const;
	float get_max_space_1d() const;

	void set_snap(float p_snap);
	void set_snap(const Vector2& p_snap);
	Vector2 get_snap() const;
	float get_snap_1d() const;

	// Labels
	void set_value_label(const String& p_label);
	String get_value_label() const;
	void set_x_label(const String& p_label);
	String get_x_label() const;
	void set_y_label(const String& p_label);
	String get_y_label() const;

	// Blend & sync modes
	void set_blend_mode(BlendMode p_blend_mode);
	BlendMode get_blend_mode() const;

	void set_cyclic_length(double p_length);
	double get_cyclic_length() const;

#ifndef DISABLE_DEPRECATED
	virtual void set_use_sync(bool p_sync) override;
	virtual bool is_using_sync() const override;
#endif // DISABLE_DEPRECATED

	void set_sync_mode(SyncMode p_sync_mode);
	SyncMode get_sync_mode() const;

	virtual NodeTimeInfo _process(ProcessState& p_process_state, AnimationNodeInstance& p_instance,
		const AnimationMixer::PlaybackInfo& p_playback_info, bool p_test_only = false) override;
	virtual String get_caption() const override;
	virtual Ref<AnimationNode> get_child_by_name(const String& p_name) const override;

	explicit AnimationNodeBlendSpace(Dimension p_dimension = DIMENSION_1D) : dimension(p_dimension)
	{
		root_node = true;
	}
};

// --- End scene/animation/animation_blend_space.h ---
#include "core/math/geometry_2d.h"
#include "scene/animation/animation_mixer.h"

#ifndef DISABLE_DEPRECATED
void AnimationNodeBlendSpace::_add_blend_point_bind_compat_110369(
	const Ref<AnimationNode>& p_node, float p_position, int p_at_index)
{
	int n = p_at_index == -1 ? blend_points_used : p_at_index;
	while (find_blend_point_by_name(itos(n)) != -1) {
		n++;
	}
	add_blend_point(p_node, p_position, p_at_index, String(itos(n)));
}

void AnimationNodeBlendSpace::_add_blend_point_bind_compat_110369(
	const Ref<AnimationNode>& p_node, const Vector2& p_position, int p_at_index)
{
	int n = p_at_index == -1 ? blend_points_used : p_at_index;
	while (find_blend_point_by_name(itos(n)) != -1) {
		n++;
	}
	add_blend_point(p_node, p_position, p_at_index, String(itos(n)));
}

void AnimationNodeBlendSpace::_bind_compatibility_methods() {}
#endif // DISABLE_DEPRECATED

Ref<AnimationNode> AnimationNodeBlendSpace::get_child_by_name(const String& p_name) const
{
	int point_index = find_blend_point_by_name(p_name);
	if (point_index != -1) {
		return get_blend_point_node(point_index);
	}
	return Ref<AnimationNode>();
}

void AnimationNodeBlendSpace::_tree_changed()
{
	AnimationNode::_tree_changed();
	_check_can_sync();
}

void AnimationNodeBlendSpace::validate_node(
	const AnimationTree* p_tree, const String& p_path) const
{
	AnimationNode::validate_node(p_tree, p_path);

	if (dimension == DIMENSION_1D) {
		if (get_blend_point_count() == 0) {
			add_validation_error(p_tree, p_path, RTR(ERR_NO_BLEND_POINT));
		}
	}
	else {
		const_cast<AnimationNodeBlendSpace*>(this)->_update_triangles();
		if (get_triangle_count() == 0) {
			add_validation_error(p_tree, p_path, RTR(ERR_NO_TRIANGLE));
		}
	}

	const_cast<AnimationNodeBlendSpace*>(this)->_check_can_sync();
	if (is_contain_invalid_point) {
		add_validation_error(p_tree, p_path, RTR(ERR_INVALID_POINT));
	}
}

void AnimationNodeBlendSpace::get_child_nodes(LocalVector<ChildNode>* r_child_nodes)
{
	for (int i = 0; i < blend_points_used; i++) {
		ChildNode cn;
		cn.name = blend_points[i].name;
		cn.node = blend_points[i].node;
		r_child_nodes->push_back(cn);
	}
}

void AnimationNodeBlendSpace::add_blend_point(const Ref<AnimationNode>& p_node,
	float p_position, int p_at_index, const String& p_name)
{
	add_blend_point(p_node, Vector2(p_position, 0.0f), p_at_index, p_name);
}

void AnimationNodeBlendSpace::add_blend_point(const Ref<AnimationNode>& p_node,
	const Vector2& p_position, int p_at_index, const String& p_name)
{
	ERR_FAIL_COND(blend_points_used >= MAX_BLEND_POINTS);
	ERR_FAIL_COND(p_node.is_null());
	ERR_FAIL_COND(p_at_index < -1 || p_at_index > blend_points_used);
#ifndef DISABLE_DEPRECATED
	if (p_name == String()) {
		_add_blend_point_bind_compat_110369(p_node, p_position, p_at_index);
		WARN_PRINT_ED("AnimationNodeBlendSpace::add_blend_point: No name provided, using safe "
					  "index as reference. In the future, empty names will be deprecated, so "
					  "explicitly passing a name is recommended.");
		return;
	}
#else
	ERR_FAIL_COND(p_name == String());
#endif

	if (p_at_index == -1 || p_at_index == blend_points_used) {
		p_at_index = blend_points_used;
	}
	else {
		for (int i = blend_points_used; i > p_at_index; i--) {
			blend_points[i] = blend_points[i - 1];
		}
		if (dimension == DIMENSION_2D) {
			for (int i = 0; i < triangles.size(); i++) {
				for (int j = 0; j < 3; j++) {
					if (triangles[i].points[j] >= p_at_index) {
						triangles.write[i].points[j]++;
					}
				}
			}
		}
	}

	blend_points[p_at_index].node = p_node;
	blend_points[p_at_index].position = p_position;
	blend_points[p_at_index].name = p_name;

	_add_node(blend_points[p_at_index].node);
	blend_points_used++;

	lengths_dirty = true;
	if (dimension == DIMENSION_2D) {
		_queue_auto_triangles();
	}

	_tree_changed();
}

void AnimationNodeBlendSpace::set_blend_point_position(int p_point, float p_position)
{
	set_blend_point_position(p_point, Vector2(p_position, 0.0f));
}

void AnimationNodeBlendSpace::set_blend_point_position(int p_point, const Vector2& p_position)
{
	ERR_FAIL_INDEX(p_point, blend_points_used);
	blend_points[p_point].position = p_position;
	if (dimension == DIMENSION_2D) {
		_queue_auto_triangles();
	}
}

Vector2 AnimationNodeBlendSpace::get_blend_point_position(int p_point) const
{
	ERR_FAIL_INDEX_V(p_point, MAX_BLEND_POINTS, Vector2());
	return blend_points[p_point].position;
}

float AnimationNodeBlendSpace::get_blend_point_position_1d(int p_point) const
{
	ERR_FAIL_INDEX_V(p_point, MAX_BLEND_POINTS, 0.0f);
	return blend_points[p_point].position.x;
}

void AnimationNodeBlendSpace::set_blend_point_node(
	int p_point, const Ref<AnimationNode>& p_node)
{
	ERR_FAIL_INDEX(p_point, blend_points_used);
	ERR_FAIL_COND(p_node.is_null());

	if (blend_points[p_point].node.is_valid()) {
		_remove_node(blend_points[p_point].node);
	}
	blend_points[p_point].node = p_node;
	_add_node(blend_points[p_point].node);

	lengths_dirty = true;
	_tree_changed();
}

Ref<AnimationNode> AnimationNodeBlendSpace::get_blend_point_node(int p_point) const
{
	ERR_FAIL_INDEX_V(p_point, MAX_BLEND_POINTS, Ref<AnimationNode>());
	return blend_points[p_point].node;
}

void AnimationNodeBlendSpace::set_blend_point_name(int p_point, const String& p_name)
{
	ERR_FAIL_INDEX(p_point, blend_points_used);
	ERR_FAIL_COND(p_name.is_empty());
	blend_points[p_point].name = p_name;
}

const String& AnimationNodeBlendSpace::get_blend_point_name(int p_point) const
{
	const static String empty = String();
	ERR_FAIL_INDEX_V(p_point, blend_points_used, empty);
	return blend_points[p_point].name;
}

int AnimationNodeBlendSpace::find_blend_point_by_name(const String& p_name) const
{
	for (int i = 0; i < blend_points_used; i++) {
		if (blend_points[i].name == p_name) {
			return i;
		}
	}
	return -1;
}

void AnimationNodeBlendSpace::remove_blend_point(int p_point)
{
	ERR_FAIL_INDEX(p_point, blend_points_used);
	if (blend_points[p_point].node.is_valid()) {
		_remove_node(blend_points[p_point].node);
	}
	for (int i = p_point; i < blend_points_used - 1; i++) {
		blend_points[i] = blend_points[i + 1];
	}
	blend_points[blend_points_used - 1].reset();
	blend_points_used--;

	if (dimension == DIMENSION_2D) {
		for (int i = triangles.size() - 1; i >= 0; i--) {
			if (triangles[i].points[0] == p_point || triangles[i].points[1] == p_point ||
				triangles[i].points[2] == p_point) {
				triangles.remove_at(i);
			}
			else {
				for (int j = 0; j < 3; j++) {
					if (triangles[i].points[j] > p_point) {
						triangles.write[i].points[j]--;
					}
				}
			}
		}
		_queue_auto_triangles();
	}

	lengths_dirty = true;
	_tree_changed();
}

int AnimationNodeBlendSpace::get_blend_point_count() const { return blend_points_used; }

void AnimationNodeBlendSpace::reorder_blend_point(int p_from_index, int p_to_index)
{
	ERR_FAIL_INDEX(p_from_index, blend_points_used);
	ERR_FAIL_INDEX(p_to_index, blend_points_used);
	if (p_from_index == p_to_index) {
		return;
	}
	SWAP(blend_points[p_from_index], blend_points[p_to_index]);
	if (dimension == DIMENSION_2D) {
		for (int i = 0; i < triangles.size(); i++) {
			for (int j = 0; j < 3; j++) {
				if (triangles[i].points[j] == p_from_index) {
					triangles.write[i].points[j] = p_to_index;
				}
				else if (triangles[i].points[j] == p_to_index) {
					triangles.write[i].points[j] = p_from_index;
				}
			}
		}
	}
	lengths_dirty = true;
	_tree_changed();
}

bool AnimationNodeBlendSpace::has_triangle(int p_x, int p_y, int p_z) const
{
	ERR_FAIL_INDEX_V(p_x, blend_points_used, false);
	ERR_FAIL_INDEX_V(p_y, blend_points_used, false);
	ERR_FAIL_INDEX_V(p_z, blend_points_used, false);

	BlendTriangle t;
	t.points[0] = p_x;
	t.points[1] = p_y;
	t.points[2] = p_z;

	SortArray<int> sort;
	sort.sort(t.points, 3);

	for (int i = 0; i < triangles.size(); i++) {
		bool all_equal = true;
		for (int j = 0; j < 3; j++) {
			if (triangles[i].points[j] != t.points[j]) {
				all_equal = false;
				break;
			}
		}
		if (all_equal) {
			return true;
		}
	}

	return false;
}

void AnimationNodeBlendSpace::add_triangle(int p_x, int p_y, int p_z, int p_at_index)
{
	ERR_FAIL_INDEX(p_x, blend_points_used);
	ERR_FAIL_INDEX(p_y, blend_points_used);
	ERR_FAIL_INDEX(p_z, blend_points_used);

	_update_triangles();

	BlendTriangle t;
	t.points[0] = p_x;
	t.points[1] = p_y;
	t.points[2] = p_z;

	SortArray<int> sort;
	sort.sort(t.points, 3);

	for (int i = 0; i < triangles.size(); i++) {
		bool all_equal = true;
		for (int j = 0; j < 3; j++) {
			if (triangles[i].points[j] != t.points[j]) {
				all_equal = false;
				break;
			}
		}
		ERR_FAIL_COND(all_equal);
	}

	if (p_at_index == -1 || p_at_index == triangles.size()) {
		triangles.push_back(t);
	}
	else {
		triangles.insert(p_at_index, t);
	}
}

int AnimationNodeBlendSpace::get_triangle_point(int p_triangle, int p_point)
{
	_update_triangles();

	ERR_FAIL_INDEX_V(p_point, 3, -1);
	ERR_FAIL_INDEX_V(p_triangle, triangles.size(), -1);
	return triangles[p_triangle].points[p_point];
}

void AnimationNodeBlendSpace::remove_triangle(int p_triangle)
{
	ERR_FAIL_INDEX(p_triangle, triangles.size());
	triangles.remove_at(p_triangle);
}

int AnimationNodeBlendSpace::get_triangle_count() const { return triangles.size(); }

void AnimationNodeBlendSpace::set_min_space(float p_min)
{
	min_space.x = p_min;
	if (min_space.x >= max_space.x) {
		min_space.x = max_space.x - 1;
	}
}

void AnimationNodeBlendSpace::set_min_space(const Vector2& p_min)
{
	min_space = p_min;
	if (min_space.x >= max_space.x) {
		min_space.x = max_space.x - 1;
	}
	if (min_space.y >= max_space.y) {
		min_space.y = max_space.y - 1;
	}
}

Vector2 AnimationNodeBlendSpace::get_min_space() const { return min_space; }

float AnimationNodeBlendSpace::get_min_space_1d() const { return min_space.x; }

void AnimationNodeBlendSpace::set_max_space(float p_max)
{
	max_space.x = p_max;
	if (max_space.x <= min_space.x) {
		max_space.x = min_space.x + 1;
	}
}

void AnimationNodeBlendSpace::set_max_space(const Vector2& p_max)
{
	max_space = p_max;
	if (max_space.x <= min_space.x) {
		max_space.x = min_space.x + 1;
	}
	if (max_space.y <= min_space.y) {
		max_space.y = min_space.y + 1;
	}
}

Vector2 AnimationNodeBlendSpace::get_max_space() const { return max_space; }

float AnimationNodeBlendSpace::get_max_space_1d() const { return max_space.x; }

void AnimationNodeBlendSpace::set_snap(float p_snap) { snap.x = p_snap; }

void AnimationNodeBlendSpace::set_snap(const Vector2& p_snap) { snap = p_snap; }

Vector2 AnimationNodeBlendSpace::get_snap() const { return snap; }

float AnimationNodeBlendSpace::get_snap_1d() const { return snap.x; }

void AnimationNodeBlendSpace::set_value_label(const String& p_label) { value_label = p_label; }

String AnimationNodeBlendSpace::get_value_label() const { return value_label; }

void AnimationNodeBlendSpace::set_x_label(const String& p_label) { x_label = p_label; }

String AnimationNodeBlendSpace::get_x_label() const { return x_label; }

void AnimationNodeBlendSpace::set_y_label(const String& p_label) { y_label = p_label; }

String AnimationNodeBlendSpace::get_y_label() const { return y_label; }

void AnimationNodeBlendSpace::_set_triangles(const Vector<int>& p_triangles)
{
	triangles.clear();
	if (p_triangles.size() % 3 != 0) {
		return;
	}
	for (int i = 0; i < p_triangles.size(); i += 3) {
		add_triangle(p_triangles[i + 0], p_triangles[i + 1], p_triangles[i + 2]);
	}
}

#ifndef DISABLE_DEPRECATED
void AnimationNodeBlendSpace::_retry_set_triangles(const Vector<int>& p_triangles)
{
	for (int i = 0; i < p_triangles.size(); i += 3) {
		int x = p_triangles[i + 0];
		int y = p_triangles[i + 1];
		int z = p_triangles[i + 2];
		if (x >= blend_points_used || y >= blend_points_used || z >= blend_points_used ||
			has_triangle(x, y, z)) {
			continue;
		}
		add_triangle(x, y, z);
	}
}
#endif // DISABLE_DEPRECATED

Vector<int> AnimationNodeBlendSpace::_get_triangles() const
{
	Vector<int> t;
	if (auto_triangles && triangles_dirty) {
		return t;
	}

	t.resize(triangles.size() * 3);
	for (int i = 0; i < triangles.size(); i++) {
		t.write[i * 3 + 0] = triangles[i].points[0];
		t.write[i * 3 + 1] = triangles[i].points[1];
		t.write[i * 3 + 2] = triangles[i].points[2];
	}
	return t;
}

Vector2 AnimationNodeBlendSpace::get_closest_point(const Vector2& p_point)
{
	_update_triangles();

	if (triangles.is_empty()) {
		return Vector2();
	}

	Vector2 best_point;
	bool first = true;

	for (int i = 0; i < triangles.size(); i++) {
		Vector2 points[3];
		for (int j = 0; j < 3; j++) {
			points[j] = get_blend_point_position(get_triangle_point(i, j));
		}

		if (Geometry2D::is_point_in_triangle(p_point, points[0], points[1], points[2])) {
			return p_point;
		}

		for (int j = 0; j < 3; j++) {
			const Vector2 segment_a = points[j];
			const Vector2 segment_b = points[(j + 1) % 3];
			Vector2 closest_point =
				Geometry2D::get_closest_point_to_segment(p_point, segment_a, segment_b);
			if (first || closest_point.distance_to(p_point) < best_point.distance_to(p_point)) {
				best_point = closest_point;
				first = false;
			}
		}
	}

	return best_point;
}

void AnimationNodeBlendSpace::_blend_triangle(
	const Vector2& p_pos, const LocalVector<Vector2>& p_points, LocalVector<float>& r_weights)
{
	if (p_pos.is_equal_approx(p_points[0])) {
		r_weights[0] = 1;
		r_weights[1] = 0;
		r_weights[2] = 0;
		return;
	}
	if (p_pos.is_equal_approx(p_points[1])) {
		r_weights[0] = 0;
		r_weights[1] = 1;
		r_weights[2] = 0;
		return;
	}
	if (p_pos.is_equal_approx(p_points[2])) {
		r_weights[0] = 0;
		r_weights[1] = 0;
		r_weights[2] = 1;
		return;
	}

	Vector2 v0 = p_points[1] - p_points[0];
	Vector2 v1 = p_points[2] - p_points[0];
	Vector2 v2 = p_pos - p_points[0];

	float d00 = v0.dot(v0);
	float d01 = v0.dot(v1);
	float d11 = v1.dot(v1);
	float d20 = v2.dot(v0);
	float d21 = v2.dot(v1);
	float denom = (d00 * d11 - d01 * d01);
	if (denom == 0) {
		r_weights[0] = 1;
		r_weights[1] = 0;
		r_weights[2] = 0;
		return;
	}
	float v = (d11 * d20 - d01 * d21) / denom;
	float w = (d00 * d21 - d01 * d20) / denom;
	float u = 1.0f - v - w;

	r_weights[0] = u;
	r_weights[1] = v;
	r_weights[2] = w;
}

void AnimationNodeBlendSpace::_check_can_sync()
{
	is_contain_invalid_point = false;
	if (sync_mode < SYNC_MODE_CYCLIC_MUTABLE) {
		return;
	}
	for (int i = 0; i < blend_points_used; i++) {
		Ref<AnimationNodeBlendNode> na = blend_points[i].node;
		if (na.is_null() || na->get_node_type() != AnimationNodeBlendNode::NODE_ANIMATION) {
			is_contain_invalid_point = true;
			break;
		}
	}
}

String AnimationNodeBlendSpace::get_caption() const
{
	return dimension == DIMENSION_2D ? "BlendSpace2D" : "BlendSpace1D";
}

void AnimationNodeBlendSpace::set_auto_triangles(bool p_enable)
{
	if (auto_triangles == p_enable) {
		return;
	}

	auto_triangles = p_enable;
	_queue_auto_triangles();
}

bool AnimationNodeBlendSpace::get_auto_triangles() const { return auto_triangles; }

void AnimationNodeBlendSpace::set_blend_mode(BlendMode p_blend_mode)
{
	blend_mode = p_blend_mode;
}

AnimationNodeBlendSpace::BlendMode AnimationNodeBlendSpace::get_blend_mode() const
{
	return blend_mode;
}

#ifndef DISABLE_DEPRECATED
void AnimationNodeBlendSpace::set_use_sync(bool p_sync)
{
	sync_mode = p_sync ? SYNC_MODE_INDEPENDENT : SYNC_MODE_NONE;
}

bool AnimationNodeBlendSpace::is_using_sync() const { return sync_mode != SYNC_MODE_NONE; }
#endif // DISABLE_DEPRECATED

void AnimationNodeBlendSpace::set_sync_mode(SyncMode p_sync_mode)
{
	sync_mode = p_sync_mode;
	_check_can_sync();
}

AnimationNodeBlendSpace::SyncMode AnimationNodeBlendSpace::get_sync_mode() const
{
	return sync_mode;
}

void AnimationNodeBlendSpace::set_cyclic_length(double p_length)
{
	cyclic_length = p_length;
	inverted_cycle_length = (p_length > CMP_EPSILON) ? (1.0 / p_length) : 0.0;
}

double AnimationNodeBlendSpace::get_cyclic_length() const { return cyclic_length; }

AnimationNode::NodeTimeInfo AnimationNodeBlendSpace::_process(
	AnimationNode::ProcessState& r_process_state, AnimationNodeInstance& p_instance,
	const AnimationMixer::PlaybackInfo& p_playback_info, bool p_test_only)
{
	return AnimationNode::NodeTimeInfo();
}

void AnimationNodeBlendSpace::_update_triangles() {}

void AnimationNodeBlendSpace::_queue_auto_triangles() {}

// ==========================================================================
// Merged from scene/animation/animation_blend_tree.cpp
// ==========================================================================

#include "scene/animation/animation_mixer.h"
#include "core/config/engine.h"
#include "scene/resources/animation.h"

LocalVector<String> (*AnimationNodeBlendNode::get_editable_animation_list)() = nullptr;

void AnimationNodeBlendNode::validate_node(
	const AnimationTree* p_tree, const String& p_path) const
{
	AnimationNode::validate_node(p_tree, p_path);

	if (node_type == NODE_ANIMATION) {
		const Ref<Animation>& animation_resource = p_tree->get_animation_or_null(animation);
		if (animation_resource.is_null()) {
			add_validation_error(
				p_tree, p_path, vformat(RTR("Animation '%s' not found."), animation));
		}
	}
}

bool AnimationNodeBlendNode::is_parameter_read_only(const String& p_parameter) const
{
	if (AnimationNode::is_parameter_read_only(p_parameter)) {
		return true;
	}

	if (node_type == NODE_ONESHOT) {
		return p_parameter == active || p_parameter == internal_active;
	}
	if (node_type == NODE_TRANSITION) {
		return p_parameter == current_state || p_parameter == current_index;
	}
	return false;
}

String AnimationNodeBlendNode::get_caption() const
{
	switch (node_type) {
	case NODE_ANIMATION:
		return "Animation";
	case NODE_ONESHOT:
		return "OneShot";
	case NODE_ADD:
		return three_way ? "Add3" : "Add2";
	case NODE_BLEND:
		return three_way ? "Blend3" : "Blend2";
	case NODE_SUB:
		return "Sub2";
	case NODE_TIME_SCALE:
		return "TimeScale";
	case NODE_TIME_SEEK:
		return "TimeSeek";
	case NODE_TRANSITION:
		return "Transition";
	case NODE_OUTPUT:
		return "Output";
	}
	return "Node";
}

bool AnimationNodeBlendNode::has_filter() const
{
	switch (node_type) {
	case NODE_ONESHOT:
	case NODE_ADD:
	case NODE_SUB:
		return true;
	case NODE_BLEND:
		return !three_way;
	case NODE_ANIMATION:
	case NODE_TIME_SCALE:
	case NODE_TIME_SEEK:
	case NODE_TRANSITION:
	case NODE_OUTPUT:
		return false;
	}
	return false;
}

AnimationNode::NodeTimeInfo AnimationNodeBlendNode::_process(ProcessState& p_process_state,
	AnimationNodeInstance& p_instance, const AnimationMixer::PlaybackInfo& p_playback_info,
	bool p_test_only)
{
	if (node_type == NODE_OUTPUT) {
		AnimationMixer::PlaybackInfo pi = p_playback_info;
		pi.weight = 1.0;
		return blend_input(p_process_state, p_instance, 0, pi, FILTER_IGNORE, true, p_test_only);
	}
	return AnimationNode::NodeTimeInfo();
}

void AnimationNodeBlendNode::set_animation(const String& p_name)
{
	animation = p_name;
	animation_version++;
	if (unlikely(animation_version == 0)) {
		animation_version = 1;
	}
	emit_changed();
}

String AnimationNodeBlendNode::get_animation() const { return animation; }

void AnimationNodeBlendNode::set_play_mode(PlayMode p_play_mode) { play_mode = p_play_mode; }

AnimationNodeBlendNode::PlayMode AnimationNodeBlendNode::get_play_mode() const
{
	return play_mode;
}

void AnimationNodeBlendNode::set_backward(
	AnimationNodeInstance& p_instance, ProcessState& p_process_state, bool p_backward)
{
}

bool AnimationNodeBlendNode::is_backward(
	AnimationNodeInstance& p_instance, ProcessState& p_process_state) const
{
	return false;
}

void AnimationNodeBlendNode::set_advance_on_start(bool p_advance_on_start)
{
	advance_on_start = p_advance_on_start;
}

bool AnimationNodeBlendNode::is_advance_on_start() const { return advance_on_start; }

void AnimationNodeBlendNode::set_use_custom_timeline(bool p_use_custom_timeline)
{
	use_custom_timeline = p_use_custom_timeline;
}

bool AnimationNodeBlendNode::is_using_custom_timeline() const { return use_custom_timeline; }

void AnimationNodeBlendNode::set_timeline_length(double p_length) { timeline_length = p_length; }

double AnimationNodeBlendNode::get_timeline_length() const { return timeline_length; }

void AnimationNodeBlendNode::set_stretch_time_scale(bool p_stretch_time_scale)
{
	stretch_time_scale = p_stretch_time_scale;
}

bool AnimationNodeBlendNode::is_stretching_time_scale() const { return stretch_time_scale; }

void AnimationNodeBlendNode::set_start_offset(double p_offset) { start_offset = p_offset; }

double AnimationNodeBlendNode::get_start_offset() const { return start_offset; }

void AnimationNodeBlendNode::set_loop_mode(Animation::LoopMode p_loop_mode)
{
	loop_mode = p_loop_mode;
}

Animation::LoopMode AnimationNodeBlendNode::get_loop_mode() const { return loop_mode; }

void AnimationNodeBlendNode::_update_animation_cache(
	AnimationTree* p_tree, AnimationNodeInstance& p_instance) const
{
	if (p_instance.cached_animation_version == animation_version) {
		return;
	}

	const Ref<Animation>& anim = p_tree->get_animation_or_null(animation);
	if (anim.is_null()) {
		return;
	}

	p_instance.cached_animation = anim;
	p_instance.cached_animation_version = animation_version;
}

void AnimationNodeBlendNode::set_fade_in_time(double p_time) { fade_in = p_time; }

double AnimationNodeBlendNode::get_fade_in_time() const { return fade_in; }

void AnimationNodeBlendNode::set_fade_out_time(double p_time) { fade_out = p_time; }

double AnimationNodeBlendNode::get_fade_out_time() const { return fade_out; }

void AnimationNodeBlendNode::set_fade_in_curve(const Ref<Curve>& p_curve)
{
	fade_in_curve = p_curve;
}

Ref<Curve> AnimationNodeBlendNode::get_fade_in_curve() const { return fade_in_curve; }

void AnimationNodeBlendNode::set_fade_out_curve(const Ref<Curve>& p_curve)
{
	fade_out_curve = p_curve;
}

Ref<Curve> AnimationNodeBlendNode::get_fade_out_curve() const { return fade_out_curve; }

void AnimationNodeBlendNode::set_auto_restart_enabled(bool p_enabled) { auto_restart = p_enabled; }

void AnimationNodeBlendNode::set_auto_restart_delay(double p_time) { auto_restart_delay = p_time; }

void AnimationNodeBlendNode::set_auto_restart_random_delay(double p_time)
{
	auto_restart_random_delay = p_time;
}

bool AnimationNodeBlendNode::is_auto_restart_enabled() const { return auto_restart; }

double AnimationNodeBlendNode::get_auto_restart_delay() const { return auto_restart_delay; }

double AnimationNodeBlendNode::get_auto_restart_random_delay() const
{
	return auto_restart_random_delay;
}

void AnimationNodeBlendNode::set_mix_mode(MixMode p_mix) { mix = p_mix; }

AnimationNodeBlendNode::MixMode AnimationNodeBlendNode::get_mix_mode() const { return mix; }

void AnimationNodeBlendNode::set_break_loop_at_end(bool p_enable) { break_loop_at_end = p_enable; }

bool AnimationNodeBlendNode::is_loop_broken_at_end() const { return break_loop_at_end; }

void AnimationNodeBlendNode::set_abort_on_reset(bool p_enable) { abort_on_reset = p_enable; }

bool AnimationNodeBlendNode::is_aborted_on_reset() const { return abort_on_reset; }

void AnimationNodeBlendNode::set_explicit_elapse(bool p_enable) { explicit_elapse = p_enable; }

bool AnimationNodeBlendNode::is_explicit_elapse() const { return explicit_elapse; }

void AnimationNodeBlendNode::set_input_count(int p_inputs)
{
	while (get_input_count() < p_inputs) {
		add_input("state_" + itos(get_input_count()));
	}
	while (get_input_count() > p_inputs) {
		remove_input(get_input_count() - 1);
	}
}

bool AnimationNodeBlendNode::add_input(const String& p_name)
{
	if (AnimationNode::add_input(p_name)) {
		if (node_type == NODE_TRANSITION) {
			input_data.push_back(InputData());
		}
		return true;
	}
	return false;
}

void AnimationNodeBlendNode::remove_input(int p_index)
{
	if (node_type == NODE_TRANSITION) {
		input_data.remove_at(p_index);
	}
	AnimationNode::remove_input(p_index);
}

bool AnimationNodeBlendNode::set_input_name(int p_input, const String& p_name)
{
	if (node_type == NODE_TRANSITION) {
		return false;
	}
	return AnimationNode::set_input_name(p_input, p_name);
}

void AnimationNodeBlendNode::set_input_as_auto_advance(int p_input, bool p_enable)
{
	ERR_FAIL_INDEX(p_input, (int)input_data.size());
	input_data[p_input].auto_advance = p_enable;
}

bool AnimationNodeBlendNode::is_input_set_as_auto_advance(int p_input) const
{
	ERR_FAIL_INDEX_V(p_input, (int)input_data.size(), false);
	return input_data[p_input].auto_advance;
}

void AnimationNodeBlendNode::set_input_break_loop_at_end(int p_input, bool p_enable)
{
	ERR_FAIL_INDEX(p_input, (int)input_data.size());
	input_data[p_input].break_loop_at_end = p_enable;
}

bool AnimationNodeBlendNode::is_input_loop_broken_at_end(int p_input) const
{
	ERR_FAIL_INDEX_V(p_input, (int)input_data.size(), false);
	return input_data[p_input].break_loop_at_end;
}

void AnimationNodeBlendNode::set_input_reset(int p_input, bool p_enable)
{
	ERR_FAIL_INDEX(p_input, (int)input_data.size());
	input_data[p_input].reset = p_enable;
}

bool AnimationNodeBlendNode::is_input_reset(int p_input) const
{
	ERR_FAIL_INDEX_V(p_input, (int)input_data.size(), true);
	return input_data[p_input].reset;
}

void AnimationNodeBlendNode::set_xfade_time(double p_fade) { xfade_time = p_fade; }

double AnimationNodeBlendNode::get_xfade_time() const { return xfade_time; }

void AnimationNodeBlendNode::set_xfade_curve(const Ref<Curve>& p_curve) { xfade_curve = p_curve; }

Ref<Curve> AnimationNodeBlendNode::get_xfade_curve() const { return xfade_curve; }

void AnimationNodeBlendNode::set_allow_transition_to_self(bool p_enable)
{
	allow_transition_to_self = p_enable;
}

bool AnimationNodeBlendNode::is_allow_transition_to_self() const
{
	return allow_transition_to_self;
}

AnimationNodeBlendNode::AnimationNodeBlendNode(NodeType p_type, bool p_three_way)
	: node_type(p_type), three_way(p_three_way)
{
	switch (node_type) {
	case NODE_ANIMATION: {
		root_node = true;
	} break;
	case NODE_ONESHOT: {
		add_input("in");
		add_input("shot");
	} break;
	case NODE_ADD: {
		if (three_way) {
			add_input("-add");
			add_input("in");
			add_input("+add");
		}
		else {
			add_input("in");
			add_input("add");
		}
	} break;
	case NODE_BLEND: {
		if (three_way) {
			add_input("-blend");
			add_input("in");
			add_input("+blend");
		}
		else {
			add_input("in");
			add_input("blend");
		}
	} break;
	case NODE_SUB: {
		add_input("in");
		add_input("sub");
	} break;
	case NODE_TIME_SCALE:
	case NODE_TIME_SEEK: {
		add_input("in");
	} break;
	case NODE_TRANSITION:
		break;
	case NODE_OUTPUT: {
		add_input("output");
	} break;
	}
}

Ref<AnimationNode> AnimationNodeBlendTree::get_node(const String& p_name) const
{
	const Node* node = nodes.getptr(p_name);
	ERR_FAIL_NULL_V(node, Ref<AnimationNode>());
	return node->node;
}

void AnimationNodeBlendTree::set_node_position(const String& p_node, const Vector2& p_position)
{
	ERR_FAIL_COND(!nodes.has(p_node));
	nodes[p_node].position = p_position;
}

Vector2 AnimationNodeBlendTree::get_node_position(const String& p_node) const
{
	ERR_FAIL_COND_V(!nodes.has(p_node), Vector2());
	return nodes[p_node].position;
}

void AnimationNodeBlendTree::get_child_nodes(LocalVector<ChildNode>* r_child_nodes)
{
	for (const KeyValue<String, Node>& E : nodes) {
		ChildNode cn;
		cn.name = E.key;
		cn.node = E.value.node;
		r_child_nodes->push_back(cn);
	}
}

bool AnimationNodeBlendTree::has_node(const String& p_name) const { return nodes.has(p_name); }

const LocalVector<String>* AnimationNodeBlendTree::get_node_connection_array(
	const String& p_name) const
{
	const Node* node = nodes.getptr(p_name);
	ERR_FAIL_NULL_V(node, nullptr);
	return &node->connections;
}

AnimationNodeBlendTree::ConnectionError AnimationNodeBlendTree::can_connect_node(
	const String& p_input_node, int p_input_index, const String& p_output_node) const
{
	if (!nodes.has(p_output_node) || p_output_node == SceneString(output)) {
		return CONNECTION_ERROR_NO_OUTPUT;
	}

	if (!nodes.has(p_input_node)) {
		return CONNECTION_ERROR_NO_INPUT;
	}

	if (p_input_node == p_output_node) {
		return CONNECTION_ERROR_SAME_NODE;
	}

	Ref<AnimationNode> input = nodes[p_input_node].node;

	if (p_input_index < 0 || p_input_index >= (int)nodes[p_input_node].connections.size()) {
		return CONNECTION_ERROR_NO_INPUT_INDEX;
	}

	if (nodes[p_input_node].connections[p_input_index] != String()) {
		return CONNECTION_ERROR_CONNECTION_EXISTS;
	}

	for (const KeyValue<String, Node>& E : nodes) {
		for (uint32_t i = 0; i < E.value.connections.size(); i++) {
			const String output = E.value.connections[i];
			if (output == p_output_node) {
				return CONNECTION_ERROR_CONNECTION_EXISTS;
			}
		}
	}
	return CONNECTION_OK;
}

void AnimationNodeBlendTree::get_node_connections(LocalVector<NodeConnection>* r_connections) const
{
	for (const KeyValue<String, Node>& E : nodes) {
		for (uint32_t i = 0; i < E.value.connections.size(); i++) {
			const String output = E.value.connections[i];
			if (output != String()) {
				NodeConnection nc;
				nc.input_node = E.key;
				nc.input_index = i;
				nc.output_node = output;
				r_connections->push_back(nc);
			}
		}
	}
}

String AnimationNodeBlendTree::get_caption() const { return "BlendTree"; }

AnimationNode::NodeTimeInfo AnimationNodeBlendTree::_process(ProcessState& p_process_state,
	AnimationNodeInstance& p_instance, const AnimationMixer::PlaybackInfo& p_playback_info,
	bool p_test_only)
{
	const Ref<AnimationNodeBlendNode> output = nodes[SceneString(output)].node;
	ERR_FAIL_COND_V(
		output.is_null() || output->get_node_type() != AnimationNodeBlendNode::NODE_OUTPUT,
		NodeTimeInfo());

	AnimationMixer::PlaybackInfo pi = p_playback_info;
	pi.weight = 1.0;

	AnimationNodeInstance& output_instance =
		p_instance.get_child_instance_by_path(SceneString(output));
	return _blend_node(p_process_state, p_instance, output_instance, pi, FILTER_IGNORE, true,
		p_test_only, nullptr);
}

LocalVector<String> AnimationNodeBlendTree::get_node_list() const
{
	LocalVector<String> list;
	list.reserve(nodes.size());
	for (const KeyValue<String, Node>& E : nodes) {
		list.push_back(E.key);
	}
	list.sort();
	return list;
}

void AnimationNodeBlendTree::set_graph_offset(const Vector2& p_graph_offset)
{
	graph_offset = p_graph_offset;
}

Vector2 AnimationNodeBlendTree::get_graph_offset() const { return graph_offset; }

Ref<AnimationNode> AnimationNodeBlendTree::get_child_by_name(const String& p_name) const
{
	return get_node(p_name);
}

void AnimationNodeBlendTree::_tree_changed() { AnimationNode::_tree_changed(); }

void AnimationNodeBlendTree::validate_node(
	const AnimationTree* p_tree, const String& p_path) const
{
	AnimationNode::validate_node(p_tree, p_path);

	// Validate output connection.
	{
		const LocalVector<String>* output_connections =
			get_node_connection_array(SceneString(output));
		const String& node_name = output_connections->operator[](0);

		if (const Node* child_node = nodes.getptr(node_name); !child_node) {
			add_validation_error(p_tree, String(p_path) + SceneString(output) + "/",
				RTR("Nothing connected to output."));
		}
	}

	// Rest of children.
	for (const KeyValue<String, Node>& E : nodes) {
		const Node& child = E.value;

		// Skip output node, already validated.
		if (E.key == SceneString(output)) {
			continue;
		}

		for (uint32_t input = 0; input < child.connections.size(); input++) {
			const String& connected_node_name = child.connections[input];
			if (const Node* connected_to = nodes.getptr(connected_node_name); !connected_to) {
				String path = String(p_path) + String(E.key) + "/";
				add_validation_error(p_tree, path, "Nothing connected to", input);
			}
		}
	}
}

#ifdef TOOLS_ENABLED
void AnimationNodeBlendTree::get_argument_options(
	const String& p_function, int p_idx, List<String>* r_options) const
{
	const String pf = p_function;
	bool add_node_options = false;
	if (p_idx == 0) {
		add_node_options =
			(pf == "get_node" || pf == "has_node" || pf == "rename_node" || pf == "remove_node" ||
				pf == "set_node_position" || pf == "get_node_position" || pf == "connect_node" ||
				pf == "disconnect_node");
	}
	else if (p_idx == 2) {
		add_node_options = (pf == "connect_node" || pf == "disconnect_node");
	}
	if (add_node_options) {
		for (const KeyValue<String, Node>& E : nodes) {
			r_options->push_back(String(E.key).quote());
		}
	}
	AnimationNode::get_argument_options(p_function, p_idx, r_options);
}
#endif

void AnimationNodeBlendTree::_initialize_node_tree()
{
	Ref<AnimationNodeBlendNode> output;
	output.instantiate(AnimationNodeBlendNode::NODE_OUTPUT);
	Node n;
	n.node = output;
	n.position = Vector2(300, 150);
	n.connections.resize(1);
	nodes[SceneString(output)] = std::move(n);
}

AnimationNodeBlendTree::AnimationNodeBlendTree()
{
	root_node = true;
	_initialize_node_tree();
}

void AnimationNodeBlendTree::reset_state() {}

// ==========================================================================
// Merged from scene/animation/animation_node_state_machine.cpp
// ==========================================================================
// --- Inlined from scene/animation/animation_node_state_machine.h ---
#include "core/math/expression.h"
#include "scene/animation/animation_mixer.h"
#include "scene/resources/curve.h"

class AnimationNodeStateMachineTransition : public Resource
{
public:
	enum SwitchMode
	{
		SWITCH_MODE_IMMEDIATE,
		SWITCH_MODE_SYNC,
		SWITCH_MODE_AT_END,
	};

	enum AdvanceMode
	{
		ADVANCE_MODE_DISABLED,
		ADVANCE_MODE_ENABLED,
		ADVANCE_MODE_AUTO,
	};

private:
	SwitchMode switch_mode = SWITCH_MODE_IMMEDIATE;
	AdvanceMode advance_mode = ADVANCE_MODE_ENABLED;
	String advance_condition;
	String advance_condition_name;
	float xfade_time = 0.0;
	Ref<Curve> xfade_curve;
	bool break_loop_at_end = false;
	bool reset = true;
	int priority = 1;
	String advance_expression;

	friend class AnimationNodeStateMachinePlayback;
	Ref<Expression> expression;

protected:

public:
	void set_switch_mode(SwitchMode p_mode);
	SwitchMode get_switch_mode() const;

	void set_advance_mode(AdvanceMode p_mode);
	AdvanceMode get_advance_mode() const;

	void set_advance_condition(const String& p_condition);
	String get_advance_condition() const;

	String get_advance_condition_name() const;

	void set_advance_expression(const String& p_expression);
	String get_advance_expression() const;

	void set_xfade_time(float p_xfade);
	float get_xfade_time() const;

	void set_break_loop_at_end(bool p_enable);
	bool is_loop_broken_at_end() const;

	void set_reset(bool p_reset);
	bool is_reset() const;

	void set_xfade_curve(const Ref<Curve>& p_curve);
	Ref<Curve> get_xfade_curve() const;

	void set_priority(int p_priority);
	int get_priority() const;

	AnimationNodeStateMachineTransition();
};

class AnimationNodeStateMachinePlayback;

class AnimationNodeStateMachine final : public AnimationNode
{
public:
	enum StateMachineType
	{
		STATE_MACHINE_TYPE_ROOT,
		STATE_MACHINE_TYPE_NESTED,
		STATE_MACHINE_TYPE_GROUPED,
	};

private:
	friend class AnimationNodeStateMachinePlayback;

	StateMachineType state_machine_type = STATE_MACHINE_TYPE_ROOT;

	struct State
	{
		Ref<AnimationNode> node;
		Vector2 position;
	};

	HashMap<String, State> states;
	bool allow_transition_to_self = false;
	bool reset_ends = false;

	struct Transition
	{
		String from;
		String to;
		Ref<AnimationNodeStateMachineTransition> transition;
	};

	Vector<Transition> transitions;

	String playback = "playback";
	bool updating_transitions = false;

	Vector2 graph_offset;

	void _remove_transition(const Ref<AnimationNodeStateMachineTransition> p_transition);
	void _rename_transitions(const String& p_name, const String& p_new_name);
	bool _can_connect(const String& p_name);

protected:
	bool _check_advance_condition(const Ref<AnimationNodeStateMachine> p_state_machine,
		const Ref<AnimationNodeStateMachineTransition> p_transition) const;

	virtual void _tree_changed() override;

	virtual void reset_state() override;

public:
	virtual void validate_node(
		const AnimationTree* p_tree, const String& p_path) const override;

	virtual bool is_parameter_read_only(const String& p_parameter) const override;

	void add_node(
		const String& p_name, Ref<AnimationNode> p_node, const Vector2& p_position = Vector2());
	void replace_node(const String& p_name, Ref<AnimationNode> p_node);
	Ref<AnimationNode> get_node(const String& p_name) const;
	void remove_node(const String& p_name);
	void rename_node(const String& p_name, const String& p_new_name);
	bool has_node(const String& p_name) const;
	String get_node_name(const Ref<AnimationNode>& p_node) const;
	LocalVector<String> get_node_list() const;

	void set_node_position(const String& p_name, const Vector2& p_position);
	Vector2 get_node_position(const String& p_name) const;

	virtual void get_child_nodes(LocalVector<ChildNode>* r_child_nodes) override;

	bool has_transition(const String& p_from, const String& p_to) const;
	bool has_transition_from(const String& p_from) const;
	bool has_transition_to(const String& p_to) const;
	int find_transition(const String& p_from, const String& p_to) const;
	Vector<int> find_transition_from(const String& p_from) const;
	Vector<int> find_transition_to(const String& p_to) const;
	void add_transition(const String& p_from, const String& p_to,
		const Ref<AnimationNodeStateMachineTransition>& p_transition);
	Ref<AnimationNodeStateMachineTransition> get_transition(int p_transition) const;
	String get_transition_from(int p_transition) const;
	String get_transition_to(int p_transition) const;
	int get_transition_count() const;
	bool is_transition_across_group(int p_transition) const;
	void remove_transition_by_index(const int p_transition);
	void remove_transition(const String& p_from, const String& p_to);

	void set_state_machine_type(StateMachineType p_state_machine_type);
	StateMachineType get_state_machine_type() const;

	void set_allow_transition_to_self(bool p_enable);
	bool is_allow_transition_to_self() const;

	void set_reset_ends(bool p_enable);
	bool are_ends_reset() const;

	bool can_edit_node(const String& p_name) const;

	void set_graph_offset(const Vector2& p_offset);
	Vector2 get_graph_offset() const;

	virtual NodeTimeInfo _process(ProcessState& p_process_state, AnimationNodeInstance& p_instance,
		const AnimationMixer::PlaybackInfo& p_playback_info, bool p_test_only = false) override;
	virtual String get_caption() const override;

	virtual Ref<AnimationNode> get_child_by_name(const String& p_name) const override;

#ifdef TOOLS_ENABLED
	virtual void get_argument_options(
		const String& p_function, int p_idx, List<String>* r_options) const override;
#endif

	Vector<String> get_nodes_with_transitions_from(const String& p_node) const;
	Vector<String> get_nodes_with_transitions_to(const String& p_node) const;

	AnimationNodeStateMachine();
};

class AnimationNodeStateMachinePlayback : public Resource
{
	friend class AnimationNodeStateMachine;

	struct AStarCost
	{
		float distance = 0.0;
		String prev;
	};

	struct TransitionInfo
	{
		String from;
		String to;
		String next;
	};

	struct NextInfo
	{
		String node;
		double xfade;
		Ref<Curve> curve;
		AnimationNodeStateMachineTransition::SwitchMode switch_mode;
		bool is_reset;
		bool break_loop_at_end;
	};

	struct ChildStateMachineInfo
	{
		Ref<AnimationNodeStateMachinePlayback> playback;
		Vector<String> path;
		bool is_reset = false;
	};

	Ref<AnimationNodeStateMachineTransition> default_transition;
	String base_path;

	AnimationNode::NodeTimeInfo current_nti;
	String current = SceneString(Start);
	Ref<Curve> current_curve;

	Ref<AnimationNodeStateMachineTransition> group_start_transition;
	Ref<AnimationNodeStateMachineTransition> group_end_transition;

	AnimationNode::NodeTimeInfo fadeing_from_nti;
	String fading_from;
	float fading_time = 0.0;
	float fading_pos = 0.0;

	Vector<String> path;
	bool playing = false;

	String start_request;
	String travel_request;
	bool reset_request = false;
	bool reset_request_on_teleport = false;
	bool _reset_request_for_fading_from = false;
	bool next_request = false;
	bool stop_request = false;
	bool teleport_request = false;

	bool is_grouped = false;

	void _clear_fading(AnimationNode::ProcessState& p_process_state,
		AnimationNodeStateMachine* p_state_machine, const String& p_state);
	void _signal_state_change(
		AnimationTree* p_animation_tree, const String& p_state, bool p_started);
	void _travel_main(const String& p_state, bool p_reset_on_teleport = true);
	void _start_main(const String& p_state, bool p_reset = true);
	void _next_main();
	void _stop_main();

	bool _make_travel_path(AnimationNode::ProcessState& p_process_state, AnimationTree* p_tree,
		AnimationNodeStateMachine* p_state_machine, bool p_is_allow_transition_to_self,
		Vector<String>& r_path, bool p_test_only);
	String _validate_path(AnimationNodeStateMachine* p_state_machine, const String& p_path);
	bool _travel(AnimationNode::ProcessState& p_process_state, AnimationTree* p_tree,
		AnimationNodeStateMachine* p_state_machine, bool p_is_allow_transition_to_self,
		bool p_test_only);
	void _start(
		AnimationNode::ProcessState& p_process_state, AnimationNodeStateMachine* p_state_machine);

	void _clear_path_children(AnimationNode::ProcessState& p_process_state, AnimationTree* p_tree,
		AnimationNodeStateMachine* p_state_machine, bool p_test_only);
	void _start_children(AnimationTree* p_tree, AnimationNodeStateMachine* p_state_machine,
		const String& p_path, bool p_test_only);

	AnimationNode::NodeTimeInfo process(AnimationNode::ProcessState& p_process_state,
		AnimationNodeInstance& p_instance, AnimationNodeStateMachine* p_state_machine,
		const AnimationMixer::PlaybackInfo p_playback_info, bool p_test_only);
	AnimationNode::NodeTimeInfo _process(AnimationNode::ProcessState& p_process_state,
		AnimationNodeInstance& p_instance, AnimationNodeStateMachine* p_state_machine,
		const AnimationMixer::PlaybackInfo p_playback_info, bool p_test_only);

	bool _transition_to_next_recursive(AnimationNode::ProcessState& p_process_state,
		AnimationNodeInstance& p_instance, AnimationTree* p_tree,
		AnimationNodeStateMachine* p_state_machine, double p_delta, bool p_test_only);
	NextInfo _find_next(AnimationNode::ProcessState& p_process_state,
		AnimationNodeInstance& p_instance, AnimationTree* p_tree,
		AnimationNodeStateMachine* p_state_machine) const;
	Ref<AnimationNodeStateMachineTransition> _check_group_transition(AnimationTree* p_tree,
		AnimationNodeStateMachine* p_state_machine,
		const AnimationNodeStateMachine::Transition& p_transition,
		Ref<AnimationNodeStateMachine>& r_state_machine, bool& r_bypass) const;

	void _set_current(AnimationNode::ProcessState& p_process_state,
		AnimationNodeStateMachine* p_state_machine, const String& p_state);
	void _set_grouped(bool p_is_grouped);
	void _set_base_path(const String& p_base_path);
	Ref<AnimationNodeStateMachinePlayback> _get_parent_playback(AnimationTree* p_tree) const;
	Ref<AnimationNodeStateMachine> _get_parent_state_machine(AnimationTree* p_tree) const;
	Ref<AnimationNodeStateMachineTransition> _get_group_start_transition() const;
	Ref<AnimationNodeStateMachineTransition> _get_group_end_transition() const;

protected:

public:
	void travel(const String& p_state, bool p_reset_on_teleport = true);
	void start(const String& p_state, bool p_reset = true);
	void next();
	void stop();
	bool is_playing() const;
	bool is_end() const;
	String get_current_node() const;
	String get_fading_from_node() const;
	Vector<String> get_travel_path() const;
	float get_current_play_pos() const;
	float get_current_length() const;

	float get_fading_from_play_pos() const;
	float get_fading_from_length() const;

	float get_fading_time() const;
	float get_fading_pos() const;

	void clear_path();
	void push_path(const String& p_state);

	AnimationNodeStateMachinePlayback();
};



// --- End scene/animation/animation_node_state_machine.h ---
#include "core/config/engine.h"

/////////////////////////////////////////////////

void AnimationNodeStateMachineTransition::set_switch_mode(SwitchMode p_mode)
{
	switch_mode = p_mode;
}

AnimationNodeStateMachineTransition::SwitchMode
AnimationNodeStateMachineTransition::get_switch_mode() const
{
	return switch_mode;
}

void AnimationNodeStateMachineTransition::set_advance_mode(AdvanceMode p_mode)
{
	advance_mode = p_mode;
}

AnimationNodeStateMachineTransition::AdvanceMode
AnimationNodeStateMachineTransition::get_advance_mode() const
{
	return advance_mode;
}

String AnimationNodeStateMachineTransition::get_advance_condition() const
{
	return advance_condition;
}

String AnimationNodeStateMachineTransition::get_advance_condition_name() const
{
	return advance_condition_name;
}

void AnimationNodeStateMachineTransition::set_advance_expression(const String& p_expression)
{
	advance_expression = p_expression;

	String advance_expression_stripped = advance_expression.strip_edges();
	if (advance_expression_stripped == String()) {
		expression.unref();
		return;
	}

	if (expression.is_null()) {
		expression.instantiate();
	}

	expression->parse(advance_expression_stripped);
}

String AnimationNodeStateMachineTransition::get_advance_expression() const
{
	return advance_expression;
}

void AnimationNodeStateMachineTransition::set_xfade_time(float p_xfade)
{
	ERR_FAIL_COND(p_xfade < 0);
	xfade_time = p_xfade;
	emit_changed();
}

float AnimationNodeStateMachineTransition::get_xfade_time() const { return xfade_time; }

void AnimationNodeStateMachineTransition::set_xfade_curve(const Ref<Curve>& p_curve)
{
	xfade_curve = p_curve;
	emit_changed();
}

Ref<Curve> AnimationNodeStateMachineTransition::get_xfade_curve() const { return xfade_curve; }

void AnimationNodeStateMachineTransition::set_break_loop_at_end(bool p_enable)
{
	break_loop_at_end = p_enable;
	emit_changed();
}

bool AnimationNodeStateMachineTransition::is_loop_broken_at_end() const
{
	return break_loop_at_end;
}

void AnimationNodeStateMachineTransition::set_reset(bool p_reset)
{
	reset = p_reset;
	emit_changed();
}

bool AnimationNodeStateMachineTransition::is_reset() const { return reset; }

void AnimationNodeStateMachineTransition::set_priority(int p_priority)
{
	priority = p_priority;
	emit_changed();
}

int AnimationNodeStateMachineTransition::get_priority() const { return priority; }

AnimationNodeStateMachineTransition::AnimationNodeStateMachineTransition() {}

////////////////////////////////////////////////////////

void AnimationNodeStateMachinePlayback::_set_current(AnimationNode::ProcessState& p_process_state,
	AnimationNodeStateMachine* p_state_machine, const String& p_state)
{
	current = p_state;
	if (current == String()) {
		group_start_transition = Ref<AnimationNodeStateMachineTransition>();
		group_end_transition = Ref<AnimationNodeStateMachineTransition>();
		return;
	}

	AnimationTree* tree = p_process_state.tree;
	Ref<AnimationNodeStateMachine> anodesm = p_state_machine->find_node_by_path(current);
	if (anodesm.is_null()) {
		group_start_transition = Ref<AnimationNodeStateMachineTransition>();
		group_end_transition = Ref<AnimationNodeStateMachineTransition>();
		_signal_state_change(tree, current, true);
		return;
	}

	Vector<int> indices = p_state_machine->find_transition_to(current);
	int group_start_size = indices.size();
	if (group_start_size) {
		group_start_transition = p_state_machine->get_transition(indices[0]);
	}
	else {
		group_start_transition = Ref<AnimationNodeStateMachineTransition>();
	}

	indices = p_state_machine->find_transition_from(current);
	int group_end_size = indices.size();
	if (group_end_size) {
		group_end_transition = p_state_machine->get_transition(indices[0]);
	}
	else {
		group_end_transition = Ref<AnimationNodeStateMachineTransition>();
	}

	// Validation.
	if (anodesm->get_state_machine_type() ==
		AnimationNodeStateMachine::STATE_MACHINE_TYPE_GROUPED) {
		indices = anodesm->find_transition_from(SceneString(Start));
		int anodesm_start_size = indices.size();
		indices = anodesm->find_transition_to(SceneString(End));
		int anodesm_end_size = indices.size();
		if (group_start_size > 1) {
			WARN_PRINT_ED("There are two or more transitions to the Grouped "
						  "AnimationNodeStateMachine in AnimationNodeStateMachine: " +
						  base_path + ", which may result in unintended transitions.");
		}
		if (group_end_size > 1) {
			WARN_PRINT_ED("There are two or more transitions from the Grouped "
						  "AnimationNodeStateMachine in AnimationNodeStateMachine: " +
						  base_path + ", which may result in unintended transitions.");
		}
		if (anodesm_start_size > 1) {
			WARN_PRINT_ED("There are two or more transitions from the Start of Grouped "
						  "AnimationNodeStateMachine in AnimationNodeStateMachine: " +
						  base_path + current + ", which may result in unintended transitions.");
		}
		if (anodesm_end_size > 1) {
			WARN_PRINT_ED("There are two or more transitions to the End of Grouped "
						  "AnimationNodeStateMachine in AnimationNodeStateMachine: " +
						  base_path + current + ", which may result in unintended transitions.");
		}
		if (anodesm_start_size != group_start_size) {
			ERR_PRINT_ED("There is a mismatch in the number of start transitions in and out of the "
						 "Grouped AnimationNodeStateMachine on AnimationNodeStateMachine: " +
						 base_path + current + ".");
		}
		if (anodesm_end_size != group_end_size) {
			ERR_PRINT_ED("There is a mismatch in the number of end transitions in and out of the "
						 "Grouped AnimationNodeStateMachine on AnimationNodeStateMachine: " +
						 base_path + current + ".");
		}
	}
	else {
		_signal_state_change(tree, current, true);
	}
}

void AnimationNodeStateMachinePlayback::_set_grouped(bool p_is_grouped)
{
	is_grouped = p_is_grouped;
}

void AnimationNodeStateMachinePlayback::travel(const String& p_state, bool p_reset_on_teleport)
{
	ERR_FAIL_COND_EDMSG(is_grouped, "Grouped AnimationNodeStateMachinePlayback must be handled by "
									"parent AnimationNodeStateMachinePlayback. You need to "
									"retrieve the parent Root/Nested AnimationNodeStateMachine.");
	ERR_FAIL_COND_EDMSG(String(p_state).contains("/Start") || String(p_state).contains("/End"),
		"Grouped AnimationNodeStateMachinePlayback doesn't allow to play Start/End directly. "
		"Instead, play the prev or next state of group in the parent AnimationNodeStateMachine.");
	_travel_main(p_state, p_reset_on_teleport);
}

void AnimationNodeStateMachinePlayback::start(const String& p_state, bool p_reset)
{
	ERR_FAIL_COND_EDMSG(is_grouped, "Grouped AnimationNodeStateMachinePlayback must be handled by "
									"parent AnimationNodeStateMachinePlayback. You need to "
									"retrieve the parent Root/Nested AnimationNodeStateMachine.");
	ERR_FAIL_COND_EDMSG(String(p_state).contains("/Start") || String(p_state).contains("/End"),
		"Grouped AnimationNodeStateMachinePlayback doesn't allow to play Start/End directly. "
		"Instead, play the prev or next state of group in the parent AnimationNodeStateMachine.");
	_start_main(p_state, p_reset);
}

void AnimationNodeStateMachinePlayback::next()
{
	ERR_FAIL_COND_EDMSG(is_grouped, "Grouped AnimationNodeStateMachinePlayback must be handled by "
									"parent AnimationNodeStateMachinePlayback. You need to "
									"retrieve the parent Root/Nested AnimationNodeStateMachine.");
	_next_main();
}

void AnimationNodeStateMachinePlayback::stop()
{
	ERR_FAIL_COND_EDMSG(is_grouped, "Grouped AnimationNodeStateMachinePlayback must be handled by "
									"parent AnimationNodeStateMachinePlayback. You need to "
									"retrieve the parent Root/Nested AnimationNodeStateMachine.");
	_stop_main();
}

void AnimationNodeStateMachinePlayback::_travel_main(
	const String& p_state, bool p_reset_on_teleport)
{
	travel_request = p_state;
	reset_request_on_teleport = p_reset_on_teleport;
	stop_request = false;
}

void AnimationNodeStateMachinePlayback::_start_main(const String& p_state, bool p_reset)
{
	travel_request = String();
	path.clear();
	reset_request = p_reset;
	start_request = p_state;
	stop_request = false;
}

void AnimationNodeStateMachinePlayback::_next_main() { next_request = true; }

void AnimationNodeStateMachinePlayback::_stop_main() { stop_request = true; }

bool AnimationNodeStateMachinePlayback::is_playing() const { return playing; }

bool AnimationNodeStateMachinePlayback::is_end() const
{
	return current == SceneString(End) && fading_from == String();
}

String AnimationNodeStateMachinePlayback::get_current_node() const { return current; }

String AnimationNodeStateMachinePlayback::get_fading_from_node() const { return fading_from; }

Vector<String> AnimationNodeStateMachinePlayback::get_travel_path() const { return path; }

float AnimationNodeStateMachinePlayback::get_current_play_pos() const
{
	return current_nti.position;
}

float AnimationNodeStateMachinePlayback::get_current_length() const { return current_nti.length; }

float AnimationNodeStateMachinePlayback::get_fading_from_play_pos() const
{
	return fadeing_from_nti.position;
}

float AnimationNodeStateMachinePlayback::get_fading_from_length() const
{
	return fadeing_from_nti.length;
}

float AnimationNodeStateMachinePlayback::get_fading_time() const { return fading_time; }

float AnimationNodeStateMachinePlayback::get_fading_pos() const { return fading_pos; }

bool _is_grouped_state_machine(const Ref<AnimationNodeStateMachine> p_node)
{
	return p_node.is_valid() && p_node->get_state_machine_type() ==
									AnimationNodeStateMachine::STATE_MACHINE_TYPE_GROUPED;
}

void AnimationNodeStateMachinePlayback::_clear_fading(AnimationNode::ProcessState& p_process_state,
	AnimationNodeStateMachine* p_state_machine, const String& p_state)
{
	if (!p_state.is_empty() && !_is_grouped_state_machine(p_state_machine->get_node(p_state))) {
		_signal_state_change(p_process_state.tree, p_state, false);
	}
	fading_from = String();
	fadeing_from_nti = AnimationNode::NodeTimeInfo();
}

void AnimationNodeStateMachinePlayback::_start(
	AnimationNode::ProcessState& p_process_state, AnimationNodeStateMachine* p_state_machine)
{
	playing = true;
	_set_current(p_process_state, p_state_machine,
		start_request != String() ? start_request : SceneString(Start));
	teleport_request = true;
	stop_request = false;
	start_request = String();
}

bool AnimationNodeStateMachinePlayback::_travel(AnimationNode::ProcessState& p_process_state,
	AnimationTree* p_tree, AnimationNodeStateMachine* p_state_machine,
	bool p_is_allow_transition_to_self, bool p_test_only)
{
	return _make_travel_path(
		p_process_state, p_tree, p_state_machine, p_is_allow_transition_to_self, path, p_test_only);
}

String AnimationNodeStateMachinePlayback::_validate_path(
	AnimationNodeStateMachine* p_state_machine, const String& p_path)
{
	if (p_state_machine->get_state_machine_type() ==
		AnimationNodeStateMachine::STATE_MACHINE_TYPE_GROUPED) {
		return p_path; // Grouped state machine doesn't allow validate-able request.
	}
	String target = p_path;
	Ref<AnimationNodeStateMachine> anodesm = p_state_machine->find_node_by_path(target);
	while (anodesm.is_valid() && anodesm->get_state_machine_type() ==
									 AnimationNodeStateMachine::STATE_MACHINE_TYPE_GROUPED) {
		Vector<int> indices = anodesm->find_transition_from(SceneString(Start));
		if (indices.size()) {
			target =
				target + "/" + anodesm->get_transition_to(indices[0]); // Find next state of Start.
		}
		else {
			break; // There is no transition in Start state of grouped state machine.
		}
		anodesm = p_state_machine->find_node_by_path(target);
	}
	return target;
}

AnimationNode::NodeTimeInfo AnimationNodeStateMachinePlayback::process(
	AnimationNode::ProcessState& p_process_state, AnimationNodeInstance& p_instance,
	AnimationNodeStateMachine* p_state_machine, const AnimationMixer::PlaybackInfo p_playback_info,
	bool p_test_only)
{
	AnimationNode::NodeTimeInfo nti =
		_process(p_process_state, p_instance, p_state_machine, p_playback_info, p_test_only);
	start_request = String();
	next_request = false;
	stop_request = false;
	reset_request_on_teleport = false;
	return nti;
}

AnimationNode::NodeTimeInfo AnimationNodeStateMachinePlayback::_process(
	AnimationNode::ProcessState& p_process_state, AnimationNodeInstance& p_instance,
	AnimationNodeStateMachine* p_state_machine, const AnimationMixer::PlaybackInfo p_playback_info,
	bool p_test_only)
{
	AnimationTree* tree = p_process_state.tree;

	double p_time = p_playback_info.time;
	double p_delta = p_playback_info.delta;
	bool p_seek = p_playback_info.seeked;
	bool p_is_external_seeking = p_playback_info.is_external_seeking;

	// Check seek to 0 (means reset) by parent AnimationNode.
	if (Math::is_zero_approx(p_time) && p_seek && !p_is_external_seeking) {
		if (p_state_machine->state_machine_type !=
				AnimationNodeStateMachine::STATE_MACHINE_TYPE_NESTED ||
			is_end() || !playing) {
			// Restart state machine.
			if (p_state_machine->get_state_machine_type() !=
				AnimationNodeStateMachine::STATE_MACHINE_TYPE_GROUPED) {
				path.clear();
				_clear_path_children(p_process_state, tree, p_state_machine, p_test_only);
			}
			_start(p_process_state, p_state_machine);
			reset_request = true;
		}
		else {
			// Reset current state.
			reset_request = true;
			teleport_request = true;
		}
	}

	if (stop_request) {
		start_request = String();
		travel_request = String();
		path.clear();
		playing = false;
		return AnimationNode::NodeTimeInfo();
	}

	if (!playing && start_request != String() && travel_request != String()) {
		return AnimationNode::NodeTimeInfo();
	}

	// Process start/travel request.
	if (start_request != String() || travel_request != String()) {
		if (p_state_machine->get_state_machine_type() !=
			AnimationNodeStateMachine::STATE_MACHINE_TYPE_GROUPED) {
			_clear_path_children(p_process_state, tree, p_state_machine, p_test_only);
		}
	}

	if (start_request != String()) {
		path.clear();
		String start_target = _validate_path(p_state_machine, start_request);
		Vector<String> start_path = String(start_target).split("/");
		start_request = start_path[0];
		if (start_path.size()) {
			_start_children(tree, p_state_machine, start_target, p_test_only);
		}
		// Teleport to start.
		if (p_state_machine->states.has(start_request)) {
			_start(p_process_state, p_state_machine);
		}
		else {
			String node = start_request;
			ERR_FAIL_V_MSG(AnimationNode::NodeTimeInfo(), "No such node: '" + node + "'");
		}
	}

	if (travel_request != String()) {
		// Fix path.
		String travel_target = _validate_path(p_state_machine, travel_request);
		Vector<String> travel_path = travel_target.split("/");
		travel_request = travel_path[0];
		String temp_travel_request = travel_request; // For the case that can't travel.
		// Process children.
		Vector<String> new_path;
		bool can_travel = _make_travel_path(p_process_state, tree, p_state_machine,
			travel_path.size() <= 1 ? p_state_machine->is_allow_transition_to_self() : false,
			new_path, p_test_only);
		if (travel_path.size()) {
			_start_children(tree, p_state_machine, travel_target, p_test_only);
		}
		// Process to travel.
		if (can_travel) {
			path = new_path;
		}
		else {
			// Can't travel, then teleport.
			if (p_state_machine->states.has(temp_travel_request)) {
				path.clear();
				if (current != temp_travel_request || reset_request_on_teleport) {
					_set_current(p_process_state, p_state_machine, temp_travel_request);
					reset_request = reset_request_on_teleport;
					teleport_request = true;
				}
			}
			else {
				ERR_FAIL_V_MSG(
					AnimationNode::NodeTimeInfo(), "No such node: '" + temp_travel_request + "'");
			}
		}
	}

	AnimationMixer::PlaybackInfo pi = p_playback_info;

	if (teleport_request) {
		teleport_request = false;
		// Clear fading on teleport.
		fading_from = String();
		fadeing_from_nti = AnimationNode::NodeTimeInfo();
		fading_pos = 0;
		// Init current length.
		pi.time = 0;
		pi.seeked = true;
		pi.is_external_seeking = false;
		pi.weight = 0;
		AnimationNodeInstance* other_instance =
			p_instance.get_child_instance_by_path_or_null(current);
		current_nti = p_state_machine->blend_node(p_process_state, p_instance, other_instance, pi,
			AnimationNode::FILTER_IGNORE, true, true);
		// Don't process first node if not necessary, instead process next node.
		_transition_to_next_recursive(
			p_process_state, p_instance, tree, p_state_machine, p_delta, p_test_only);
	}

	// Check current node existence.
	if (!p_state_machine->states.has(current)) {
		playing = false; // Current does not exist.
		_set_current(p_process_state, p_state_machine, String());
		return AnimationNode::NodeTimeInfo();
	}

	// Special case for grouped state machine Start/End to make priority with parent blend (means
	// don't treat Start and End states as RESET animations).
	bool is_start_of_group = false;
	bool is_end_of_group = false;
	if (!p_state_machine->are_ends_reset() ||
		p_state_machine->get_state_machine_type() ==
			AnimationNodeStateMachine::STATE_MACHINE_TYPE_GROUPED) {
		is_start_of_group = fading_from == SceneString(Start);
		is_end_of_group = current == SceneString(End);
	}

	// Calc blend amount by cross-fade.
	float fade_blend = 1.0;
	if (fading_time && fading_from != String()) {
		if (!p_state_machine->states.has(fading_from)) {
			fading_from = String();
		}
		else {
			if (!p_seek) {
				fading_pos += Math::abs(p_delta);
			}
			fade_blend = MIN(1.0, fading_pos / fading_time);
		}
	}
	if (current_curve.is_valid()) {
		fade_blend = current_curve->sample(fade_blend);
	}
	fade_blend = Math::is_zero_approx(fade_blend) ? CMP_EPSILON : fade_blend;
	if (is_start_of_group) {
		fade_blend = 1.0;
	}
	else if (is_end_of_group) {
		fade_blend = 0.0;
	}

	// Main process.
	pi = p_playback_info;
	pi.weight = fade_blend;
	if (reset_request) {
		reset_request = false;
		pi.time = 0;
		pi.seeked = true;
	}
	AnimationNodeInstance* other_instance = p_instance.get_child_instance_by_path_or_null(current);
	current_nti = p_state_machine->blend_node(p_process_state, p_instance, other_instance, pi,
		AnimationNode::FILTER_IGNORE, true,
		p_test_only); // Blend values must be more than CMP_EPSILON to process discrete keys in
					  // edge.

	// Cross-fade process.
	if (fading_from != String()) {
		double fade_blend_inv = 1.0 - fade_blend;
		fade_blend_inv = Math::is_zero_approx(fade_blend_inv) ? CMP_EPSILON : fade_blend_inv;
		if (is_start_of_group) {
			fade_blend_inv = 0.0;
		}
		else if (is_end_of_group) {
			fade_blend_inv = 1.0;
		}

		pi = p_playback_info;
		pi.weight = fade_blend_inv;
		if (_reset_request_for_fading_from) {
			_reset_request_for_fading_from = false;
			pi.time = 0;
			pi.seeked = true;
		}
		AnimationNodeInstance* fading_from_instance =
			p_instance.get_child_instance_by_path_or_null(fading_from);
		fadeing_from_nti = p_state_machine->blend_node(p_process_state, p_instance,
			fading_from_instance, pi, AnimationNode::FILTER_IGNORE, true,
			p_test_only); // Blend values must be more than CMP_EPSILON to process discrete keys in
						  // edge.

		if (Animation::is_greater_or_equal_approx(fading_pos, fading_time)) {
			// Finish fading.
			_clear_fading(p_process_state, p_state_machine, fading_from);
		}
	}

	// Find next and see when to transition.
	bool will_end = _transition_to_next_recursive(
						p_process_state, p_instance, tree, p_state_machine, p_delta, p_test_only) ||
					current == SceneString(End);

	// Predict remaining time.
	if (will_end || ((p_state_machine->get_state_machine_type() ==
						 AnimationNodeStateMachine::STATE_MACHINE_TYPE_NESTED) &&
						!p_state_machine->has_transition_from(current))) {
		// There is no next transition.
		if (fading_from != String()) {
			return Animation::is_greater_approx(
					   current_nti.get_remain(), fadeing_from_nti.get_remain())
					   ? current_nti
					   : fadeing_from_nti;
		}
		return current_nti;
	}

	if (!is_end()) {
		current_nti.is_infinity = true;
	}

	return current_nti;
}

bool AnimationNodeStateMachinePlayback::_transition_to_next_recursive(
	AnimationNode::ProcessState& p_process_state, AnimationNodeInstance& p_instance,
	AnimationTree* p_tree, AnimationNodeStateMachine* p_state_machine, double p_delta,
	bool p_test_only)
{
	_reset_request_for_fading_from = false;

	AnimationMixer::PlaybackInfo pi;
	pi.delta = p_delta;
	NextInfo next;
	Vector<String> transition_path;
	transition_path.push_back(current);
	while (true) {
		next = _find_next(p_process_state, p_instance, p_tree, p_state_machine);

		if (transition_path.has(next.node)) {
			WARN_PRINT_ONCE_ED("AnimationNodeStateMachinePlayback: " + base_path +
							   "playback has detected one or more looped transitions in a single "
							   "frame and aborted to prevent an infinite loop. You may need to "
							   "check the transition settings.");
			break; // Maybe infinity loop, do nothing more.
		}

		transition_path.push_back(next.node);

		// Setting for fading.
		if (next.xfade) {
			// Time to fade.
			fading_from = current;
			fading_time = next.xfade;
			fading_pos = 0;
		}
		else {
			if (reset_request) {
				// There is no possibility of processing doubly. Now we can apply reset actually in
				// here.
				pi.time = 0;
				pi.seeked = true;
				pi.is_external_seeking = false;
				pi.weight = 0;
				AnimationNodeInstance* other_instance =
					p_instance.get_child_instance_by_path_or_null(current);
				p_state_machine->blend_node(p_process_state, p_instance, other_instance, pi,
					AnimationNode::FILTER_IGNORE, true, p_test_only);
			}
			_clear_fading(p_process_state, p_state_machine, current);
			fading_time = 0;
			fading_pos = 0;
		}

		// If it came from path, remove path.
		if (path.size()) {
			path.remove_at(0);
		}

		// Update current status.
		_set_current(p_process_state, p_state_machine, next.node);
		current_curve = next.curve;

		if (current == SceneString(End)) {
			break;
		}

		_reset_request_for_fading_from =
			reset_request; // To avoid processing doubly, it must be reset in the fading process
						   // within _process().
		reset_request = next.is_reset;

		fadeing_from_nti = current_nti;

		AnimationNodeInstance* other_instance =
			p_instance.get_child_instance_by_path_or_null(current);
		if (next.switch_mode == AnimationNodeStateMachineTransition::SWITCH_MODE_SYNC) {
			pi.time = current_nti.position;
			pi.seeked = true;
			pi.is_external_seeking = false;
			pi.weight = 0;
			p_state_machine->blend_node(p_process_state, p_instance, other_instance, pi,
				AnimationNode::FILTER_IGNORE, true, p_test_only);
		}

		// Just get length to find next recursive.
		pi.time = 0;
		pi.is_external_seeking = false;
		pi.weight = 0;
		pi.seeked = next.is_reset;
		current_nti = p_state_machine->blend_node(p_process_state, p_instance, other_instance, pi,
			AnimationNode::FILTER_IGNORE, true,
			true); // Just retrieve remain length, don't process.

		// Fading must be processed.
		if (fading_time) {
			break;
		}
	}

	return next.node == SceneString(End);
}

Ref<AnimationNodeStateMachineTransition> AnimationNodeStateMachinePlayback::_check_group_transition(
	AnimationTree* p_tree, AnimationNodeStateMachine* p_state_machine,
	const AnimationNodeStateMachine::Transition& p_transition,
	Ref<AnimationNodeStateMachine>& r_state_machine, bool& r_bypass) const
{
	Ref<AnimationNodeStateMachineTransition> temp_transition;
	Ref<AnimationNodeStateMachinePlayback> parent_playback;
	if (r_state_machine->get_state_machine_type() ==
		AnimationNodeStateMachine::STATE_MACHINE_TYPE_GROUPED) {
		if (p_transition.from == SceneString(Start)) {
			parent_playback = _get_parent_playback(p_tree);
			if (parent_playback.is_valid()) {
				r_bypass = true;
				temp_transition = parent_playback->_get_group_start_transition();
			}
		}
		else if (p_transition.to == SceneString(End)) {
			parent_playback = _get_parent_playback(p_tree);
			if (parent_playback.is_valid()) {
				temp_transition = parent_playback->_get_group_end_transition();
			}
		}
		if (temp_transition.is_valid()) {
			r_state_machine = _get_parent_state_machine(p_tree);
			return temp_transition;
		}
	}
	return p_transition.transition;
}

AnimationNodeStateMachinePlayback::NextInfo AnimationNodeStateMachinePlayback::_find_next(
	AnimationNode::ProcessState& p_process_state, AnimationNodeInstance& p_instance,
	AnimationTree* p_tree, AnimationNodeStateMachine* p_state_machine) const
{
	NextInfo next;
	if (path.size()) {
		for (int i = 0; i < p_state_machine->transitions.size(); i++) {
			Ref<AnimationNodeStateMachine> anodesm = p_state_machine;
			bool bypass = false;
			Ref<AnimationNodeStateMachineTransition> ref_transition = _check_group_transition(
				p_tree, p_state_machine, p_state_machine->transitions[i], anodesm, bypass);
			if (ref_transition->get_advance_mode() ==
				AnimationNodeStateMachineTransition::ADVANCE_MODE_DISABLED) {
				continue;
			}
			if (p_state_machine->transitions[i].from == current &&
				p_state_machine->transitions[i].to == path[0]) {
				next.node = path[0];
				next.xfade = ref_transition->get_xfade_time();
				next.curve = ref_transition->get_xfade_curve();
				next.switch_mode = ref_transition->get_switch_mode();
				next.is_reset = ref_transition->is_reset();
				next.break_loop_at_end = ref_transition->is_loop_broken_at_end();
			}
		}
	}
	else {
		int auto_advance_to = -1;
		float priority_best = 1e20;
		for (int i = 0; i < p_state_machine->transitions.size(); i++) {
			Ref<AnimationNodeStateMachine> anodesm = p_state_machine;
			bool bypass = false;
			Ref<AnimationNodeStateMachineTransition> ref_transition = _check_group_transition(
				p_tree, p_state_machine, p_state_machine->transitions[i], anodesm, bypass);
			if (ref_transition->get_advance_mode() ==
				AnimationNodeStateMachineTransition::ADVANCE_MODE_DISABLED) {
				continue;
			}
			if (p_state_machine->transitions[i].from == current) {
				if (ref_transition->get_priority() <= priority_best) {
					priority_best = ref_transition->get_priority();
					auto_advance_to = i;
				}
			}
		}

		if (auto_advance_to != -1) {
			next.node = p_state_machine->transitions[auto_advance_to].to;
			Ref<AnimationNodeStateMachine> anodesm = p_state_machine;
			bool bypass = false;
			Ref<AnimationNodeStateMachineTransition> ref_transition =
				_check_group_transition(p_tree, p_state_machine,
					p_state_machine->transitions[auto_advance_to], anodesm, bypass);
			next.xfade = ref_transition->get_xfade_time();
			next.curve = ref_transition->get_xfade_curve();
			next.switch_mode = ref_transition->get_switch_mode();
			next.is_reset = ref_transition->is_reset();
			next.break_loop_at_end = ref_transition->is_loop_broken_at_end();
		}
	}

	return next;
}

void AnimationNodeStateMachinePlayback::clear_path() { path.clear(); }

void AnimationNodeStateMachinePlayback::push_path(const String& p_state)
{
	path.push_back(p_state);
}

void AnimationNodeStateMachinePlayback::_set_base_path(const String& p_base_path)
{
	base_path = p_base_path;
}

Ref<AnimationNodeStateMachine> AnimationNodeStateMachinePlayback::_get_parent_state_machine(
	AnimationTree* p_tree) const
{
	if (base_path.is_empty()) {
		return Ref<AnimationNodeStateMachine>();
	}
	Vector<String> split = base_path.split("/");
	ERR_FAIL_COND_V_MSG(split.size() < 3, Ref<AnimationNodeStateMachine>(), "Path is too short.");
	split = split.slice(1, split.size() - 2);
	Ref<AnimationNode> root = p_tree->get_root_animation_node();
	ERR_FAIL_COND_V_MSG(root.is_null(), Ref<AnimationNodeStateMachine>(),
		"There is no root AnimationNode in AnimationTree: " + String(p_tree->get_name()));
	String anodesm_path = String("/").join(split);
	Ref<AnimationNodeStateMachine> anodesm =
		!anodesm_path.size() ? root : root->find_node_by_path(anodesm_path);
	ERR_FAIL_COND_V_MSG(anodesm.is_null(), Ref<AnimationNodeStateMachine>(),
		"Can't get state machine with path: " + anodesm_path);
	return anodesm;
}

Ref<AnimationNodeStateMachineTransition>
AnimationNodeStateMachinePlayback::_get_group_start_transition() const
{
	ERR_FAIL_COND_V_MSG(group_start_transition.is_null(),
		Ref<AnimationNodeStateMachineTransition>(), "Group start transition is null.");
	return group_start_transition;
}

Ref<AnimationNodeStateMachineTransition>
AnimationNodeStateMachinePlayback::_get_group_end_transition() const
{
	ERR_FAIL_COND_V_MSG(group_end_transition.is_null(), Ref<AnimationNodeStateMachineTransition>(),
		"Group end transition is null.");
	return group_end_transition;
}

AnimationNodeStateMachinePlayback::AnimationNodeStateMachinePlayback()
{
	set_local_to_scene(true); // Only one per instantiated scene.
	default_transition.instantiate();
	default_transition->set_xfade_time(0);
	default_transition->set_reset(true);
	default_transition->set_advance_mode(AnimationNodeStateMachineTransition::ADVANCE_MODE_AUTO);
	default_transition->set_switch_mode(AnimationNodeStateMachineTransition::SWITCH_MODE_IMMEDIATE);
}

///////////////////////////////////////////////////////

bool AnimationNodeStateMachine::is_parameter_read_only(const String& p_parameter) const
{
	if (AnimationNode::is_parameter_read_only(p_parameter)) {
		return true;
	}

	if (p_parameter == playback) {
		return true;
	}
	return false;
}

void AnimationNodeStateMachine::add_node(
	const String& p_name, Ref<AnimationNode> p_node, const Vector2& p_position)
{
	ERR_FAIL_COND(states.has(p_name));
	ERR_FAIL_COND(p_node.is_null());
	ERR_FAIL_COND(String(p_name).contains_char('/'));

	State state_new;
	state_new.node = p_node;
	state_new.position = p_position;

	states[p_name] = state_new;

	_add_node(p_node);
	emit_changed();
	_tree_changed();
}

void AnimationNodeStateMachine::replace_node(const String& p_name, Ref<AnimationNode> p_node)
{
	ERR_FAIL_COND(states.has(p_name) == false);
	ERR_FAIL_COND(p_node.is_null());
	ERR_FAIL_COND(String(p_name).contains_char('/'));

	{
		Ref<AnimationNode> node = states[p_name].node;
		if (node.is_valid()) {
			_remove_node(node);
		}
	}

	states[p_name].node = p_node;

	_add_node(p_node);
	emit_changed();
	_tree_changed();
}

AnimationNodeStateMachine::StateMachineType
AnimationNodeStateMachine::get_state_machine_type() const
{
	return state_machine_type;
}

void AnimationNodeStateMachine::set_allow_transition_to_self(bool p_enable)
{
	allow_transition_to_self = p_enable;
}

bool AnimationNodeStateMachine::is_allow_transition_to_self() const
{
	return allow_transition_to_self;
}

void AnimationNodeStateMachine::set_reset_ends(bool p_enable) { reset_ends = p_enable; }

bool AnimationNodeStateMachine::are_ends_reset() const { return reset_ends; }

Ref<AnimationNode> AnimationNodeStateMachine::get_node(const String& p_name) const
{
	ERR_FAIL_COND_V_EDMSG(
		!states.has(p_name), Ref<AnimationNode>(), String(p_name) + " is not found current state.");

	return states[p_name].node;
}

String AnimationNodeStateMachine::get_node_name(const Ref<AnimationNode>& p_node) const
{
	for (const KeyValue<String, State>& E : states) {
		if (E.value.node == p_node) {
			return E.key;
		}
	}

	ERR_FAIL_V(String());
}

void AnimationNodeStateMachine::get_child_nodes(LocalVector<ChildNode>* r_child_nodes)
{
	Vector<String> nodes;

	for (const KeyValue<String, State>& E : states) {
		nodes.push_back(E.key);
	}

	nodes.sort();

	for (int i = 0; i < nodes.size(); i++) {
		ChildNode cn;
		cn.name = nodes[i];
		cn.node = states[cn.name].node;
		r_child_nodes->push_back(cn);
	}
}

bool AnimationNodeStateMachine::has_node(const String& p_name) const
{
	return states.has(p_name);
}

void AnimationNodeStateMachine::_rename_transitions(
	const String& p_name, const String& p_new_name)
{
	if (updating_transitions) {
		return;
	}

	updating_transitions = true;
	for (int i = 0; i < transitions.size(); i++) {
		if (transitions[i].from == p_name) {
			transitions.write[i].from = p_new_name;
		}
		if (transitions[i].to == p_name) {
			transitions.write[i].to = p_new_name;
		}
	}
	updating_transitions = false;
}

LocalVector<String> AnimationNodeStateMachine::get_node_list() const
{
	LocalVector<String> nodes;
	nodes.reserve(states.size());
	for (const KeyValue<String, State>& E : states) {
		nodes.push_back(E.key);
	}
	nodes.sort();
	return nodes;
}

bool AnimationNodeStateMachine::has_transition(
	const String& p_from, const String& p_to) const
{
	for (int i = 0; i < transitions.size(); i++) {
		if (transitions[i].from == p_from && transitions[i].to == p_to) {
			return true;
		}
	}
	return false;
}

bool AnimationNodeStateMachine::has_transition_from(const String& p_from) const
{
	for (int i = 0; i < transitions.size(); i++) {
		if (transitions[i].from == p_from) {
			return true;
		}
	}
	return false;
}

bool AnimationNodeStateMachine::has_transition_to(const String& p_to) const
{
	for (int i = 0; i < transitions.size(); i++) {
		if (transitions[i].to == p_to) {
			return true;
		}
	}
	return false;
}

int AnimationNodeStateMachine::find_transition(
	const String& p_from, const String& p_to) const
{
	for (int i = 0; i < transitions.size(); i++) {
		if (transitions[i].from == p_from && transitions[i].to == p_to) {
			return i;
		}
	}
	return -1;
}

Vector<int> AnimationNodeStateMachine::find_transition_from(const String& p_from) const
{
	Vector<int> ret;
	for (int i = 0; i < transitions.size(); i++) {
		if (transitions[i].from == p_from) {
			ret.push_back(i);
		}
	}
	return ret;
}

Vector<int> AnimationNodeStateMachine::find_transition_to(const String& p_to) const
{
	Vector<int> ret;
	for (int i = 0; i < transitions.size(); i++) {
		if (transitions[i].to == p_to) {
			ret.push_back(i);
		}
	}
	return ret;
}

bool AnimationNodeStateMachine::_can_connect(const String& p_name)
{
	if (states.has(p_name)) {
		return true;
	}

	String node_name = p_name;
	if (node_name.get_slice_count("/") < 2) {
		return false;
	}

	return false;
}

Ref<AnimationNodeStateMachineTransition> AnimationNodeStateMachine::get_transition(
	int p_transition) const
{
	ERR_FAIL_INDEX_V(p_transition, transitions.size(), Ref<AnimationNodeStateMachineTransition>());
	return transitions[p_transition].transition;
}

String AnimationNodeStateMachine::get_transition_from(int p_transition) const
{
	ERR_FAIL_INDEX_V(p_transition, transitions.size(), String());
	return transitions[p_transition].from;
}

String AnimationNodeStateMachine::get_transition_to(int p_transition) const
{
	ERR_FAIL_INDEX_V(p_transition, transitions.size(), String());
	return transitions[p_transition].to;
}

bool AnimationNodeStateMachine::is_transition_across_group(int p_transition) const
{
	ERR_FAIL_INDEX_V(p_transition, transitions.size(), false);
	if (get_state_machine_type() == AnimationNodeStateMachine::STATE_MACHINE_TYPE_GROUPED) {
		if (transitions[p_transition].from == SceneString(Start) ||
			transitions[p_transition].to == SceneString(End)) {
			return true;
		}
	}
	return false;
}

int AnimationNodeStateMachine::get_transition_count() const { return transitions.size(); }

void AnimationNodeStateMachine::remove_transition(const String& p_from, const String& p_to)
{
	for (int i = 0; i < transitions.size(); i++) {
		if (transitions[i].from == p_from && transitions[i].to == p_to) {
			remove_transition_by_index(i);
			return;
		}
	}
}

void AnimationNodeStateMachine::_remove_transition(
	const Ref<AnimationNodeStateMachineTransition> p_transition)
{
	for (int i = 0; i < transitions.size(); i++) {
		if (transitions[i].transition == p_transition) {
			remove_transition_by_index(i);
			return;
		}
	}
}

void AnimationNodeStateMachine::set_graph_offset(const Vector2& p_offset)
{
	graph_offset = p_offset;
}

Vector2 AnimationNodeStateMachine::get_graph_offset() const { return graph_offset; }

String AnimationNodeStateMachine::get_caption() const { return "StateMachine"; }

Ref<AnimationNode> AnimationNodeStateMachine::get_child_by_name(const String& p_name) const
{
	return get_node(p_name);
}

void AnimationNodeStateMachine::reset_state()
{
	states.clear();
	transitions.clear();
	playback = "playback";
	graph_offset = Vector2();

	Ref<AnimationNode> s;
	s.instantiate();
	s->set_endpoint_type(AnimationNode::ENDPOINT_START);
	State start;
	start.node = s;
	start.position = Vector2(200, 100);
	states[SceneString(Start)] = start;

	Ref<AnimationNode> e;
	e.instantiate();
	e->set_endpoint_type(AnimationNode::ENDPOINT_END);
	State end;
	end.node = e;
	end.position = Vector2(900, 100);
	states[SceneString(End)] = end;

	emit_changed();
	_tree_changed();
}

void AnimationNodeStateMachine::set_node_position(
	const String& p_name, const Vector2& p_position)
{
	ERR_FAIL_COND(!states.has(p_name));
	states[p_name].position = p_position;
}

Vector2 AnimationNodeStateMachine::get_node_position(const String& p_name) const
{
	ERR_FAIL_COND_V(!states.has(p_name), Vector2());
	return states[p_name].position;
}

void AnimationNodeStateMachine::_tree_changed()
{
	emit_changed();
	AnimationNode::_tree_changed();
}

#ifdef TOOLS_ENABLED
void AnimationNodeStateMachine::get_argument_options(
	const String& p_function, int p_idx, List<String>* r_options) const
{
	const String pf = p_function;
	bool add_state_options = false;
	if (p_idx == 0) {
		add_state_options =
			(pf == "get_node" || pf == "has_node" || pf == "rename_node" || pf == "remove_node" ||
				pf == "replace_node" || pf == "set_node_position" || pf == "get_node_position");
	}
	else if (p_idx <= 1) {
		add_state_options =
			(pf == "has_transition" || pf == "add_transition" || pf == "remove_transition");
	}
	if (add_state_options) {
		for (const KeyValue<String, State>& E : states) {
			r_options->push_back(String(E.key).quote());
		}
	}
	AnimationNode::get_argument_options(p_function, p_idx, r_options);
}
#endif

Vector<String> AnimationNodeStateMachine::get_nodes_with_transitions_from(
	const String& p_node) const
{
	Vector<String> result;
	for (const Transition& transition : transitions) {
		if (transition.from == p_node) {
			result.push_back(transition.to);
		}
	}
	return result;
}

Vector<String> AnimationNodeStateMachine::get_nodes_with_transitions_to(
	const String& p_node) const
{
	Vector<String> result;
	for (const Transition& transition : transitions) {
		if (transition.to == p_node) {
			result.push_back(transition.from);
		}
	}
	return result;
}

AnimationNodeStateMachine::AnimationNodeStateMachine()
{
	root_node = true;

	Ref<AnimationNode> s;
	s.instantiate();
	s->set_endpoint_type(AnimationNode::ENDPOINT_START);
	State start;
	start.node = s;
	start.position = Vector2(200, 100);
	states[SceneString(Start)] = start;

	Ref<AnimationNode> e;
	e.instantiate();
	e->set_endpoint_type(AnimationNode::ENDPOINT_END);
	State end;
	end.node = e;
	end.position = Vector2(900, 100);
	states[SceneString(End)] = end;
}

void AnimationNodeStateMachinePlayback::_signal_state_change(
	AnimationTree*, String const&, bool)
{
}

void AnimationNodeStateMachine::remove_transition_by_index(int) {}

void AnimationNodeStateMachinePlayback::_start_children(
	AnimationTree*, AnimationNodeStateMachine*, String const&, bool)
{
}

bool AnimationNodeStateMachinePlayback::_make_travel_path(AnimationNode::ProcessState&,
	AnimationTree*, AnimationNodeStateMachine*, bool, Vector<String>&, bool)
{
	return true;
}

Ref<AnimationNodeStateMachinePlayback> AnimationNodeStateMachinePlayback::_get_parent_playback(
	AnimationTree*) const
{
	Ref<AnimationNodeStateMachinePlayback> ansmp = memnew(AnimationNodeStateMachinePlayback);
	return ansmp;
}

void AnimationNodeStateMachinePlayback::_clear_path_children(
	AnimationNode::ProcessState&, AnimationTree*, AnimationNodeStateMachine*, bool)
{
}

bool AnimationNodeStateMachine::can_edit_node(String const&) const { return true; }

void AnimationNodeStateMachine::validate_node(
	const AnimationTree* p_tree, const String& p_node) const
{
}

AnimationNode::NodeTimeInfo AnimationNodeStateMachine::_process(
	AnimationNode::ProcessState& r_process_state, AnimationNodeInstance& p_instance,
	const AnimationMixer::PlaybackInfo& p_playback_info, bool p_test_only)
{
	return AnimationNode::NodeTimeInfo();
}

// ============================================================================
// Merged from scene/animation/animation_mixer.cpp
// ============================================================================

#include "scene/animation/animation_mixer.h"
#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/string/ustring.h"
#include "scene/2d/node_2d.h"
#include "scene/animation/animation_mixer.h"
#include "scene/animation/animation_mixer.h"
#include "scene/resources/animation.h"
#include "servers/audio/audio_server.h"
#include "servers/audio/audio_stream.h"

#ifndef _3D_DISABLED
#include "scene/3d/camera_3d.h"
#include "scene/3d/mesh_instance_3d.h"
#include "scene/3d/node_3d.h"
#include "scene/3d/skeleton_3d.h"
#endif // _3D_DISABLED

void AnimationMixer::_animation_added(const String& p_name, const String& p_library)
{
	_animation_set_cache_update();
}

void AnimationMixer::_animation_removed(const String& p_name, const String& p_library)
{
	const String name =
		p_library == String() ? p_name : String(String(p_library) + "/" + String(p_name));

	if (!animation_set.has(name)) {
		return; // No need to update because not the one from the library being used.
	}

	_animation_set_cache_update();

	_remove_animation(name);
}

void AnimationMixer::_animation_changed(const String& p_name) { _clear_caches(); }

void AnimationMixer::get_animation_library_list(LocalVector<String>* p_libraries) const
{
	for (const AnimationLibraryData& lib : animation_libraries) {
		p_libraries->push_back(lib.name);
	}
}

Ref<AnimationLibrary> AnimationMixer::get_animation_library(const String& p_name) const
{
	for (const AnimationLibraryData& lib : animation_libraries) {
		if (lib.name == p_name) {
			return lib.library;
		}
	}
	ERR_FAIL_V(Ref<AnimationLibrary>());
}

bool AnimationMixer::has_animation_library(const String& p_name) const
{
	for (const AnimationLibraryData& lib : animation_libraries) {
		if (lib.name == p_name) {
			return true;
		}
	}

	return false;
}

const String& AnimationMixer::find_animation_library(const Ref<Animation>& p_animation) const
{
	for (const KeyValue<String, AnimationData>& E : animation_set) {
		if (E.value.animation == p_animation) {
			return E.value.animation_library;
		}
	}
	const static String empty = String();
	return empty;
}

LocalVector<String> AnimationMixer::get_sorted_animation_list() const
{
	LocalVector<String> animations;
	get_animation_list(&animations);
	animations.sort();
	return animations;
}

void AnimationMixer::get_animation_list(LocalVector<String>* p_animations) const
{
	p_animations->reserve(p_animations->size() + animation_set.size());
	for (const KeyValue<String, AnimationData>& E : animation_set) {
		p_animations->push_back(E.key);
	}
}

const Ref<Animation>& AnimationMixer::get_animation(const String& p_name) const
{
	const Ref<Animation>& animation = get_animation_or_null(p_name);
	ERR_FAIL_COND_V_MSG(
		animation.is_null(), animation, vformat("Animation not found: \"%s\".", p_name));
	return animation;
}

const Ref<Animation>& AnimationMixer::get_animation_or_null(const String& p_name) const
{
	const AnimationData* ad = animation_set.getptr(p_name);
	if (!ad) {
		const static Ref<Animation> empty = Ref<Animation>();
		return empty;
	}
	return ad->animation;
}

bool AnimationMixer::has_animation(const String& p_name) const
{
	return animation_set.has(p_name);
}

String AnimationMixer::find_animation(const Ref<Animation>& p_animation) const
{
	for (const KeyValue<String, AnimationData>& E : animation_set) {
		if (E.value.animation == p_animation) {
			return E.key;
		}
	}
	return String();
}

void AnimationMixer::_set_process(bool p_process, bool p_force)
{
	if (processing == p_process && !p_force) {
		return;
	}

	switch (callback_mode_process) {
	case ANIMATION_CALLBACK_MODE_PROCESS_PHYSICS:
#ifdef TOOLS_ENABLED
		set_physics_process_internal(p_process && active && !editing);
#else
		set_physics_process_internal(p_process && active);
#endif // TOOLS_ENABLED
		break;
	case ANIMATION_CALLBACK_MODE_PROCESS_IDLE:
#ifdef TOOLS_ENABLED
		set_process_internal(p_process && active && !editing);
#else
		set_process_internal(p_process && active);
#endif // TOOLS_ENABLED
		break;
	case ANIMATION_CALLBACK_MODE_PROCESS_MANUAL:
		break;
	}

	processing = p_process;
}

void AnimationMixer::set_active(bool p_active)
{
	if (active == p_active) {
		return;
	}

	active = p_active;
	_set_active(active);
	_set_process(processing, true);

	if (!active && is_inside_tree()) {
		_clear_caches();
	}
}

bool AnimationMixer::is_active() const { return active; }

void AnimationMixer::set_root_node(const NodePath& p_path)
{
	root_node = p_path;
	_clear_caches();
}

NodePath AnimationMixer::get_root_node() const { return root_node; }

void AnimationMixer::set_deterministic(bool p_deterministic)
{
	deterministic = p_deterministic;
	_clear_caches();
}

bool AnimationMixer::is_deterministic() const { return deterministic; }

void AnimationMixer::set_callback_mode_process(AnimationCallbackModeProcess p_mode)
{
	if (callback_mode_process == p_mode) {
		return;
	}

	bool was_active = is_active();
	if (was_active) {
		set_active(false);
	}

	callback_mode_process = p_mode;

	if (was_active) {
		set_active(true);
	}
}

AnimationMixer::AnimationCallbackModeProcess AnimationMixer::get_callback_mode_process() const
{
	return callback_mode_process;
}

AnimationMixer::AnimationCallbackModeMethod AnimationMixer::get_callback_mode_method() const
{
	return callback_mode_method;
}

AnimationMixer::AnimationCallbackModeDiscrete AnimationMixer::get_callback_mode_discrete() const
{
	return callback_mode_discrete;
}

void AnimationMixer::set_audio_max_polyphony(int p_audio_max_polyphony)
{
	ERR_FAIL_COND(p_audio_max_polyphony < 0 || p_audio_max_polyphony > 128);
	audio_max_polyphony = p_audio_max_polyphony;
}

int AnimationMixer::get_audio_max_polyphony() const { return audio_max_polyphony; }

#ifdef TOOLS_ENABLED

bool AnimationMixer::is_editing() const { return editing; }

void AnimationMixer::set_dummy(bool p_dummy) { dummy = p_dummy; }

bool AnimationMixer::is_dummy() const { return dummy; }
#endif // TOOLS_ENABLED

void AnimationMixer::_clear_caches(bool p_clear_track_cache) {}

void AnimationMixer::_init_root_motion_cache()
{
	root_motion_cache.loc = Vector3(0, 0, 0);
	root_motion_cache.rot = Quaternion(0, 0, 0, 1);
	root_motion_cache.scale = Vector3(1, 1, 1);
	root_motion_position = Vector3(0, 0, 0);
	root_motion_rotation = Quaternion(0, 0, 0, 1);
	root_motion_scale = Vector3(0, 0, 0);
	root_motion_position_accumulator = Vector3(0, 0, 0);
	root_motion_rotation_accumulator = Quaternion(0, 0, 0, 1);
	root_motion_scale_accumulator = Vector3(1, 1, 1);
}

void AnimationMixer::_create_track_num_to_track_cache_for_animation(
	const Ref<Animation>& p_animation)
{
	if (animation_track_num_to_track_cache.has(p_animation)) {
		// In AnimationMixer::_update_caches, it retrieves all animations via
		// AnimationMixer::get_animation_list Since multiple AnimationLibraries can share the same
		// Animation, it is possible that the cache is already created.
		return;
	}
	LocalVector<TrackCache*>& track_num_to_track_cache =
		animation_track_num_to_track_cache.insert_new(p_animation, LocalVector<TrackCache*>())
			->value;
	const LocalVector<Animation::Track*>& tracks = p_animation->get_tracks();

	track_num_to_track_cache.resize(tracks.size());
	for (uint32_t i = 0; i < tracks.size(); i++) {
		TrackCache** track_ptr = track_cache.getptr(tracks[i]->get_unique_id());
		if (track_ptr == nullptr) {
			track_num_to_track_cache[i] = nullptr;
		}
		else {
			track_num_to_track_cache[i] = *track_ptr;
		}
	}
}

bool AnimationMixer::_blend_pre_process(
	double p_delta, int p_track_count, const AHashMap<NodePath, int>& p_track_map)
{
	return true;
}

void AnimationMixer::_blend_capture(double p_delta) { blend_capture(p_delta); }

void AnimationMixer::blend_capture(double p_delta)
{
	if (capture_cache.animation.is_null()) {
		return;
	}

	capture_cache.remain -= p_delta * capture_cache.step;
	if (Animation::is_less_or_equal_approx(capture_cache.remain, 0)) {
		if (capture_cache.animation.is_valid()) {
			animation_track_num_to_track_cache.erase(capture_cache.animation);
		}
		capture_cache.clear();
		return;
	}

	real_t weight = Tween::run_equation(
		capture_cache.trans_type, capture_cache.ease_type, capture_cache.remain, 0.0, 1.0, 1.0);

	// Blend with other animations.
	real_t inv = 1.0 - weight;
	for (AnimationInstance& ai : animation_instances) {
		ai.playback_info.weight *= inv;
	}

	// Build capture animation instance.
	PlaybackInfo pi;
	pi.weight = weight;

	AnimationInstance ai;
	ai.animation = capture_cache.animation;
	ai.playback_info = pi;

	animation_instances.push_back(ai);
}

void AnimationMixer::_blend_calc_total_weight()
{
	for (const AnimationInstance& ai : animation_instances) {
		const Ref<Animation>& a = ai.animation;
		real_t weight = ai.playback_info.weight;
		if (Math::is_zero_approx(weight)) {
			continue;
		}
		Span<real_t> track_weights = ai.playback_info.track_weights != nullptr
										 ? *ai.playback_info.track_weights
										 : Span<real_t>();

		LocalVector<TrackCache*>* t_cache = animation_track_num_to_track_cache.getptr(a);
		ERR_CONTINUE_EDMSG(!t_cache, "No animation in cache.");
		LocalVector<TrackCache*>& track_num_to_track_cache = *t_cache;

		uint64_t pass_id = ++animation_instance_weight_pass_counter;
		// Handle wrap (slower but rare).
		if (unlikely(pass_id == 0)) {
			for (KeyValue<Animation::TrackCacheID, TrackCache*>& kv : track_cache) {
				if (kv.value) {
					kv.value->animation_instance_weight_applied_at = 0;
				}
			}
			animation_instance_weight_pass_counter = 1;
			pass_id = 1;
		}

		const LocalVector<Animation::Track*>& tracks = a->get_tracks();
		Animation::Track* const* tracks_ptr = tracks.ptr();
		int count = tracks.size();
		for (int i = 0; i < count; i++) {
			Animation::Track* animation_track = tracks_ptr[i];
			if (!animation_track->enabled) {
				continue;
			}
			TrackCache* track = track_num_to_track_cache[i];
			if (track == nullptr) {
				// No path, but avoid error spamming.
				continue;
			}

			// In some cases (e.g. transform TrackCache),
			// multiple Animation::Tracks (e.g. TYPE_POSITION_3D, TYPE_ROTATION_3D and
			// TYPE_SCALE_3D) can point to the same TrackCache instance. So we need to make sure
			// that the weight is added only once per AnimationInstance.
			if (track->animation_instance_weight_applied_at == pass_id) {
				continue;
			}

			int blend_idx = track->blend_idx;
			ERR_CONTINUE(blend_idx < 0 || blend_idx >= track_count);
			real_t blend;
			if (!track_weights.is_empty() && blend_idx < static_cast<int>(track_weights.size())) {
				blend = track_weights[blend_idx] * weight;
			}
			else {
				blend = weight;
			}
			track->total_weight += blend;
			track->animation_instance_weight_applied_at = pass_id;
		}
	}
}

void AnimationMixer::make_animation_instance(
	const String& p_name, const PlaybackInfo& p_playback_info)
{
	const Ref<Animation>& animation = get_animation_or_null(p_name);
	ERR_FAIL_COND(animation.is_null());

	AnimationInstance ai;
	ai.animation = animation;
	ai.playback_info = p_playback_info;

	animation_instances.push_back(std::move(ai));
}

void AnimationMixer::clear_animation_instances() { animation_instances.clear(); }

void AnimationMixer::advance(double p_time) { _process_animation(p_time); }

void AnimationMixer::clear_caches() { _clear_caches(); }

NodePath AnimationMixer::get_root_motion_track() const { return root_motion_track; }

void AnimationMixer::set_root_motion_local(bool p_enabled) { root_motion_local = p_enabled; }

bool AnimationMixer::is_root_motion_local() const { return root_motion_local; }

Vector3 AnimationMixer::get_root_motion_position() const { return root_motion_position; }

Quaternion AnimationMixer::get_root_motion_rotation() const { return root_motion_rotation; }

Vector3 AnimationMixer::get_root_motion_scale() const { return root_motion_scale; }

Vector3 AnimationMixer::get_root_motion_position_accumulator() const
{
	return root_motion_position_accumulator;
}

Quaternion AnimationMixer::get_root_motion_rotation_accumulator() const
{
	return root_motion_rotation_accumulator;
}

Vector3 AnimationMixer::get_root_motion_scale_accumulator() const
{
	return root_motion_scale_accumulator;
}

void AnimationMixer::set_reset_on_save_enabled(bool p_enabled) { reset_on_save = p_enabled; }

bool AnimationMixer::is_reset_on_save_enabled() const { return reset_on_save; }

bool AnimationMixer::can_apply_reset() const { return has_animation(SceneString(RESET)); }

void AnimationMixer::_node_removed(Node* p_node) { _clear_caches(); }

void AnimationMixer::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_ENTER_TREE: {
		if (!processing) {
			set_physics_process_internal(false);
			set_process_internal(false);
		}
		_clear_caches();
	} break;

	case NOTIFICATION_INTERNAL_PROCESS: {
		if (active && callback_mode_process == ANIMATION_CALLBACK_MODE_PROCESS_IDLE) {
			_process_animation(get_process_delta_time());
		}
	} break;

	case NOTIFICATION_INTERNAL_PHYSICS_PROCESS: {
		if (active && callback_mode_process == ANIMATION_CALLBACK_MODE_PROCESS_PHYSICS) {
			_process_animation(get_physics_process_delta_time());
		}
	} break;

	case NOTIFICATION_EXIT_TREE: {
		_clear_caches();
	} break;
	}
}

#ifdef TOOLS_ENABLED
void AnimationMixer::get_argument_options(
	const String& p_function, int p_idx, List<String>* r_options) const
{
	const String pf = p_function;
	if (p_idx == 0) {
		if (pf == "get_animation" || pf == "has_animation") {
			for (const String& name : get_sorted_animation_list()) {
				r_options->push_back(String(name).quote());
			}
		}
		else if (pf == "get_animation_library" || pf == "has_animation_library" ||
				   pf == "remove_animation_library" || pf == "rename_animation_library") {
			LocalVector<String> al;
			get_animation_library_list(&al);
			for (const String& name : al) {
				r_options->push_back(String(name).quote());
			}
		}
	}
	Node::get_argument_options(p_function, p_idx, r_options);
}
#endif

AnimationMixer::AnimationMixer() { root_node = NodePath(".."); }

AnimationMixer::~AnimationMixer() {}

void AnimatedValuesBackup::set_data(
	const AHashMap<Animation::TrackCacheID, AnimationMixer::TrackCache*, HashHasher>& p_data)
{
	clear_data();

	for (const KeyValue<Animation::TrackCacheID, AnimationMixer::TrackCache*>& E : p_data) {
		AnimationMixer::TrackCache* track = get_cache_copy(E.value);
		if (!track) {
			continue; // Some types of tracks do not get a copy and must be ignored.
		}

		data.insert(E.key, track);
	}
}

AHashMap<Animation::TrackCacheID, AnimationMixer::TrackCache*, HashHasher>
AnimatedValuesBackup::get_data() const
{
	AHashMap<Animation::TrackCacheID, AnimationMixer::TrackCache*, HashHasher> ret;
	for (const KeyValue<Animation::TrackCacheID, AnimationMixer::TrackCache*>& E : data) {
		AnimationMixer::TrackCache* track = get_cache_copy(E.value);
		ERR_CONTINUE(
			!track); // Backup shouldn't contain tracks that cannot be copied, this is a mistake.

		ret.insert(E.key, track);
	}
	return ret;
}

void AnimatedValuesBackup::clear_data()
{
	for (KeyValue<Animation::TrackCacheID, AnimationMixer::TrackCache*>& K : data) {
		memdelete(K.value);
	}
	data.clear();
}

AnimationMixer::TrackCache* AnimatedValuesBackup::get_cache_copy(
	AnimationMixer::TrackCache* p_cache) const
{
	switch (p_cache->type) {
	case Animation::TYPE_BEZIER:
	case Animation::TYPE_VALUE:
	case Animation::TYPE_POSITION_3D:
	case Animation::TYPE_ROTATION_3D:
	case Animation::TYPE_SCALE_3D:
	case Animation::TYPE_BLEND_SHAPE:
	case Animation::TYPE_AUDIO: {
		return memnew(AnimationMixer::TrackCache(*p_cache));
	}

	case Animation::TYPE_METHOD:
	case Animation::TYPE_ANIMATION: {
		// Nothing to do here.
	} break;
	}
	return nullptr;
}

void AnimationMixer::_animation_set_cache_update() {}

void AnimationMixer::_remove_animation(const String& p_name) {}

void AnimationMixer::_process_animation(double p_delta, bool p_update_only) {}

void AnimationMixer::_animation_renamed(
	const String& p_name, const String& p_to_name, const String& p_library)
{
}

void AnimationMixer::_set_active(bool p_active) {}

uint32_t AnimationMixer::_get_libraries_property_usage() const { return 0; }

void AnimationMixer::_blend_post_process() {}

// ==========================================================================
// Merged from scene/animation/animation_player.cpp
// ==========================================================================

// --- animation_player.compat.inc ---
#ifndef DISABLE_DEPRECATED

#include "scene/animation/animation_mixer.h"

void AnimationPlayer::_set_process_callback_bind_compat_80813(
	AnimationPlayer::AnimationProcessCallback p_mode)
{
	set_callback_mode_process(
		static_cast<AnimationMixer::AnimationCallbackModeProcess>(static_cast<int>(p_mode)));
}

AnimationPlayer::AnimationProcessCallback
AnimationPlayer::_get_process_callback_bind_compat_80813() const
{
	return static_cast<AnimationProcessCallback>(static_cast<int>(get_callback_mode_process()));
}

AnimationPlayer::AnimationMethodCallMode
AnimationPlayer::_get_method_call_mode_bind_compat_80813() const
{
	return static_cast<AnimationMethodCallMode>(static_cast<int>(get_callback_mode_method()));
}

void AnimationPlayer::_set_root_bind_compat_80813(const NodePath& p_root) { set_root_node(p_root); }

NodePath AnimationPlayer::_get_root_bind_compat_80813() const { return get_root_node(); }

void AnimationPlayer::_seek_bind_compat_80813(double p_time, bool p_update)
{
	seek(p_time, p_update, false);
}

String AnimationPlayer::_get_current_animation_bind_compat_110767() const
{
	return get_current_animation();
}

void AnimationPlayer::_set_current_animation_bind_compat_110767(const String& p_animation)
{
	set_current_animation(p_animation);
}

String AnimationPlayer::_get_assigned_animation_bind_compat_110767() const
{
	return get_assigned_animation();
}

void AnimationPlayer::_set_assigned_animation_bind_compat_110767(const String& p_animation)
{
	set_assigned_animation(p_animation);
}

String AnimationPlayer::_get_autoplay_bind_compat_110767() const { return get_autoplay(); }

void AnimationPlayer::_set_autoplay_bind_compat_110767(const String& p_name)
{
	set_autoplay(p_name);
}

void AnimationPlayer::_bind_compatibility_methods() {}

#endif // DISABLE_DEPRECATED
#include "scene/animation/animation_mixer.h"
#include "core/config/engine.h"
#include "core/os/os.h"
#include "scene/main/scene_tree.h"

void AnimationPlayer::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_READY: {
		if (!Engine::is_editor_hint() && animation_set.has(autoplay)) {
			set_active(active);
			play(autoplay);
			_check_immediately_after_start();
		}
	} break;
	}
}

void AnimationPlayer::_process_playback_data(PlaybackData& cd, double p_delta, float p_blend,
	bool p_seeked, bool p_internal_seeked, bool p_started, bool p_is_current)
{
	double speed = speed_scale * cd.speed_scale;
	bool backwards = std::signbit(speed); // Negative zero means playing backwards too.
	double delta = p_started ? 0 : p_delta * speed;
	double next_pos = cd.pos + delta;

	double start = cd.get_start_time();
	double end = cd.get_end_time();
	AnimationData* p_from = &animation_set[cd.animation_name];

	Animation::LoopedFlag looped_flag = Animation::LOOPED_FLAG_NONE;

	switch (p_from->animation->get_loop_mode()) {
	case Animation::LOOP_NONE: {
		if (Animation::is_less_approx(next_pos, start)) {
			next_pos = start;
		}
		else if (Animation::is_greater_approx(next_pos, end)) {
			next_pos = end;
		}
		delta = next_pos - cd.pos; // Fix delta (after determination of backwards because negative
								   // zero is lost here).
	} break;

	case Animation::LOOP_LINEAR: {
		if (Animation::is_less_approx(next_pos, start) &&
			Animation::is_greater_or_equal_approx(cd.pos, start)) {
			looped_flag = Animation::LOOPED_FLAG_START;
		}
		if (Animation::is_greater_approx(next_pos, end) &&
			Animation::is_less_or_equal_approx(cd.pos, end)) {
			looped_flag = Animation::LOOPED_FLAG_END;
		}
		next_pos = Math::fposmod(next_pos - start, end - start) + start;
	} break;

	case Animation::LOOP_PINGPONG: {
		if (Animation::is_less_approx(next_pos, start) &&
			Animation::is_greater_or_equal_approx(cd.pos, start)) {
			cd.speed_scale *= -1.0;
			looped_flag = Animation::LOOPED_FLAG_START;
		}
		if (Animation::is_greater_approx(next_pos, end) &&
			Animation::is_less_or_equal_approx(cd.pos, end)) {
			cd.speed_scale *= -1.0;
			looped_flag = Animation::LOOPED_FLAG_END;
		}
		next_pos = Math::pingpong(next_pos - start, end - start) + start;
	} break;

	default:
		break;
	}

	double prev_pos = cd.pos; // The animation may be changed during process, so it is safer that
							  // the state is changed before process.

	// End detection.
	if (p_is_current) {
		if (p_from->animation->get_loop_mode() == Animation::LOOP_NONE) {
			if (!backwards && Animation::is_less_or_equal_approx(prev_pos, end) &&
				Math::is_equal_approx(next_pos, end)) {
				// Playback finished.
				next_pos = end; // Snap to the edge.
				end_reached = true;
				end_notify = Animation::is_less_approx(
					prev_pos, end); // Notify only if not already at the end.
				p_blend = 1.0;
			}
			if (backwards && Animation::is_greater_or_equal_approx(prev_pos, start) &&
				Math::is_equal_approx(next_pos, start)) {
				// Playback finished.
				next_pos = start; // Snap to the edge.
				end_reached = true;
				end_notify = Animation::is_greater_approx(
					prev_pos, start); // Notify only if not already at the beginning.
				p_blend = 1.0;
			}
		}
	}

	cd.pos = next_pos;

	PlaybackInfo pi;
	if (p_started) {
		pi.time = prev_pos;
		pi.delta = 0;
		pi.start = start;
		pi.end = end;
		pi.seeked = true;
	}
	else {
		pi.time = next_pos;
		pi.delta = delta;
		pi.start = start;
		pi.end = end;
		pi.seeked = p_seeked;
	}
	if (Math::is_zero_approx(pi.delta) && backwards) {
		pi.delta = -0.0; // Sign is needed to handle converted Continuous track from Discrete track
						 // correctly.
	}
	// Immediately after playback, discrete keys should be retrieved with EXACT mode since behind
	// keys must be ignored at that time.
	pi.is_external_seeking = !p_internal_seeked && !p_started;
	pi.looped_flag = looped_flag;
	pi.weight = p_blend;
	make_animation_instance(cd.animation_name, pi);
}

float AnimationPlayer::get_current_blend_amount()
{
	Playback& c = playback;
	float blend = 1.0;
	for (const Blend& b : c.blend) {
		blend = blend - b.blend_left;
	}
	return MAX(0, blend);
}

void AnimationPlayer::_blend_playback_data(double p_delta, bool p_started)
{
	Playback& c = playback;

	bool seeked = c.seeked; // The animation may be changed during process, so it is safer that the
							// state is changed before process.
	bool internal_seeked = c.internal_seeked;

	if (!Math::is_zero_approx(p_delta)) {
		c.seeked = false;
		c.internal_seeked = false;
	}

	// Second, process current animation to check if the animation end reached.
	_process_playback_data(
		c.current, p_delta, get_current_blend_amount(), seeked, internal_seeked, p_started, true);

	// Finally, if not end the animation, do blending.
	if (end_reached) {
		playback.blend.clear();
		if (end_notify) {
			finished_anim = playback.assigned;
		}
		return;
	}
	LocalVector<int> to_erase;
	for (uint32_t i = 0; i < c.blend.size(); i++) {
		Blend& b = c.blend[i];
		b.blend_left = MAX(0, b.blend_left - Math::abs(speed_scale * p_delta) / b.blend_time);
		if (Animation::is_less_or_equal_approx(b.blend_left, 0)) {
			to_erase.push_back(i);
			b.blend_left = CMP_EPSILON; // May want to play last frame.
		}
		// Note: There may be issues if an animation event triggers an animation change while this
		// blend is active, so it is best to use "deferred" calls instead of "immediate" for
		// animation events that can trigger new animations.
		_process_playback_data(b.data, p_delta, b.blend_left, false, false, false);
	}
	for (int i = to_erase.size() - 1; i >= 0; i--) {
		c.blend.remove_at(to_erase[i]);
	}
}

void AnimationPlayer::_blend_capture(double p_delta)
{
	blend_capture(p_delta * Math::abs(speed_scale));
}

void AnimationPlayer::queue(const String& p_name)
{
	if (!is_playing()) {
		play(p_name);
	}
	else {
		playback_queue.push_back(p_name);
	}
}

void AnimationPlayer::clear_queue() { playback_queue.clear(); }

void AnimationPlayer::play_backwards(const String& p_name, double p_custom_blend)
{
	play(p_name, p_custom_blend, -1, true);
}

void AnimationPlayer::play_section_with_markers_backwards(const String& p_name,
	const String& p_start_marker, const String& p_end_marker, double p_custom_blend)
{
	play_section_with_markers(p_name, p_start_marker, p_end_marker, p_custom_blend, -1, true);
}

void AnimationPlayer::play_section_backwards(
	const String& p_name, double p_start_time, double p_end_time, double p_custom_blend)
{
	play_section(p_name, p_start_time, p_end_time, p_custom_blend, -1, true);
}

void AnimationPlayer::play(
	const String& p_name, double p_custom_blend, float p_custom_scale, bool p_from_end)
{
	if (auto_capture) {
		play_with_capture(p_name, auto_capture_duration, p_custom_blend, p_custom_scale, p_from_end,
			auto_capture_transition_type, auto_capture_ease_type);
	}
	else {
		_play(p_name, p_custom_blend, p_custom_scale, p_from_end);
	}
}

void AnimationPlayer::_play(
	const String& p_name, double p_custom_blend, float p_custom_scale, bool p_from_end)
{
	play_section_with_markers(
		p_name, String(), String(), p_custom_blend, p_custom_scale, p_from_end);
}

void AnimationPlayer::play_section_with_markers(const String& p_name,
	const String& p_start_marker, const String& p_end_marker, double p_custom_blend,
	float p_custom_scale, bool p_from_end)
{
	String name = p_name;

	if (name == String()) {
		name = playback.assigned;
	}

	AnimationData* ad = animation_set.getptr(name);
	ERR_FAIL_NULL_MSG(ad, vformat("Animation not found: %s.", name));

	const Ref<Animation>& animation = ad->animation;

	ERR_FAIL_COND_MSG(p_start_marker == p_end_marker && !p_start_marker.is_empty(),
		vformat("Start marker and end marker cannot be the same marker: %s.", p_start_marker));
	ERR_FAIL_COND_MSG(!p_start_marker.is_empty() && !animation->has_marker(p_start_marker),
		vformat("Marker %s not found in animation: %s.", p_start_marker, name));
	ERR_FAIL_COND_MSG(!p_end_marker.is_empty() && !animation->has_marker(p_end_marker),
		vformat("Marker %s not found in animation: %s.", p_end_marker, name));

	double start_time = !p_start_marker.is_empty() ? animation->get_marker_time(p_start_marker) : -1;
	double end_time = !p_end_marker.is_empty() ? animation->get_marker_time(p_end_marker) : -1;

	ERR_FAIL_COND_MSG(
		!p_start_marker.is_empty() && !p_end_marker.is_empty() && Animation::is_greater_approx(start_time, end_time),
		vformat("End marker %s is placed earlier than start marker %s in animation: %s.",
			p_end_marker, p_start_marker, name));

	if (!p_start_marker.is_empty() && Animation::is_less_approx(start_time, 0)) {
		WARN_PRINT_ED(vformat("Negative time start marker: %s is invalid in the section, so the "
							  "start of the animation: %s is used instead.",
			p_start_marker, playback.current.animation_name));
	}
	if (!p_end_marker.is_empty() && Animation::is_less_approx(end_time, 0)) {
		WARN_PRINT_ED(vformat("Negative time end marker: %s is invalid in the section, so the end "
							  "of the animation: %s is used instead.",
			p_end_marker, playback.current.animation_name));
	}

	play_section(name, start_time, end_time, p_custom_blend, p_custom_scale, p_from_end);
}

void AnimationPlayer::play_with_capture(const String& p_name, double p_duration,
	double p_custom_blend, float p_custom_scale, bool p_from_end,
	Tween::TransitionType p_trans_type, Tween::EaseType p_ease_type)
{
	_capture(p_name, p_from_end, p_duration, p_trans_type, p_ease_type);
	_play(p_name, p_custom_blend, p_custom_scale, p_from_end);
}

bool AnimationPlayer::is_playing() const { return playing; }

String AnimationPlayer::get_current_animation() const
{
	return (is_playing() ? playback.assigned : String());
}

String AnimationPlayer::get_assigned_animation() const { return playback.assigned; }

void AnimationPlayer::pause() { _stop_internal(false, false); }

void AnimationPlayer::stop(bool p_keep_state) { _stop_internal(true, p_keep_state); }

void AnimationPlayer::set_speed_scale(float p_speed) { speed_scale = p_speed; }

float AnimationPlayer::get_speed_scale() const { return speed_scale; }

float AnimationPlayer::get_playing_speed() const
{
	if (!playing) {
		return 0;
	}
	return speed_scale * playback.current.speed_scale;
}

void AnimationPlayer::seek_internal(
	double p_time, bool p_update, bool p_update_only, bool p_is_internal_seek)
{
	if (!active) {
		return;
	}

	bool is_backward = Animation::is_less_approx(p_time, playback.current.pos);

	_check_immediately_after_start();

	playback.current.pos = p_time;
	if (!playback.current.is_enabled) {
		if (!playback.assigned.is_empty()) {
			AnimationData* ad = animation_set.getptr(playback.assigned);
			ERR_FAIL_NULL_MSG(ad, vformat("Animation not found: %s.", playback.assigned));
			playback.current.is_enabled = true;
			playback.current.animation_name = playback.assigned;
			playback.current.animation_length = ad->animation->get_length();
		}
		if (!playback.current.is_enabled) {
			return; // There is no animation.
		}
	}

	double start = playback.current.get_start_time();
	double end = playback.current.get_end_time();

	// Clamp the seek position.
	p_time = CLAMP(p_time, start, end);

	playback.seeked = true;
	playback.internal_seeked = p_is_internal_seek;

	if (p_update) {
		_process_animation(is_backward ? -0.0 : 0.0, p_update_only);
		playback.seeked =
			false; // If animation was proceeded here, no more seek in internal process.
	}
}

void AnimationPlayer::seek(double p_time, bool p_update, bool p_update_only)
{
	seek_internal(p_time, p_update, p_update_only);
}

void AnimationPlayer::advance(double p_time)
{
	_check_immediately_after_start();
	AnimationMixer::advance(p_time);
}

void AnimationPlayer::_check_immediately_after_start()
{
	if (playback.started) {
		_process_animation(
			0); // Force process current key for Discrete/Method/Audio/AnimationPlayback. Then,
				// started flag is cleared.
	}
}

bool AnimationPlayer::is_valid() const { return playback.current.is_enabled; }

double AnimationPlayer::get_current_animation_position() const
{
	ERR_FAIL_COND_V_MSG(
		!playback.current.is_enabled, 0, "AnimationPlayer has no current animation.");
	return playback.current.pos;
}

double AnimationPlayer::get_current_animation_length() const
{
	ERR_FAIL_COND_V_MSG(
		!playback.current.is_enabled, 0, "AnimationPlayer has no current animation.");
	return playback.current.animation_length;
}

void AnimationPlayer::set_section_with_markers(
	const String& p_start_marker, const String& p_end_marker)
{
	ERR_FAIL_COND_MSG(!playback.current.is_enabled, "AnimationPlayer has no current animation.");
	ERR_FAIL_COND_MSG(p_start_marker == p_end_marker && !p_start_marker.is_empty(),
		vformat("Start marker and end marker cannot be the same marker: %s.", p_start_marker));
	ERR_FAIL_COND_MSG(
		!p_start_marker.is_empty() &&
			!animation_set[playback.current.animation_name].animation->has_marker(p_start_marker),
		vformat("Marker %s not found in animation: %s.", p_start_marker,
			playback.current.animation_name));
	ERR_FAIL_COND_MSG(
		!p_end_marker.is_empty() &&
			!animation_set[playback.current.animation_name].animation->has_marker(p_end_marker),
		vformat("Marker %s not found in animation: %s.", p_end_marker,
			playback.current.animation_name));
	double start_time =
		!p_start_marker.is_empty() ? animation_set[playback.current.animation_name].animation->get_marker_time(
							 p_start_marker)
					   : -1;
	double end_time =
		!p_end_marker.is_empty() ? animation_set[playback.current.animation_name].animation->get_marker_time(
						   p_end_marker)
					 : -1;
	if (!p_start_marker.is_empty() && Animation::is_less_approx(start_time, 0)) {
		WARN_PRINT_ONCE_ED(vformat("Marker %s time must be positive in animation: %s.",
			p_start_marker, playback.current.animation_name));
	}
	if (!p_end_marker.is_empty() && Animation::is_less_approx(end_time, 0)) {
		WARN_PRINT_ONCE_ED(vformat("Marker %s time must be positive in animation: %s.",
			p_end_marker, playback.current.animation_name));
	}
	set_section(start_time, end_time);
}

void AnimationPlayer::set_section(double p_start_time, double p_end_time)
{
	ERR_FAIL_COND_MSG(!playback.current.is_enabled, "AnimationPlayer has no current animation.");
	ERR_FAIL_COND_MSG(Animation::is_greater_or_equal_approx(p_start_time, 0) &&
						  Animation::is_greater_or_equal_approx(p_end_time, 0) &&
						  Animation::is_greater_or_equal_approx(p_start_time, p_end_time),
		vformat("Start time %f is greater than end time %f.", p_start_time, p_end_time));
	playback.current.start_time = p_start_time;
	playback.current.end_time = p_end_time;
	playback.current.pos = CLAMP(
		playback.current.pos, playback.current.get_start_time(), playback.current.get_end_time());
}

void AnimationPlayer::reset_section()
{
	playback.current.start_time = -1;
	playback.current.end_time = -1;
}

double AnimationPlayer::get_section_start_time() const
{
	ERR_FAIL_COND_V_MSG(!playback.current.is_enabled, playback.current.start_time,
		"AnimationPlayer has no current animation.");
	return playback.current.get_start_time();
}

double AnimationPlayer::get_section_end_time() const
{
	ERR_FAIL_COND_V_MSG(!playback.current.is_enabled, playback.current.end_time,
		"AnimationPlayer has no current animation.");
	return playback.current.get_end_time();
}

bool AnimationPlayer::has_section() const
{
	return Animation::is_greater_or_equal_approx(playback.current.start_time, 0) ||
		   Animation::is_greater_or_equal_approx(playback.current.end_time, 0);
}

void AnimationPlayer::set_autoplay(const String& p_name)
{
	if (is_inside_tree() && !Engine::is_editor_hint()) {
		WARN_PRINT("Setting autoplay after the node has been added to the scene has no effect.");
	}

	autoplay = p_name;
}

String AnimationPlayer::get_autoplay() const { return autoplay; }

void AnimationPlayer::set_movie_quit_on_finish_enabled(bool p_enabled)
{
	movie_quit_on_finish = p_enabled;
}

bool AnimationPlayer::is_movie_quit_on_finish_enabled() const { return movie_quit_on_finish; }

void AnimationPlayer::set_clear_cache_on_stop_enabled(bool p_enabled)
{
	clear_cache_on_stop = p_enabled;
}

bool AnimationPlayer::is_clear_cache_on_stop_enabled() const { return clear_cache_on_stop; }

void AnimationPlayer::_animation_changed(const String& p_name)
{
	AnimationMixer::_animation_changed(p_name);
	if (playback.current.is_enabled && playback.current.animation_name == p_name &&
		animation_set.has(p_name)) {
		playback.current.animation_length = animation_set[p_name].animation->get_length();
	}
}

void AnimationPlayer::animation_set_next(const String& p_animation, const String& p_next)
{
	ERR_FAIL_COND_MSG(
		!animation_set.has(p_animation), vformat("Animation not found: %s.", p_animation));
	animation_next_set[p_animation] = p_next;
}

String AnimationPlayer::animation_get_next(const String& p_animation) const
{
	const String* next = animation_next_set.getptr(p_animation);
	if (!next) {
		return String();
	}
	return *next;
}

void AnimationPlayer::set_default_blend_time(double p_default) { default_blend_time = p_default; }

double AnimationPlayer::get_default_blend_time() const { return default_blend_time; }

void AnimationPlayer::set_blend_time(
	const String& p_animation1, const String& p_animation2, double p_time)
{
	ERR_FAIL_COND_MSG(
		!animation_set.has(p_animation1), vformat("Animation not found: %s.", p_animation1));
	ERR_FAIL_COND_MSG(
		!animation_set.has(p_animation2), vformat("Animation not found: %s.", p_animation2));
	ERR_FAIL_COND_MSG(p_time < 0, "Blend time cannot be smaller than 0.");

	BlendKey bk;
	bk.from = p_animation1;
	bk.to = p_animation2;
	if (Math::is_zero_approx(p_time)) {
		blend_times.erase(bk);
	}
	else {
		blend_times[bk] = p_time;
	}
}

double AnimationPlayer::get_blend_time(
	const String& p_animation1, const String& p_animation2) const
{
	BlendKey bk;
	bk.from = p_animation1;
	bk.to = p_animation2;

	if (const double* blend_time = blend_times.getptr(bk)) {
		return *blend_time;
	}
	else {
		return 0;
	}
}

bool AnimationPlayer::is_auto_capture() const { return auto_capture; }

void AnimationPlayer::set_auto_capture_duration(double p_auto_capture_duration)
{
	auto_capture_duration = p_auto_capture_duration;
}

double AnimationPlayer::get_auto_capture_duration() const { return auto_capture_duration; }

void AnimationPlayer::set_auto_capture_transition_type(
	Tween::TransitionType p_auto_capture_transition_type)
{
	auto_capture_transition_type = p_auto_capture_transition_type;
}

Tween::TransitionType AnimationPlayer::get_auto_capture_transition_type() const
{
	return auto_capture_transition_type;
}

void AnimationPlayer::set_auto_capture_ease_type(Tween::EaseType p_auto_capture_ease_type)
{
	auto_capture_ease_type = p_auto_capture_ease_type;
}

Tween::EaseType AnimationPlayer::get_auto_capture_ease_type() const
{
	return auto_capture_ease_type;
}

#ifdef TOOLS_ENABLED
void AnimationPlayer::get_argument_options(
	const String& p_function, int p_idx, List<String>* r_options) const
{
	const String pf = p_function;
	if (p_idx == 0 &&
		(pf == "play" || pf == "play_backwards" || pf == "has_animation" || pf == "queue")) {
		for (const String& name : get_sorted_animation_list()) {
			r_options->push_back(String(name).quote());
		}
	}
	AnimationMixer::get_argument_options(p_function, p_idx, r_options);
}
#endif

void AnimationPlayer::_animation_removed(const String& p_name, const String& p_library)
{
	AnimationMixer::_animation_removed(p_name, p_library);

	const String& name =
		p_library == String() ? p_name : String(String(p_library) + "/" + String(p_name));

	if (!animation_set.has(name)) {
		return; // No need to update because not the one from the library being used.
	}

	_animation_set_cache_update();

	// Erase blends if needed
	LocalVector<BlendKey> to_erase;
	for (const KeyValue<BlendKey, double>& E : blend_times) {
		const BlendKey& bk = E.key;
		if (bk.from == name || bk.to == name) {
			to_erase.push_back(bk);
		}
	}

	for (const BlendKey& bk : to_erase) {
		blend_times.erase(bk);
	}
}

void AnimationPlayer::_rename_animation(const String& p_from_name, const String& p_to_name)
{
	// Rename autoplay or blends if needed.
	LocalVector<BlendKey> to_erase;
	HashMap<BlendKey, double, BlendKey> to_insert;
	for (const KeyValue<BlendKey, double>& E : blend_times) {
		BlendKey bk = E.key;
		BlendKey new_bk = bk;
		bool erase = false;
		if (bk.from == p_from_name) {
			new_bk.from = p_to_name;
			erase = true;
		}
		if (bk.to == p_from_name) {
			new_bk.to = p_to_name;
			erase = true;
		}

		if (erase) {
			to_erase.push_back(bk);
			to_insert[new_bk] = E.value;
		}
	}

	for (const BlendKey& bk : to_erase) {
		blend_times.erase(bk);
	}

	while (to_insert.size()) {
		blend_times[to_insert.begin()->key] = to_insert.begin()->value;
		to_insert.remove(to_insert.begin());
	}

	if (autoplay == p_from_name) {
		autoplay = p_to_name;
	}
}

AnimationPlayer::AnimationPlayer() {}

AnimationPlayer::~AnimationPlayer() {}

void AnimationPlayer::_capture(
	String const&, bool, double, Tween::TransitionType, Tween::EaseType)
{
}

void AnimationPlayer::_stop_internal(bool, bool) {}

void AnimationPlayer::play_section(String const&, double, double, double, float, bool) {}

void AnimationPlayer::set_current_animation(String const&) {}

void AnimationPlayer::set_assigned_animation(String const&) {}

bool AnimationPlayer::_blend_pre_process(
		double p_delta, int p_track_count, const AHashMap<NodePath, int>& p_track_map)
{
	return false;
}

void AnimationPlayer::_blend_post_process() {}

// ==========================================================================
// Merged from scene/animation/root_motion_view.cpp
// ==========================================================================

#ifndef _3D_DISABLED

#include "core/config/engine.h"
// --- Inlined from scene/animation/root_motion_view.h ---
#include "scene/3d/visual_instance_3d.h"
#include "scene/resources/mesh.h"

class RootMotionView : public Node3D {
public:
	RenderInstance3D render_instance;

	Ref<ImmediateMesh> immediate;
	NodePath path;
	real_t cell_size = 1.0;
	real_t radius = 10.0;
	bool use_in_game = false;
	Color color = Color(0.5, 0.5, 1.0);
	bool first = true;
	bool zero_y = true;

	Ref<Material> immediate_material;

	Transform3D accumulated;

protected:
	void _notification(int p_what);
	virtual void fti_update_servers_xform() override;

public:
	virtual bool has_render_instance() const override { return true; }
	RID get_instance() const { return render_instance.get_instance(); }
	void set_base(const RID &p_base) { render_instance.set_base(p_base); }
	RID get_base() const { return render_instance.get_base(); }

	void set_animation_mixer(const NodePath &p_path);
	NodePath get_animation_mixer() const;

	void set_color(const Color &p_color);
	Color get_color() const;

	void set_cell_size(float p_size);
	float get_cell_size() const;

	void set_radius(float p_radius);
	float get_radius() const;

	void set_zero_y(bool p_zero_y);
	bool get_zero_y() const;

	AABB get_aabb() const;

	RootMotionView();
	~RootMotionView();
};

// --- End scene/animation/root_motion_view.h ---
#include "scene/animation/animation_mixer.h"
#include "scene/resources/material.h"

void RootMotionView::set_animation_mixer(const NodePath& p_path)
{
	path = p_path;
	first = true;
}

NodePath RootMotionView::get_animation_mixer() const { return path; }

void RootMotionView::set_color(const Color& p_color)
{
	color = p_color;
	first = true;
}

Color RootMotionView::get_color() const { return color; }

void RootMotionView::set_cell_size(float p_size)
{
	cell_size = p_size;
	first = true;
}

float RootMotionView::get_cell_size() const { return cell_size; }

void RootMotionView::set_radius(float p_radius)
{
	radius = p_radius;
	first = true;
}

float RootMotionView::get_radius() const { return radius; }

void RootMotionView::set_zero_y(bool p_zero_y) { zero_y = p_zero_y; }

bool RootMotionView::get_zero_y() const { return zero_y; }

AABB RootMotionView::get_aabb() const
{
	return AABB(Vector3(-radius, 0, -radius), Vector3(radius * 2, 0.001, radius * 2));
}


void RootMotionView::_notification(int p_what)
{
	render_instance.notification(this, p_what);
}

void RootMotionView::fti_update_servers_xform()
{
	render_instance.fti_update_servers_xform(this);
}

RootMotionView::RootMotionView()
{
	if (Engine::is_editor_hint()) {
		set_process_internal(true);
	}
	immediate.instantiate();
	set_base(immediate->get_rid());
}

RootMotionView::~RootMotionView() { set_base(RID()); }

#endif // _3D_DISABLED

// ============================================================================
// Merged from scene/animation/tween.cpp
// ============================================================================

#include "scene/animation/animation_mixer.h"
// --- Inlined from scene/animation/easing_equations.h ---
#include "core/math/math_funcs.h"

/*
 * Derived from Robert Penner's easing equations: http://robertpenner.com/easing/
 *
 * Copyright (c) 2001 Robert Penner
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

namespace Linear {
inline real_t in(real_t t, real_t b, real_t c, real_t d) {
	return c * t / d + b;
}
}; // namespace Linear

namespace Sine {
inline real_t in(real_t t, real_t b, real_t c, real_t d) {
	return -c * Math::cos(t / d * (Math::PI / 2)) + c + b;
}

inline real_t out(real_t t, real_t b, real_t c, real_t d) {
	return c * Math::sin(t / d * (Math::PI / 2)) + b;
}

inline real_t in_out(real_t t, real_t b, real_t c, real_t d) {
	return -c / 2 * (Math::cos(Math::PI * t / d) - 1) + b;
}

inline real_t out_in(real_t t, real_t b, real_t c, real_t d) {
	if (t < d / 2) {
		return out(t * 2, b, c / 2, d);
	}
	real_t h = c / 2;
	return in(t * 2 - d, b + h, h, d);
}
}; // namespace Sine

namespace Quint {
inline real_t in(real_t t, real_t b, real_t c, real_t d) {
	return c * Math::pow(t / d, 5) + b;
}

inline real_t out(real_t t, real_t b, real_t c, real_t d) {
	return c * (Math::pow(t / d - 1, 5) + 1) + b;
}

inline real_t in_out(real_t t, real_t b, real_t c, real_t d) {
	t = t / d * 2;

	if (t < 1) {
		return c / 2 * Math::pow(t, 5) + b;
	}
	return c / 2 * (Math::pow(t - 2, 5) + 2) + b;
}

inline real_t out_in(real_t t, real_t b, real_t c, real_t d) {
	if (t < d / 2) {
		return out(t * 2, b, c / 2, d);
	}
	real_t h = c / 2;
	return in(t * 2 - d, b + h, h, d);
}
}; // namespace Quint

namespace Quart {
inline real_t in(real_t t, real_t b, real_t c, real_t d) {
	return c * Math::pow(t / d, 4) + b;
}

inline real_t out(real_t t, real_t b, real_t c, real_t d) {
	return -c * (Math::pow(t / d - 1, 4) - 1) + b;
}

inline real_t in_out(real_t t, real_t b, real_t c, real_t d) {
	t = t / d * 2;

	if (t < 1) {
		return c / 2 * Math::pow(t, 4) + b;
	}
	return -c / 2 * (Math::pow(t - 2, 4) - 2) + b;
}

inline real_t out_in(real_t t, real_t b, real_t c, real_t d) {
	if (t < d / 2) {
		return out(t * 2, b, c / 2, d);
	}
	real_t h = c / 2;
	return in(t * 2 - d, b + h, h, d);
}
}; // namespace Quart

namespace Quad {
inline real_t in(real_t t, real_t b, real_t c, real_t d) {
	return c * Math::pow(t / d, 2) + b;
}

inline real_t out(real_t t, real_t b, real_t c, real_t d) {
	t /= d;
	return -c * t * (t - 2) + b;
}

inline real_t in_out(real_t t, real_t b, real_t c, real_t d) {
	t = t / d * 2;

	if (t < 1) {
		return c / 2 * Math::pow(t, 2) + b;
	}
	return -c / 2 * ((t - 1) * (t - 3) - 1) + b;
}

inline real_t out_in(real_t t, real_t b, real_t c, real_t d) {
	if (t < d / 2) {
		return out(t * 2, b, c / 2, d);
	}
	real_t h = c / 2;
	return in(t * 2 - d, b + h, h, d);
}
}; // namespace Quad

namespace Expo {
inline real_t in(real_t t, real_t b, real_t c, real_t d) {
	if (t == 0) {
		return b;
	}
	return c * Math::pow(2, 10 * (t / d - 1)) + b - c * 0.001;
}

inline real_t out(real_t t, real_t b, real_t c, real_t d) {
	if (t == d) {
		return b + c;
	}
	return c * 1.001 * (-Math::pow(2, -10 * t / d) + 1) + b;
}

inline real_t in_out(real_t t, real_t b, real_t c, real_t d) {
	if (t == 0) {
		return b;
	}

	if (t == d) {
		return b + c;
	}

	t = t / d * 2;

	if (t < 1) {
		return c / 2 * Math::pow(2, 10 * (t - 1)) + b - c * 0.0005;
	}
	return c / 2 * 1.0005 * (-Math::pow(2, -10 * (t - 1)) + 2) + b;
}

inline real_t out_in(real_t t, real_t b, real_t c, real_t d) {
	if (t < d / 2) {
		return out(t * 2, b, c / 2, d);
	}
	real_t h = c / 2;
	return in(t * 2 - d, b + h, h, d);
}
}; // namespace Expo

namespace Elastic {
inline real_t in(real_t t, real_t b, real_t c, real_t d) {
	if (t == 0) {
		return b;
	}

	t /= d;
	if (t == 1) {
		return b + c;
	}

	t -= 1;
	float p = d * 0.3f;
	float a = c * Math::pow(2, 10 * t);
	float s = p / 4;

	return -(a * Math::sin((t * d - s) * (2 * Math::PI) / p)) + b;
}

inline real_t out(real_t t, real_t b, real_t c, real_t d) {
	if (t == 0) {
		return b;
	}

	t /= d;
	if (t == 1) {
		return b + c;
	}

	float p = d * 0.3f;
	float s = p / 4;

	return (c * Math::pow(2, -10 * t) * Math::sin((t * d - s) * (2 * Math::PI) / p) + c + b);
}

inline real_t in_out(real_t t, real_t b, real_t c, real_t d) {
	if (t == 0) {
		return b;
	}

	if ((t /= d / 2) == 2) {
		return b + c;
	}

	float p = d * (0.3f * 1.5f);
	float a = c;
	float s = p / 4;

	if (t < 1) {
		t -= 1;
		a *= Math::pow(2, 10 * t);
		return -0.5f * (a * Math::sin((t * d - s) * (2 * Math::PI) / p)) + b;
	}

	t -= 1;
	a *= Math::pow(2, -10 * t);
	return a * Math::sin((t * d - s) * (2 * Math::PI) / p) * 0.5f + c + b;
}

inline real_t out_in(real_t t, real_t b, real_t c, real_t d) {
	if (t < d / 2) {
		return out(t * 2, b, c / 2, d);
	}
	real_t h = c / 2;
	return in(t * 2 - d, b + h, h, d);
}
}; // namespace Elastic

namespace Cubic {
inline real_t in(real_t t, real_t b, real_t c, real_t d) {
	t /= d;
	return c * t * t * t + b;
}

inline real_t out(real_t t, real_t b, real_t c, real_t d) {
	t = t / d - 1;
	return c * (t * t * t + 1) + b;
}

inline real_t in_out(real_t t, real_t b, real_t c, real_t d) {
	t /= d / 2;
	if (t < 1) {
		return c / 2 * t * t * t + b;
	}

	t -= 2;
	return c / 2 * (t * t * t + 2) + b;
}

inline real_t out_in(real_t t, real_t b, real_t c, real_t d) {
	if (t < d / 2) {
		return out(t * 2, b, c / 2, d);
	}
	real_t h = c / 2;
	return in(t * 2 - d, b + h, h, d);
}
}; // namespace Cubic

namespace Circ {
inline real_t in(real_t t, real_t b, real_t c, real_t d) {
	t /= d;
	return -c * (Math::sqrt(1 - t * t) - 1) + b;
}

inline real_t out(real_t t, real_t b, real_t c, real_t d) {
	t = t / d - 1;
	return c * Math::sqrt(1 - t * t) + b;
}

inline real_t in_out(real_t t, real_t b, real_t c, real_t d) {
	t /= d / 2;
	if (t < 1) {
		return -c / 2 * (Math::sqrt(1 - t * t) - 1) + b;
	}

	t -= 2;
	return c / 2 * (Math::sqrt(1 - t * t) + 1) + b;
}

inline real_t out_in(real_t t, real_t b, real_t c, real_t d) {
	if (t < d / 2) {
		return out(t * 2, b, c / 2, d);
	}
	real_t h = c / 2;
	return in(t * 2 - d, b + h, h, d);
}
}; // namespace Circ

namespace Bounce {
inline real_t out(real_t t, real_t b, real_t c, real_t d) {
	t /= d;

	if (t < (1 / 2.75f)) {
		return c * (7.5625f * t * t) + b;
	}

	if (t < (2 / 2.75f)) {
		t -= 1.5f / 2.75f;
		return c * (7.5625f * t * t + 0.75f) + b;
	}

	if (t < (2.5 / 2.75)) {
		t -= 2.25f / 2.75f;
		return c * (7.5625f * t * t + 0.9375f) + b;
	}

	t -= 2.625f / 2.75f;
	return c * (7.5625f * t * t + 0.984375f) + b;
}

inline real_t in(real_t t, real_t b, real_t c, real_t d) {
	return c - out(d - t, 0, c, d) + b;
}

inline real_t in_out(real_t t, real_t b, real_t c, real_t d) {
	if (t < d / 2) {
		return in(t * 2, b, c / 2, d);
	}
	real_t h = c / 2;
	return out(t * 2 - d, b + h, h, d);
}

inline real_t out_in(real_t t, real_t b, real_t c, real_t d) {
	if (t < d / 2) {
		return out(t * 2, b, c / 2, d);
	}
	real_t h = c / 2;
	return in(t * 2 - d, b + h, h, d);
}
}; // namespace Bounce

namespace Back {
inline real_t in(real_t t, real_t b, real_t c, real_t d) {
	float s = 1.70158f;
	t /= d;

	return c * t * t * ((s + 1) * t - s) + b;
}

inline real_t out(real_t t, real_t b, real_t c, real_t d) {
	float s = 1.70158f;
	t = t / d - 1;

	return c * (t * t * ((s + 1) * t + s) + 1) + b;
}

inline real_t in_out(real_t t, real_t b, real_t c, real_t d) {
	float s = 1.70158f * 1.525f;
	t /= d / 2;

	if (t < 1) {
		return c / 2 * (t * t * ((s + 1) * t - s)) + b;
	}

	t -= 2;
	return c / 2 * (t * t * ((s + 1) * t + s) + 2) + b;
}

inline real_t out_in(real_t t, real_t b, real_t c, real_t d) {
	if (t < d / 2) {
		return out(t * 2, b, c / 2, d);
	}
	real_t h = c / 2;
	return in(t * 2 - d, b + h, h, d);
}
}; // namespace Back

namespace Spring {
inline real_t out(real_t t, real_t b, real_t c, real_t d) {
	t /= d;
	real_t s = 1.0 - t;
	t = (Math::sin(t * Math::PI * (0.2 + 2.5 * t * t * t)) * Math::pow(s, (real_t)2.2) + t) * (1.0 + (1.2 * s));
	return c * t + b;
}

inline real_t in(real_t t, real_t b, real_t c, real_t d) {
	return c - out(d - t, 0, c, d) + b;
}

inline real_t in_out(real_t t, real_t b, real_t c, real_t d) {
	if (t < d / 2) {
		return in(t * 2, b, c / 2, d);
	}
	real_t h = c / 2;
	return out(t * 2 - d, b + h, h, d);
}

inline real_t out_in(real_t t, real_t b, real_t c, real_t d) {
	if (t < d / 2) {
		return out(t * 2, b, c / 2, d);
	}
	real_t h = c / 2;
	return in(t * 2 - d, b + h, h, d);
}
}; // namespace Spring

// --- End scene/animation/easing_equations.h ---
#include "scene/main/node.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/animation.h"

#define CHECK_VALID()                                                                              \
	ERR_FAIL_COND_V_MSG(                                                                           \
		!valid, nullptr, "Tween invalid. Either finished or created outside scene tree.");         \
	ERR_FAIL_COND_V_MSG(                                                                           \
		started, nullptr, "Can't append to a Tween that has started. Use stop() first.");

Tween::interpolater Tween::interpolaters[Tween::TRANS_MAX][Tween::EASE_MAX] = {
	{&Linear::in, &Linear::in, &Linear::in, &Linear::in}, // Linear is the same for each easing.
	{&Sine::in, &Sine::out, &Sine::in_out, &Sine::out_in},
	{&Quint::in, &Quint::out, &Quint::in_out, &Quint::out_in},
	{&Quart::in, &Quart::out, &Quart::in_out, &Quart::out_in},
	{&Quad::in, &Quad::out, &Quad::in_out, &Quad::out_in},
	{&Expo::in, &Expo::out, &Expo::in_out, &Expo::out_in},
	{&Elastic::in, &Elastic::out, &Elastic::in_out, &Elastic::out_in},
	{&Cubic::in, &Cubic::out, &Cubic::in_out, &Cubic::out_in},
	{&Circ::in, &Circ::out, &Circ::in_out, &Circ::out_in},
	{&Bounce::in, &Bounce::out, &Bounce::in_out, &Bounce::out_in},
	{&Back::in, &Back::out, &Back::in_out, &Back::out_in},
	{&Spring::in, &Spring::out, &Spring::in_out, &Spring::out_in},
};

void Tween::_start_tweeners()
{
	if (tweeners.is_empty()) {
		dead = true;
		ERR_FAIL_MSG("Tween without commands, aborting.");
	}

	for (Ref<Tweener>& tweener : tweeners[current_step]) {
		tweener->start();
	}
}

void Tween::_stop_internal(bool p_reset)
{
	running = false;
	if (p_reset) {
		started = false;
		dead = false;
		total_time = 0;
	}
}

Ref<Tweener> Tween::tween_interval(double p_time)
{
	CHECK_VALID();
	Ref<Tweener> tweener;
	tweener.instantiate(Tweener::TWEENER_INTERVAL, p_time);
	append(tweener);
	return tweener;
}

Ref<Tweener> Tween::tween_subtween(Tween* rp_subtween)
{
	CHECK_VALID();
	ERR_FAIL_NULL_V(rp_subtween, nullptr);

	Ref<Tweener> tweener;
	tweener.instantiate(Ref<Tween>(rp_subtween));

	// Remove the tween from its parent tree, if it has one.
	// If the user created this tween without a parent tree attached,
	// then this step isn't necessary.
	if (tweener->subtween->is_in_tree) {
		SceneTree::remove_tween(tweener->subtween);
		tweener->subtween->is_in_tree = false;
	}
	subtweens.push_back(rp_subtween);
	append(tweener);
	return tweener;
}

Ref<Tweener> Tween::tween_callback(void (*p_callback)())
{
	CHECK_VALID();
	Ref<Tweener> tweener;
	tweener.instantiate(Tweener::TWEENER_CALLBACK);
	tweener->callback = p_callback;
	append(tweener);
	return tweener;
}

Ref<Tweener> Tween::tween_property(const Vector<String>& p_property, double p_duration)
{
	CHECK_VALID();
	Ref<Tweener> tweener;
	tweener.instantiate(Tweener::TWEENER_PROPERTY, p_duration);
	tweener->property = p_property;
	append(tweener);
	return tweener;
}

Ref<Tweener> Tween::tween_method(double p_duration)
{
	CHECK_VALID();
	Ref<Tweener> tweener;
	tweener.instantiate(Tweener::TWEENER_METHOD, p_duration);
	append(tweener);
	return tweener;
}

void Tween::append(Ref<Tweener> p_tweener)
{
	p_tweener->set_tween(this);

	if (parallel_enabled) {
		current_step = MAX(current_step, 0);
	}
	else {
		current_step++;
	}
	parallel_enabled = default_parallel;

	tweeners.resize(current_step + 1);
	tweeners[current_step].push_back(p_tweener);
}

void Tween::stop() { _stop_internal(true); }

void Tween::pause() { _stop_internal(false); }

void Tween::play()
{
	ERR_FAIL_COND_MSG(!valid, "Tween invalid. Either finished or created outside scene tree.");
	ERR_FAIL_COND_MSG(dead, "Can't play finished Tween, use stop() first to reset its state.");
	running = true;
}

void Tween::kill()
{
	running = false; // For the sake of is_running().
	valid = false;
	dead = true;

	// Kill all subtweens of this tween.
	for (Ref<Tween>& st : subtweens) {
		st->kill();
	}
}

bool Tween::has_tweeners() const { return !tweeners.is_empty(); }

bool Tween::is_running() { return running; }

bool Tween::is_valid() { return valid; }

void Tween::clear()
{
	valid = false;
	tweeners.clear();
}

Tween* Tween::set_process_mode(TweenProcessMode p_mode)
{
	process_mode = p_mode;
	return this;
}

Tween::TweenProcessMode Tween::get_process_mode() const { return process_mode; }

Tween* Tween::set_pause_mode(TweenPauseMode p_mode)
{
	pause_mode = p_mode;
	return this;
}

Tween::TweenPauseMode Tween::get_pause_mode() const { return pause_mode; }

Tween* Tween::set_ignore_time_scale(bool p_ignore)
{
	ignore_time_scale = p_ignore;
	return this;
}

bool Tween::is_ignoring_time_scale() const { return ignore_time_scale; }

Tween* Tween::set_parallel(bool p_parallel)
{
	default_parallel = p_parallel;
	parallel_enabled = p_parallel;
	return this;
}

Tween* Tween::set_loops(int p_loops)
{
	loops = p_loops;
	return this;
}

int Tween::get_loops_left() const
{
	if (loops <= 0) {
		return -1; // Infinite loop.
	}
	else {
		return loops - loops_done;
	}
}

Tween* Tween::set_speed_scale(float p_speed)
{
	speed_scale = p_speed;
	return this;
}

Tween* Tween::set_trans(TransitionType p_trans)
{
	default_transition = p_trans;
	return this;
}

Tween::TransitionType Tween::get_trans() const { return default_transition; }

Tween* Tween::set_ease(EaseType p_ease)
{
	default_ease = p_ease;
	return this;
}

Tween::EaseType Tween::get_ease() const { return default_ease; }

Tween* Tween::parallel()
{
	parallel_enabled = true;
	return this;
}

Tween* Tween::chain()
{
	parallel_enabled = false;
	return this;
}

bool Tween::custom_step(double p_delta)
{
	ERR_FAIL_COND_V_MSG(in_step, true, "Can't call custom_step() during another Tween step.");

	bool r = running;
	running = true;
	bool ret = step(p_delta);
	running = running && r; // Running might turn false when Tween finished.
	return ret;
}

bool Tween::step(double p_delta) { return true; }

bool Tween::can_process(bool p_tree_paused) const
{
	if (is_bound && pause_mode == TWEEN_PAUSE_BOUND) {
		Node* node = get_bound_node();
		if (node) {
			return node->is_inside_tree() && node->can_process();
		}
	}

	return !p_tree_paused || pause_mode == TWEEN_PAUSE_PROCESS;
}

double Tween::get_total_time() const { return total_time; }

real_t Tween::run_equation(TransitionType p_trans_type, EaseType p_ease_type, real_t p_time,
	real_t p_initial, real_t p_delta, real_t p_duration)
{
	if (p_duration == 0) {
		// Special case to avoid dividing by 0 in equations.
		return p_initial + p_delta;
	}

	interpolater func = interpolaters[p_trans_type][p_ease_type];
	return func(p_time, p_initial, p_delta, p_duration);
}

Tween* Tween::bind_node(const Node* rp_node)
{
	ERR_FAIL_NULL_V(rp_node, this);
	bound_node = const_cast<Node*>(rp_node);
	is_bound = true;
	return this;
}

Node* Tween::get_bound_node() const { return bound_node; }

String Tween::_to_string() { return String(); }

Tween::Tween() { ERR_FAIL_MSG("Tween can't be created directly. Use create_tween() method."); }

Tween::Tween(bool p_in_tree)
{
	is_in_tree = p_in_tree;
	valid = true;
}

Tween::~Tween() = default;

Ref<Tweener> Tweener::from(double p_value)
{
	initial_val = p_value;
	do_continue = false;
	return this;
}

Ref<Tweener> Tweener::from_current()
{
	do_continue = false;
	return this;
}

Ref<Tweener> Tweener::as_relative()
{
	relative = true;
	return this;
}

Ref<Tweener> Tweener::set_trans(Tween::TransitionType p_trans)
{
	trans_type = p_trans;
	return this;
}

Ref<Tweener> Tweener::set_ease(Tween::EaseType p_ease)
{
	ease_type = p_ease;
	return this;
}

Ref<Tweener> Tweener::set_custom_interpolator(real_t (*p_interpolator)(real_t))
{
	custom_interpolator = p_interpolator;
	return this;
}

Ref<Tweener> Tweener::set_delay(double p_delay)
{
	delay = p_delay;
	return this;
}

Ref<Tweener> Tweener::set_timeout(double p_timeout)
{
	timeout = p_timeout;
	return this;
}

void Tweener::set_tween(const Ref<Tween>& p_tween)
{
	tween = p_tween.ptr();
	if (type == TWEENER_PROPERTY || type == TWEENER_METHOD) {
		if (trans_type == Tween::TRANS_MAX && p_tween.is_valid()) {
			trans_type = p_tween->get_trans();
		}
		if (ease_type == Tween::EASE_MAX && p_tween.is_valid()) {
			ease_type = p_tween->get_ease();
		}
	}
}

Ref<Tween> Tweener::_get_tween() { return Ref<Tween>(tween); }

void Tweener::_finish() { finished = true; }

void Tweener::start()
{
	elapsed_time = 0;
	finished = false;

	switch (type) {
	case TWEENER_SUBTWEEN: {
		if (subtween.is_null()) {
			_finish();
			return;
		}
		// Reset the subtween.
		subtween->stop();

		// It's possible that a subtween could be killed before it is started;
		// if so, we just want to skip it entirely.
		if (subtween->is_valid()) {
			subtween->play();
		}
		else {
			_finish();
		}
	} break;
	case TWEENER_AWAIT: {
		received = false;
	} break;
	case TWEENER_INTERVAL:
	case TWEENER_CALLBACK:
	case TWEENER_PROPERTY:
	case TWEENER_METHOD:
		break;
	}
}

bool Tweener::step(double& r_delta)
{
	switch (type) {
	case TWEENER_INTERVAL: {
		if (finished) {
			return false;
		}

		elapsed_time += r_delta;

		if (elapsed_time < duration) {
			r_delta = 0;
			return true;
		}
		else {
			r_delta = elapsed_time - duration;
			_finish();
			return false;
		}
	}
	case TWEENER_SUBTWEEN: {
		if (finished) {
			return false;
		}

		elapsed_time += r_delta;

		if (elapsed_time < delay) {
			r_delta = 0;
			return true;
		}

		if (subtween.is_null() || !subtween->step(r_delta)) {
			r_delta = elapsed_time - delay - (subtween.is_valid() ? subtween->get_total_time() : 0.0);
			_finish();
			return false;
		}

		r_delta = 0;
		return true;
	}
	case TWEENER_CALLBACK: {
		if (finished) {
			return false;
		}

		elapsed_time += r_delta;
		if (elapsed_time < delay) {
			r_delta = 0;
			return true;
		}

		if (callback) {
			callback();
		}
		r_delta = elapsed_time - delay;
		_finish();
		return false;
	}
	case TWEENER_PROPERTY:
	case TWEENER_METHOD:
	case TWEENER_AWAIT: {
		return true;
	}
	}
	return false;
}

Tweener::Tweener()
{
	ERR_FAIL_MSG("Tweener can't be created directly. Use the tween_*() methods in Tween.");
}

Tweener::Tweener(Type p_type) : type(p_type) {}

Tweener::Tweener(Type p_type, double p_duration) : type(p_type), duration(p_duration) {}

Tweener::Tweener(const Ref<Tween>& p_subtween) : subtween(p_subtween), type(TWEENER_SUBTWEEN) {}
