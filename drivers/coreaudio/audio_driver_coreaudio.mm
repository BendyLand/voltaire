/**************************************************************************/
/*  audio_driver_coreaudio.mm                                             */
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

#import "audio_driver_coreaudio.h"

#ifdef COREAUDIO_ENABLED

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/types.h"
#include "core/os/os.h"

#define kOutputBus 0
#define kInputBus 1

#ifdef MACOS_ENABLED
OSStatus AudioDriverCoreAudio::input_device_address_cb(AudioObjectID inObjectID,
		UInt32 inNumberAddresses, const AudioObjectPropertyAddress *inAddresses,
		void *inClientData) {
	AudioDriverCoreAudio *driver = static_cast<AudioDriverCoreAudio *>(inClientData);

	// If our selected input device is the Default, call set_input_device to update the
	// kAudioOutputUnitProperty_CurrentDevice property
	if (driver->input_device_name == "Default") {
		driver->set_input_device("Default");
	}

	return noErr;
}

OSStatus AudioDriverCoreAudio::output_device_address_cb(AudioObjectID inObjectID,
		UInt32 inNumberAddresses, const AudioObjectPropertyAddress *inAddresses,
		void *inClientData) {
	AudioDriverCoreAudio *driver = static_cast<AudioDriverCoreAudio *>(inClientData);

	// If our selected output device is the Default call set_output_device to update the
	// kAudioOutputUnitProperty_CurrentDevice property
	if (driver->output_device_name == "Default") {
		driver->set_output_device("Default");
	}

	return noErr;
}

// Switch to kAudioObjectPropertyElementMain everywhere to remove deprecated warnings.
#if (TARGET_OS_OSX && __MAC_OS_X_VERSION_MAX_ALLOWED < 120000) || (TARGET_OS_IOS && __IPHONE_OS_VERSION_MAX_ALLOWED < 150000)
#define kAudioObjectPropertyElementMain kAudioObjectPropertyElementMaster
#endif
#endif

Error AudioDriverCoreAudio::init() {
	AudioComponentDescription desc;
	memset(&desc, 0, sizeof(desc));
	desc.componentType = kAudioUnitType_Output;
#ifdef MACOS_ENABLED
	desc.componentSubType = kAudioUnitSubType_HALOutput;
#else
	desc.componentSubType = kAudioUnitSubType_RemoteIO;
#endif
	desc.componentManufacturer = kAudioUnitManufacturer_Apple;

	AudioComponent comp = AudioComponentFindNext(nullptr, &desc);
	ERR_FAIL_NULL_V(comp, FAILED);

	OSStatus result = AudioComponentInstanceNew(comp, &audio_unit);
	ERR_FAIL_COND_V(result != noErr, FAILED);

#ifdef MACOS_ENABLED
	AudioObjectPropertyAddress prop;
	prop.mSelector = kAudioHardwarePropertyDefaultOutputDevice;
	prop.mScope = kAudioObjectPropertyScopeGlobal;
	prop.mElement = kAudioObjectPropertyElementMain;

	result = AudioObjectAddPropertyListener(kAudioObjectSystemObject, &prop, &output_device_address_cb, this);
	ERR_FAIL_COND_V(result != noErr, FAILED);
#endif

	AudioStreamBasicDescription strdesc;

	memset(&strdesc, 0, sizeof(strdesc));
	UInt32 size = sizeof(strdesc);
	result = AudioUnitGetProperty(audio_unit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, kOutputBus, &strdesc, &size);
	ERR_FAIL_COND_V(result != noErr, FAILED);

	switch (strdesc.mChannelsPerFrame) {
		case 2: // Stereo
		case 4: // Surround 3.1
		case 6: // Surround 5.1
		case 8: // Surround 7.1
			channels = strdesc.mChannelsPerFrame;
			break;

		default:
			// Unknown number of channels, default to stereo
			channels = 2;
			break;
	}

#ifdef MACOS_ENABLED
	AudioDeviceID device_id;
	UInt32 dev_id_size = sizeof(AudioDeviceID);

	AudioObjectPropertyAddress property_dev_id = { kAudioHardwarePropertyDefaultOutputDevice, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMain };
	result = AudioObjectGetPropertyData(kAudioObjectSystemObject, &property_dev_id, 0, nullptr, &dev_id_size, &device_id);
	ERR_FAIL_COND_V(result != noErr, FAILED);

	double hw_mix_rate;
	UInt32 hw_mix_rate_size = sizeof(hw_mix_rate);

	AudioObjectPropertyAddress property_sr = { kAudioDevicePropertyNominalSampleRate, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMain };
	result = AudioObjectGetPropertyData(device_id, &property_sr, 0, nullptr, &hw_mix_rate_size, &hw_mix_rate);
	ERR_FAIL_COND_V(result != noErr, FAILED);
#else
	double hw_mix_rate = [AVAudioSession sharedInstance].sampleRate;
#endif
	mix_rate = hw_mix_rate;

	memset(&strdesc, 0, sizeof(strdesc));
	strdesc.mFormatID = kAudioFormatLinearPCM;
	strdesc.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger | kLinearPCMFormatFlagIsPacked;
	strdesc.mChannelsPerFrame = channels;
	strdesc.mSampleRate = mix_rate;
	strdesc.mFramesPerPacket = 1;
	strdesc.mBitsPerChannel = 16;
	strdesc.mBytesPerFrame = strdesc.mBitsPerChannel * strdesc.mChannelsPerFrame / 8;
	strdesc.mBytesPerPacket = strdesc.mBytesPerFrame * strdesc.mFramesPerPacket;

	result = AudioUnitSetProperty(audio_unit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, kOutputBus, &strdesc, sizeof(strdesc));
	ERR_FAIL_COND_V(result != noErr, FAILED);

	uint32_t latency = Engine::get_audio_output_latency();
	// Sample rate is independent of channels (ref: https://stackoverflow.com/questions/11048825/audio-sample-frequency-rely-on-channels)
	buffer_frames = Math::closest_power_of_2(latency * (uint32_t)mix_rate / (uint32_t)1000);

#ifdef MACOS_ENABLED
	result = AudioUnitSetProperty(audio_unit, kAudioDevicePropertyBufferFrameSize, kAudioUnitScope_Global, kOutputBus, &buffer_frames, sizeof(UInt32));
	ERR_FAIL_COND_V(result != noErr, FAILED);
#endif

	unsigned int buffer_size = buffer_frames * channels;
	samples_in.resize(buffer_size);

	print_verbose("CoreAudio: detected " + itos(channels) + " channels");
	print_verbose("CoreAudio: output sampling rate: " + itos(mix_rate) + " Hz");
	print_verbose("CoreAudio: output audio buffer frames: " + itos(buffer_frames) + " calculated latency: " + itos(buffer_frames * 1000 / mix_rate) + "ms");

	AURenderCallbackStruct callback;
	memset(&callback, 0, sizeof(AURenderCallbackStruct));
	callback.inputProc = &AudioDriverCoreAudio::output_callback;
	callback.inputProcRefCon = this;
	result = AudioUnitSetProperty(audio_unit, kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Input, kOutputBus, &callback, sizeof(callback));
	ERR_FAIL_COND_V(result != noErr, FAILED);

	result = AudioUnitInitialize(audio_unit);
	ERR_FAIL_COND_V(result != noErr, FAILED);

	if (GLOBAL_GET("audio/driver/enable_input")) {
		return init_input_device();
	}
	return OK;
}

OSStatus AudioDriverCoreAudio::output_callback(void *inRefCon,
		AudioUnitRenderActionFlags *ioActionFlags,
		const AudioTimeStamp *inTimeStamp,
		UInt32 inBusNumber, UInt32 inNumberFrames,
		AudioBufferList *ioData) {
	AudioDriverCoreAudio *ad = static_cast<AudioDriverCoreAudio *>(inRefCon);

	if (!ad->active || !ad->try_lock()) {
		for (unsigned int i = 0; i < ioData->mNumberBuffers; i++) {
			AudioBuffer *abuf = &ioData->mBuffers[i];
			memset(abuf->mData, 0, abuf->mDataByteSize);
		}
		return 0;
	}

	ad->start_counting_ticks();

	for (unsigned int i = 0; i < ioData->mNumberBuffers; i++) {
		AudioBuffer *abuf = &ioData->mBuffers[i];
		unsigned int frames_left = inNumberFrames;
		int16_t *out = (int16_t *)abuf->mData;

		while (frames_left) {
			unsigned int frames = MIN(frames_left, ad->buffer_frames);
			ad->audio_server_process(frames, ad->samples_in.ptrw());

			for (unsigned int j = 0; j < frames * ad->channels; j++) {
				out[j] = ad->samples_in[j] >> 16;
			}

			frames_left -= frames;
			out += frames * ad->channels;
		}
	}

	ad->stop_counting_ticks();
	ad->unlock();

	return 0;
}

OSStatus AudioDriverCoreAudio::input_callback(void *inRefCon,
		AudioUnitRenderActionFlags *ioActionFlags,
		const AudioTimeStamp *inTimeStamp,
		UInt32 inBusNumber, UInt32 inNumberFrames,
		AudioBufferList *ioData) {
	AudioDriverCoreAudio *ad = static_cast<AudioDriverCoreAudio *>(inRefCon);
	if (!ad->active) {
		return 0;
	}

	ad->lock();
	ad->start_counting_ticks();

	AudioBufferList bufferList;
	bufferList.mNumberBuffers = 1;
	bufferList.mBuffers[0].mData = nullptr;
	bufferList.mBuffers[0].mNumberChannels = ad->capture_channels;
	bufferList.mBuffers[0].mDataByteSize = ad->buffer_size * sizeof(int16_t);

	OSStatus result = AudioUnitRender(ad->input_unit, ioActionFlags, inTimeStamp, inBusNumber, inNumberFrames, &bufferList);
	if (result == noErr) {
		int16_t *data = (int16_t *)bufferList.mBuffers[0].mData;
		for (unsigned int i = 0; i < inNumberFrames * ad->capture_channels; i++) {
			int32_t sample = data[i] << 16;
			ad->input_buffer_write(sample);

			if (ad->capture_channels == 1) {
				// In case input device is single channel convert it to Stereo
				ad->input_buffer_write(sample);
			}
		}
	} else {
		ERR_PRINT("AudioUnitRender failed, code: " + itos(result));
	}

	ad->stop_counting_ticks();
	ad->unlock();

	return result;
}

void AudioDriverCoreAudio::start() {
	if (!active && audio_unit != nullptr) {
		OSStatus result = AudioOutputUnitStart(audio_unit);
		if (result != noErr) {
			ERR_PRINT("AudioOutputUnitStart failed, code: " + itos(result));
		} else {
			active = true;
		}
	}
}

void AudioDriverCoreAudio::stop() {
	if (active) {
		OSStatus result = AudioOutputUnitStop(audio_unit);
		if (result != noErr) {
			ERR_PRINT("AudioOutputUnitStop failed, code: " + itos(result));
		} else {
			active = false;
		}
	}
}

int AudioDriverCoreAudio::get_mix_rate() const {
	return mix_rate;
}

int AudioDriverCoreAudio::get_input_mix_rate() const {
	return capture_mix_rate;
}

AudioDriver::SpeakerMode AudioDriverCoreAudio::get_speaker_mode() const {
	return get_speaker_mode_by_total_channels(channels);
}

void AudioDriverCoreAudio::lock() {
	mutex.lock();
}

void AudioDriverCoreAudio::unlock() {
	mutex.unlock();
}

bool AudioDriverCoreAudio::try_lock() {
	return mutex.try_lock();
}

void AudioDriverCoreAudio::finish() {
	finish_input_device();

	if (audio_unit) {
		OSStatus result;

		lock();

		AURenderCallbackStruct callback;
		memset(&callback, 0, sizeof(AURenderCallbackStruct));
		result = AudioUnitSetProperty(audio_unit, kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Input, kOutputBus, &callback, sizeof(callback));
		if (result != noErr) {
			ERR_PRINT("AudioUnitSetProperty failed");
		}

		if (active) {
			result = AudioOutputUnitStop(audio_unit);
			if (result != noErr) {
				ERR_PRINT("AudioOutputUnitStop failed");
			}

			active = false;
		}

		result = AudioUnitUninitialize(audio_unit);
		if (result != noErr) {
			ERR_PRINT("AudioUnitUninitialize failed");
		}

#ifdef MACOS_ENABLED
		AudioObjectPropertyAddress prop;
		prop.mSelector = kAudioHardwarePropertyDefaultOutputDevice;
		prop.mScope = kAudioObjectPropertyScopeGlobal;
		prop.mElement = kAudioObjectPropertyElementMain;

		result = AudioObjectRemovePropertyListener(kAudioObjectSystemObject, &prop, &output_device_address_cb, this);
		if (result != noErr) {
			ERR_PRINT("AudioObjectRemovePropertyListener failed");
		}
#endif

		result = AudioComponentInstanceDispose(audio_unit);
		if (result != noErr) {
			ERR_PRINT("AudioComponentInstanceDispose failed");
		}

		audio_unit = nullptr;
		unlock();
	}
}

Error AudioDriverCoreAudio::init_input_device() {
	AudioComponentDescription desc;
	memset(&desc, 0, sizeof(desc));
	desc.componentType = kAudioUnitType_Output;
#ifdef MACOS_ENABLED
	desc.componentSubType = kAudioUnitSubType_HALOutput;
#else
	desc.componentSubType = kAudioUnitSubType_RemoteIO;
#endif
	desc.componentManufacturer = kAudioUnitManufacturer_Apple;

	AudioComponent comp = AudioComponentFindNext(nullptr, &desc);
	ERR_FAIL_NULL_V(comp, FAILED);

	OSStatus result = AudioComponentInstanceNew(comp, &input_unit);
	ERR_FAIL_COND_V(result != noErr, FAILED);

#ifdef MACOS_ENABLED
	AudioObjectPropertyAddress prop;
	prop.mSelector = kAudioHardwarePropertyDefaultInputDevice;
	prop.mScope = kAudioObjectPropertyScopeGlobal;
	prop.mElement = kAudioObjectPropertyElementMain;

	result = AudioObjectAddPropertyListener(kAudioObjectSystemObject, &prop, &input_device_address_cb, this);
	ERR_FAIL_COND_V(result != noErr, FAILED);
#endif

	UInt32 flag = 1;
	result = AudioUnitSetProperty(input_unit, kAudioOutputUnitProperty_EnableIO, kAudioUnitScope_Input, kInputBus, &flag, sizeof(flag));
	ERR_FAIL_COND_V(result != noErr, FAILED);
#ifdef MACOS_ENABLED
	flag = 0;
	result = AudioUnitSetProperty(input_unit, kAudioOutputUnitProperty_EnableIO, kAudioUnitScope_Output, kOutputBus, &flag, sizeof(flag));
	ERR_FAIL_COND_V(result != noErr, FAILED);
#endif

	UInt32 size;
#ifdef MACOS_ENABLED
	AudioDeviceID device_id;
	size = sizeof(AudioDeviceID);
	AudioObjectPropertyAddress property = { kAudioHardwarePropertyDefaultInputDevice, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMain };

	result = AudioObjectGetPropertyData(kAudioObjectSystemObject, &property, 0, nullptr, &size, &device_id);
	ERR_FAIL_COND_V(result != noErr, FAILED);

	result = AudioUnitSetProperty(input_unit, kAudioOutputUnitProperty_CurrentDevice, kAudioUnitScope_Global, 0, &device_id, sizeof(AudioDeviceID));
	ERR_FAIL_COND_V(result != noErr, FAILED);
#endif

	AudioStreamBasicDescription strdesc;
	memset(&strdesc, 0, sizeof(strdesc));
	size = sizeof(strdesc);
	result = AudioUnitGetProperty(input_unit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, kInputBus, &strdesc, &size);
	ERR_FAIL_COND_V(result != noErr, FAILED);

	switch (strdesc.mChannelsPerFrame) {
		case 1: // Mono
			capture_channels = 1;
			break;

		case 2: // Stereo
			capture_channels = 2;
			break;

		default:
			// Unknown number of channels, default to stereo
			capture_channels = 2;
			break;
	}

#ifdef MACOS_ENABLED
	double hw_mix_rate;
	UInt32 hw_mix_rate_size = sizeof(hw_mix_rate);

	AudioObjectPropertyAddress property_sr = { kAudioDevicePropertyNominalSampleRate, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMain };
	result = AudioObjectGetPropertyData(device_id, &property_sr, 0, nullptr, &hw_mix_rate_size, &hw_mix_rate);
	ERR_FAIL_COND_V(result != noErr, FAILED);
#else
	double hw_mix_rate = [AVAudioSession sharedInstance].sampleRate;
#endif
	capture_mix_rate = hw_mix_rate;

	memset(&strdesc, 0, sizeof(strdesc));
	strdesc.mFormatID = kAudioFormatLinearPCM;
	strdesc.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger | kLinearPCMFormatFlagIsPacked;
	strdesc.mChannelsPerFrame = capture_channels;
	strdesc.mSampleRate = capture_mix_rate;
	strdesc.mFramesPerPacket = 1;
	strdesc.mBitsPerChannel = 16;
	strdesc.mBytesPerFrame = strdesc.mBitsPerChannel * strdesc.mChannelsPerFrame / 8;
	strdesc.mBytesPerPacket = strdesc.mBytesPerFrame * strdesc.mFramesPerPacket;

	result = AudioUnitSetProperty(input_unit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, kInputBus, &strdesc, sizeof(strdesc));
	ERR_FAIL_COND_V(result != noErr, FAILED);

	uint32_t latency = Engine::get_audio_output_latency();
	// Sample rate is independent of channels (ref: https://stackoverflow.com/questions/11048825/audio-sample-frequency-rely-on-channels)
	capture_buffer_frames = Math::closest_power_of_2(latency * (uint32_t)capture_mix_rate / (uint32_t)1000);

	buffer_size = capture_buffer_frames * capture_channels;

	AURenderCallbackStruct callback;
	memset(&callback, 0, sizeof(AURenderCallbackStruct));
	callback.inputProc = &AudioDriverCoreAudio::input_callback;
	callback.inputProcRefCon = this;
	result = AudioUnitSetProperty(input_unit, kAudioOutputUnitProperty_SetInputCallback, kAudioUnitScope_Global, kInputBus, &callback, sizeof(callback));
	ERR_FAIL_COND_V(result != noErr, FAILED);

	result = AudioUnitInitialize(input_unit);
	ERR_FAIL_COND_V(result != noErr, FAILED);

	print_verbose("CoreAudio: input sampling rate: " + itos(capture_mix_rate) + " Hz");
	print_verbose("CoreAudio: input audio buffer frames: " + itos(capture_buffer_frames) + " calculated latency: " + itos(capture_buffer_frames * 1000 / capture_mix_rate) + "ms");

	return OK;
}

void AudioDriverCoreAudio::finish_input_device() {
	if (input_unit) {
		lock();

		AURenderCallbackStruct callback;
		memset(&callback, 0, sizeof(AURenderCallbackStruct));
		OSStatus result = AudioUnitSetProperty(input_unit, kAudioOutputUnitProperty_SetInputCallback, kAudioUnitScope_Global, 0, &callback, sizeof(callback));
		if (result != noErr) {
			ERR_PRINT("AudioUnitSetProperty failed");
		}

		result = AudioUnitUninitialize(input_unit);
		if (result != noErr) {
			ERR_PRINT("AudioUnitUninitialize failed");
		}

#ifdef MACOS_ENABLED
		AudioObjectPropertyAddress prop;
		prop.mSelector = kAudioHardwarePropertyDefaultInputDevice;
		prop.mScope = kAudioObjectPropertyScopeGlobal;
		prop.mElement = kAudioObjectPropertyElementMain;

		result = AudioObjectRemovePropertyListener(kAudioObjectSystemObject, &prop, &input_device_address_cb, this);
		if (result != noErr) {
			ERR_PRINT("AudioObjectRemovePropertyListener failed");
		}
#endif

		result = AudioComponentInstanceDispose(input_unit);
		if (result != noErr) {
			ERR_PRINT("AudioComponentInstanceDispose failed");
		}

		input_unit = nullptr;
		unlock();
	}
}

Error AudioDriverCoreAudio::input_start() {
	ERR_FAIL_NULL_V(input_unit, FAILED);

	input_buffer_init(capture_buffer_frames);

	OSStatus result = AudioOutputUnitStart(input_unit);
	if (result != noErr) {
		ERR_PRINT("AudioOutputUnitStart failed, code: " + itos(result));
	}

	return OK;
}

Error AudioDriverCoreAudio::input_stop() {
	if (input_unit) {
		OSStatus result = AudioOutputUnitStop(input_unit);
		if (result != noErr) {
			ERR_PRINT("AudioOutputUnitStop failed, code: " + itos(result));
		}
	}

	return OK;
}

#ifdef MACOS_ENABLED

PackedStringArray AudioDriverCoreAudio::_get_device_list(bool input) {
	PackedStringArray list;

	list.push_back("Default");

	AudioObjectPropertyAddress prop;

	prop.mSelector = kAudioHardwarePropertyDevices;
	prop.mScope = kAudioObjectPropertyScopeGlobal;
	prop.mElement = kAudioObjectPropertyElementMain;

	UInt32 size = 0;
	AudioObjectGetPropertyDataSize(kAudioObjectSystemObject, &prop, 0, nullptr, &size);
	AudioDeviceID *audioDevices = (AudioDeviceID *)memalloc(size);
	ERR_FAIL_NULL_V_MSG(audioDevices, list, "Out of memory.");
	AudioObjectGetPropertyData(kAudioObjectSystemObject, &prop, 0, nullptr, &size, audioDevices);

	UInt32 deviceCount = size / sizeof(AudioDeviceID);
	for (UInt32 i = 0; i < deviceCount; i++) {
		prop.mScope = input ? kAudioDevicePropertyScopeInput : kAudioDevicePropertyScopeOutput;
		prop.mSelector = kAudioDevicePropertyStreamConfiguration;

		AudioObjectGetPropertyDataSize(audioDevices[i], &prop, 0, nullptr, &size);
		AudioBufferList *bufferList = (AudioBufferList *)memalloc(size);
		ERR_FAIL_NULL_V_MSG(bufferList, list, "Out of memory.");
		AudioObjectGetPropertyData(audioDevices[i], &prop, 0, nullptr, &size, bufferList);

		UInt32 channelCount = 0;
		for (UInt32 j = 0; j < bufferList->mNumberBuffers; j++) {
			channelCount += bufferList->mBuffers[j].mNumberChannels;
		}

		memfree(bufferList);

		if (channelCount >= 1) {
			CFStringRef cfname;

			size = sizeof(CFStringRef);
			prop.mSelector = kAudioObjectPropertyName;

			AudioObjectGetPropertyData(audioDevices[i], &prop, 0, nullptr, &size, &cfname);

			CFIndex length = CFStringGetLength(cfname);
			CFIndex maxSize = CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8) + 1;
			char *buffer = (char *)memalloc(maxSize);
			ERR_FAIL_NULL_V_MSG(buffer, list, "Out of memory.");
			if (CFStringGetCString(cfname, buffer, maxSize, kCFStringEncodingUTF8)) {
				// Append the ID to the name in case we have devices with duplicate name
				list.push_back(String::utf8(buffer) + " (" + itos(audioDevices[i]) + ")");
			}

			memfree(buffer);
		}
	}

	memfree(audioDevices);

	return list;
}

void AudioDriverCoreAudio::_set_device(const String &output_device, bool input) {
	AudioDeviceID device_id;
	bool found = false;
	if (output_device != "Default") {
		AudioObjectPropertyAddress prop;

		prop.mSelector = kAudioHardwarePropertyDevices;
		prop.mScope = kAudioObjectPropertyScopeGlobal;
		prop.mElement = kAudioObjectPropertyElementMain;

		UInt32 size = 0;
		AudioObjectGetPropertyDataSize(kAudioObjectSystemObject, &prop, 0, nullptr, &size);
		AudioDeviceID *audioDevices = (AudioDeviceID *)memalloc(size);
		ERR_FAIL_NULL_MSG(audioDevices, "Out of memory.");
		AudioObjectGetPropertyData(kAudioObjectSystemObject, &prop, 0, nullptr, &size, audioDevices);

		UInt32 deviceCount = size / sizeof(AudioDeviceID);
		for (UInt32 i = 0; i < deviceCount && !found; i++) {
			prop.mScope = input ? kAudioDevicePropertyScopeInput : kAudioDevicePropertyScopeOutput;
			prop.mSelector = kAudioDevicePropertyStreamConfiguration;

			AudioObjectGetPropertyDataSize(audioDevices[i], &prop, 0, nullptr, &size);
			AudioBufferList *bufferList = (AudioBufferList *)memalloc(size);
			ERR_FAIL_NULL_MSG(bufferList, "Out of memory.");
			AudioObjectGetPropertyData(audioDevices[i], &prop, 0, nullptr, &size, bufferList);

			UInt32 channelCount = 0;
			for (UInt32 j = 0; j < bufferList->mNumberBuffers; j++) {
				channelCount += bufferList->mBuffers[j].mNumberChannels;
			}

			memfree(bufferList);

			if (channelCount >= 1) {
				CFStringRef cfname;

				size = sizeof(CFStringRef);
				prop.mSelector = kAudioObjectPropertyName;

				AudioObjectGetPropertyData(audioDevices[i], &prop, 0, nullptr, &size, &cfname);

				CFIndex length = CFStringGetLength(cfname);
				CFIndex maxSize = CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8) + 1;
				char *buffer = (char *)memalloc(maxSize);
				ERR_FAIL_NULL_MSG(buffer, "Out of memory.");
				if (CFStringGetCString(cfname, buffer, maxSize, kCFStringEncodingUTF8)) {
					String name = String::utf8(buffer) + " (" + itos(audioDevices[i]) + ")";
					if (name == output_device) {
						device_id = audioDevices[i];
						found = true;
					}
				}

				memfree(buffer);
			}
		}

		memfree(audioDevices);
	}

	if (!found) {
		// If we haven't found the desired device get the system default one
		UInt32 size = sizeof(AudioDeviceID);
		UInt32 elem = input ? kAudioHardwarePropertyDefaultInputDevice : kAudioHardwarePropertyDefaultOutputDevice;
		AudioObjectPropertyAddress property = { elem, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMain };

		OSStatus result = AudioObjectGetPropertyData(kAudioObjectSystemObject, &property, 0, nullptr, &size, &device_id);
		ERR_FAIL_COND(result != noErr);

		found = true;
	}

	if (found) {
		OSStatus result = AudioUnitSetProperty(input ? input_unit : audio_unit, kAudioOutputUnitProperty_CurrentDevice, kAudioUnitScope_Global, 0, &device_id, sizeof(AudioDeviceID));
		ERR_FAIL_COND(result != noErr);

		if (input) {
			// Reset audio input to keep synchronization.
			input_position = 0;
			input_size = 0;
		}
	}
}

PackedStringArray AudioDriverCoreAudio::get_output_device_list() {
	return _get_device_list();
}

String AudioDriverCoreAudio::get_output_device() {
	return output_device_name;
}

void AudioDriverCoreAudio::set_output_device(const String &p_name) {
	output_device_name = p_name;
	if (active) {
		_set_device(output_device_name);
	}
}

PackedStringArray AudioDriverCoreAudio::get_input_device_list() {
	return _get_device_list(true);
}

String AudioDriverCoreAudio::get_input_device() {
	return input_device_name;
}

void AudioDriverCoreAudio::set_input_device(const String &p_name) {
	input_device_name = p_name;
	if (active) {
		_set_device(input_device_name, true);
	}
}

#endif

AudioDriverCoreAudio::AudioDriverCoreAudio() {
	samples_in.clear();
}

#endif // COREAUDIO_ENABLED

// === Merged from drivers/apple/foundation_helpers.mm ===
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

#import "drivers/apple/foundation_helpers.h"

#import "core/string/ustring.h"
#import "core/templates/span.h"

#import <CoreFoundation/CFString.h>

namespace conv {

NSString *to_nsstring(const String &p_str) {
	return [[NSString alloc] initWithBytes:(const void *)p_str.ptr()
									length:p_str.length() * sizeof(char32_t)
								  encoding:NSUTF32LittleEndianStringEncoding];
}

NSString *to_nsstring(Span<char> p_str) {
	return [[NSString alloc] initWithBytes:(const void *)p_str.ptr()
									length:p_str.size()
								  encoding:NSASCIIStringEncoding];
}

NSString *to_nsstring(const CharString &p_str) {
	return [[NSString alloc] initWithBytes:(const void *)p_str.ptr()
									length:p_str.length()
								  encoding:NSUTF8StringEncoding];
}

String to_string(NSString *p_str) {
	CFStringRef str = (__bridge CFStringRef)p_str;
	CFStringEncoding fastest = CFStringGetFastestEncoding(str);
	// Sometimes, CFString will return a pointer to it's encoded data,
	// so we can create the string without allocating intermediate buffers.
	const char *p = CFStringGetCStringPtr(str, fastest);
	if (p) {
		switch (fastest) {
			case kCFStringEncodingASCII:
				return String::ascii(Span(p, CFStringGetLength(str)));
			case kCFStringEncodingUTF8:
				return String::utf8(p);
			case kCFStringEncodingUTF32LE:
				return String::utf32(Span((char32_t *)p, CFStringGetLength(str)));
			default:
				break;
		}
	}

	CFRange range = CFRangeMake(0, CFStringGetLength(str));
	CFIndex byte_len = 0;
	// Try to losslessly convert the string directly into a String's buffer to avoid intermediate allocations.
	CFIndex n = CFStringGetBytes(str, range, kCFStringEncodingUTF32LE, 0, NO, nil, 0, &byte_len);
	if (n == range.length) {
		String res;
		res.resize_uninitialized((byte_len / sizeof(char32_t)) + 1);
		res[n] = 0;
		n = CFStringGetBytes(str, range, kCFStringEncodingUTF32LE, 0, NO, (UInt8 *)res.ptrw(), res.length() * sizeof(char32_t), nil);
		return res;
	}

	return String::utf8(p_str.UTF8String);
}

} //namespace conv

// === Merged from drivers/apple/os_log_logger.cpp ===
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

#include "drivers/coreaudio/audio_driver_coreaudio.h"

#include "core/os/memory.h"
#include "core/string/print_string.h"

#include <cstdio> // For vsnprintf.

OsLogLogger::OsLogLogger(const char *p_subsystem) {
	const char *subsystem = p_subsystem;
	if (!subsystem) {
		subsystem = "org.godotengine.godot";
		os_log_info(OS_LOG_DEFAULT, "Missing subsystem for os_log logging; using %{public}s", subsystem);
	}

	log = os_log_create(subsystem, "engine");
	error_log = os_log_create(subsystem, error_type_string(ErrorType::ERR_ERROR));
	warning_log = os_log_create(subsystem, error_type_string(ErrorType::ERR_WARNING));
	script_log = os_log_create(subsystem, error_type_string(ErrorType::ERR_SCRIPT));
	shader_log = os_log_create(subsystem, error_type_string(ErrorType::ERR_SHADER));
}

void OsLogLogger::logv(const char *p_format, va_list p_list, bool p_err) {
	constexpr int static_buf_size = 1024;
	char static_buf[static_buf_size] = { '\0' };
	char *buf = static_buf;
	va_list list_copy;
	va_copy(list_copy, p_list);
	int len = vsnprintf(buf, static_buf_size, p_format, p_list);
	if (len >= static_buf_size) {
		buf = (char *)Memory::alloc_static(len + 1);
		vsnprintf(buf, len + 1, p_format, list_copy);
	}
	va_end(list_copy);

	// Choose appropriate log type based on error flag.
	os_log_type_t log_type = p_err ? OS_LOG_TYPE_ERROR : OS_LOG_TYPE_INFO;
	os_log_with_type(log, log_type, "%{public}s", buf);

	if (len >= static_buf_size) {
		Memory::free_static(buf);
	}
}

void OsLogLogger::log_error(const char *p_function, const char *p_file, int p_line, const char *p_code, const char *p_rationale, bool p_editor_notify, ErrorType p_type, const Vector<Ref<ScriptBacktrace>> &p_script_backtraces) {
	os_log_t selected_log;
	switch (p_type) {
		case ERR_WARNING:
			selected_log = warning_log;
			break;
		case ERR_SCRIPT:
			selected_log = script_log;
			break;
		case ERR_SHADER:
			selected_log = shader_log;
			break;
		case ERR_ERROR:
		default:
			selected_log = error_log;
			break;
	}
	const char *err_details;
	if (p_rationale && *p_rationale) {
		err_details = p_rationale;
	} else {
		err_details = p_code;
	}

	// Choose log level based on error type.
	os_log_type_t log_type;
	switch (p_type) {
		case ERR_WARNING:
			log_type = OS_LOG_TYPE_DEFAULT;
			break;
		case ERR_ERROR:
		case ERR_SCRIPT:
		case ERR_SHADER:
		default:
			log_type = OS_LOG_TYPE_ERROR;
			break;
	}

	// Append script backtraces, if any.
	String back_trace;
	for (const Ref<ScriptBacktrace> &backtrace : p_script_backtraces) {
		if (backtrace.is_valid() && !backtrace->is_empty()) {
			back_trace += "\n";
			back_trace += backtrace->format(strlen(error_type_indent(p_type)));
		}
	}

	if (back_trace.is_empty()) {
		os_log_with_type(selected_log, log_type, "%{public}s:%d:%{public}s(): %{public}s %{public}s", p_file, p_line, p_function, err_details, p_code);
	} else {
		os_log_with_type(selected_log, log_type, "%{public}s:%d:%{public}s(): %{public}s %{public}s%{public}s", p_file, p_line, p_function, err_details, p_code, back_trace.utf8().ptr());
	}
}

// === Merged from drivers/apple/thread_apple.cpp ===
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

#include "core/os/thread.h"

#include "core/error/error_macros.h"
#include "core/string/ustring.h"

SafeNumeric<uint64_t> Thread::id_counter(1); // The first value after .increment() is 2, hence by default the main thread ID should be 1.
thread_local Thread::ID Thread::caller_id = Thread::id_counter.increment();

struct ThreadData {
	Thread::Callback callback;
	void *userdata;
	Thread::ID caller_id;
};

void *Thread::thread_callback(void *p_data) {
	ThreadData *thread_data = static_cast<ThreadData *>(p_data);

	// Set the caller ID for this thread
	caller_id = thread_data->caller_id;

	ScriptServer::thread_enter(); // Scripts may need to attach a stack.

	// Call the actual callback
	thread_data->callback(thread_data->userdata);

	ScriptServer::thread_exit();

	// Clean up
	memdelete(thread_data);

	return nullptr;
}

void Thread::make_main_thread() {
	if (caller_id == MAIN_ID) {
		return; // We're already the main thread
	}
	CRASH_COND_MSG(!is_main_thread_assigned.set_if_clear(), "A second thread attempted to become the main thread.");
	caller_id = MAIN_ID;
}

void Thread::release_main_thread() {
	CRASH_COND_MSG(caller_id != MAIN_ID, "Trying to release main thread from a thread that isn't main.");
	CRASH_COND(!is_main_thread_assigned.clear_if_set());
	caller_id = id_counter.increment();
}

Error Thread::set_name(const String &p_name) {
	int err = pthread_setname_np(p_name.utf8().get_data());
	return err == 0 ? OK : ERR_INVALID_PARAMETER;
}

Thread::ID Thread::start(Thread::Callback p_callback, void *p_user, const Settings &p_settings) {
	ERR_FAIL_COND_V_MSG(id != UNASSIGNED_ID, UNASSIGNED_ID, "A Thread object has been re-started without wait_to_finish() having been called on it.");
	id = id_counter.increment();

	ThreadData *thread_data = memnew(ThreadData);
	thread_data->callback = p_callback;
	thread_data->userdata = p_user;
	thread_data->caller_id = id;

	// Create the thread
	pthread_attr_t attr;
	pthread_attr_init(&attr);

	switch (p_settings.priority) {
		case PRIORITY_LOW:
			pthread_attr_set_qos_class_np(&attr, QOS_CLASS_UTILITY, 0);
			break;
		case PRIORITY_NORMAL:
			pthread_attr_set_qos_class_np(&attr, QOS_CLASS_USER_INITIATED, 0);
			break;
		case PRIORITY_HIGH:
			pthread_attr_set_qos_class_np(&attr, QOS_CLASS_USER_INTERACTIVE, 0);
			break;
	}

	// The default stack size for secondary threads on Apple platforms is 512KiB.
	// This is insufficient when using a library like SPIRV-Cross, which can generate deep stacks and result in a stack overflow.
	// It also creates a problematic discrepancy with other platforms, where secondary threads are often at least 1 MiB.
	pthread_attr_setstacksize(&attr,
#if __has_feature(address_sanitizer) || __has_feature(thread_sanitizer)
			// ASan (and to some degree TSan) needs a lot of extra stack size.
			4 * 1024 * 1024 // 4 MiB
#elif !defined(__OPTIMIZE__)
			// Unoptimized builds also need a larger stack size.
			2 * 1024 * 1024 // 2 MiB
#else
			1 * 1024 * 1024 // 1 MiB
#endif
	);

	// Create the thread
	pthread_create(&pthread, &attr, thread_callback, thread_data);

	// Clean up attributes
	pthread_attr_destroy(&attr);

	return id;
}

void Thread::wait_to_finish() {
	ERR_FAIL_COND_MSG(id == UNASSIGNED_ID, "Attempt of waiting to finish on a thread that was never started.");
	ERR_FAIL_COND_MSG(id == get_caller_id(), "Threads can't wait to finish on themselves, another thread must wait.");

	int err = pthread_join(pthread, nullptr);
	if (err != 0) {
		ERR_FAIL_MSG("Thread::wait_to_finish() failed to join thread.");
	}
	pthread = pthread_t();
	id = UNASSIGNED_ID;
}

Thread::~Thread() {
	if (id != UNASSIGNED_ID) {
#ifdef DEBUG_ENABLED
		WARN_PRINT(
				"A Thread object is being destroyed without its completion having been realized.\n"
				"Please call wait_to_finish() on it to ensure correct cleanup.");
#endif
		pthread_detach(pthread);
	}
}
