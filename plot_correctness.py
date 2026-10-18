# /// script
# requires-python = ">=3.10"
# dependencies = ["matplotlib>=3.8"]
# ///

import csv
import sys
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt

source = Path(sys.argv[1] if len(sys.argv) > 1 else "build/correctness.csv")
with source.open() as stream:
    rows = [{key: float(value) for key, value in row.items()} for row in csv.DictReader(stream)]

strikes = [row["strike"] for row in rows]
expected = [row["input_iv"] for row in rows]
fig, (smile, errors) = plt.subplots(
    2, 1, figsize=(9, 6), sharex=True,
    gridspec_kw={"height_ratios": [2, 1]}, layout="constrained",
)
fig.suptitle(
    f"Synthetic smile recovery | F = {rows[0]['forward']:g}, T = {rows[0]['expiry']:g} year"
)
smile.plot(strikes, [100 * value for value in expected], color="black", label="Input smile")
for index, (kind, marker, color) in enumerate([
    ("call", "o", "tab:blue"), ("put", "x", "tab:orange"),
]):
    recovered = [row[f"{kind}_iv"] for row in rows]
    error = [value - target for value, target in zip(recovered, expected)]
    smile.plot(
        strikes, [100 * value for value in recovered], linestyle="none",
        marker=marker, markersize=5, markerfacecolor="none", color=color,
        markevery=(4 * index, 8), label=f"Jäckel {kind}s",
    )
    errors.plot(strikes, error, color=color, label=f"{kind.capitalize()}s")

smile.set_ylabel("Implied volatility (%)")
smile.set_title(r"Input: $\sigma = 0.20 - 0.10\log(K/F) + 0.40\log^2(K/F)$", fontsize=10)
errors.set(xlabel="Strike", ylabel="IV error (decimal)")
errors.axhline(0, color="black", linewidth=0.7)
errors.ticklabel_format(axis="y", style="sci", scilimits=(0, 0))
for axis in (smile, errors):
    axis.grid(alpha=0.25)
    axis.legend(frameon=False)

figure_dir = Path(__file__).resolve().parent / "figures"
figure_dir.mkdir(parents=True, exist_ok=True)
destination = figure_dir / "correctness.png"
fig.savefig(destination, dpi=300)
print(destination)
plt.close(fig)
