import trimesh
import numpy as np
import struct
import argparse

def compute_sdf(mesh, grid_size=128):
    """
    Computes a Signed Distance Field (SDF) for a mesh using a voxel grid.
    Returns a 3D numpy array of SDF values.
    """
    # Generate a grid of points that covers the bounding box of the mesh
    min_bound = mesh.bounds[0]
    max_bound = mesh.bounds[1]
    grid = np.mgrid[min_bound[0]:max_bound[0]:complex(grid_size),
           min_bound[1]:max_bound[1]:complex(grid_size),
           min_bound[2]:max_bound[2]:complex(grid_size)]

    grid_points = np.vstack([grid[0].ravel(), grid[1].ravel(), grid[2].ravel()]).T

    # Ensure grid_points is of shape (n, 3)
    grid_points = np.array(grid_points)

    # Compute SDF values for all points at once
    sdf_values = mesh.nearest.on_surface(grid_points)[1]  # Get distances for all points
    sdf_grid = sdf_values.reshape((grid_size, grid_size, grid_size))

    return sdf_grid

def save_sdf_to_file(sdf_grid, filename="sdf_data.bin"):
    """
    Saves the SDF grid to a binary file.
    """
    with open(filename, "wb") as file:
        # Write the grid size (int) and the sdf grid as floats
        grid_size = sdf_grid.shape[0]
        file.write(struct.pack("I", grid_size))  # Write grid size as an unsigned int
        file.write(sdf_grid.astype(np.float32).tobytes())  # Write the SDF grid data as floats

def main():
    parser = argparse.ArgumentParser(description='Generate and visualize a Signed Distance Field (SDF) from a mesh.')
    parser.add_argument('mesh_path', help='Path to the input mesh file')
    parser.add_argument('sdf_path', help='Path to save the SDF output file')
    parser.add_argument('--res', type=int, default=64, help='Resolution of the voxel grid')
    args = parser.parse_args()

    # Load the mesh
    mesh = trimesh.load_mesh(args.mesh_path)

    # Compute the SDF
    sdf_grid = compute_sdf(mesh, args.res)

    # Save the SDF to a file
    save_sdf_to_file(sdf_grid, args.sdf_path)

if __name__ == "__main__":
    main()
