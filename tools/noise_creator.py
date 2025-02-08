import streamlit as st
import noise
import numpy as np
import matplotlib.pyplot as plt
import os
import time
import threading
from scipy.spatial import Voronoi, distance
from PIL import Image

# Streamlit UI
st.title("🌥️ Cloud Noise Editor")

# Select noise type
noise_type = st.selectbox("Select Noise Type", ["Perlin", "Worley", "Curl", "Perlin-Worley"])

# Select 2D or 3D
dimensionality = st.radio("Noise Dimension", ["2D", "3D"])

# Resolution slider
size = st.slider("Resolution", 16, 512, 64, step=16)

# 3D Slice Selector
z_slice = 0
if dimensionality == "3D":
    z_slice = st.slider("Z-Slice", 0, size - 1, 0)

# Common Parameters
seed = st.slider("Seed", 0, 1000, 42)

# Parameters for Perlin and Curl noise
if noise_type in ["Perlin", "Curl", "Perlin-Worley"]:
    scale = st.slider("Scale", 1.0, 100.0, 30.0, step=1.0)
    octaves = st.slider("Octaves", 1, 8, 5)
    persistence = st.slider("Persistence", 0.1, 1.0, 0.5, step=0.05)
    lacunarity = st.slider("Lacunarity", 1.0, 4.0, 2.0, step=0.1)

# Function to save an image
def save_image(image_array, filename):
    image = Image.fromarray((image_array * 255).astype(np.uint8))
    image.save(filename)

# Function to generate 3D Perlin noise
def generate_perlin_3d():
    world = np.zeros((size, size, size))
    for z in range(size):
        for y in range(size):
            for x in range(size):
                world[z, y, x] = noise.pnoise3(
                    x / scale, y / scale, z / scale,
                    octaves=octaves, persistence=persistence,
                    lacunarity=lacunarity, repeatx=size, repeaty=size, repeatz=size, base=seed
                )
    return (world - world.min()) / (world.max() - world.min())  # Normalize

# Function to generate 3D Worley noise
def generate_worley_3d():
    np.random.seed(seed)
    points = np.random.randint(0, size, (50, 3))  # Random 3D points
    world = np.zeros((size, size, size))

    for z in range(size):
        for y in range(size):
            for x in range(size):
                distances = [distance.euclidean((x, y, z), point) for point in points]
                world[z, y, x] = min(distances)

    return (world - world.min()) / (world.max() - world.min())  # Normalize

# Function to generate 3D Curl noise
def generate_curl_3d():
    perlin_x = generate_perlin_3d()
    perlin_y = generate_perlin_3d()
    curl_noise = np.gradient(perlin_x, axis=1) - np.gradient(perlin_y, axis=0)
    return (curl_noise - curl_noise.min()) / (curl_noise.max() - curl_noise.min())  # Normalize

# Function to generate 3D Perlin-Worley hybrid noise
def generate_perlin_worley_3d():
    return (generate_perlin_3d() + generate_worley_3d()) / 2  # Blend both noises

# Function to generate 2D Perlin noise
def generate_perlin_2d():
    world = np.zeros((size, size))
    for y in range(size):
        for x in range(size):
            world[y, x] = noise.pnoise2(
                x / scale, y / scale,
                octaves=octaves, persistence=persistence,
                lacunarity=lacunarity, repeatx=size, repeaty=size, base=seed
            )
    return (world - world.min()) / (world.max() - world.min())  # Normalize

# Function to generate 2D Worley noise
def generate_worley_2d():
    np.random.seed(seed)
    points = np.random.randint(0, size, (50, 2))  # Random feature points
    vor = Voronoi(points)
    world = np.zeros((size, size))

    for y in range(size):
        for x in range(size):
            distances = [distance.euclidean((x, y), point) for point in points]
            world[y, x] = min(distances)

    return (world - world.min()) / (world.max() - world.min())  # Normalize

# Function to generate 2D Curl noise
def generate_curl_2d():
    perlin_x = generate_perlin_2d()
    perlin_y = generate_perlin_2d()
    curl_noise = np.gradient(perlin_x, axis=1) - np.gradient(perlin_y, axis=0)
    return (curl_noise - curl_noise.min()) / (curl_noise.max() - curl_noise.min())  # Normalize

# Function to generate 2D Perlin-Worley hybrid noise
def generate_perlin_worley_2d():
    return (generate_perlin_2d() + generate_worley_2d()) / 2  # Blend both noises
# Function to generate the selected noise
def generate_noise():
    if dimensionality == "3D":
        if noise_type == "Perlin":
            return generate_perlin_3d()
        elif noise_type == "Worley":
            return generate_worley_3d()
        elif noise_type == "Curl":
            return generate_curl_3d()
        elif noise_type == "Perlin-Worley":
            return generate_perlin_worley_3d()
    else:
        if noise_type == "Perlin":
            return generate_perlin_2d()
        elif noise_type == "Worley":
            return generate_worley_2d()
        elif noise_type == "Curl":
            return generate_curl_2d()
        elif noise_type == "Perlin-Worley":
            return generate_perlin_worley_2d()


# Display noise
noise_data = generate_noise()
noise_image = noise_data[z_slice] if dimensionality == "3D" else noise_data

fig, ax = plt.subplots()
ax.imshow(noise_image, cmap="gray")
ax.axis("off")
st.pyplot(fig)

# Path input for export
save_path = st.text_input("📁 Enter full export file path with filename", "noise_exports/noise.png")

# Validate file extension
if not save_path.lower().endswith(".png"):
    st.error("❌ Please specify a valid PNG file path (including .png extension).")

# Parallel Export Function
def export_noise(file_path):
    start_time = time.time()
    st.session_state["export_status"] = f"🔄 Exporting {dimensionality} noise..."
    
    os.makedirs(os.path.dirname(file_path), exist_ok=True)
    
    if dimensionality == "2D":
        save_image(noise_image, file_path)
    else:
        for z in range(size):
            save_image(noise_data[z], file_path.replace(".png", f"_slice_{z:03d}.png"))
            progress = (z + 1) / size * 100
            elapsed = time.time() - start_time
            estimated_total = elapsed / (z + 1) * size
            remaining = estimated_total - elapsed
            st.session_state["export_status"] = f"Exporting slice {z+1}/{size} ({progress:.1f}%) - ⏳ {remaining:.1f}s remaining..."
    
    st.session_state["export_status"] = f"✅ Export completed in {time.time() - start_time:.2f} sec."

# Button for Exporting
if st.button("📤 Export as PNG") and save_path.lower().endswith(".png"):
    st.session_state["export_status"] = "Initializing..."
    threading.Thread(target=export_noise, args=(save_path,), daemon=True).start()

# Show export status
if "export_status" in st.session_state:
    st.info(st.session_state["export_status"])
