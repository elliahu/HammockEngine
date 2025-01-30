from mesh_to_sdf import mesh_to_voxels
import trimesh
import skimage
import argparse
import numpy as np

def main():
    parser = argparse.ArgumentParser(description="Generate an SDF from a mesh")
    parser.add_argument('input_mesh', type=str, help="Input mesh file (e.g., .glb)")
    parser.add_argument('output_file', type=str, help="Output file to save the SDF")
    parser.add_argument('--res', type=int, help="Voxel resolution", default=64)
    args = parser.parse_args()

    mesh = trimesh.load(args.input_mesh)

    voxels = mesh_to_voxels(mesh, args.res - 2, pad=True)

    voxels = voxels.astype(np.float32)  # Ensure the data is in float32 format

    voxels.tofile(args.output_file)  # Save as raw binary file
    print(f"Voxels saved to {args.output_file} as raw binary floats")
    print(voxels.shape)
    print(min(voxels.flatten()))
    print(max(voxels.flatten()))
    print(len(voxels.flatten()))



    vertices, faces, normals, _ = skimage.measure.marching_cubes(voxels, level=0)
    mesh = trimesh.Trimesh(vertices=vertices, faces=faces, vertex_normals=normals)
    mesh.show()

if __name__ == "__main__":
    main()
