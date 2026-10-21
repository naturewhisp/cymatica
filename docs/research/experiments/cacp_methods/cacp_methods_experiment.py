"""CACP perceptual-evaluation methods experiment (M2.4 Inc-5 dossier).

Inputs : fixture_probe_v1.wav (48 kHz stereo 16-bit) and fixture_intents.csv (planned NoteOns, seed 1337).
Outputs: onset-detection results vs Music Intent (text), annotated/plain mel spectrogram PNGs, a blind
         window set with a separate key file, and timings.
Only numpy and the standard library are used. All analysis parameters are fixed a priori (not tuned on the
ground truth); the sensitivity sweep is reported, not used to select a value.

Usage: python cacp_methods_experiment.py <fixture.wav> <intents.csv> <out_dir>
"""
import csv
import json
import os
import random
import struct
import sys
import time
import wave
import zlib

import numpy as np

SR = 48000
N_FFT = 2048
HOP = 256                 # 5.33 ms
TOL_S = 0.050             # mir_eval default onset tolerance
MIN_SEP_S = 0.030         # minimum distance between detected onsets
LOG_GAMMA = 100.0         # log compression log(1 + gamma |X|)
THR_WIN_S = 0.100         # half width of the moving-average threshold window
DELTAS = (0.05, 0.10, 0.20)  # threshold offsets on the normalised novelty (0.10 = nominal)
BOUNDARIES = (0, 3072000, 5705142, 8777142)


def read_wav(path):
    with wave.open(path) as w:
        assert w.getframerate() == SR and w.getnchannels() == 2 and w.getsampwidth() == 2
        x = np.frombuffer(w.readframes(w.getnframes()), dtype='<i2').reshape(-1, 2).astype(np.float64) / 32768.0
    return x


def stft_mag(sig):
    win = np.hanning(N_FFT)
    n = 1 + (len(sig) - N_FFT) // HOP
    out = np.empty((n, N_FFT // 2 + 1), dtype=np.float32)
    chunk = 2048
    for s in range(0, n, chunk):
        k = np.arange(s, min(n, s + chunk))
        idx = np.arange(N_FFT)[None, :] + HOP * k[:, None]
        out[s:s + len(k)] = np.abs(np.fft.rfft(sig[idx] * win, axis=1))
    return out  # [time, freq]


def mel_filterbank(n_mels=128, fmin=30.0, fmax=16000.0):
    def hz2mel(f):
        return 2595.0 * np.log10(1.0 + f / 700.0)

    def mel2hz(m):
        return 700.0 * (10 ** (m / 2595.0) - 1.0)
    mels = np.linspace(hz2mel(fmin), hz2mel(fmax), n_mels + 2)
    hz = mel2hz(mels)
    bins = np.fft.rfftfreq(N_FFT, 1.0 / SR)
    fb = np.zeros((n_mels, len(bins)))
    for m in range(n_mels):
        lo, ce, hi = hz[m], hz[m + 1], hz[m + 2]
        fb[m] = np.clip(np.minimum((bins - lo) / (ce - lo), (hi - bins) / (hi - ce)), 0, None)
    return fb


def novelty(mag):
    lm = np.log1p(LOG_GAMMA * mag)
    d = np.diff(lm, axis=0)
    nov = np.concatenate([[0.0], np.maximum(d, 0.0).sum(axis=1)])
    nov /= max(nov.max(), 1e-12)
    return nov


def pick_peaks(nov, delta):
    w = int(round(THR_WIN_S * SR / HOP))
    kernel = np.ones(2 * w + 1) / (2 * w + 1)
    thr = np.convolve(nov, kernel, mode='same') + delta
    cand = np.flatnonzero((nov[1:-1] > nov[:-2]) & (nov[1:-1] >= nov[2:]) & (nov[1:-1] > thr[1:-1])) + 1
    sep = int(round(MIN_SEP_S * SR / HOP))
    out = []
    for c in cand:
        if out and c - out[-1] < sep:
            if nov[c] > nov[out[-1]]:
                out[-1] = c
            continue
        out.append(c)
    # frame index -> time of the frame centre
    return (np.array(out) * HOP + N_FFT / 2) / SR


def match(ref, est, tol=TOL_S):
    """Greedy one-to-one matching in time order (mir_eval-style tolerance window)."""
    ref = np.sort(ref)
    est = np.sort(est)
    used = np.zeros(len(est), dtype=bool)
    hits = []
    j0 = 0
    for i, r in enumerate(ref):
        while j0 < len(est) and est[j0] < r - tol:
            j0 += 1
        best, bd = -1, tol + 1
        j = j0
        while j < len(est) and est[j] <= r + tol:
            if not used[j] and abs(est[j] - r) < bd:
                best, bd = j, abs(est[j] - r)
            j += 1
        if best >= 0:
            used[best] = True
            hits.append((i, best, est[best] - r))
    tp = len(hits)
    p = tp / len(est) if len(est) else 0.0
    rc = tp / len(ref) if len(ref) else 0.0
    f = 2 * p * rc / (p + rc) if p + rc else 0.0
    return p, rc, f, hits


def png_write(path, rgb):
    h, w, _ = rgb.shape
    raw = b''.join(b'\x00' + rgb[y].astype(np.uint8).tobytes() for y in range(h))

    def chunk(t, d):
        return struct.pack('>I', len(d)) + t + d + struct.pack('>I', zlib.crc32(t + d) & 0xffffffff)
    with open(path, 'wb') as f:
        f.write(b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 2, 0, 0, 0))
                + chunk(b'IDAT', zlib.compress(raw, 9)) + chunk(b'IEND', b''))


def colormap(v):
    # perceptually ordered dark-blue -> purple -> orange -> yellow ramp (magma-like anchors)
    anchors = np.array([[0, 0, 4], [59, 15, 112], [140, 41, 129], [222, 73, 104], [254, 159, 109], [252, 253, 191]], float)
    pos = np.clip(v, 0, 1) * (len(anchors) - 1)
    i = np.minimum(pos.astype(int), len(anchors) - 2)
    t = (pos - i)[..., None]
    return anchors[i] * (1 - t) + anchors[i + 1] * t


def mel_image(mag_win, fb, width, height_px_per_mel=2, db_range=80.0):
    mel = mag_win @ fb.T                       # [time, mel]
    db = 20 * np.log10(np.maximum(mel, 1e-10))
    db = np.clip((db - (db.max() - db_range)) / db_range, 0, 1)
    # resample time axis to `width` columns by max-pooling
    t = db.shape[0]
    edges = np.linspace(0, t, width + 1).astype(int)
    cols = np.stack([db[edges[k]:max(edges[k + 1], edges[k] + 1)].max(axis=0) for k in range(width)])
    img = cols.T[::-1]                         # mel low at bottom
    img = np.repeat(img, height_px_per_mel, axis=0)
    return colormap(img)


def frame_to_bar(fr):
    if fr < 3072000:
        return fr / 96000
    if fr < 5705142:
        return 32 + (fr - 3072000) / (4 * SR * 60 / 140)
    return 64 + (fr - 5705142) / 96000


def bar_to_frame(bar):
    if bar < 32:
        return int(bar * 96000)
    if bar < 64:
        return 3072000 + int((bar - 32) * 4 * SR * 60 / 140)
    return 5705142 + int((bar - 64) * 96000)


def main():
    wav_path, csv_path, out = sys.argv[1], sys.argv[2], sys.argv[3]
    os.makedirs(out, exist_ok=True)
    report = {}
    x = read_wav(wav_path)
    content = x[:BOUNDARIES[-1]]
    mono = content.mean(axis=1)
    side = (content[:, 0] - content[:, 1]) / 2

    rows = [r for r in csv.DictReader(open(csv_path)) if not r['event_id'].startswith('#')]
    on_frames = np.array([int(r['sample_frame']) for r in rows])
    roles = np.array([int(r['role']) for r in rows])
    uniq = np.unique(on_frames)
    report['planned_noteons'] = int(len(rows))
    report['unique_onset_frames'] = int(len(uniq))

    t0 = time.perf_counter()
    mag = stft_mag(mono)
    t_stft = time.perf_counter() - t0
    t0 = time.perf_counter()
    nov = novelty(mag)
    t_nov = time.perf_counter() - t0
    report['timing_s'] = {'stft_mono_182s': round(t_stft, 3), 'novelty': round(t_nov, 3),
                          'frames': int(mag.shape[0]), 'n_fft': N_FFT, 'hop': HOP}

    ref_t = uniq / SR
    res = {}
    for delta in DELTAS:
        est = pick_peaks(nov, delta)
        p, rc, f, hits = match(ref_t, est)
        offs = np.array([h[2] for h in hits]) * 1000 if hits else np.array([0.0])
        per_section = {}
        for s in range(3):
            lo, hi = BOUNDARIES[s] / SR, BOUNDARIES[s + 1] / SR
            rs = ref_t[(ref_t >= lo) & (ref_t < hi)]
            es = est[(est >= lo) & (est < hi)]
            ps, rs_, fs, _ = match(rs, es)
            per_section[s] = {'precision': round(ps, 3), 'recall': round(rs_, 3), 'f': round(fs, 3), 'ref': int(len(rs)), 'est': int(len(es))}
        # per-role recall: a planned onset of role r counts as detected if any detection lies within TOL
        per_role = {}
        for r, name in enumerate(['Pulso', 'Corpo', 'Trama', 'Vettore']):
            fr = np.unique(on_frames[roles == r]) / SR
            if len(fr) == 0:
                continue
            j = np.searchsorted(est, fr)
            d = np.minimum(np.abs(est[np.clip(j, 0, len(est) - 1)] - fr), np.abs(est[np.clip(j - 1, 0, len(est) - 1)] - fr))
            per_role[name] = {'onsets': int(len(fr)), 'recall': round(float((d <= TOL_S).mean()), 3)}
        res[str(delta)] = {'detected': int(len(est)), 'precision': round(p, 3), 'recall': round(rc, 3), 'f': round(f, 3),
                           'offset_ms_median': round(float(np.median(offs)), 2),
                           'offset_ms_p05_p95': [round(float(np.percentile(offs, 5)), 2), round(float(np.percentile(offs, 95)), 2)],
                           'per_section': per_section, 'per_role_recall': per_role}
    report['onsets'] = res

    # --- spectrogram images ---
    fb = mel_filterbank()
    report['png'] = {}
    full = mel_image(mag, fb, width=1830)
    ann = full.copy()
    for b in range(0, 97, 4):
        col = int(round(bar_to_frame(b) / BOUNDARIES[-1] * (ann.shape[1] - 1)))
        ann[:, col] = [255, 255, 255] if b in (32, 64) else [110, 110, 110]
    png_write(os.path.join(out, 'fixture_mel_full_plain.png'), full)
    png_write(os.path.join(out, 'fixture_mel_full_annotated.png'), ann)

    # blind window set: Trama absent (bars 0, 8) vs present (16, 24, 40, 72); 4-bar windows, shuffled labels
    windows = [int(v) for v in os.environ.get('CACP_BLIND_BARS', '0,8,16,24,40,72').split(',')]
    rng = random.Random(int(os.environ.get('CACP_BLIND_SEED', '20261021')))
    labels = list(os.environ.get('CACP_BLIND_LABELS', 'ABCDEF'))
    rng.shuffle(labels)
    key = {}
    for bar, lab in zip(windows, labels):
        a, b = bar_to_frame(bar), bar_to_frame(bar + 4)
        fa, fb_ = a // HOP, b // HOP
        img = mel_image(mag[fa:fb_], fb, width=900)
        png_write(os.path.join(out, f'blind_window_{lab}.png'), img)
        sm = 10 * np.log10((side[a:b] ** 2).sum() / max(((content[a:b].mean(axis=1)) ** 2).sum(), 1e-20))
        key[lab] = {'bar': bar, 'trama_planned_onsets': int(((on_frames >= a) & (on_frames < b) & (roles == 2)).sum()),
                    'side_mid_db': round(float(sm), 2)}
    json.dump(key, open(os.path.join(out, os.environ.get('CACP_BLIND_KEY', 'blind_key.json')), 'w'), indent=2, sort_keys=True)
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
