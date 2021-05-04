import numpy as np
import open3d as o3d

import sys

root = sys.argv[1]
mesh = o3d.io.read_triangle_mesh(f'{root}/mesh2pc/{sys.argv[2]}.obj')
mesh.compute_vertex_normals()
# pcd = o3d.geometry.PointCloud()
# pcd.points = mesh.vertices
# pcd.colors = mesh.vertex_colors
# pcd.normals = mesh.vertex_normals
# o3d.visualization.draw_geometries([pcd])

# mesh.rotate(mesh.get_rotation_matrix_from_xyz((np.pi/2,0,np.pi/4)), center=False)
pc = mesh.sample_points_uniformly(4096)
# pc.estimate_normals()

# o3d.visualization.draw_geometries([pc], point_show_normal=True)
print(root)

y_max = np.max(pc.points, axis=0)[1]
y_min = np.min(pc.points, axis=0)[1]

y_scale = 150 / (y_max - y_min)
print(y_scale)

y_offset = 0

positions = np.asarray(pc.points) * y_scale
normals = np.asarray(pc.normals)

for pos in positions:
    print(pos[0], pos[1] + y_offset, pos[2])
for nor in normals:
    print(nor[0], nor[1], nor[2])
