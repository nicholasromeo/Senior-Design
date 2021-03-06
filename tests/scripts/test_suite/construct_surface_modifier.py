from ovito.io import *
from ovito.modifiers import *
import numpy as np

import sys
import os

if "ovito.plugins.CrystalAnalysis" not in sys.modules: sys.exit()

node = import_file("../../files/CFG/lammps_dumpi-42-1100-510000.cfg")

modifier = ConstructSurfaceModifier()
node.modifiers.append(modifier)

print("Parameter defaults:")

print("  only_selected: {}".format(modifier.only_selected))
modifier.only_selected = False

print("  radius: {}".format(modifier.radius))
modifier.radius = 3.8

print("  smoothing_level: {}".format(modifier.smoothing_level))
modifier.smoothing_level = 0

print("  cap_color: {}".format(modifier.mesh_display.cap_color))
print("  cap_transparency: {}".format(modifier.mesh_display.cap_transparency))
print("  show_cap: {}".format(modifier.mesh_display.show_cap))
print("  smooth_shading: {}".format(modifier.mesh_display.smooth_shading))
print("  surface_color: {}".format(modifier.mesh_display.surface_color))
print("  surface_transparency: {}".format(modifier.mesh_display.surface_transparency))

node.compute()

print("Output:")
print("  solid_volume= {}".format(node.output.attributes['ConstructSurfaceMesh.solid_volume']))
print("  surface_area= {}".format(node.output.attributes['ConstructSurfaceMesh.surface_area']))
print(node.output)

surface_mesh = node.output.surface

assert(surface_mesh.locate_point((0,0,0)) == 1)
assert(surface_mesh.locate_point((82.9433, -43.5068, 26.4005), eps=1e-1) == 0)
assert(surface_mesh.locate_point((80.0, -47.2837, 26.944)) == -1)

surface_mesh.get_cutting_planes()
surface_mesh.get_face_adjacency()
surface_mesh.get_faces()
surface_mesh.get_vertices()
surface_mesh.set_cutting_planes([[0,1,3,0.4]])
assert(len(surface_mesh.get_cutting_planes()) == 1)

# Backward compatibility with Ovito 2.8.2:
surface_mesh.export_vtk("_surface_mesh.vtk", node.output.cell)
surface_mesh.export_cap_vtk("_surfacecap_mesh.vtk", node.output.cell)
os.remove("_surface_mesh.vtk")
os.remove("_surfacecap_mesh.vtk")

