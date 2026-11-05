// glitchcal: scratch positive control for R-M25-8. Not part of CYMATICA.
// Raw WASAPI shared-mode render with injected stalls, mirroring the miniaudio write pattern
// (Start before first write, 30 ms buffer, GetBuffer(480) after each event), while sampling
// GetCurrentPadding/IAudioClock and running a raw loopback capture with flags and device position.
// Usage: glitchcal <amplitude> <cycles> <out_prefix>
#define WIN32_LEAN_AND_MEAN
#define NTDDI_VERSION 0x0A00000F
#define _WIN32_WINNT 0x0A00
#include <windows.h>
#include <initguid.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <endpointvolume.h>
#include <functiondiscoverykeys_devpkey.h>
#include <avrt.h>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <thread>
#include <vector>
#include <algorithm>
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "avrt.lib")

#define CK(x) do { HRESULT _h = (x); if (FAILED(_h)) { printf("FAIL %s hr=0x%08lX line %d\n", #x, (unsigned long)_h, __LINE__); return 1; } } while (0)

static LONGLONG qpcFreq;
static inline LONGLONG qpcNow() { LARGE_INTEGER q; QueryPerformanceCounter(&q); return q.QuadPart; }
static inline double qpcMs(LONGLONG d) { return 1000.0 * (double)d / (double)qpcFreq; }

struct RenderSample {
    LONGLONG tWake;      // QPC at loop wake
    LONGLONG tCallDone;  // QPC after padding + position calls
    UINT32 padding;
    UINT64 pos;
    UINT64 posQpc;       // 100 ns
    UINT32 framesWritten;
    int stallIdx;        // -1 or index of stall injected right after this sample
};
struct CapPacket {
    UINT32 frames; DWORD flags; UINT64 devPos; UINT64 qpc; LONGLONG tGot; UINT64 startIdx;
};

static std::atomic<bool> gStop{false};
static std::vector<float> gCap;      // first channel
static std::atomic<UINT64> gCapN{0};
static std::vector<CapPacket> gPk;
static std::atomic<size_t> gPkN{0};
static HRESULT gCapHr = S_OK;
static int gCapStallMs = 0;

static void captureThread(IAudioCaptureClient* cc, HANDLE ev, UINT32 ch) {
    DWORD ti = 0; HANDLE av = AvSetMmThreadCharacteristicsW(L"Pro Audio", &ti);
    (void)av;
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
    while (!gStop.load(std::memory_order_relaxed)) {
        WaitForSingleObject(ev, 20);
        if (gCapStallMs > 0 && gCapN.load() > 72000) { int ms = gCapStallMs; gCapStallMs = 0; printf("capture stall %d ms at frame %llu\n", ms, gCapN.load()); Sleep(ms); }
        UINT32 next = 0;
        while (SUCCEEDED(cc->GetNextPacketSize(&next)) && next > 0) {
            BYTE* d = nullptr; UINT32 n = 0; DWORD f = 0; UINT64 dp = 0, qp = 0;
            HRESULT hr = cc->GetBuffer(&d, &n, &f, &dp, &qp);
            if (FAILED(hr)) { gCapHr = hr; return; }
            size_t pi = gPkN.load(std::memory_order_relaxed);
            UINT64 start = gCapN.load(std::memory_order_relaxed);
            if (pi < gPk.size()) { gPk[pi] = {n, f, dp, qp, qpcNow(), start}; gPkN.store(pi + 1); }
            const float* s = (const float*)d;
            for (UINT32 i = 0; i < n && start + i < gCap.size(); ++i)
                gCap[start + i] = (f & AUDCLNT_BUFFERFLAGS_SILENT) ? 0.f : s[i * ch];
            gCapN.store(start + n);
            cc->ReleaseBuffer(n);
        }
    }
    AvRevertMmThreadCharacteristics(av);
}

int main(int argc, char** argv) {
    double amp = argc > 1 ? atof(argv[1]) : 1e-4;
    int cycles = argc > 2 ? atoi(argv[2]) : 3;
    const char* outp = argc > 3 ? argv[3] : "gc";
    LARGE_INTEGER f; QueryPerformanceFrequency(&f); qpcFreq = f.QuadPart;
    CK(CoInitializeEx(nullptr, COINIT_MULTITHREADED));
    IMMDeviceEnumerator* en = nullptr;
    CK(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&en)));
    const char* want = argc > 4 ? argv[4] : "-";
    gCapStallMs = argc > 6 ? atoi(argv[6]) : 0;
    IMMDevice* dev = nullptr; CK(en->GetDefaultAudioEndpoint(eRender, eConsole, &dev));
    {
        IMMDeviceCollection* col = nullptr; CK(en->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &col));
        UINT nDev = 0; col->GetCount(&nDev);
        for (UINT i = 0; i < nDev; ++i) {
            IMMDevice* d2 = nullptr; col->Item(i, &d2); IPropertyStore* p2 = nullptr; d2->OpenPropertyStore(STGM_READ, &p2);
            PROPVARIANT v2; PropVariantInit(&v2); p2->GetValue(PKEY_Device_FriendlyName, &v2);
            char nm8[256]; WideCharToMultiByte(CP_UTF8, 0, v2.pwszVal, -1, nm8, 256, nullptr, nullptr);
            printf("active endpoint %u: %s\n", i, nm8);
            if (strcmp(want, "-") != 0 && strstr(nm8, want)) { dev->Release(); dev = d2; d2 = nullptr; }
            if (d2) d2->Release();
        }
        if (argc > 5 && strcmp(argv[5], "list") == 0) return 0;
    }
    {
        IAudioEndpointVolume* ev = nullptr;
        if (SUCCEEDED(dev->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, nullptr, (void**)&ev))) {
            float sc = -1; BOOL mu = 0; DWORD hw = 0; ev->GetMasterVolumeLevelScalar(&sc); ev->GetMute(&mu); ev->QueryHardwareSupport(&hw);
            printf("endpoint volume scalar=%.3f mute=%d hwsupport=0x%lX\n", sc, mu, hw); ev->Release();
        }
    }
    IPropertyStore* ps = nullptr; CK(dev->OpenPropertyStore(STGM_READ, &ps));
    PROPVARIANT pv; PropVariantInit(&pv); CK(ps->GetValue(PKEY_Device_FriendlyName, &pv));
    printf("endpoint: %ls\n", pv.pwszVal);

    IAudioClient3* q3 = nullptr;
    if (SUCCEEDED(dev->Activate(__uuidof(IAudioClient3), CLSCTX_ALL, nullptr, (void**)&q3))) {
        WAVEFORMATEX* w = nullptr; q3->GetMixFormat(&w);
        UINT32 d=0,fu=0,mi=0,ma=0; HRESULT h = q3->GetSharedModeEnginePeriod(w, &d, &fu, &mi, &ma);
        printf("engine periods hr=0x%08lX default=%u fundamental=%u min=%u max=%u\n", (unsigned long)h, d, fu, mi, ma);
        WAVEFORMATEX* w2 = nullptr; UINT32 cur = 0; h = q3->GetCurrentSharedModeEnginePeriod(&w2, &cur);
        printf("current engine period hr=0x%08lX frames=%u\n", (unsigned long)h, cur);
        CoTaskMemFree(w); CoTaskMemFree(w2); q3->Release();
    }

    IAudioClient* rc = nullptr; CK(dev->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&rc));
    WAVEFORMATEX* wf = nullptr; CK(rc->GetMixFormat(&wf));
    bool isFloat = wf->wFormatTag == WAVE_FORMAT_IEEE_FLOAT ||
        (wf->wFormatTag == WAVE_FORMAT_EXTENSIBLE && ((WAVEFORMATEXTENSIBLE*)wf)->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT);
    printf("mix: tag=0x%04X ch=%u rate=%lu bits=%u float=%d\n", wf->wFormatTag, wf->nChannels, wf->nSamplesPerSec, wf->wBitsPerSample, isFloat);
    if (!isFloat || wf->wBitsPerSample != 32) { printf("unsupported mix format\n"); return 1; }
    const UINT32 ch = wf->nChannels, rate = wf->nSamplesPerSec, reqFrames = rate / 100;
    REFERENCE_TIME defP = 0, minP = 0; CK(rc->GetDevicePeriod(&defP, &minP));
    printf("device period default=%.3f ms min=%.3f ms\n", defP / 1e4, minP / 1e4);
    HANDLE evR = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    CK(rc->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_EVENTCALLBACK, 300000, 0, wf, nullptr));
    CK(rc->SetEventHandle(evR));
    UINT32 bufFrames = 0; CK(rc->GetBufferSize(&bufFrames));
    REFERENCE_TIME lat = 0; rc->GetStreamLatency(&lat);
    printf("render buffer=%u frames (%.2f ms), stream latency=%.3f ms\n", bufFrames, 1000.0 * bufFrames / rate, lat / 1e4);
    IAudioRenderClient* rr = nullptr; CK(rc->GetService(IID_PPV_ARGS(&rr)));
    IAudioClock* clk = nullptr; CK(rc->GetService(IID_PPV_ARGS(&clk)));
    UINT64 clkF = 0; CK(clk->GetFrequency(&clkF)); printf("clock frequency=%llu\n", clkF);
    IAudioEffectsManager* fx = nullptr;
    HRESULT hfx = rc->GetService(IID_PPV_ARGS(&fx));
    printf("IAudioEffectsManager hr=0x%08lX\n", (unsigned long)hfx);
    if (SUCCEEDED(hfx)) {
        AUDIO_EFFECT* e = nullptr; UINT32 n = 0; HRESULT h = fx->GetAudioEffects(&e, &n);
        printf("effects hr=0x%08lX count=%u\n", (unsigned long)h, n);
        for (UINT32 i = 0; i < n; ++i) {
            wchar_t g[64]; StringFromGUID2(e[i].id, g, 64);
            printf("  effect %ls canSet=%d state=%d\n", g, (int)e[i].canSetState, (int)e[i].state);
        }
        CoTaskMemFree(e); fx->Release();
    }

    IAudioClient* lc = nullptr; CK(dev->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&lc));
    HANDLE evC = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    CK(lc->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK | AUDCLNT_STREAMFLAGS_EVENTCALLBACK, 20000000, 0, wf, nullptr));
    CK(lc->SetEventHandle(evC));
    UINT32 capBuf = 0; lc->GetBufferSize(&capBuf); printf("loopback buffer=%u frames\n", capBuf);
    IAudioCaptureClient* cc = nullptr; CK(lc->GetService(IID_PPV_ARGS(&cc)));

    std::vector<int> stallV = {15, 25, 35, 45, 60, 100};
    if (argc > 5) { stallV.clear(); const char* c = argv[5]; while (*c) { stallV.push_back(atoi(c)); while (*c && *c != ',') ++c; if (*c == ',') ++c; } }
    const int* stallMs = stallV.data();
    const int nStall = (int)stallV.size();
    const double baselineS = 3.0, gapS = 1.5;
    const double totalS = baselineS + gapS * nStall * cycles + 2.0;
    const size_t maxS = (size_t)(totalS * 300);
    std::vector<RenderSample> log(maxS);
    gCap.assign((size_t)((totalS + 3) * rate), 0.f);
    gPk.resize((size_t)((totalS + 3) * 1200));
    struct StallRec { int ms; LONGLONG t0, t1; UINT64 renderedFrames; };
    std::vector<StallRec> stalls;
    size_t nLog = 0;
    double costMaxPad = 0, costMaxPos = 0, costSumPad = 0, costSumPos = 0; UINT64 costN = 0;

    std::thread cap(captureThread, cc, evC, ch);
    CK(lc->Start());
    Sleep(200);
    DWORD ti = 0; HANDLE av = AvSetMmThreadCharacteristicsW(L"Games", &ti);
    printf("render mmcss=%d\n", av != nullptr);
    CK(rc->Start());
    const LONGLONG t0 = qpcNow();
    const double w = 2.0 * 3.14159265358979323846 * 997.0 / rate;
    UINT64 phaseN = 0;
    int nextStall = 0; const int totalStalls = nStall * cycles;
    double nextT = baselineS;
    for (;;) {
        DWORD wr = WaitForSingleObject(evR, 200);
        if (wr != WAIT_OBJECT_0) { printf("wait timeout\n"); break; }
        LONGLONG tw = qpcNow();
        UINT32 pad = 0; rc->GetCurrentPadding(&pad);
        LONGLONG t1 = qpcNow();
        UINT64 p = 0, pq = 0; clk->GetPosition(&p, &pq);
        LONGLONG t2 = qpcNow();
        double cp = qpcMs(t1 - tw), cq = qpcMs(t2 - t1);
        costMaxPad = max(costMaxPad, cp); costMaxPos = max(costMaxPos, cq); costSumPad += cp; costSumPos += cq; ++costN;
        UINT32 written = 0;
        for (;;) {
            BYTE* d = nullptr;
            HRESULT hr = rr->GetBuffer(reqFrames, &d);
            if (hr == AUDCLNT_E_BUFFER_TOO_LARGE) break;
            if (FAILED(hr)) { printf("GetBuffer hr=0x%08lX\n", (unsigned long)hr); break; }
            float* o = (float*)d;
            for (UINT32 i = 0; i < reqFrames; ++i) {
                float v = (float)(amp * sin(w * (double)phaseN++));
                for (UINT32 c = 0; c < ch; ++c) o[i * ch + c] = v;
            }
            rr->ReleaseBuffer(reqFrames, 0);
            written += reqFrames;
        }
        if (nLog < log.size()) log[nLog++] = {tw, t2, pad, p, pq, written, -1};
        double t = qpcMs(qpcNow() - t0) / 1000.0;
        if (nextStall < totalStalls && t >= nextT) {
            int ms = stallMs[nextStall % nStall];
            LONGLONG s0 = qpcNow(), target = s0 + (LONGLONG)(ms * qpcFreq / 1000);
            while (qpcNow() < target) { YieldProcessor(); }
            stalls.push_back({ms, s0, qpcNow(), phaseN});
            if (nLog) log[nLog - 1].stallIdx = nextStall;
            ++nextStall; nextT += gapS;
        }
        if (t >= totalS - 0.2) break;
    }
    Sleep(300);
    rc->Stop(); Sleep(100); gStop = true; SetEvent(evC); cap.join(); lc->Stop();
    if (av) AvRevertMmThreadCharacteristics(av);

    // ---- analysis
    const UINT64 nCap = gCapN.load(); const size_t nPk = gPkN.load();
    printf("render samples=%zu loopback frames=%llu packets=%zu capHr=0x%08lX\n", nLog, nCap, nPk, (unsigned long)gCapHr);
    printf("call cost: padding mean=%.4f max=%.4f ms; GetPosition mean=%.4f max=%.4f ms\n", costSumPad / costN, costMaxPad, costSumPos / costN, costMaxPos);

    // padding/gap in baseline (first 3 s minus first 0.5 s)
    double blGapMax = 0; UINT32 blPadMin = 0xFFFFFFFF, blPadMax = 0; double blIntMax = 0; int blN = 0;
    for (size_t i = 1; i < nLog; ++i) {
        double t = qpcMs(log[i].tWake - t0) / 1000.0;
        if (t < 0.5 || t > baselineS - 0.05) continue;
        double dq = (double)(log[i].posQpc - log[i-1].posQpc) / 1e4;
        double dp = 1000.0 * (double)(log[i].pos - log[i-1].pos) / (double)clkF;
        blGapMax = max(blGapMax, dq - dp);
        blPadMin = min(blPadMin, log[i].padding); blPadMax = max(blPadMax, log[i].padding);
        blIntMax = max(blIntMax, qpcMs(log[i].tWake - log[i-1].tWake)); ++blN;
    }
    printf("baseline n=%d padding min=%u max=%u frames; interval max=%.2f ms; position gap max=%.3f ms\n", blN, blPadMin, blPadMax, blIntMax, blGapMax);

    // silence runs in capture (below threshold), for the tone at amp
    const float th = (float)(amp * 0.02);
    struct Run { UINT64 start; UINT64 len; };
    std::vector<Run> runs; { UINT64 s = 0; bool in = false;
        for (UINT64 i = 0; i < nCap; ++i) {
            bool q = fabsf(gCap[(size_t)i]) < th;
            if (q && !in) { in = true; s = i; }
            if (!q && in) { in = false; if (i - s >= 12) runs.push_back({s, i - s}); }
        }
        if (in && nCap - s >= 12) runs.push_back({s, nCap - s});
    }
    double rms = 0; { UINT64 a = (UINT64)(0.8 * rate), b = (UINT64)(2.5 * rate); for (UINT64 i = a; i < b && i < nCap; ++i) rms += gCap[(size_t)i] * gCap[(size_t)i]; rms = sqrt(rms / (double)(b > a ? b - a : 1)); }
    printf("loopback baseline rms=%.3e (tone rms expected %.3e); silence runs>=0.25ms: %zu\n", rms, amp / sqrt(2.0), runs.size());

    // map a capture frame to QPC through packets (qpc reported by GetBuffer, 100 ns)
    auto frameToQpcMs = [&](UINT64 idx) -> double {
        size_t lo = 0, hi = nPk; while (lo + 1 < hi) { size_t m = (lo + hi) / 2; if (gPk[m].startIdx <= idx) lo = m; else hi = m; }
        double pkMs = (double)gPk[lo].qpc / 1e4;
        return pkMs + 1000.0 * (double)(idx - gPk[lo].startIdx) / rate;
    };
    const double t0Ms100 = 1e3 * (double)t0 / (double)qpcFreq;

    // packets: flags and device position continuity
    UINT64 disc = 0, silent = 0, tserr = 0, posJumps = 0; size_t firstDisc = (size_t)-1;
    for (size_t i = 0; i < nPk; ++i) {
        if (gPk[i].flags & AUDCLNT_BUFFERFLAGS_DATA_DISCONTINUITY) { ++disc; if (firstDisc == (size_t)-1) firstDisc = i; }
        if (gPk[i].flags & AUDCLNT_BUFFERFLAGS_SILENT) ++silent;
        if (gPk[i].flags & AUDCLNT_BUFFERFLAGS_TIMESTAMP_ERROR) ++tserr;
        if (i && gPk[i].devPos != gPk[i-1].devPos + gPk[i-1].frames) ++posJumps;
    }
    printf("packets: discontinuity=%llu first_idx=%lld silent=%llu tserr=%llu devpos_noncontiguous=%llu\n", disc, (long long)(firstDisc == (size_t)-1 ? -1 : (long long)firstDisc), silent, tserr, posJumps);

    FILE* fp = nullptr; char nm[260];
    snprintf(nm, sizeof nm, "%s_stalls.csv", outp); fopen_s(&fp, nm, "w");
    if (fp) fprintf(fp, "cycle,stall_ms,actual_ms,interval_ms,min_padding_after,padding_at_wake,position_gap_sum_ms,capture_silence_ms,capture_disc_flags,capture_devpos_jumps,capture_devpos_jump_frames\n");
    printf("%-5s %-6s %-8s %-9s %-8s %-9s %-9s %-6s %-6s\n", "cycle", "stall", "actual", "interval", "padWake", "posGapMs", "capSilMs", "disc", "jumps");
    double cumPosLost = 0, cumSil = 0;
    int sidx = 0;
    for (size_t i = 0; i + 1 < nLog; ++i) {
        if (log[i].stallIdx < 0) continue;
        int k = log[i].stallIdx; const StallRec& s = stalls[k];
        double interval = qpcMs(log[i+1].tWake - log[i].tWake);
        // position gap accumulated over the following 300 ms
        double gapSum = 0; UINT32 padMin = 0xFFFFFFFF;
        for (size_t j = i + 1; j < nLog && qpcMs(log[j].tWake - s.t1) < 300.0; ++j) {
            double dq = (double)(log[j].posQpc - log[j-1].posQpc) / 1e4;
            double dp = 1000.0 * (double)(log[j].pos - log[j-1].pos) / (double)clkF;
            gapSum += dq - dp;
            padMin = min(padMin, log[j].padding);
        }
        double wMs0 = qpcMs(s.t0 - t0) - 100.0, wMs1 = qpcMs(s.t1 - t0) + 400.0;
        double sil = 0;
        for (auto& r : runs) {
            double tq = frameToQpcMs(r.start) - t0Ms100;
            if (tq >= wMs0 && tq <= wMs1) sil += 1000.0 * r.len / rate;
        }
        int dflags = 0, jumps = 0; long long jumpFrames = 0;
        for (size_t pi = 1; pi < nPk; ++pi) {
            double tq = (double)gPk[pi].qpc / 1e4 - t0Ms100;
            if (tq < wMs0 || tq > wMs1) continue;
            if (gPk[pi].flags & AUDCLNT_BUFFERFLAGS_DATA_DISCONTINUITY) ++dflags;
            if (gPk[pi].devPos != gPk[pi-1].devPos + gPk[pi-1].frames) { ++jumps; jumpFrames += (long long)gPk[pi].devPos - (long long)(gPk[pi-1].devPos + gPk[pi-1].frames); }
        }
        cumPosLost += gapSum; cumSil += sil;
        printf("%-5d %-6d %-8.2f %-9.2f %-8u %-9.2f %-9.2f %-6d %-6d (minPad300=%u)\n", k / nStall, s.ms, qpcMs(s.t1 - s.t0), interval, log[i+1].padding, gapSum, sil, dflags, jumps, padMin);
        if (fp) fprintf(fp, "%d,%d,%.3f,%.3f,%u,%u,%.3f,%.3f,%d,%d,%lld\n", k / nStall, s.ms, qpcMs(s.t1 - s.t0), interval, padMin, log[i+1].padding, gapSum, sil, dflags, jumps, jumpFrames);
        ++sidx;
    }
    if (fp) fclose(fp);
    double streamMs = qpcMs(log[nLog-1].tWake - log[0].tWake);
    printf("total: stream wall=%.1f ms; position advanced=%.1f ms; deficit=%.2f ms; capture frames/rate=%.1f ms; sum silence=%.2f ms; sum gap=%.2f ms\n",
        streamMs, 1000.0 * (double)(log[nLog-1].pos - log[0].pos) / (double)clkF,
        streamMs - 1000.0 * (double)(log[nLog-1].pos - log[0].pos) / (double)clkF, 1000.0 * (double)nCap / rate, cumSil, cumPosLost);
    // silence runs outside stall windows
    int outside = 0; for (auto& r : runs) { double tq = frameToQpcMs(r.start) - t0Ms100; bool in = false;
        for (auto& s : stalls) { if (tq >= qpcMs(s.t0 - t0) - 100 && tq <= qpcMs(s.t1 - t0) + 400) in = true; }
        if (!in && tq > 300) { ++outside; if (outside <= 8) printf("  run outside stalls: t=%.1f ms len=%.2f ms\n", tq, 1000.0 * r.len / rate); } }
    printf("silence runs outside stall windows (after 300 ms): %d\n", outside);
    snprintf(nm, sizeof nm, "%s_cap.f32", outp); fopen_s(&fp, nm, "wb"); if (fp) { fwrite(gCap.data(), 4, (size_t)nCap, fp); fclose(fp); }
    return 0;
}

