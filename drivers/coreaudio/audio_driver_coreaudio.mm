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

#import "drivers/unix/os_unix.h"

#ifdef COREAUDIO_ENABLED

#include "core/os/os.h"
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

#import "drivers/unix/os_unix.h"

#import "core/string/ustring.h"

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

#include "drivers/unix/os_unix.h"

#include "core/os/memory.h"
#include "core/string/ustring.h"

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

#include "core/os/mutex.h"

#include "core/typedefs.h"
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

// === Merged from modules/camera/camera_apple.mm ===
#include "modules/modules_enabled.gen.h"
#if defined(MODULE_CAMERA_ENABLED)
/*  camera_apple.mm                                                       */
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

///@TODO this is a near duplicate of CameraIOS, we should find a way to combine those to minimize code duplication!!!!
// If you fix something here, make sure you fix it there as well!

#import "servers/rendering/renderer.h"

#include "servers/rendering/renderer.h"

#import <AVFoundation/AVFoundation.h>
#ifdef IOS_ENABLED
#import <UIKit/UIKit.h>
#endif

//////////////////////////////////////////////////////////////////////////
// MyCaptureSession - This is a little helper class so we can capture our frames

@interface MyCaptureSession : AVCaptureSession <AVCaptureVideoDataOutputSampleBufferDelegate> {
	Ref<CameraFeed> feed;
	size_t width[2];
	size_t height[2];
	Vector<uint8_t> img_data[2];

	AVCaptureDeviceInput *input;
	AVCaptureVideoDataOutput *output;
}

@end

@implementation MyCaptureSession

- (id)initForFeed:(Ref<CameraFeed>)p_feed andDevice:(AVCaptureDevice *)p_device {
	if (self = [super init]) {
		NSError *error;
		feed = p_feed;
		width[0] = 0;
		height[0] = 0;
		width[1] = 0;
		height[1] = 0;

#ifdef IOS_ENABLED
		if ([p_device lockForConfiguration:&error]) {
			if ([p_device isFocusModeSupported:AVCaptureFocusModeContinuousAutoFocus]) {
				[p_device setFocusMode:AVCaptureFocusModeContinuousAutoFocus];
			}
			if ([p_device isExposureModeSupported:AVCaptureExposureModeContinuousAutoExposure]) {
				[p_device setExposureMode:AVCaptureExposureModeContinuousAutoExposure];
			}
			if ([p_device isWhiteBalanceModeSupported:AVCaptureWhiteBalanceModeContinuousAutoWhiteBalance]) {
				[p_device setWhiteBalanceMode:AVCaptureWhiteBalanceModeContinuousAutoWhiteBalance];
			}

			[p_device unlockForConfiguration];
		}
#endif // IOS_ENABLED

		[self beginConfiguration];

#ifdef IOS_ENABLED
		self.sessionPreset = AVCaptureSessionPreset1280x720;
#endif // IOS_ENABLED

		input = [[AVCaptureDeviceInput alloc] initWithDevice:p_device error:&error];

		if (!input) {
			print_line("Couldn't get input device for camera");
			[self commitConfiguration];
			return nil;
		}
		if (![self canAddInput:input]) {
			print_line("Couldn't add input to capture session");
			input = nullptr;
			[self commitConfiguration];
			return nil;
		}
		[self addInput:input];

		output = [AVCaptureVideoDataOutput new];
		if (!output) {
			print_line("Couldn't get output device for camera");
			[self removeInput:input];
			input = nullptr;
			[self commitConfiguration];
			return nil;
		}
		if (![self canAddOutput:output]) {
			print_line("Couldn't add output to capture session");
			[self removeInput:input];
			input = nullptr;
			output = nullptr;
			[self commitConfiguration];
			return nil;
		}

		NSDictionary *settings = @{ (NSString *)kCVPixelBufferPixelFormatTypeKey : @(kCVPixelFormatType_420YpCbCr8BiPlanarFullRange) };
		output.videoSettings = settings;

		// discard if the data output queue is blocked (as we process the still image)
		[output setAlwaysDiscardsLateVideoFrames:YES];

		// now set ourselves as the delegate to receive new frames.
		[output setSampleBufferDelegate:self queue:dispatch_get_main_queue()];

		// this takes ownership
		[self addOutput:output];

		[self commitConfiguration];

		// kick off our session..
		[self startRunning];
	};
	return self;
}

- (void)cleanup {
	// stop running
	[self stopRunning];

	// cleanup
	[self beginConfiguration];

	// remove input
	if (input) {
		[self removeInput:input];
		// don't release this
		input = nullptr;
	}

	// free up our output
	if (output) {
		[self removeOutput:output];
		[output setSampleBufferDelegate:nil queue:nullptr];
		output = nullptr;
	}

	[self commitConfiguration];
}

- (void)captureOutput:(AVCaptureOutput *)captureOutput didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer fromConnection:(AVCaptureConnection *)connection {
	// This gets called every time our camera has a new image for us to process.
	// May need to investigate in a way to throttle this if we get more images then we're rendering frames..

	// For now, version 1, we're just doing the bare minimum to make this work...
	CVImageBufferRef pixelBuffer = CMSampleBufferGetImageBuffer(sampleBuffer);
	if (pixelBuffer == nullptr) {
		return;
	}

	// It says that we need to lock this on the documentation pages but it's not in the samples
	// need to lock our base address so we can access our pixel buffers, better safe then sorry?
	CVPixelBufferLockBaseAddress(pixelBuffer, kCVPixelBufferLock_ReadOnly);

	// Check if we have the expected number of planes (Y and CbCr).
	size_t planeCount = CVPixelBufferGetPlaneCount(pixelBuffer);
	if (planeCount < 2) {
		static bool plane_count_error_logged = false;
		if (!plane_count_error_logged) {
			ERR_PRINT("Unexpected plane count in pixel buffer (expected 2, got " + itos(planeCount) + ")");
			plane_count_error_logged = true;
		}
		CVPixelBufferUnlockBaseAddress(pixelBuffer, kCVPixelBufferLock_ReadOnly);
		return;
	}

	// get our buffers
	unsigned char *dataY = (unsigned char *)CVPixelBufferGetBaseAddressOfPlane(pixelBuffer, 0);
	unsigned char *dataCbCr = (unsigned char *)CVPixelBufferGetBaseAddressOfPlane(pixelBuffer, 1);
	if (dataY == nullptr || dataCbCr == nullptr) {
		static bool buffer_access_error_logged = false;
		if (!buffer_access_error_logged) {
			ERR_PRINT("Couldn't access pixel buffer plane data");
			buffer_access_error_logged = true;
		}
		CVPixelBufferUnlockBaseAddress(pixelBuffer, kCVPixelBufferLock_ReadOnly);
		return;
	}

	Ref<Image> img[2];

	{
		// do Y
		size_t new_width = CVPixelBufferGetWidthOfPlane(pixelBuffer, 0);
		size_t new_height = CVPixelBufferGetHeightOfPlane(pixelBuffer, 0);
		size_t row_stride = CVPixelBufferGetBytesPerRowOfPlane(pixelBuffer, 0);

		if ((width[0] != new_width) || (height[0] != new_height)) {
			width[0] = new_width;
			height[0] = new_height;
			img_data[0].resize(new_width * new_height);
		}

		uint8_t *w = img_data[0].ptrw();
		if (new_width == row_stride) {
			memcpy(w, dataY, new_width * new_height);
		} else {
			for (size_t i = 0; i < new_height; i++) {
				memcpy(w, dataY, new_width);
				w += new_width;
				dataY += row_stride;
			}
		}

		img[0].instantiate();
		img[0]->set_data(new_width, new_height, 0, Image::FORMAT_R8, img_data[0]);
	}

	{
		// do CbCr
		size_t new_width = CVPixelBufferGetWidthOfPlane(pixelBuffer, 1);
		size_t new_height = CVPixelBufferGetHeightOfPlane(pixelBuffer, 1);
		size_t row_stride = CVPixelBufferGetBytesPerRowOfPlane(pixelBuffer, 1);

		if ((width[1] != new_width) || (height[1] != new_height)) {
			width[1] = new_width;
			height[1] = new_height;
			img_data[1].resize(2 * new_width * new_height);
		}

		uint8_t *w = img_data[1].ptrw();
		if (new_width * 2 == row_stride) {
			memcpy(w, dataCbCr, 2 * new_width * new_height);
		} else {
			for (size_t i = 0; i < new_height; i++) {
				memcpy(w, dataCbCr, new_width * 2);
				w += new_width * 2;
				dataCbCr += row_stride;
			}
		}

		///TODO OpenGL doesn't support FORMAT_RG8, need to do some form of conversion
		img[1].instantiate();
		img[1]->set_data(new_width, new_height, 0, Image::FORMAT_RG8, img_data[1]);
	}

	// set our texture...
	feed->set_ycbcr_images(img[0], img[1]);

	// and unlock
	CVPixelBufferUnlockBaseAddress(pixelBuffer, kCVPixelBufferLock_ReadOnly);
}

@end

//////////////////////////////////////////////////////////////////////////
// CameraFeedApple - Subclass for camera feeds in macOS

class CameraFeedApple : public CameraFeed {
	VLTRSOFTCLASS(CameraFeedApple, CameraFeed);

private:
	AVCaptureDevice *device;
	MyCaptureSession *capture_session;
	bool device_locked;
	bool was_active_before_pause = false;

public:
	AVCaptureDevice *get_device() const;

	CameraFeedApple();
	~CameraFeedApple();

	void set_device(AVCaptureDevice *p_device);

	void handle_rotation_change(int p_orientation);
	void handle_pause();
	void handle_resume();

	bool activate_feed() override;
	void deactivate_feed() override;
};

AVCaptureDevice *CameraFeedApple::get_device() const {
	return device;
}

CameraFeedApple::CameraFeedApple() {
	device = nullptr;
	capture_session = nullptr;
	device_locked = false;
	transform = Transform2D(1.0, 0.0, 0.0, 1.0, 0.0, 0.0);
}

CameraFeedApple::~CameraFeedApple() {
	if (is_active()) {
		deactivate_feed();
	}
}

void CameraFeedApple::set_device(AVCaptureDevice *p_device) {
	device = p_device;

	// get some info
	NSString *device_name = p_device.localizedName;
	name = String::utf8(device_name.UTF8String);
	position = CameraFeed::FEED_UNSPECIFIED;
	if ([p_device position] == AVCaptureDevicePositionBack) {
		position = CameraFeed::FEED_BACK;
	} else if ([p_device position] == AVCaptureDevicePositionFront) {
		position = CameraFeed::FEED_FRONT;
	};
}

void CameraFeedApple::handle_rotation_change(int p_orientation) {
	// UIInterfaceOrientation values:
	// 1 = UIInterfaceOrientationPortrait
	// 2 = UIInterfaceOrientationPortraitUpsideDown
	// 3 = UIInterfaceOrientationLandscapeLeft
	// 4 = UIInterfaceOrientationLandscapeRight
	int display_rotation = 0;
	switch (p_orientation) {
		case 1:
			display_rotation = 0;
			break;
		case 2:
			display_rotation = 180;
			break;
		case 3:
			display_rotation = 270;
			break;
		case 4:
			display_rotation = 90;
			break;
		default:
			display_rotation = 0;
			break;
	}

	// iOS camera sensor orientation is 90 degrees.
	int sensor_orientation = 90;
	int sign = position == CameraFeed::FEED_FRONT ? 1 : -1;
	int image_rotation = (sensor_orientation - display_rotation * sign + 360) % 360;

	transform = Transform2D();
	transform = transform.rotated(Math::deg_to_rad((float)image_rotation));
}

void CameraFeedApple::handle_pause() {
	if (capture_session) {
		was_active_before_pause = true;
		deactivate_feed();
	} else {
		was_active_before_pause = false;
	}
}

void CameraFeedApple::handle_resume() {
	if (was_active_before_pause) {
		activate_feed();
		was_active_before_pause = false;
	}
}

bool CameraFeedApple::activate_feed() {
	if (capture_session) {
		// Already recording.
		return true;
	}

	// Start camera capture, check permission.
	AVAuthorizationStatus status = [AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeVideo];
	if (status == AVAuthorizationStatusAuthorized) {
		capture_session = [[MyCaptureSession alloc] initForFeed:this andDevice:device];
		return capture_session != nullptr;
	} else if (status == AVAuthorizationStatusNotDetermined) {
		// Request permission asynchronously.
		[AVCaptureDevice requestAccessForMediaType:AVMediaTypeVideo
								 completionHandler:^(BOOL granted) {
									 if (granted) {
										 capture_session = [[MyCaptureSession alloc] initForFeed:this andDevice:device];
									 }
								 }];
		return false;
	} else if (status == AVAuthorizationStatusDenied) {
		print_line("Camera permission denied by user.");
		return false;
	} else if (status == AVAuthorizationStatusRestricted) {
		print_line("Camera access restricted.");
		return false;
	}
	return false;
}

void CameraFeedApple::deactivate_feed() {
	// end camera capture if we have one
	if (capture_session) {
		[capture_session cleanup];
		capture_session = nullptr;
	};
	if (device_locked) {
		[device unlockForConfiguration];
		device_locked = false;
	}
}

//////////////////////////////////////////////////////////////////////////
// MyDeviceNotifications - This is a little helper class gets notifications
// when devices are connected/disconnected

@interface MyDeviceNotifications : NSObject

@end

@implementation MyDeviceNotifications

- (void)devices_changed:(NSNotification *)notification {
	CameraApple::update_feeds();
}

- (id)init {
	if (self = [super init]) {
		[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(devices_changed:) name:AVCaptureDeviceWasConnectedNotification object:nil];
		[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(devices_changed:) name:AVCaptureDeviceWasDisconnectedNotification object:nil];
	}
	return self;
}

- (void)dealloc {
	// remove notifications
	[[NSNotificationCenter defaultCenter] removeObserver:self name:AVCaptureDeviceWasConnectedNotification object:nil];
	[[NSNotificationCenter defaultCenter] removeObserver:self name:AVCaptureDeviceWasDisconnectedNotification object:nil];
}

@end

MyDeviceNotifications *device_notifications = nil;

//////////////////////////////////////////////////////////////////////////
// CameraApple - Subclass for our camera server on macOS

void CameraApple::finalize() {
	device_notifications = nil;
}

void CameraApple::update_feeds() {
	ERR_FAIL_NULL(CameraServer::data);

	NSArray<AVCaptureDevice *> *devices = nullptr;
#ifdef APPLE_EMBEDDED_ENABLED
	{
		NSMutableArray *deviceTypes = [NSMutableArray array];
		if (@available(iOS 14.0, visionOS 2.1, *)) {
			[deviceTypes addObject:AVCaptureDeviceTypeBuiltInWideAngleCamera];
		}
#ifdef IOS_ENABLED
		[deviceTypes addObject:AVCaptureDeviceTypeBuiltInTelephotoCamera];
		[deviceTypes addObject:AVCaptureDeviceTypeBuiltInDualCamera];
		[deviceTypes addObject:AVCaptureDeviceTypeBuiltInTrueDepthCamera];
		[deviceTypes addObject:AVCaptureDeviceTypeBuiltInUltraWideCamera];
		[deviceTypes addObject:AVCaptureDeviceTypeBuiltInDualWideCamera];
		[deviceTypes addObject:AVCaptureDeviceTypeBuiltInTripleCamera];
#endif // IOS_ENABLED
		AVCaptureDeviceDiscoverySession *session = [AVCaptureDeviceDiscoverySession discoverySessionWithDeviceTypes:deviceTypes mediaType:AVMediaTypeVideo position:AVCaptureDevicePositionUnspecified];
		devices = session.devices;
	}
#else // APPLE_EMBEDDED_ENABLED
	AVCaptureDeviceDiscoverySession *session;
	if (@available(macOS 14.0, *)) {
		session = [AVCaptureDeviceDiscoverySession discoverySessionWithDeviceTypes:[NSArray arrayWithObjects:AVCaptureDeviceTypeExternal, AVCaptureDeviceTypeBuiltInWideAngleCamera, AVCaptureDeviceTypeContinuityCamera, nil] mediaType:AVMediaTypeVideo position:AVCaptureDevicePositionUnspecified];
	} else {
		session = [AVCaptureDeviceDiscoverySession discoverySessionWithDeviceTypes:[NSArray arrayWithObjects:AVCaptureDeviceTypeExternalUnknown, AVCaptureDeviceTypeBuiltInWideAngleCamera, nil] mediaType:AVMediaTypeVideo position:AVCaptureDevicePositionUnspecified];
	}
	devices = session.devices;
#endif // APPLE_EMBEDDED_ENABLED

	// Deactivate feeds that are gone before removing them.
	for (int i = CameraServer::data->feeds.size() - 1; i >= 0; i--) {
		Ref<CameraFeedApple> feed = (Ref<CameraFeedApple>)CameraServer::data->feeds[i];
		if (feed.is_null()) {
			continue;
		}

		if (![devices containsObject:feed->get_device()]) {
			if (feed->is_active()) {
				feed->deactivate_feed();
			}
			CameraServer::remove_feed(feed);
		}
	}

	for (AVCaptureDevice *device in devices) {
		bool found = false;
		for (int i = 0; i < CameraServer::data->feeds.size() && !found; i++) {
			Ref<CameraFeedApple> feed = (Ref<CameraFeedApple>)CameraServer::data->feeds[i];
			if (feed.is_null()) {
				continue;
			}
			if (feed->get_device() == device) {
				found = true;
			}
		}

		if (!found) {
			Ref<CameraFeedApple> newfeed;
			newfeed.instantiate();
			newfeed->set_device(device);

			CameraServer::add_feed(newfeed);
		}
	}

#ifdef IOS_ENABLED
	// Update rotation for all feeds.
	UIInterfaceOrientation orientation = UIInterfaceOrientationUnknown;
	UIWindow *window = [UIApplication sharedApplication].delegate.window;
	UIWindowScene *windowScene = window.windowScene;
	if (windowScene) {
		orientation = windowScene.interfaceOrientation;
	}
	handle_display_rotation_change((int)orientation);
#endif // IOS_ENABLED
}

void CameraApple::set_monitoring_feeds(bool p_monitoring_feeds) {
	if (p_monitoring_feeds == CameraServer::is_monitoring_feeds()) {
		return;
	}

	CameraServer::set_monitoring_feeds_state(p_monitoring_feeds);
	if (p_monitoring_feeds) {
		// Find available cameras we have at this time.
		update_feeds();

		// Get notified on feed changes.
		device_notifications = [[MyDeviceNotifications alloc] init];
	} else {
		// Stop monitoring feed changes.
		device_notifications = nil;
	}
}

void CameraApple::handle_display_rotation_change(int p_orientation) {
	ERR_FAIL_NULL(CameraServer::data);
	for (int i = 0; i < CameraServer::data->feeds.size(); i++) {
		Ref<CameraFeedApple> feed = (Ref<CameraFeedApple>)CameraServer::data->feeds[i];
		if (feed.is_valid()) {
			feed->handle_rotation_change(p_orientation);
		}
	}
}

void CameraApple::handle_application_pause() {
	ERR_FAIL_NULL(CameraServer::data);
	for (int i = 0; i < CameraServer::data->feeds.size(); i++) {
		Ref<CameraFeedApple> feed = (Ref<CameraFeedApple>)CameraServer::data->feeds[i];
		if (feed.is_valid()) {
			feed->handle_pause();
		}
	}
}

void CameraApple::handle_application_resume() {
	ERR_FAIL_NULL(CameraServer::data);
	for (int i = 0; i < CameraServer::data->feeds.size(); i++) {
		Ref<CameraFeedApple> feed = (Ref<CameraFeedApple>)CameraServer::data->feeds[i];
		if (feed.is_valid()) {
			feed->handle_resume();
		}
	}
}

#ifdef APPLE_EMBEDDED_ENABLED

void register_camera_external_module() {
	CameraServer::bind<CameraApple>();
}

void unregister_camera_external_module() {
}

#endif // APPLE_EMBEDDED_ENABLED
#endif // defined(MODULE_CAMERA_ENABLED)

// === Merged from modules/openxr/extensions/platform/openxr_metal_extension.mm ===
#include "modules/modules_enabled.gen.h"
#if defined(MODULE_OPENXR_ENABLED) && defined(METAL_ENABLED)
/*  openxr_metal_extension.mm                                             */
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

#import "modules/openxr/openxr_platform_inc.h"

#include "servers/rendering/renderer.h"

#import "drivers/metal/rendering_device_driver_metal3.h"
#include "servers/rendering/renderer.h"
#include "servers/rendering/rendering_server.h"

HashMap<String, bool *> OpenXRMetalExtension::get_requested_extensions(XrVersion p_version) {
	HashMap<String, bool *> request_extensions;

	request_extensions[XR_KHR_METAL_ENABLE_EXTENSION_NAME] = nullptr;

	return request_extensions;
}

void OpenXRMetalExtension::on_instance_created(const XrInstance p_instance) {
	// Obtain pointers to functions we're accessing here.
	ERR_FAIL_COND(!OpenXRAPI::is_allocated());

	EXT_INIT_XR_FUNC(xrGetMetalGraphicsRequirementsKHR);
	EXT_INIT_XR_FUNC(xrEnumerateSwapchainImages);
}

bool OpenXRMetalExtension::check_graphics_api_support() {
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), false);

	// TODO We may need to do a callback like we do in Vulkan where we run this first
	// and provide the obtained metalDevice to our Metal driver to use.
	// But according to Stuart Macs only have 1 device so it should always be the
	// same one and we should be able to get away with not doing this just yet.
	// If we do go forward with this, this means that just like with Vulkan,
	// we have to start with OpenXR before Metal can be setup, and we thus
	// can't support applications that want to add XR as an optional/temporary
	// feature that users enable when needed.

	XrSystemId system_id = OpenXRAPI::get_system_id();
	XrInstance instance = OpenXRAPI::get_instance();

	XrGraphicsRequirementsMetalKHR metal_requirements;
	metal_requirements.type = XR_TYPE_GRAPHICS_REQUIREMENTS_METAL_KHR;
	metal_requirements.next = nullptr;
	metal_requirements.metalDevice = nullptr;

	XrResult result = xrGetMetalGraphicsRequirementsKHR(instance, system_id, &metal_requirements);
	if (!OpenXRAPI::xr_result(result, "Failed to get Metal graphics requirements!")) {
		return false;
	}

	// See what metal device we are using...
	ERR_FAIL_NULL_V(RD::data, false);

	void *our_metal_device = (void *)RD::get_driver_resource(RD::DRIVER_RESOURCE_LOGICAL_DEVICE);

	// Make sure we're using the same one.
	ERR_FAIL_COND_V(metal_requirements.metalDevice != our_metal_device, false);

	return true;
}

XrGraphicsBindingMetalKHR OpenXRMetalExtension::graphics_binding_metal;

void *OpenXRMetalExtension::set_session_create_and_get_next_pointer(void *p_next_pointer) {
	if (!check_graphics_api_support()) {
		return p_next_pointer;
	}

	ERR_FAIL_NULL_V(RD::data, p_next_pointer);

	graphics_binding_metal.type = XR_TYPE_GRAPHICS_BINDING_METAL_KHR;
	graphics_binding_metal.next = p_next_pointer;
	graphics_binding_metal.commandQueue = (void *)RD::get_driver_resource(RD::DRIVER_RESOURCE_COMMAND_QUEUE);

	return &graphics_binding_metal;
}

void OpenXRMetalExtension::get_usable_swapchain_formats(Vector<int64_t> &p_usable_swap_chains) {
	p_usable_swap_chains.push_back(MTLPixelFormatRGBA8Unorm_sRGB);
	p_usable_swap_chains.push_back(MTLPixelFormatBGRA8Unorm_sRGB);
	p_usable_swap_chains.push_back(MTLPixelFormatRGBA8Uint);
}

void OpenXRMetalExtension::get_usable_depth_formats(Vector<int64_t> &p_usable_swap_chains) {
	p_usable_swap_chains.push_back(MTLPixelFormatDepth24Unorm_Stencil8);
	p_usable_swap_chains.push_back(MTLPixelFormatDepth32Float_Stencil8);
	p_usable_swap_chains.push_back(MTLPixelFormatDepth32Float);
}

#define ENUM_TO_STRING_CASE(m_e) \
	case m_e: { \
		return String(#m_e); \
	} break;

String OpenXRMetalExtension::get_swapchain_format_name(int64_t p_swapchain_format) const {
	// This really should be in vulkan_context...
	MTLPixelFormat format = MTLPixelFormat(p_swapchain_format);
	switch (format) {
		ENUM_TO_STRING_CASE(MTLPixelFormatRGBA8Unorm)
		ENUM_TO_STRING_CASE(MTLPixelFormatRGBA8Unorm_sRGB)
		ENUM_TO_STRING_CASE(MTLPixelFormatBGRA8Unorm)
		ENUM_TO_STRING_CASE(MTLPixelFormatBGRA8Unorm_sRGB)
		ENUM_TO_STRING_CASE(MTLPixelFormatRGBA8Uint)
		ENUM_TO_STRING_CASE(MTLPixelFormatDepth24Unorm_Stencil8)
		ENUM_TO_STRING_CASE(MTLPixelFormatDepth32Float_Stencil8)
		ENUM_TO_STRING_CASE(MTLPixelFormatDepth32Float)
		default: {
			return String("Swapchain format ") + String::num_int64(int64_t(p_swapchain_format));
		} break;
	}
}

bool OpenXRMetalExtension::get_swapchain_image_data(XrSwapchain p_swapchain, int64_t p_swapchain_format, uint32_t p_width, uint32_t p_height, uint32_t p_sample_count, uint32_t p_array_size, void **r_swapchain_graphics_data) {
	LocalVector<XrSwapchainImageMetalKHR, uint32_t, false, true> images;

	ERR_FAIL_NULL_V(RD::data, false);

	uint32_t swapchain_length;
	XrResult result = xrEnumerateSwapchainImages(p_swapchain, 0, &swapchain_length, nullptr);
	if (XR_FAILED(result)) {
		print_line("OpenXR: Failed to get swapchain image count [", OpenXRAPI::get_error_string(result), "]");
		return false;
	}

	images.resize(swapchain_length);

	for (XrSwapchainImageMetalKHR &image : images) {
		image.type = XR_TYPE_SWAPCHAIN_IMAGE_METAL_KHR;
		image.next = nullptr;
		image.texture = nullptr;
	}

	result = xrEnumerateSwapchainImages(p_swapchain, swapchain_length, &swapchain_length, (XrSwapchainImageBaseHeader *)images.ptr());
	if (XR_FAILED(result)) {
		print_line("OpenXR: Failed to get swapchain images [", OpenXRAPI::get_error_string(result), "]");
		return false;
	}

	SwapchainGraphicsData *data = memnew(SwapchainGraphicsData);
	if (data == nullptr) {
		print_line("OpenXR: Failed to allocate memory for swapchain data");
		return false;
	}
	*r_swapchain_graphics_data = data;
	data->is_multiview = (p_array_size > 1);

	RenderingDevice::DataFormat format = RenderingDevice::DATA_FORMAT_R8G8B8A8_SRGB;
	RenderingDevice::TextureSamples samples = RenderingDevice::TEXTURE_SAMPLES_1;
	uint64_t usage_flags = RenderingDevice::TEXTURE_USAGE_SAMPLING_BIT;

	switch (p_swapchain_format) {
		case MTLPixelFormatRGBA8Unorm_sRGB:
			// Even though this is an sRGB framebuffer format we're using UNORM here.
			// The reason here is because Godot does a linear to sRGB conversion while
			// with the sRGB format, this conversion would be doubled by the hardware.
			// This also means we're reading the values as is for our preview on screen.
			// The OpenXR runtime however is still treating this as an sRGB format and
			// will thus do an sRGB -> Linear conversion as expected.
			//format = RenderingDevice::DATA_FORMAT_R8G8B8A8_SRGB;
			format = RenderingDevice::DATA_FORMAT_R8G8B8A8_UNORM;
			usage_flags |= RenderingDevice::TEXTURE_USAGE_COLOR_ATTACHMENT_BIT;
			break;
		case MTLPixelFormatBGRA8Unorm_sRGB:
			format = RenderingDevice::DATA_FORMAT_B8G8R8A8_UNORM;
			usage_flags |= RenderingDevice::TEXTURE_USAGE_COLOR_ATTACHMENT_BIT;
			break;
		case MTLPixelFormatRGBA8Uint:
			format = RenderingDevice::DATA_FORMAT_R8G8B8A8_UINT;
			usage_flags |= RenderingDevice::TEXTURE_USAGE_COLOR_ATTACHMENT_BIT;
			break;
		case MTLPixelFormatDepth32Float:
			format = RenderingDevice::DATA_FORMAT_D32_SFLOAT;
			usage_flags |= RenderingDevice::TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | RenderingDevice::TEXTURE_USAGE_DEPTH_RESOLVE_ATTACHMENT_BIT;
			break;
		case MTLPixelFormatDepth24Unorm_Stencil8:
			format = RenderingDevice::DATA_FORMAT_D24_UNORM_S8_UINT;
			usage_flags |= RenderingDevice::TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | RenderingDevice::TEXTURE_USAGE_DEPTH_RESOLVE_ATTACHMENT_BIT;
			break;
		case MTLPixelFormatDepth32Float_Stencil8:
			format = RenderingDevice::DATA_FORMAT_D32_SFLOAT_S8_UINT;
			usage_flags |= RenderingDevice::TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | RenderingDevice::TEXTURE_USAGE_DEPTH_RESOLVE_ATTACHMENT_BIT;
			break;
		default:
			// Continue with our default value.
			print_line("OpenXR: Unsupported swapchain format", p_swapchain_format);
			break;
	}

	switch (p_sample_count) {
		case 1:
			samples = RenderingDevice::TEXTURE_SAMPLES_1;
			break;
		case 2:
			samples = RenderingDevice::TEXTURE_SAMPLES_2;
			break;
		case 4:
			samples = RenderingDevice::TEXTURE_SAMPLES_4;
			break;
		case 8:
			samples = RenderingDevice::TEXTURE_SAMPLES_8;
			break;
		case 16:
			samples = RenderingDevice::TEXTURE_SAMPLES_16;
			break;
		case 32:
			samples = RenderingDevice::TEXTURE_SAMPLES_32;
			break;
		case 64:
			samples = RenderingDevice::TEXTURE_SAMPLES_64;
			break;
		default:
			// Continue with our default value.
			print_line("OpenXR: Unsupported sample count", p_sample_count);
			break;
	}

	Vector<RID> texture_rids;

	// Create Godot texture objects for each entry in our swapchain.
	for (uint64_t i = 0; i < swapchain_length; i++) {
		// Note, the formats we sent to render_device are ignored on metal.
		RID image_rid = RD::texture_create_from_extension(
				p_array_size == 1 ? RenderingDevice::TEXTURE_TYPE_2D : RenderingDevice::TEXTURE_TYPE_2D_ARRAY,
				format,
				samples,
				usage_flags,
				(uint64_t)images[i].texture,
				p_width,
				p_height,
				1,
				p_array_size,
				1);

		texture_rids.push_back(image_rid);
	}

	data->texture_rids = texture_rids;

	return true;
}

void OpenXRMetalExtension::cleanup_swapchain_graphics_data(void **p_swapchain_graphics_data) {
	if (*p_swapchain_graphics_data == nullptr) {
		return;
	}

	SwapchainGraphicsData *data = (SwapchainGraphicsData *)*p_swapchain_graphics_data;

	ERR_FAIL_NULL(RD::data);

	for (const RID &texture_rid : data->texture_rids) {
		RD::free_rid(texture_rid);
	}
	data->texture_rids.clear();

	memdelete(data);
	*p_swapchain_graphics_data = nullptr;
}

bool OpenXRMetalExtension::create_projection_fov(const XrFovf p_fov, double p_z_near, double p_z_far, Projection &r_camera_matrix) {
	OpenXRUtil::XrMatrix4x4f matrix;
	OpenXRUtil::XrMatrix4x4f_CreateProjectionFov(&matrix, p_fov, (float)p_z_near, (float)p_z_far);

	for (int j = 0; j < 4; j++) {
		for (int i = 0; i < 4; i++) {
			r_camera_matrix.columns[j][i] = matrix.m[j * 4 + i];
		}
	}

	return true;
}

RID OpenXRMetalExtension::get_texture(void *p_swapchain_graphics_data, int p_image_index) {
	SwapchainGraphicsData *data = (SwapchainGraphicsData *)p_swapchain_graphics_data;
	ERR_FAIL_NULL_V(data, RID());

	ERR_FAIL_INDEX_V(p_image_index, data->texture_rids.size(), RID());
	return data->texture_rids[p_image_index];
}
#endif // defined(MODULE_OPENXR_ENABLED) && defined(METAL_ENABLED)
