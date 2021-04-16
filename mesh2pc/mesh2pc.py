import numpy as np
import open3d as o3d

mesh = o3d.io.read_triangle_mesh('test.obj')
# pcd = o3d.geometry.PointCloud()
# pcd.points = mesh.vertices
# pcd.colors = mesh.vertex_colors
# pcd.normals = mesh.vertex_normals
# open3d.visualization.draw_geometries([pcd])

pc = mesh.sample_points_uniformly(4096)
positions = np.asarray(pc.points)
normals = np.asarray(pc.normals)
for pos in positions:
    print(pos[0], pos[1], pos[2])
for nor in normals:
    print(nor[0], nor[1], nor[2])
