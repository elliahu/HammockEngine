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

# Select noise type for each channel
st.subheader("Channel Configuration")
col1, col2, col3, col4 = st.columns(4)

with col1:
    red_noise = st.selectbox("Red Channel", ["None", "Perlin", "Worley", "Curl", "Perlin-Worley"], key="red")
with col2:
    green_noise = st.selectbox("Green Channel", ["None", "Perlin", "Worley", "Curl", "Perlin-Worley"], key="green")
with col3:
    blue_noise = st.selectbox("Blue Channel", ["None", "Perlin", "Worley", "Curl", "Perlin-Worley"], key="blue")
with col4:
    alpha_noise = st.selectbox("Alpha Channel", ["None", "Perlin", "Worley", "Curl", "Perlin-Worley"], key="alpha")

# Display options
st.subheader("Display Settings")
show_channels = st.multiselect("Show Channels", ["Red", "Green", "Blue", "Alpha"], default=["Red"])

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

# Channel-specific parameters
def show_noise_params(noise_type, channel):
    if noise_type in ["Perlin", "Curl", "Perlin-Worley"]:
        return {
            'scale': st.slider(f"Scale ({channel})", 1.0, 100.0, 30.0, step=1.0, key=f"scale_{channel}"),
            'octaves': st.slider(f"Octaves ({channel})", 1, 8, 5, key=f"octaves_{channel}"),
            'persistence': st.slider(f"Persistence ({channel})", 0.1, 1.0, 0.5, step=0.05, key=f"persistence_{channel}"),
            'lacunarity': st.slider(f"Lacunarity ({channel})", 1.0, 4.0, 2.0, step=0.1, key=f"lacunarity_{channel}")
        }
    return {}

# Show parameters for active channels
st.subheader("Channel Parameters")
params = {}
if red_noise != "None":
    st.markdown("### 🔴 Red Channel Parameters")
    params['red'] = show_noise_params(red_noise, 'red')
if green_noise != "None":
    st.markdown("### 🟢 Green Channel Parameters")
    params['green'] = show_noise_params(green_noise, 'green')
if blue_noise != "None":
    st.markdown("### 🔵 Blue Channel Parameters")
    params['blue'] = show_noise_params(blue_noise, 'blue')
if alpha_noise != "None":
    st.markdown("### ⚪ Alpha Channel Parameters")
    params['alpha'] = show_noise_params(alpha_noise, 'alpha')

# [Previous noise generation functions remain the same]
def generate_perlin_2d(params):
    world = np.zeros((size, size))
    for y in range(size):
        for x in range(size):
            world[y, x] = noise.pnoise2(
                x / params['scale'], y / params['scale'],
                octaves=params['octaves'], persistence=params['persistence'],
                lacunarity=params['lacunarity'], repeatx=size, repeaty=size, base=seed
            )
    return (world - world.min()) / (world.max() - world.min())

def generate_perlin_3d(params):
    world = np.zeros((size, size, size))
    for z in range(size):
        for y in range(size):
            for x in range(size):
                world[z, y, x] = noise.pnoise3(
                    x / params['scale'], y / params['scale'], z / params['scale'],
                    octaves=params['octaves'], persistence=params['persistence'],
                    lacunarity=params['lacunarity'], repeatx=size, repeaty=size, repeatz=size, base=seed
                )
    return (world - world.min()) / (world.max() - world.min())

def generate_worley_2d():
    np.random.seed(seed)
    points = np.random.randint(0, size, (50, 2))
    world = np.zeros((size, size))
    
    for y in range(size):
        for x in range(size):
            distances = [distance.euclidean((x, y), point) for point in points]
            world[y, x] = 1 - min(distances)
    
    return (world - world.min()) / (world.max() - world.min())

def generate_worley_3d():
    np.random.seed(seed)
    points = np.random.randint(0, size, (50, 3))
    world = np.zeros((size, size, size))
    
    for z in range(size):
        for y in range(size):
            for x in range(size):
                distances = [distance.euclidean((x, y, z), point) for point in points]
                world[z, y, x] = 1 - min(distances)
    
    return (world - world.min()) / (world.max() - world.min())

def generate_curl_2d(params):
    perlin_x = generate_perlin_2d(params)
    perlin_y = generate_perlin_2d(params)
    curl_noise = np.gradient(perlin_x, axis=1) - np.gradient(perlin_y, axis=0)
    return (curl_noise - curl_noise.min()) / (curl_noise.max() - curl_noise.min())

def generate_curl_3d(params):
    perlin_x = generate_perlin_3d(params)
    perlin_y = generate_perlin_3d(params)
    curl_noise = np.gradient(perlin_x, axis=1) - np.gradient(perlin_y, axis=0)
    return (curl_noise - curl_noise.min()) / (curl_noise.max() - curl_noise.min())

def generate_perlin_worley_2d(params):
    perlin = generate_perlin_2d(params)
    worley = generate_worley_2d()
    return (perlin + worley) / 2

def generate_perlin_worley_3d(params):
    perlin = generate_perlin_3d(params)
    worley = generate_worley_3d()
    return (perlin + worley) / 2

def generate_channel_noise(noise_type, channel_params, is_3d=False):
    if noise_type == "None":
        return np.zeros((size, size, size) if is_3d else (size, size))
    
    noise_generators = {
        "Perlin": (generate_perlin_3d, generate_perlin_2d),
        "Worley": (generate_worley_3d, generate_worley_2d),
        "Curl": (generate_curl_3d, generate_curl_2d),
        "Perlin-Worley": (generate_perlin_worley_3d, generate_perlin_worley_2d)
    }
    
    if noise_type not in noise_generators:
        return np.zeros((size, size, size) if is_3d else (size, size))
        
    generator_3d, generator_2d = noise_generators[noise_type]
    
    if noise_type == "Worley":
        return generator_3d() if is_3d else generator_2d()
    else:
        return generator_3d(channel_params) if is_3d else generator_2d(channel_params)

# Generate noise for each channel
is_3d = dimensionality == "3D"
noise_data = {
    'red': generate_channel_noise(red_noise, params.get('red', {}), is_3d) if red_noise != "None" else None,
    'green': generate_channel_noise(green_noise, params.get('green', {}), is_3d) if green_noise != "None" else None,
    'blue': generate_channel_noise(blue_noise, params.get('blue', {}), is_3d) if blue_noise != "None" else None,
    'alpha': generate_channel_noise(alpha_noise, params.get('alpha', {}), is_3d) if alpha_noise != "None" else None
}

# Combine channels for display
def combine_channels(noise_data, show_channels, z_slice=None):
    # For single channel display, return grayscale
    if len(show_channels) == 1:
        channel = show_channels[0]
        key = channel.lower()
        if noise_data[key] is not None:
            if z_slice is not None and len(noise_data[key].shape) == 3:
                return np.stack([noise_data[key][z_slice]] * 3, axis=-1)
            else:
                return np.stack([noise_data[key]] * 3, axis=-1)
        return np.zeros((size, size, 3))
    
    # For multiple channels, combine them
    rgba = np.zeros((size, size, 4))
    channel_mapping = {
        'Red': (0, 'red'), 
        'Green': (1, 'green'), 
        'Blue': (2, 'blue'), 
        'Alpha': (3, 'alpha')
    }
    
    for channel in channel_mapping:
        idx, key = channel_mapping[channel]
        if channel in show_channels and noise_data[key] is not None:
            if z_slice is not None and len(noise_data[key].shape) == 3:
                rgba[:, :, idx] = noise_data[key][z_slice]
            else:
                rgba[:, :, idx] = noise_data[key]
    
    # If alpha is not selected, set it to 1
    if 'Alpha' not in show_channels:
        rgba[:, :, 3] = 1.0
    
    # Convert RGBA to RGB for display
    rgb = np.zeros((size, size, 3))
    rgb = rgba[:, :, :3] * rgba[:, :, 3:4] + (1 - rgba[:, :, 3:4])
    
    return rgb

# Display combined noise
combined_noise = combine_channels(noise_data, show_channels, z_slice if is_3d else None)

fig, ax = plt.subplots()
if len(show_channels) == 1:
    ax.imshow(combined_noise, cmap='gray')
else:
    ax.imshow(combined_noise)
ax.axis("off")
st.pyplot(fig)

# Export functionality
save_path = st.text_input("📁 Enter full export file path with filename", "noise_exports/noise.png")

if not save_path.lower().endswith(".png"):
    st.error("❌ Please specify a valid PNG file path (including .png extension).")

def export_noise(file_path):
    start_time = time.time()
    st.session_state["export_status"] = f"🔄 Exporting {dimensionality} noise..."
    
    os.makedirs(os.path.dirname(file_path), exist_ok=True)
    
    if dimensionality == "2D":
        # Convert to RGBA for export
        rgba = np.zeros((size, size, 4))
        for i, (channel, key) in enumerate([('Red', 'red'), ('Green', 'green'), ('Blue', 'blue'), ('Alpha', 'alpha')]):
            if channel in show_channels and noise_data[key] is not None:
                rgba[:, :, i] = noise_data[key]
            elif i < 3:  # RGB channels
                rgba[:, :, i] = 0
            else:  # Alpha channel
                rgba[:, :, i] = 1
        
        image = Image.fromarray((rgba * 255).astype(np.uint8), 'RGBA')
        image.save(file_path)
    else:
        for z in range(size):
            rgba = np.zeros((size, size, 4))
            for i, (channel, key) in enumerate([('Red', 'red'), ('Green', 'green'), ('Blue', 'blue'), ('Alpha', 'alpha')]):
                if channel in show_channels and noise_data[key] is not None:
                    rgba[:, :, i] = noise_data[key][z]
                elif i < 3:  # RGB channels
                    rgba[:, :, i] = 0
                else:  # Alpha channel
                    rgba[:, :, i] = 1
            
            image = Image.fromarray((rgba * 255).astype(np.uint8), 'RGBA')
            image.save(file_path.replace(".png", f"_slice_{z:03d}.png"))
            
            progress = (z + 1) / size * 100
            elapsed = time.time() - start_time
            estimated_total = elapsed / (z + 1) * size
            remaining = estimated_total - elapsed
            st.session_state["export_status"] = f"Exporting slice {z+1}/{size} ({progress:.1f}%) - ⏳ {remaining:.1f}s remaining..."
    
    st.session_state["export_status"] = f"✅ Export completed in {time.time() - start_time:.2f} sec."

if st.button("📤 Export as PNG") and save_path.lower().endswith(".png"):
    st.session_state["export_status"] = "Initializing..."
    threading.Thread(target=export_noise, args=(save_path,), daemon=True).start()

if "export_status" in st.session_state:
    st.info(st.session_state["export_status"])