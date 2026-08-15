"""
metodologia: 
    - czas i pamieć mierzona w osobnych przebiegach benchmark.gd 
    - watek próbkujący pamięć robi syscalle co interval_ms, co przy bardzo krotkich wywolaniach erode() (małe mapy, dziesiatki-setki us) mogłoby zaburzyć pomiar czasu
    - dla pamieci używana jedna liczba wątków (memory_thread_count w benchmark.gd), w przeciwieństwie do czasów

struktura katalogów:
    - data/times/     - {algo}_times.csv
    - data/memory/    - {algo}_memory_sampled.csv
    - data/overhead/  - overhead_profile.csv
    - data/fixtures/  - {label}_terrain_{size}.bin (fixture + wyniki correctness)
    - plots/          - tu trafiaja wygenerowane wykresy

brakujace pliki pomijane z komunikatem
każdy wykres generowany dla "pipe" i "particle" (hydraulic) 
"""
import os
import glob
import struct

import numpy as np
import pandas as pd
import matplotlib as mpl
import matplotlib.pyplot as plt
import matplotlib.ticker as mticker



TIMES_DIR = "data/times"
MEMORY_DIR = "data/memory"
OVERHEAD_DIR = "data/overhead"
FIXTURES_DIR = "data/fixtures"

OUTPUT_DIR = "plots"

# implementacje pipe/particle - nazwa pliku uzyta przez benchmark.gd (algo)
PIPE_IMPLS = {
    "optimized": "pipe_optimized",
    "baseline": "pipe_cpp",
    "gds": "pipe_gds",
}
PARTICLE_IMPLS = {
    "baseline": "hydraulic_cpp",
    "gds": "hydraulic_gds",
}
ALGO_IMPLS = {"pipe": PIPE_IMPLS, "particle": PARTICLE_IMPLS}

# fallback dla wykresow "vs watki", gdy implementacja nie ma żadnej zmiennosci watkowej (jak particle)
# tylko do narysowania poziomych linii referencyjnych na sensownym zakresie X
DEFAULT_THREAD_RANGE = (1, 6)

OVERHEAD_FILE = "overhead_profile.csv"

# poprawność numeryczna: label uzyty w correctness_path() - plik {label}_terrain_{size}.bin
CORRECTNESS_LABELS = {
    "baseline": "pipe_baseline",
    "optimized": "pipe_optimized",
    "gds": "pipe_gds",
}

# rozbieżność w funkcji liczby iteracji - pliki div_{label}_{iters}.bin
DIVERGENCE_LABELS = {
    "optimized": "optimized",
    "gds": "gds",
}
DIVERGENCE_BASELINE = "baseline"

# analityczne wyliczenie bajtow buforow - particle ma nieregularny brush (vector<vector<>>), wiec brak wzoru (ale chyba jednak zbędne)
ANALYTICAL_BYTES = {
    "pipe": lambda size: 11 * size * size * 4,
}

# real-time (animation_erosion.gd) - nazwa pliku bez sufiksu (.csv / _summary.csv)
REALTIME_FAMILIES = {
    "pipe": "realtime_pipe",
    "particle": "realtime_particle",
}
REALTIME_DIR = "data/realtime"
# jakoś 16.667 ms
FPS_BUDGET_MS = 1000.0 / 60.0

# w sumie już tylko dla threada ta zmiana
IMPL_STYLE = {
    "baseline": {"color": "#2E5EAA", "linestyle": "--"},
    "gds": {"color": "#C1443C", "linestyle": "--"},
    "optimized": {"color": "#3C9D6B", "linestyle": "-"},
}

ITERATIONS = {
    "pipe": {32: 2, 64: 8, 128: 32, 256: 128, 512: 512},
    "particle": {32: 1250, 64: 5000, 128: 20000, 256: 80000, 512: 320000},
}

# zamiana na polskie nazwy na wykresach, xd
IMPL_LABEL = {
    "baseline": "C++ referencyjna",
    "gds": "GDScript",
    "optimized": "C++ zoptymalizowana",
}

ALGO_LABEL = {
    "pipe": "m. przepływowy",
    "particle": "m. cząsteczkowy",
}

SPEEDUP_COLS = {
    "opt_vs_base": f"{IMPL_LABEL['optimized']} / {IMPL_LABEL['baseline']}",
    "base_vs_gds": f"{IMPL_LABEL['baseline']} / {IMPL_LABEL['gds']}",
    "opt_vs_gds": f"{IMPL_LABEL['optimized']} / {IMPL_LABEL['gds']}",
}



def impl_label(impl: str) -> str:
    return IMPL_LABEL.get(impl, impl)

def algo_label(algo: str) -> str:
    return ALGO_LABEL.get(algo, algo)

def threads_label(n) -> str:
    n = int(n)
    if n == 1:
        return "1 wątek"
    if 2 <= n % 10 <= 4 and not 12 <= n % 100 <= 14:
        return f"{n} wątki"
    return f"{n} wątków"

# ŚCIEŻKI PER KATEGORIA
def times_file(name: str) -> str:
    return os.path.join(TIMES_DIR, name)

def memory_file(name: str) -> str:
    return os.path.join(MEMORY_DIR, name)

def overhead_file(name: str) -> str:
    return os.path.join(OVERHEAD_DIR, name)

def fixtures_file(name: str) -> str:
    return os.path.join(FIXTURES_DIR, name)

def output_file(name: str) -> str:
    return os.path.join(OUTPUT_DIR, name)

def realtime_file(name: str) -> str:
    return os.path.join(REALTIME_DIR, name)



# STYL WYKRESÓW
def apply_style() -> None:
    mpl.rcParams.update({
        "figure.figsize": (8, 5),
        "figure.dpi": 120,
        "figure.facecolor": "white",
        "font.family": "sans-serif",
        "font.size": 11,
        "axes.titlesize": 13,
        "axes.titleweight": "bold",
        "axes.labelsize": 11,
        "axes.facecolor": "white",
        "axes.edgecolor": "#444444",
        "axes.linewidth": 0.8,
        "axes.grid": True,
        "axes.axisbelow": True,
        "grid.color": "#dddddd",
        "grid.linewidth": 0.6,
        "axes.spines.top": False,
        "axes.spines.right": False,
        "legend.frameon": False,
        "legend.fontsize": 9.5,
        "xtick.color": "#333333",
        "ytick.color": "#333333",
        "lines.linewidth": 1.8,
        "lines.markersize": 5,
        "axes.prop_cycle": mpl.cycler(color=["#2E5EAA", "#C1443C", "#3C9D6B", "#D99A2B", "#7A5DA3", "#4C4C4C"]),
    })

def _color_for(idx: int) -> str:
    cycle = plt.rcParams["axes.prop_cycle"].by_key()["color"]
    return cycle[idx % len(cycle)]

# wyłącza notację naukową, realne liczby na osi Y
def _plain_y_axis(ax) -> None:
    ax.yaxis.set_major_formatter(mticker.ScalarFormatter())
    ax.ticklabel_format(style="plain", axis="y")

# dwa warianty tego samego wykresu (liniowy + logarytmiczny) obok siebie
def save_linear_log_pair(draw, filename_stem: str, suptitle: str, tag: str = "") -> None:
    fig, axes = plt.subplots(1, 2, figsize=(14, 5))
    for ax, log_variant in zip(axes, (False, True)):
        draw(ax, log_variant)
        ax.set_title("skala liniowa" if not log_variant else "skala log.")
    # ta sama wielkość co po lewej
    axes[1].set_ylabel("")
    fig.suptitle(suptitle, fontweight="bold")
    fig.tight_layout()
    out = output_file(f"{filename_stem}_combined.png")
    fig.savefig(out)
    print(f"[{tag}] saved - {out}")
    plt.show()
    plt.close(fig)



# HELPERY
def read_csv_with_meta(path: str):
    meta = {}
    with open(path, "r", encoding="utf-8") as f:
        lines = f.readlines()

    header_line = 0
    for i, line in enumerate(lines):
        if line.startswith("#"):
            key, _, val = line[1:].strip().partition("=")
            meta[key.strip()] = val.strip()
            header_line = i + 1
        else:
            break

    df = pd.read_csv(path, skiprows=header_line)
    return df, meta

# średnia po odrzuceniu skrajnych wartości
def trimmed_mean(values, drop: int = 2) -> float:
    v = np.sort(np.asarray(values, dtype=float))
    core = v if len(v) <= 2 * drop else v[drop: len(v) - drop]
    return float(core.mean())

def trimmed_std(values, drop: int = 2) -> float:
    v = np.sort(np.asarray(values, dtype=float))
    core = v if len(v) <= 2 * drop else v[drop: len(v) - drop]
    return float(core.std())

def _exists(path: str) -> bool:
    if not os.path.exists(path):
        print(f" (skipped - missing file: {path})")
        return False
    return True



# 1. POPRAWNOŚĆ NUMERYCZNA
def load_terrain_binary(path: str) -> np.ndarray:
    with open(path, "rb") as f:
        size = struct.unpack("<i", f.read(4))[0]
        raw = f.read(size * size * 4)
    return np.frombuffer(raw, dtype="<f4").reshape(size, size)

def _terrain_path(label: str, size: int) -> str:
    return fixtures_file(f"{label}_terrain_{size}.bin")

def _compare_terrains(name_a: str, arr_a: np.ndarray, name_b: str, arr_b: np.ndarray) -> None:
    if arr_a.shape != arr_b.shape:
        print(f"  {name_a} vs {name_b}: ERROR - different dimensions {arr_a.shape} vs {arr_b.shape}")
        return

    diff = arr_a - arr_b
    max_abs = float(np.max(np.abs(diff)))
    rmse = float(np.sqrt(np.mean(diff ** 2)))
    rng = float(arr_b.max() - arr_b.min())
    rel = (max_abs / rng * 100.0) if rng > 0 else 0.0
    print(f"  {name_a} vs {name_b}:  max|delta|={max_abs:.3e}  RMSE={rmse:.3e}  ({rel:.4f}% range)")

def check_numerical_correctness() -> None:
    print("\n=== NUMERICAL CORRECTNESS (pipe) ===")
    pattern = fixtures_file("pipe_baseline_terrain_*.bin")
    sizes = []
    for p in glob.glob(pattern):
        stem = os.path.basename(p).replace("pipe_baseline_terrain_", "").replace(".bin", "")
        if stem.isdigit():
            sizes.append(int(stem))
    sizes.sort()

    if not sizes:
        print(f" (skipped - missing files: pipe_baseline_terrain_*.bin in {FIXTURES_DIR})")
        return

    for size in sizes:
        print(f"[mapa {size}x{size}]")
        terrains = {}
        for impl, label in CORRECTNESS_LABELS.items():
            p = _terrain_path(label, size)
            if os.path.exists(p):
                terrains[impl] = load_terrain_binary(p)

        if "baseline" not in terrains:
            print("  (brak baseline - pomijam)")
            continue
        if "optimized" in terrains:
            _compare_terrains("optimized", terrains["optimized"], "baseline", terrains["baseline"])
        if "gds" in terrains:
            _compare_terrains("gds", terrains["gds"], "baseline", terrains["baseline"])

def plot_divergence_vs_iterations() -> None:
    print("\n=== DIVERGENCE vs ITERATIONS (pipe) ===")
    pattern = fixtures_file(f"div_{DIVERGENCE_BASELINE}_*.bin")
    iters_list = []
    for p in glob.glob(pattern):
        stem = os.path.basename(p).replace(f"div_{DIVERGENCE_BASELINE}_", "").replace(".bin", "")
        if stem.isdigit():
            iters_list.append(int(stem))
    iters_list.sort()

    if not iters_list:
        print(f" (skipped - missing files: div_{DIVERGENCE_BASELINE}_*.bin in {FIXTURES_DIR})")
        return

    series = {impl: {"iters": [], "max": [], "rmse": []} for impl in DIVERGENCE_LABELS}
    terrain_range = None

    for it in iters_list:
        base_path = fixtures_file(f"div_{DIVERGENCE_BASELINE}_{it}.bin")
        if not os.path.exists(base_path):
            continue
        base = load_terrain_binary(base_path)
        if terrain_range is None:
            terrain_range = float(base.max() - base.min())

        for impl, label in DIVERGENCE_LABELS.items():
            p = fixtures_file(f"div_{label}_{it}.bin")
            if not os.path.exists(p):
                continue
            arr = load_terrain_binary(p)
            if arr.shape != base.shape:
                continue
            diff = arr - base
            series[impl]["iters"].append(it)
            series[impl]["max"].append(float(np.max(np.abs(diff))))
            series[impl]["rmse"].append(float(np.sqrt(np.mean(diff ** 2))))
            print(f"  iters={it:5d} {impl} vs baseline:  max|delta|={series[impl]['max'][-1]:.3e}")

    fig, ax = plt.subplots()
    for impl in DIVERGENCE_LABELS:
        s = series[impl]
        if not s["iters"]:
            continue
        style = IMPL_STYLE.get(impl, {})
        ax.plot(s["iters"], s["max"], marker="o", label=f"{impl_label(impl)} względem referencyjnej", **style)

    # poziom odniesienia: amplituda terenu - powyżej niej różnice są wizualnie istotne
    if terrain_range:
        ax.axhline(terrain_range, color="#888888", linestyle=":", linewidth=1.0)
        ax.text(iters_list[0], terrain_range * 1.15, "amplituda terenu", fontsize=8, color="#666666")

    ax.set_xscale("log", base=2)
    ax.set_yscale("log")
    ax.set_xticks(iters_list)
    ax.set_xticklabels(iters_list)
    ax.set_xlabel("Liczba iteracji")
    ax.set_ylabel(r"max$|\Delta|$ wysokości")
    ax.set_title(f"{algo_label('pipe')}: narastanie rozbieżności numerycznej")
    ax.legend()

    fig.tight_layout()
    out = output_file("pipe_divergence_vs_iterations.png")
    fig.savefig(out)
    print(f"[pipe] saved - {out}")
    plt.show()
    plt.close(fig)



# 2. NARZUT WYWOLANIA (overhead)
def analyze_overhead() -> None:
    print("\n=== OVERHEAD - mean, dropped 2 extremes ===")
    path = overhead_file(OVERHEAD_FILE)
    if not _exists(path):
        return

    df, _meta = read_csv_with_meta(path)
    value_col = "per_call_us" if "per_call_us" in df.columns else "per_call_ns"

    rows = []
    for (variant, arr_size), g in df.groupby(["variant", "array_size"]):
        mean_us = trimmed_mean(g[value_col].values)
        std_us = trimmed_std(g[value_col].values)
        rows.append((variant, int(arr_size), mean_us * 1000.0, std_us * 1000.0, len(g)))

    result = pd.DataFrame(rows, columns=["variant", "array_size", "mean_ns", "std_ns", "n"])
    result = result.sort_values(["variant", "array_size"]).reset_index(drop=True)
    with pd.option_context("display.float_format", lambda x: f"{x:,.1f}"):
        print(result.to_string(index=False))



# 3. CZAS - dane wspólne
def load_timing(algo: str):
    frames = []
    for impl, algo_name in ALGO_IMPLS[algo].items():
        path = times_file(f"{algo_name}_times.csv")
        if not os.path.exists(path):
            continue
        df, _meta = read_csv_with_meta(path)
        df["impl"] = impl
        frames.append(df)
    return pd.concat(frames, ignore_index=True) if frames else None

def _agg_timing(df):
    return (
        df.groupby(["impl", "threads_used", "map_width"])["duration_us"]
        .apply(lambda s: trimmed_mean(s.values))
        .reset_index(name="mean_us")
    )


# 3.1. CZAS vs LICZBA WATKÓW
def plot_time_vs_threads(algo: str) -> None:
    df = load_timing(algo)
    if df is None:
        print(f"\n[{algo}] skipped (time vs threads) - missing files *_times.csv in {TIMES_DIR}")
        return

    agg = _agg_timing(df)
    max_size = int(agg["map_width"].max())
    sub = agg[agg["map_width"] == max_size]

    opt_threads = sorted(sub[sub["impl"] == "optimized"]["threads_used"].unique())
    x_lo, x_hi = (min(opt_threads), max(opt_threads)) if len(opt_threads) > 1 else DEFAULT_THREAD_RANGE

    def draw(ax, log_variant: bool) -> None:
        for impl, g in sub.groupby("impl"):
            style = IMPL_STYLE.get(impl, {"color": "#4C4C4C", "linestyle": "-"})
            if impl == "optimized" and len(g) > 1:
                g = g.sort_values("threads_used")
                ax.plot(g["threads_used"], g["mean_us"] / 1000.0, marker="o", color=style["color"], linestyle=style["linestyle"], label=impl_label(impl))
            else:
                ref = g["mean_us"].mean() / 1000.0
                ax.hlines(ref, x_lo, x_hi, color=style["color"], linestyles=style["linestyle"], label=f"{impl_label(impl)} ({threads_label(1)})")

        ax.set_xlabel("Liczba wątków")
        ax.set_ylabel("Czas [ms]")

        if log_variant:
            ax.set_yscale("log")
            ax.set_title(f"{algo_label(algo)}: czas w funkcji liczby wątków, {max_size}x{max_size}, skala log.")
        else:
            _plain_y_axis(ax)
            ax.set_title(f"{algo_label(algo)}: czas w funkcji liczby wątków, {max_size}x{max_size}")
        ax.legend()

    for log_variant in (False, True):
        fig, ax = plt.subplots()
        draw(ax, log_variant)
        fig.tight_layout()
        out = output_file(f"{algo}_time_vs_threads_{'log' if log_variant else 'linear'}.png")
        fig.savefig(out)
        print(f"[{algo}] saved - {out}")
        plt.show()
        plt.close(fig)

    save_linear_log_pair(draw, f"{algo}_time_vs_threads", f"{algo_label(algo)}: czas w funkcji liczby wątków, {max_size}x{max_size}", algo)


# 3.2. CZAS vs ROZMIAR MAPY oraz SKALOWANIE (ns/komorke) - liniowo i logarytmicznie
def _plot_vs_mapsize(algo: str, agg, y_col: str, y_label: str, title_suffix: str, filename_stem: str, unit_divisor: float = 1.0) -> None:
    sizes = sorted(agg["map_width"].unique())

    def draw(ax, log_variant: bool) -> None:
        for impl, g in agg.groupby("impl"):
            best = g["threads_used"].max()
            gg = g[g["threads_used"] == best].sort_values("map_width")
            label = f"{impl_label(impl)} ({threads_label(best)})" if impl == "optimized" else impl_label(impl)
            ax.plot(gg["map_width"], gg[y_col] / unit_divisor, marker="o", label=label)

        ax.set_xscale("log", base=2)
        ax.set_xticks(sizes)
        ax.set_xticklabels(sizes)
        ax.set_xlabel("Rozmiar mapy (bok)")
        ax.set_ylabel(y_label)

        if log_variant:
            ax.set_yscale("log")
            ax.set_title(f"{algo_label(algo)}: {title_suffix}, skala log.")
        else:
            _plain_y_axis(ax)
            ax.set_title(f"{algo_label(algo)}: {title_suffix}")
        ax.legend()

    for log_variant in (False, True):
        fig, ax = plt.subplots()
        draw(ax, log_variant)
        fig.tight_layout()
        out = output_file(f"{filename_stem}_{'log' if log_variant else 'linear'}.png")
        fig.savefig(out)
        print(f"[{algo}] saved - {out}")
        plt.show()
        plt.close(fig)

    save_linear_log_pair(draw, filename_stem, f"{algo_label(algo)}: {title_suffix}", algo)

def plot_time_vs_mapsize(algo: str) -> None:
    df = load_timing(algo)
    if df is None:
        print(f"\n[{algo}] skipped (time vs map size) - missing data")
        return
    agg = _agg_timing(df)

    # no niech sie jeszcze wypisze na życzenie
    print(f"\n=== {algo}: TIME vs MAP SIZE ===")
    rows = []
    for impl, g in agg.groupby("impl"):
        best_threads = int(g["threads_used"].max())
        gg = g[g["threads_used"] == best_threads].sort_values("map_width")
        for _, r in gg.iterrows():
            rows.append({
                "implementacja": impl_label(impl),
                "wątki": best_threads,
                "mapa": f"{int(r['map_width'])}x{int(r['map_width'])}",
                "czas_ms": r["mean_us"] / 1000.0,
            })

    result = pd.DataFrame(rows)
    with pd.option_context("display.float_format", lambda x: f"{x:.3f}"):
        print(result.to_string(index=False))

    _plot_vs_mapsize(algo, agg, "mean_us", "Czas [ms]", "czas w funkcji rozmiaru mapy", f"{algo}_time_vs_mapsize", unit_divisor=1000.0)

# czy koszt na komórkę jest stały czy się zmienia (np. przejście między poziomami cache)
def plot_time_scaling(algo: str) -> None:
    df = load_timing(algo)
    if df is None:
        print(f"\n[{algo}] skipped (time scaling) - missing data")
        return
    agg = _agg_timing(df).copy()
    agg["cells"] = agg["map_width"].astype(float) ** 2
    agg["iters"] = agg["map_width"].map(ITERATIONS[algo]).astype(float)
    agg["ns_per_cell"] = agg["mean_us"] * 1000.0 / (agg["cells"] * agg["iters"])
    _plot_vs_mapsize(algo, agg, "ns_per_cell", "Czas na komórkę i iterację [ns]", "czas na komórkę i iterację a rozmiar mapy", f"{algo}_time_scaling", unit_divisor=1.0)


# 3.3. PRZYŚPIESZENIE
# optimized/baseline vs liczba watkow
def plot_speedup_vs_baseline(algo: str) -> None:
    df = load_timing(algo)
    if df is None:
        return
    agg = _agg_timing(df)
    if "baseline" not in agg["impl"].unique() or "optimized" not in agg["impl"].unique():
        print(f"\n[{algo}] skipped speedup-vs-baseline - missing implementations 'baseline' or 'optimized'")
        return

    max_size = int(agg["map_width"].max())
    base = agg[(agg["impl"] == "baseline") & (agg["map_width"] == max_size)]
    opt = agg[(agg["impl"] == "optimized") & (agg["map_width"] == max_size)].sort_values("threads_used")
    if base.empty or opt.empty:
        return

    base_time = base["mean_us"].iloc[0]
    speedup = base_time / opt["mean_us"]

    fig, ax = plt.subplots()
    ax.plot(opt["threads_used"], speedup, marker="o", color=_color_for(1))
    ax.axhline(1.0, color="#999999", linestyle="--", linewidth=1)
    ax.set_xlabel("Liczba wątków")
    ax.set_ylabel("Przyśpieszenie względem C++ (1 wątek)")
    ax.set_title(f"{algo_label(algo)}: przyśpieszenie wersji zoptymalizowanej, {max_size}x{max_size}")

    fig.tight_layout()
    out = output_file(f"{algo}_speedup_vs_baseline.png")
    fig.savefig(out)
    print(f"[{algo}] saved - {out}")
    plt.show()
    plt.close(fig)

def plot_speedup_vs_gds(algo: str) -> None:
    df = load_timing(algo)
    if df is None:
        print(f"\n[{algo}] skipped speedup-vs-gds - missing data")
        return
    agg = _agg_timing(df)
    if "baseline" not in agg["impl"].unique() or "gds" not in agg["impl"].unique():
        print(f"\n[{algo}] skipped speedup-vs-gds - missing implementations 'baseline' or 'gds'")
        return

    max_size = int(agg["map_width"].max())
    base = agg[(agg["impl"] == "baseline") & (agg["map_width"] == max_size)]
    gds = agg[(agg["impl"] == "gds") & (agg["map_width"] == max_size)]
    if base.empty or gds.empty:
        return

    base_time = base["mean_us"].iloc[0]
    gds_time = gds["mean_us"].iloc[0]
    base_vs_gds = gds_time / base_time

    opt = None
    if "optimized" in agg["impl"].unique():
        opt = agg[(agg["impl"] == "optimized") & (agg["map_width"] == max_size)].sort_values("threads_used")

    x_range = DEFAULT_THREAD_RANGE
    if opt is not None and len(opt) > 1:
        x_range = (int(opt["threads_used"].min()), int(opt["threads_used"].max()))

    fig, ax = plt.subplots()
    ax.hlines(base_vs_gds, x_range[0], x_range[1], linestyles="--", color=_color_for(0), label=f"{impl_label('baseline')} / {impl_label('gds')}")

    if opt is not None and len(opt) > 1:
        opt_vs_gds = gds_time / opt["mean_us"]
        ax.plot(opt["threads_used"], opt_vs_gds, marker="o", color=_color_for(1), label=f"{impl_label('optimized')} / {impl_label('gds')}")

    ax.axhline(1.0, color="#999999", linestyle=":", linewidth=1)
    ax.set_xlabel("Liczba wątków")
    ax.set_ylabel("Przyśpieszenie względem GDScript")
    ax.set_title(f"{algo_label(algo)}: przyśpieszenie C++ względem GDScript, {max_size}x{max_size}")
    ax.legend()

    fig.tight_layout()
    out = output_file(f"{algo}_speedup_vs_gds.png")
    fig.savefig(out)
    print(f"[{algo}] saved - {out}")
    plt.show()
    plt.close(fig)

# 3.4. PRZYŚPIESZENIE vs ROZMIAR MAPY (maks. liczba wątków) + tabele
def _best_thread_rows(agg, impl: str):
    g = agg[agg["impl"] == impl]
    if g.empty:
        return None
    idx = g.groupby("map_width")["threads_used"].idxmax()
    return g.loc[idx].sort_values("map_width")

def build_speedup_vs_mapsize(algo: str):
    df = load_timing(algo)
    if df is None:
        return None
    agg = _agg_timing(df)

    times, threads = {}, {}
    for impl in ("baseline", "gds", "optimized"):
        rows = _best_thread_rows(agg, impl)
        if rows is None:
            continue
        times[impl] = rows.set_index("map_width")["mean_us"]
        threads[impl] = rows.set_index("map_width")["threads_used"].astype(int)

    if "gds" not in times and not ("baseline" in times and "optimized" in times):
        return None

    sizes = sorted({s for t in times.values() for s in t.index})
    out = pd.DataFrame(index=pd.Index(sizes, name="map_width"))

    if "optimized" in times and "baseline" in times:
        out["opt_vs_base"] = times["baseline"] / times["optimized"]
    if "baseline" in times and "gds" in times:
        out["base_vs_gds"] = times["gds"] / times["baseline"]
    if "optimized" in times and "gds" in times:
        out["opt_vs_gds"] = times["gds"] / times["optimized"]
    if "optimized" in threads:
        out["threads"] = threads["optimized"]

    return out

def build_speedup_vs_threads(algo: str):
    df = load_timing(algo)
    if df is None:
        return None, None
    agg = _agg_timing(df)
    max_size = int(agg["map_width"].max())
    sub = agg[agg["map_width"] == max_size]

    base = sub[sub["impl"] == "baseline"]
    gds = sub[sub["impl"] == "gds"]
    opt = sub[sub["impl"] == "optimized"].sort_values("threads_used")
    if opt.empty:
        return None, max_size

    out = pd.DataFrame({"watki": opt["threads_used"].astype(int).values})
    if not base.empty:
        out["opt_vs_base"] = (base["mean_us"].iloc[0] / opt["mean_us"]).values
    if not gds.empty:
        out["opt_vs_gds"] = (gds["mean_us"].iloc[0] / opt["mean_us"]).values
        if not base.empty:
            out["base_vs_gds"] = gds["mean_us"].iloc[0] / base["mean_us"].iloc[0]
    return out, max_size

def report_speedup_tables(algo: str) -> None:
    print(f"\n=== {algo}: SPEEDUP (x razy) ===")

    thr_table, max_size = build_speedup_vs_threads(algo)
    if thr_table is None:
        print("  (brak implementacji 'optimized' - tabela vs liczba watkow pominieta)")
    else:
        t = thr_table.rename(columns={"watki": "wątki", **SPEEDUP_COLS})
        print(f"\n--- vs liczba wątków, mapa {max_size}x{max_size} ---")
        with pd.option_context("display.float_format", lambda x: f"{x:.2f}"):
            print(t.to_string(index=False))

    size_table = build_speedup_vs_mapsize(algo)
    if size_table is None:
        print("  (brak danych - tabela vs rozmiar mapy pominieta)")
        return

    s = size_table.copy()
    threads_note = ""
    if "threads" in s.columns:
        uniq = sorted(set(s["threads"].dropna().astype(int)))
        threads_note = f", {threads_label(uniq[0])}" if len(uniq) == 1 else ", maks. liczba wątków"
        # stala liczba watkow, nie ma sensu jako kolumna
        s = s.drop(columns=["threads"]) if len(uniq) == 1 else s
    s.index = [f"{int(i)}x{int(i)}" for i in s.index]
    s.index.name = "mapa"
    print(f"\n--- vs rozmiar mapy{threads_note} ---")
    with pd.option_context("display.float_format", lambda x: f"{x:.2f}"):
        print(s.rename(columns={**SPEEDUP_COLS, "threads": "wątki"}).to_string())

def plot_speedup_vs_mapsize(algo: str) -> None:
    table = build_speedup_vs_mapsize(algo)
    if table is None or table.empty:
        print(f"\n[{algo}] skipped speedup-vs-mapsize - missing data")
        return

    sizes = [int(s) for s in table.index]
    threads_note = ""
    if "threads" in table.columns:
        uniq = sorted(set(table["threads"].dropna().astype(int)))
        threads_note = f", {threads_label(uniq[-1])}" if len(uniq) == 1 else ", maks. liczba wątków"

    series = [
        ("opt_vs_base", f"{impl_label('optimized')} / {impl_label('baseline')}", IMPL_STYLE["gds"]["color"]),
        ("base_vs_gds", f"{impl_label('baseline')} / {impl_label('gds')}", IMPL_STYLE["baseline"]["color"]),
        ("opt_vs_gds", f"{impl_label('optimized')} / {impl_label('gds')}", IMPL_STYLE["optimized"]["color"]),
    ]

    def draw(ax, log_variant: bool) -> None:
        for col, label, color in series:
            if col not in table.columns:
                continue
            s = table[col].dropna()
            if s.empty:
                continue
            ax.plot([int(i) for i in s.index], s.values, marker="o", color=color, label=label)

        ax.axhline(1.0, color="#999999", linestyle="--", linewidth=1)
        ax.set_xscale("log", base=2)
        ax.set_xticks(sizes)
        ax.set_xticklabels(sizes)
        ax.set_xlabel("Rozmiar mapy (bok)")
        ax.set_ylabel("Przyśpieszenie [razy]")

        title = f"{algo_label(algo)}: przyśpieszenie a rozmiar mapy{threads_note}"
        if log_variant:
            ax.set_yscale("log")
            ax.set_title(f"{title}, skala log.")
        else:
            _plain_y_axis(ax)
            ax.set_title(title)
        ax.legend()

    for log_variant in (False, True):
        fig, ax = plt.subplots()
        draw(ax, log_variant)
        fig.tight_layout()
        out = output_file(f"{algo}_speedup_vs_mapsize_{'log' if log_variant else 'linear'}.png")
        fig.savefig(out)
        print(f"[{algo}] saved - {out}")
        plt.show()
        plt.close(fig)

    save_linear_log_pair(draw, f"{algo}_speedup_vs_mapsize", f"{algo_label(algo)}: przyśpieszenie a rozmiar mapy{threads_note}", algo)


# 4. PAMIĘĆ W CZASIE
def load_memory(algo: str):
    frames = []
    for impl, algo_name in ALGO_IMPLS[algo].items():
        path = memory_file(f"{algo_name}_memory_sampled.csv")
        if not os.path.exists(path):
            continue
        df, _meta = read_csv_with_meta(path)
        df["impl"] = impl
        frames.append(df)
    return pd.concat(frames, ignore_index=True) if frames else None

# każda implementacja dostaje swój panel
def plot_memory(algo: str, target_bins: int = 60) -> None:
    df = load_memory(algo)
    if df is None:
        print(f"\n[{algo}] skipped memory plot - missing files *_memory_sampled.csv in {MEMORY_DIR}")
        return

    max_size = int(df["map_width"].max())
    df = df[df["map_width"] == max_size]

    impls = [i for i in ALGO_IMPLS[algo] if i in df["impl"].unique()]
    if not impls:
        print(f"\n[{algo}] skipped memory plot - missing data for known implementations")
        return

    fig, axes = plt.subplots(1, len(impls), figsize=(5.5 * len(impls), 5), sharey=True)
    if len(impls) == 1:
        axes = [axes]

    for idx, impl in enumerate(impls):
        ax = axes[idx]
        color = _color_for(idx)
        g = df[df["impl"] == impl]

        t_max_ms = (g["elapsed_us"].max() / 1000.0) if not g.empty else 1.0
        bin_ms = max(t_max_ms / target_bins, 0.1)

        for _run, gg in g.groupby("run"):
            gg = gg.sort_values("elapsed_us")
            base = gg["private_bytes"].iloc[0]
            ax.plot(gg["elapsed_us"] / 1000.0, (gg["private_bytes"] - base) / 1e6, color=color, alpha=0.18, linewidth=0.8)

        gb = g.sort_values(["run", "elapsed_us"]).copy()
        gb["delta_bytes"] = gb.groupby("run")["private_bytes"].transform(lambda s: s - s.iloc[0])
        gb["t_bin"] = (gb["elapsed_us"] / 1000.0 // bin_ms) * bin_ms
        med = gb.groupby("t_bin")["delta_bytes"].median() / 1e6
        ax.plot(med.index, med.values, color=color, linewidth=2.2)

        formula = ANALYTICAL_BYTES.get(algo)
        if formula:
            ax.axhline(formula(max_size) / 1e6, color="#999999", linestyle="--", linewidth=1, label="wartość analityczna")

        ax.set_xlabel("Czas [ms]")
        if idx == 0:
            ax.set_ylabel("Przyrost pamięci względem stanu początkowego [MB]")
        _plain_y_axis(ax)
        ax.set_title(f"{impl_label(impl)} (przebiegi: {g['run'].nunique()})")
        if formula:
            ax.legend()

    fig.suptitle(f"{algo_label(algo)}: zużycie pamięci w czasie, {max_size}x{max_size}", fontweight="bold")
    fig.tight_layout()
    out = output_file(f"{algo}_memory.png")
    fig.savefig(out)
    print(f"[{algo}] saved - {out}")
    plt.show()
    plt.close(fig)

def report_analytical_memory() -> None:
    print("\n=== Analytical buffer footprint (pipe) ===")
    formula = ANALYTICAL_BYTES["pipe"]
    for size in (32, 64, 128, 256, 512):
        print(f"  mapa {size}x{size}: {formula(size) / 1e6:.2f} MB")

# delta pamięci z próbek pierwsza - ostatnia (dzięki czemu zbędny jest snapshot) vs analityczne
def report_measured_vs_analytical_memory(algo: str) -> None:
    df = load_memory(algo)
    if df is None:
        return

    print(f"\n=== {algo}: measured memory delta (from samples) vs analytical ===")
    formula = ANALYTICAL_BYTES.get(algo)

    for impl, g in df.groupby("impl"):
        for size, gs in g.groupby("map_width"):
            deltas = []
            for _run, gr in gs.groupby("run"):
                gr = gr.sort_values("elapsed_us")
                if len(gr) < 2:
                    continue
                deltas.append(float(gr["private_bytes"].iloc[-1] - gr["private_bytes"].iloc[0]))
            if not deltas:
                continue
            mean_delta = trimmed_mean(deltas) if len(deltas) > 4 else float(np.mean(deltas))
            line = f"  {impl:10s} size={int(size):4d}  measured~{mean_delta / 1e6:7.2f} MB"
            if formula:
                line += f"  analytical={formula(int(size)) / 1e6:7.2f} MB"
            print(line)



# 5. REAL-TIME (animation_erosion.gd)
# (mediana/p95) tutaj jeden wiersz podsumowania to ciągłyu przebieg - liczone z rozkładu klatek w jego obbrębie
# więc to analiza rozkładu opóźnień, nie porównanie powtórzeń pomiarów
def load_realtime_summary(family: str):
    name = f"{REALTIME_FAMILIES[family]}_summary.csv"
    path = realtime_file(name)
    if not os.path.exists(path):
        print(f"  (pominieto - brak pliku: {path})")
        return None
    df, _meta = read_csv_with_meta(path)
    # within_60fps_budget zapisywane w GDScript jako string "true"/"false"
    df["within_60fps_budget"] = df["within_60fps_budget"].astype(str).str.lower() == "true"
    return df

def report_max_iterations_table(family: str) -> None:
    print(f"\n=== {family}: max iterations_at_once in 60 FPS budget" f"(p95 <= {FPS_BUDGET_MS:.2f} ms) ===")
    df = load_realtime_summary(family)
    if df is None:
        return

    rows = []
    for (impl, map_size), g in df.groupby(["impl", "map_size"]):
        within = g[g["within_60fps_budget"]]
        if within.empty:
            rows.append((impl, int(map_size), 0, 0.0, float("nan")))
            continue
        max_iters = int(within["iterations_at_once"].max())
        row = within[within["iterations_at_once"] == max_iters].iloc[0]
        rows.append((impl, int(map_size), max_iters, float(row["iters_per_sec"]), float(row["p95_ms"])))

    result = pd.DataFrame(rows, columns=["impl", "map_size", "max_iters_at_once", "iters_per_sec", "p95_ms"])
    result = result.sort_values(["impl", "map_size"]).reset_index(drop=True)
    with pd.option_context("display.float_format", lambda x: f"{x:,.2f}"):
        print(result.to_string(index=False))

    # tabela przestawna, wiersze = rozmiar mapy, kolumny = implementacja, wartosci = max iterations_at_once w budżecie
    pivot = result.pivot(index="map_size", columns="impl", values="max_iters_at_once")
    print(f"\n--- {family}: max iterations_at_once in 60 FPS budget (inverted table) ---")
    print(pivot.to_string())

    # zero w calym wierszu = implementacja nie miesci sie w budzecie nawet
    zero_rows = result[result["max_iters_at_once"] == 0]
    if not zero_rows.empty:
        print(f"\nWARNING: this configs does NOT fit in the 60 FPS budget even with iterations_at_once=1:")
        for _, r in zero_rows.iterrows():
            print(f" {r['impl']} @ mapa {int(r['map_size'])}")

# p95 czasu klatki vs iterations_at_once, jedna linia na implementację przy największym zamierzonym rozmiarze mpay - w poziomie linia budżetu 60 FPS
def plot_realtime_frametime(family: str) -> None:
    df = load_realtime_summary(family)
    if df is None:
        print(f"\n[{family}] skipped real-time plot (no data available)")
        return

    max_size = int(df["map_size"].max())
    sub = df[df["map_size"] == max_size]

    fig, ax = plt.subplots()
    for idx, (impl, g) in enumerate(sub.groupby("impl")):
        g = g.sort_values("iterations_at_once")
        ax.plot(g["iterations_at_once"], g["p95_ms"], marker="o", color=_color_for(idx), label=impl_label(impl))

    ax.axhline(FPS_BUDGET_MS, color="#999999", linestyle="--", linewidth=1, label="budżet 60 kl./s (16.67 ms)")
    ax.set_xlabel("Liczba iteracji na klatkę")
    ax.set_ylabel("Czas klatki, p95 [ms]")
    _plain_y_axis(ax)
    ax.set_title(f"{algo_label(family)}: czas klatki (p95) a liczba iteracji, {max_size}x{max_size}")
    ax.legend()

    fig.tight_layout()
    out = output_file(f"{family}_realtime_frametime.png")
    fig.savefig(out)
    print(f"[{family}] saved - {out}")
    plt.show()
    plt.close(fig)

# max iterations_at_once mieszczące się w budżecie 60 FPS, w funkcji rozmiaru mapy - jedna linia na implementację
def plot_realtime_budget_vs_mapsize(family: str) -> None:
    df = load_realtime_summary(family)
    if df is None:
        print(f"\n[{family}] skipped real-time budget plot (no data available)")
        return

    rows = []
    for (impl, map_size), g in df.groupby(["impl", "map_size"]):
        within = g[g["within_60fps_budget"]]
        max_iters = int(within["iterations_at_once"].max()) if not within.empty else 0
        rows.append((impl, int(map_size), max_iters))
    agg = pd.DataFrame(rows, columns=["impl", "map_size", "max_iters"])

    sizes = sorted(agg["map_size"].unique())

    fig, ax = plt.subplots()
    for idx, (impl, g) in enumerate(agg.groupby("impl")):
        g = g.sort_values("map_size")
        ax.plot(g["map_size"], g["max_iters"], marker="o", color=_color_for(idx), label=impl_label(impl))

    ax.set_xscale("log", base=2)
    ax.set_xticks(sizes)
    ax.set_xticklabels(sizes)
    ax.set_xlabel("Rozmiar mapy (bok)")
    ax.set_ylabel("Maks. liczba iteracji w budżecie 60 kl./s")
    _plain_y_axis(ax)
    ax.set_title(f"{algo_label(family)}: budżet 60 kl./s w funkcji rozmiaru mapy")
    ax.legend()

    fig.tight_layout()
    out = output_file(f"{family}_realtime_budget.png")
    fig.savefig(out)
    print(f"[{family}] saved - {out}")
    plt.show()
    plt.close(fig)



def main() -> None:
    apply_style()

    check_numerical_correctness()
    plot_divergence_vs_iterations()
    analyze_overhead()
    report_analytical_memory()

    for algo in ("pipe", "particle"):
        report_measured_vs_analytical_memory(algo)

        plot_time_vs_threads(algo)
        plot_time_vs_mapsize(algo)
        plot_time_scaling(algo)
        plot_speedup_vs_baseline(algo)
        plot_speedup_vs_gds(algo)
        plot_speedup_vs_mapsize(algo)
        report_speedup_tables(algo)
        plot_memory(algo)

    for family in ("pipe", "particle"):
        report_max_iterations_table(family)
        plot_realtime_frametime(family)
        plot_realtime_budget_vs_mapsize(family)

if __name__ == "__main__":
    main()