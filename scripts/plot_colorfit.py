#!/usr/bin/env python3

import colorsys
import pandas as pd
import plotly.express as px

# Read data
objects_data = pd.read_csv('colorfit_objects.csv')
clusters_data = pd.read_csv('colorfit_clusters.csv')

objects_colorsource = {
    "RGB value": [dict(color=objects_data['csscolor']), dict(colorscale='')],
    "Weight": [dict(color=objects_data['weight']), dict(colorscale='Inferno')],
    "Object type": [dict(color=objects_data['type']), dict(colorscale='Rainbow')],
    "Cluster index": [dict(color=objects_data['clusterindex']), dict(colorscale='Rainbow')]
}
objects_sizesource = {
    "Fixed": dict(size=''),
    "Weight": dict(size=objects_data['weight']),
    "Object type": dict(size=objects_data['type']),
    "Cluster index": dict(size=objects_data['clusterindex'])
}
objects_markeropacity = 1.0


def create_colorbuttons():
    buttons = []
    for colorsource in objects_colorsource.keys():
        color = objects_colorsource[colorsource][0]
        mapping = objects_colorsource[colorsource][1]
        button = dict(
            label=colorsource,
            method="restyle",
            args=[
                dict(marker=dict(**color, **mapping, showscale=True))]
        )
        buttons.append(button)
    return buttons


def create_sizebuttons():
    buttons = []
    for sizesource in objects_sizesource.keys():
        size = objects_sizesource[sizesource]
        button = dict(
            label=sizesource,
            method="restyle",
            args=[
                dict(marker=dict(**size, showscale=True))]
        )
        buttons.append(button)
    return buttons


# Show object scatter plot
objects_markerdata = ['weight', 'type', 'clusterindex']
objects_fig = px.scatter_3d(
    objects_data, x='r', y='g', z='b', hover_data=objects_markerdata)
# Set axis ranges
objects_fig.update_layout(
    scene=dict(
        xaxis=dict(nticks=16, range=[0, 255],),
        yaxis=dict(nticks=16, range=[0, 255],),
        zaxis=dict(nticks=16, range=[0, 255],),))
# Create buttons when layout is updated
objects_fig.update_layout(
    updatemenus=[dict(buttons=create_colorbuttons(), x=0.1, xanchor="left"),
                 dict(buttons=create_sizebuttons(), x=0.37, xanchor="left")],
    annotations=[dict(text="Color:", x=0, xref="paper", y=1.0, yref="paper", align="left", showarrow=False),
                 dict(text="Size:", x=0.25, xref="paper", y=1.0, yref="paper", showarrow=False)])
# Show RGB color by default
objects_fig.update_traces(marker=dict(
    color=objects_data['csscolor'],
    opacity=objects_markeropacity))
# Switch color source using selector
objects_fig.update_traces(
    selector=dict(meta={'sizesource': list(objects_sizesource.keys())[0]}))
objects_fig.update_traces(
    selector=dict(meta={'colorsource': list(objects_colorsource.keys())[0]}))
objects_fig.show()

# Show cluster scatter plot
clusters_fig = px.scatter_3d(
    clusters_data, x='r', y='g', z='b')
# Set axis ranges
clusters_fig.update_layout(
    scene=dict(
        xaxis=dict(nticks=16, range=[0, 255],),
        yaxis=dict(nticks=16, range=[0, 255],),
        zaxis=dict(nticks=16, range=[0, 255],),))
# Show RGB color by default
clusters_fig.update_traces(marker=dict(
    size=10.0,
    color=clusters_data['csscolor'],
    opacity=objects_markeropacity))
clusters_fig.show()

# Read data and show color weights (we should sort data by hue)
weights_markerdata = ['weight']
weights_fig = px.histogram(objects_data, x="csscolor", y="weight",
                           histfunc="avg", nbins=int(len(weights_markerdata) / 1000), hover_data=weights_markerdata)
# weights_fig = px.bar(weights_data, x='csscolor', y='weight', barmode='group', color_discrete_sequence=weights_data['csscolor'], hover_data=weights_markerdata)
# Color markers by RGB
weights_fig.update_traces(marker_color=objects_data['csscolor'])
# weights_fig.show()
