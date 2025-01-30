import trimesh
import numpy as np
import matplotlib.pyplot as plt
from scipy.ndimage import distance_transform_edt
import argparse

def load_mesh(file_path):
    """Load a mesh from a GLB/GLTF file and return a single mesh."""
    scene = trimesh.load(file_path)

    # If the scene contains multiple meshes, combine them into a single mesh
    if isinstance(scene, trimesh.Scene):
        mesh = scene.dump(concatenate=True)  # Combine all meshes into one
    else:
        mesh = scene  # If it's already a single mesh, use it directly

    # Ensure the mesh is watertight (required for voxelization)
    if not mesh.is_watertight:
        print("Mesh is not watertight. Attempting to fix...")
        mesh.fill_holes()  # Try to fix holes
        if not mesh.is_watertight:
            raise ValueError("Mesh could not be made watertight.")

    return mesh

def voxelize_mesh(mesh, voxel_resolution=64):
    """Voxelize the mesh."""
    voxels = mesh.voxelized(pitch=1.0 / voxel_resolution)
    return voxels

def create_sdf(voxels):
    """Create a Signed Distance Field (SDF) from voxelized mesh."""
    # Get the voxel grid
    voxel_grid = voxels.matrix

    # Compute the SDF using a distance transform
    sdf = np.zeros_like(voxel_grid, dtype=np.float32)
    for i in range(voxel_grid.shape[0]):
        for j in range(voxel_grid.shape[1]):
            for k in range(voxel_grid.shape[2]):
                if voxel_grid[i, j, k]:
                    sdf[i, j, k] = -1  # Inside the mesh
                else:
                    sdf[i, j, k] = 1   # Outside the mesh

    # Apply a distance transform to the SDF
    sdf = distance_transform_edt(sdf)

    return sdf

def save_sdf_as_raw(sdf, file_path):
    """Save the SDF as a raw binary file."""
    # Normalize the SDF to the range [0, 1] for Vulkan compatibility
    sdf_normalized = (sdf - np.min(sdf)) / (np.max(sdf) - np.min(sdf))

    # Flatten the 3D array and write to a binary file
    sdf_normalized.astype(np.float32).tofile(file_path)
    print(f"SDF saved as raw binary file to {file_path}")

def visualize_sdf(sdf):
    """Visualize the SDF using matplotlib."""
    # Extract a slice from the SDF
    slice_idx = sdf.shape[0] // 2
    sdf_slice = sdf[slice_idx, :, :]

    # Plot the SDF slice
    plt.imshow(sdf_slice, cmap='viridis')
    plt.colorbar(label='Distance')
    plt.title('SDF Slice')
    plt.show()

def main():
    parser = argparse.ArgumentParser(
        prog='Mesh to SDF generator',
        description='Loads a mesh and saves it as a SDF in a .raw format')
    parser.add_argument('mesh_path')
    parser.add_argument('sdf_path')
    parser.add_argument("--res", type=int, help="Resolution of voxel grid", default=64)
    args = parser.parse_args()

    # Load the mesh
    mesh = load_mesh(args.mesh_path)

    # Voxelize the mesh
    voxels = voxelize_mesh(mesh, voxel_resolution=args.res)

    # Create the SDF
    sdf = create_sdf(voxels)

    # Save the SDF as a raw binary file
    save_sdf_as_raw(sdf, args.sdf_path)

    # Visualize the SDF
    visualize_sdf(sdf)

if __name__ == "__main__":
    main()