#!/usr/bin/env python3

from dune.grid import structuredGrid
from dumux.discretization import GridGeometry

gridView = structuredGrid([0, 0], [1, 1], [5, 5])

gridGeometry = GridGeometry(gridView, discMethod="cctpfa")

print("The total number of scvs is {}".format(gridGeometry.numScv))
print("The total number of scvfs is {}".format(gridGeometry.numScvf))

for e in gridView.elements:
    fvGeometry = gridGeometry.localView
    fvGeometry.bind(e)

    for scv in fvGeometry.scvs:
        print(f"scv dofIndex: {scv.dofIndex}")
        print(f"scv center: {scv.center}")
        print(f"scv volume: {scv.volume}")
