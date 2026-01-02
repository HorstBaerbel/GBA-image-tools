#!/usr/bin/env python3

import sys
import pandas as pd
import numpy as np
import plotly.graph_objects as go
from plotly.subplots import make_subplots


def main():
    if len(sys.argv) < 2:
        print("Usage: python plot_stats.py <csv_file>")
        sys.exit(1)

    filename = sys.argv[1]

    # Load CSV
    df = pd.read_csv(filename)

    # Frames
    frames = df.iloc[:, 0].values
    # f0–f255
    data = df.iloc[:, 1:257].values
    # Alphabet size, Entropy and Compression ratio
    alphabetsize = df["alphabetsize"].values
    entropy = df["entropy"].values
    ratio = df["ratio"].values

    # Surface grid
    features = np.arange(256)
    X, Y = np.meshgrid(frames, features)
    Z = data.T  # (256, n_frames)

    # --- SUBPLOTS ---
    # Left: 1 large surface
    # Right: 2 stacked 2D line charts
    fig = make_subplots(
        rows=3, cols=2,
        specs=[
            [{"type": "surface", "rowspan": 3}, {"type": "xy"}],
            [None, {"type": "xy"}],
            [None, {"type": "xy"}]
        ],
        column_widths=[0.70, 0.30],
        row_heights=[0.33, 0.33, 0.33],
        horizontal_spacing=0.08,
        vertical_spacing=0.12
    )

    # --- 3D SURFACE (left, spanning both rows) ---
    fig.add_trace(
        go.Surface(
            x=X,
            y=Y,
            z=Z,
            name="Histogram",
            showscale=False
        ),
        row=1, col=1
    )

    # --- ALPHABET SIZE PLOT (top-right) ---
    fig.add_trace(
        go.Scatter(
            x=frames,
            y=alphabetsize,
            mode="lines",
            name="Alphabet size",
            line=dict(width=2, color="green")
        ),
        row=1, col=2
    )
    # --- ENTROPY PLOT (middle-right) ---
    fig.add_trace(
        go.Scatter(
            x=frames,
            y=entropy,
            mode="lines",
            name="Entropy",
            line=dict(width=2, color="red")
        ),
        row=2, col=2
    )
    # --- RATIO PLOT (bottom-right) ---
    fig.add_trace(
        go.Scatter(
            x=frames,
            y=ratio,
            mode="lines",
            name="Compression ratio",
            line=dict(width=2, color="blue")
        ),
        row=3, col=2
    )

    # Axis labels for the 2D charts
    fig.update_xaxes(title="Frame", row=1, col=2)
    fig.update_yaxes(title="Alphabet size", row=1, col=2)
    fig.update_xaxes(title="Frame", row=2, col=2)
    fig.update_yaxes(title="Entropy (bits)", row=2, col=2)
    fig.update_xaxes(title="Frame", row=3, col=2)
    fig.update_yaxes(title="Compression ratio", row=3, col=2)

    # Layout
    fig.update_layout(
        title=f"Data stats for {filename}",
        height=850,
        width=1500,
        scene=dict(
            xaxis_title="Frame #",
            yaxis_title="Byte value",
            zaxis_title="Count (normalized)"
        )
    )

    fig.show()


if __name__ == "__main__":
    main()