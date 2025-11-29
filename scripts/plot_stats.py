import sys
import pandas as pd
import numpy as np
import plotly.graph_objects as go
from plotly.subplots import make_subplots


def main():
    if len(sys.argv) < 2:
        print("Usage: python plot_audio_surface_and_stats.py <csv_file>")
        sys.exit(1)

    filename = sys.argv[1]

    # Load CSV
    df = pd.read_csv(filename)

    # Frames
    frames = df.iloc[:, 0].values
    # f0–f255
    data = df.iloc[:, 1:257].values
    # Entropy + Ratio
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
        rows=2, cols=2,
        specs=[
            [{"type": "surface", "rowspan": 2}, {"type": "xy"}],
            [None, {"type": "xy"}]
        ],
        column_widths=[0.70, 0.30],
        row_heights=[0.50, 0.50],
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

    # --- ENTROPY PLOT (top-right) ---
    fig.add_trace(
        go.Scatter(
            x=frames,
            y=entropy,
            mode="lines",
            name="Entropy",
            line=dict(width=2, color="red")
        ),
        row=1, col=2
    )

    # --- RATIO PLOT (bottom-right) ---
    fig.add_trace(
        go.Scatter(
            x=frames,
            y=ratio,
            mode="lines",
            name="Compression Ratio",
            line=dict(width=2, color="blue")
        ),
        row=2, col=2
    )

    # Axis labels for the 2D charts
    fig.update_xaxes(title="Frame", row=1, col=2)
    fig.update_yaxes(title="Entropy (bits)", row=1, col=2)
    fig.update_xaxes(title="Frame", row=2, col=2)
    fig.update_yaxes(title="Compression ratio", row=2, col=2)

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