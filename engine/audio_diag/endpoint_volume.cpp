#include "endpoint_volume.h"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <audiopolicy.h>
#include <endpointvolume.h>
#include <mmdeviceapi.h>
#endif

namespace cymatica::audio {

#ifdef _WIN32
namespace {

template <typename T>
struct ComPtr {
    T* p{nullptr};
    ~ComPtr() {
        if (p != nullptr) p->Release();
    }
};

} // namespace
#endif

// Read only: queries the default render endpoint, changes nothing. Called outside the audio callback.
EndpointVolumeReading readDefaultEndpointVolume() noexcept {
    EndpointVolumeReading r;
#ifdef _WIN32
    const HRESULT init = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    const bool uninit = SUCCEEDED(init); // S_OK or S_FALSE; RPC_E_CHANGED_MODE keeps the caller's apartment
    {
        ComPtr<IMMDeviceEnumerator> enumerator;
        ComPtr<IMMDevice> device;
        ComPtr<IAudioEndpointVolume> volume;
        if (SUCCEEDED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
                                       reinterpret_cast<void**>(&enumerator.p))) &&
            SUCCEEDED(enumerator.p->GetDefaultAudioEndpoint(eRender, eConsole, &device.p)) &&
            SUCCEEDED(device.p->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, nullptr,
                                         reinterpret_cast<void**>(&volume.p)))) {
            float db = 0.0f, scalar = 0.0f, minDb = 0.0f, maxDb = 0.0f, stepDb = 0.0f;
            BOOL mute = FALSE;
            DWORD hardware = 0;
            if (SUCCEEDED(volume.p->GetMasterVolumeLevel(&db)) && SUCCEEDED(volume.p->GetMasterVolumeLevelScalar(&scalar)) &&
                SUCCEEDED(volume.p->GetMute(&mute)) && SUCCEEDED(volume.p->GetVolumeRange(&minDb, &maxDb, &stepDb)) &&
                SUCCEEDED(volume.p->QueryHardwareSupport(&hardware))) {
                r.valid = true;
                r.masterDb = db;
                r.scalar = scalar;
                r.mute = mute != FALSE;
                r.minDb = minDb;
                r.maxDb = maxDb;
                r.stepDb = stepDb;
                r.hardwareVolume = (hardware & ENDPOINT_HARDWARE_SUPPORT_VOLUME) != 0;
                r.hardwareMute = (hardware & ENDPOINT_HARDWARE_SUPPORT_MUTE) != 0;
            }
        }
    }
    if (uninit) CoUninitialize();
#endif
    return r;
}

SessionVolumeReading readProcessSessionVolumes() noexcept {
    SessionVolumeReading r;
#ifdef _WIN32
    const HRESULT init = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    const bool uninit = SUCCEEDED(init);
    {
        ComPtr<IMMDeviceEnumerator> enumerator;
        ComPtr<IMMDevice> device;
        ComPtr<IAudioSessionManager2> manager;
        ComPtr<IAudioSessionEnumerator> sessions;
        int count = 0;
        if (SUCCEEDED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
                                       reinterpret_cast<void**>(&enumerator.p))) &&
            SUCCEEDED(enumerator.p->GetDefaultAudioEndpoint(eRender, eConsole, &device.p)) &&
            SUCCEEDED(device.p->Activate(__uuidof(IAudioSessionManager2), CLSCTX_ALL, nullptr, reinterpret_cast<void**>(&manager.p))) &&
            SUCCEEDED(manager.p->GetSessionEnumerator(&sessions.p)) && SUCCEEDED(sessions.p->GetCount(&count))) {
            r.valid = true;
            const DWORD self = GetCurrentProcessId();
            for (int i = 0; i < count && r.count < r.sessions.size(); ++i) {
                ComPtr<IAudioSessionControl> control;
                ComPtr<IAudioSessionControl2> control2;
                DWORD pid = 0;
                if (FAILED(sessions.p->GetSession(i, &control.p)) ||
                    FAILED(control.p->QueryInterface(__uuidof(IAudioSessionControl2), reinterpret_cast<void**>(&control2.p))) ||
                    FAILED(control2.p->GetProcessId(&pid)) || pid != self) {
                    continue;
                }
                SessionVolumeEntry e;
                AudioSessionState state = AudioSessionStateInactive;
                if (SUCCEEDED(control.p->GetState(&state))) e.active = state == AudioSessionStateActive;
                ComPtr<ISimpleAudioVolume> simple;
                if (SUCCEEDED(control.p->QueryInterface(__uuidof(ISimpleAudioVolume), reinterpret_cast<void**>(&simple.p)))) {
                    float level = 1.0f;
                    BOOL mute = FALSE;
                    if (SUCCEEDED(simple.p->GetMasterVolume(&level))) e.level = level;
                    if (SUCCEEDED(simple.p->GetMute(&mute))) e.mute = mute != FALSE;
                }
                ComPtr<IChannelAudioVolume> channel;
                UINT32 channels = 0;
                if (SUCCEEDED(control.p->QueryInterface(__uuidof(IChannelAudioVolume), reinterpret_cast<void**>(&channel.p))) &&
                    SUCCEEDED(channel.p->GetChannelCount(&channels))) {
                    e.channels = channels < e.channelLevels.size() ? channels : static_cast<UINT32>(e.channelLevels.size());
                    for (UINT32 c = 0; c < e.channels; ++c) {
                        float level = 1.0f;
                        if (SUCCEEDED(channel.p->GetChannelVolume(c, &level))) e.channelLevels[c] = level;
                    }
                }
                r.sessions[r.count++] = e;
            }
        }
    }
    if (uninit) CoUninitialize();
#endif
    return r;
}

} // namespace cymatica::audio
