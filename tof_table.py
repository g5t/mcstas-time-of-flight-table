"""Utilities for reading the TableManager TOF table output format.

The file format is a JSON representation of a ``scipp.Dataset``:

  coords
    time      (time [bin-edge]) – bin edges in seconds
    distance  (recorder)       – recorder distance from source in metres
    recorder  (recorder)       – recorder name strings

  data items, all with dims (recorder, time)
    tp  [s]             – sum of t * p per bin  (weight-averaged time sum)
    p1  [dimensionless] – sum of p per bin       (weight sum)
    p2  [dimensionless] – sum of p² per bin      (squared-weight sum)
    n   [dimensionless] – number of hits per bin (count)

Each variable follows the ``niess.io.scipp.variable_to_dict`` convention::

    {"unit": <str|null>, "dtype": "...", "dims": [...], "values": [...]}

String variables (e.g. ``recorder``) carry ``"unit": null``, which JSON
decodes to Python ``None``; ``sc.array(unit=None)`` correctly creates a
variable with no unit.
"""
from __future__ import annotations

import json
from pathlib import Path


def load(path) -> "scipp.Dataset":
    """Load a TableManager output file and return a :class:`scipp.Dataset`.

    Parameters
    ----------
    path:
        Path to the JSON file written by ``table_manager_write_output_file``.

    Returns
    -------
    scipp.Dataset
        Dataset with coords ``time``, ``distance``, ``recorder`` and data
        items ``tp``, ``p1``, ``p2``, ``n``.
    """
    import scipp as sc
    from niess.io.scipp import dict_to_variable

    with open(path) as f:
        obj = json.load(f)

    if obj.get("type") != "scipp.Dataset":
        raise ValueError(
            f"Expected type 'scipp.Dataset', got {obj.get('type')!r}"
        )

    coords = {k: dict_to_variable(v) for k, v in obj["coords"].items()}
    data   = {k: dict_to_variable(v) for k, v in obj["data"].items()}
    return sc.Dataset(data=data, coords=coords)
