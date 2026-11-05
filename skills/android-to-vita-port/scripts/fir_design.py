#!/usr/bin/env python3
"""Measure a fractional-delay Kaiser FIR resampler.

This mirrors the resampler used in the Bruce Lee port:
  - P phases, 2*K taps, cutoff fc (relative to input Nyquist), Kaiser beta
  - per-phase DC normalization
  - streaming block size (default 64)

Usage:
    python fir_design.py --tone 15000
    python fir_design.py --tone 10000 --k 24 --fc 0.42 --beta 10
    python fir_design.py --sweep

It prints the largest non-fundamental peak (alias/spur) and its frequency.
Requires numpy.
"""
from __future__ import annotations

import argparse
import math

try:
    import numpy as np
except Exception:
    print("numpy is required: pip install numpy")
    raise SystemExit(2)


def kaiser_i0(x):
    x = np.asarray(x, dtype=float)
    s = np.ones_like(x)
    y = x * x / 4.0
    term = np.ones_like(x)
    for k in range(1, 80):
        term = term * y / (k * k)
        s += term
        if np.max(np.abs(term)) < 1e-13 * np.max(s):
            break
    return s


def make_table(phases: int, k: int, fc: float, beta: float) -> np.ndarray:
    table = np.zeros((phases, 2 * k), dtype=float)
    taps = np.arange(-k + 1, k + 1)
    for p in range(phases):
        frac = p / phases
        t = taps - frac
        h = np.where(
            np.abs(t) < 1e-9,
            2.0 * fc,
            np.sin(2.0 * np.pi * fc * t) / (np.pi * np.where(t == 0, 1, t)),
        )
        win = np.zeros_like(t)
        inside = np.abs(t) < k
        win[inside] = (
            kaiser_i0(beta * np.sqrt(1.0 - (t[inside] / k) ** 2))
            / kaiser_i0(beta)
        )
        h *= win
        total = h.sum()
        if total:
            h /= total
        table[p] = h
    return table


def resample(x: np.ndarray, table: np.ndarray, k: int,
             fs_in: float, fs_out: float, block: int = 64) -> np.ndarray:
    step = fs_in / fs_out
    phases = table.shape[0]
    out = []
    pos = 0.0
    pad = np.concatenate([np.zeros(k), x, np.zeros(k)])
    taps = np.arange(-k + 1, k + 1)
    for start in range(0, len(x), block):
        chunk = x[start:start + block]
        n = len(chunk)
        while pos < n:
            i = int(pos)
            frac = pos - i
            p = min(int(frac * phases), phases - 1)
            seg = pad[k + start + i + taps]
            out.append(float(np.dot(seg, table[p])))
            pos += step
        pos -= n
        if pos < 0:
            pos = 0.0
    return np.asarray(out)


def measure(x: np.ndarray, y: np.ndarray, fs_in: float, fs_out: float,
            tone: float) -> tuple[float, float]:
    y = y[2000:-2000]
    win = np.hanning(len(y))
    spec = np.abs(np.fft.rfft(y * win))
    freqs = np.fft.rfftfreq(len(y), 1.0 / fs_out)
    fund = spec[np.argmin(np.abs(freqs - tone))]
    copy = spec.copy()
    copy[np.abs(freqs - tone) < 60] = 0.0
    idx = int(np.argmax(copy))
    ratio = 20.0 * math.log10(copy[idx] / max(fund, 1e-12))
    return float(freqs[idx]), ratio


def run(tone: float, args) -> None:
    fs_in = args.fs_in
    fs_out = args.fs_out
    table = make_table(args.phases, args.k, args.fc, args.beta)
    n = int(fs_in * args.seconds)
    t = np.arange(n) / fs_in
    x = 0.5 * np.sin(2.0 * np.pi * tone * t)
    y = resample(x, table, args.k, fs_in, fs_out, args.block)
    spur, ratio = measure(x, y, fs_in, fs_out, tone)
    print(f"tone={tone:7.0f} Hz  spur={spur:8.0f} Hz  "
          f"level={ratio:7.1f} dBc")


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--tone", type=float, default=15000.0)
    ap.add_argument("--sweep", action="store_true")
    ap.add_argument("--phases", type=int, default=1024)
    ap.add_argument("--k", type=int, default=24)
    ap.add_argument("--fc", type=float, default=0.42)
    ap.add_argument("--beta", type=float, default=10.0)
    ap.add_argument("--fs-in", type=float, default=44100.0)
    ap.add_argument("--fs-out", type=float, default=48000.0)
    ap.add_argument("--seconds", type=float, default=0.5)
    ap.add_argument("--block", type=int, default=64)
    args = ap.parse_args()

    if args.sweep:
        for tone in (1000, 5000, 10000, 15000, 17000, 18000, 19000):
            run(tone, args)
    else:
        run(args.tone, args)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
