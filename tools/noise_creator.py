import streamlit as st
import noise
import numpy as np
import matplotlib.pyplot as plt
import os
import time
import threading
from scipy.spatial import Voronoi, distance
from PIL import Image

# Move control elements to sidebar
st.sidebar.title("🌥️ Cloud Noise Editor")

# Channel selection in sidebar
st.sidebar.subheader("Channel Configuration")
red_noise = st.sidebar.selectbox("Red Channel", ["None", "Perlin", "Worley", "Curl", "Perlin-Worley"], key="red")
green_noise = st.sidebar.selectbox("Green Channel", ["None", "Perlin", "Worley", "Curl", "Perlin-Worley"], key="green")
blue_noise = st.sidebar.selectbox("Blue Channel", ["None", "Perlin", "Worley", "Curl", "Perlin-Worley"], key="blue")
alpha_noise = st.sidebar.selectbox("Alpha Channel", ["None", "Perlin", "Worley", "Curl", "Perlin-Worley"], key="alpha")

# Display settings in sidebar
st.sidebar.subheader("Display Settings")
show_channels = st.sidebar.multiselect("Show Channels", ["Red", "Green", "Blue", "Alpha"], default=["Red"])
dimensionality = st.sidebar.radio("Noise Dimension", ["2D", "3D"])
size = st.sidebar.slider("Resolution", 16, 512, 64, step=16)

if dimensionality == "3D":
    z_slice = st.sidebar.slider("Z-Slice", 0, size - 1, 0)

seed = st.sidebar.slider("Seed", 0, 1000, 42)

# Channel-specific parameters
def show_noise_params(noise_type, channel):
    if noise_type in ["Perlin", "Curl", "Perlin-Worley"]:
        return {
            'scale': st.sidebar.slider(f"Scale ({channel})", 1.0, 100.0, 30.0, step=1.0, key=f"scale_{channel}"),
            'octaves': st.sidebar.slider(f"Octaves ({channel})", 1, 8, 5, key=f"octaves_{channel}"),
            'persistence': st.sidebar.slider(f"Persistence ({channel})", 0.1, 1.0, 0.5, step=0.05, key=f"persistence_{channel}"),
            'lacunarity': st.sidebar.slider(f"Lacunarity ({channel})", 1.0, 4.0, 2.0, step=0.1, key=f"lacunarity_{channel}")
        }
    elif noise_type == "Worley":
        return {
            'num_points': st.sidebar.slider(f"Number of Points ({channel})", 1, 100, 20, key=f"points_{channel}"),
            'distance_func': st.sidebar.selectbox(f"Distance Function ({channel})", ['Euclidean', 'Manhattan', 'Chebyshev'], key=f"dist_{channel}"),
            'combination': st.sidebar.selectbox(f"Combination Method ({channel})", ['Nearest', 'Second Nearest', 'Difference'], key=f"comb_{channel}")
        }
    return {}

# Channel parameters in sidebar
st.sidebar.subheader("Channel Parameters")
params = {}
if red_noise != "None":
    st.sidebar.markdown("### 🔴 Red Channel")
    params['red'] = show_noise_params(red_noise, 'red')
if green_noise != "None":
    st.sidebar.markdown("### 🟢 Green Channel")
    params['green'] = show_noise_params(green_noise, 'green')
if blue_noise != "None":
    st.sidebar.markdown("### 🔵 Blue Channel")
    params['blue'] = show_noise_params(blue_noise, 'blue')
if alpha_noise != "None":
    st.sidebar.markdown("### ⚪ Alpha Channel")
    params['alpha'] = show_noise_params(alpha_noise, 'alpha')

# Main content area for preview and export
st.title("Preview & Export")

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

def generate_worley_2d(params):
    np.random.seed(seed)
    num_points = params.get('num_points', 20)
    points = np.random.randint(0, size, (num_points, 2))
    world = np.zeros((size, size))
    
    distance_funcs = {
        'Euclidean': lambda p1, p2: distance.euclidean(p1, p2),
        'Manhattan': lambda p1, p2: distance.cityblock(p1, p2),
        'Chebyshev': lambda p1, p2: distance.chebyshev(p1, p2)
    }
    
    dist_func = distance_funcs[params.get('distance_func', 'Euclidean')]
    combination = params.get('combination', 'Nearest')
    
    for y in range(size):
        for x in range(size):
            distances = sorted([dist_func((x, y), point) for point in points])
            if combination == 'Nearest':
                world[y, x] = 1 - distances[0]
            elif combination == 'Second Nearest':
                world[y, x] = 1 - distances[1]
            else:  # Difference
                world[y, x] = 1 - (distances[1] - distances[0])
    
    return (world - world.min()) / (world.max() - world.min())

def generate_worley_3d(params):
    np.random.seed(seed)
    num_points = params.get('num_points', 20)
    points = np.random.randint(0, size, (num_points, 3))
    world = np.zeros((size, size, size))
    
    distance_funcs = {
        'Euclidean': lambda p1, p2: distance.euclidean(p1, p2),
        'Manhattan': lambda p1, p2: distance.cityblock(p1, p2),
        'Chebyshev': lambda p1, p2: distance.chebyshev(p1, p2)
    }
    
    dist_func = distance_funcs[params.get('distance_func', 'Euclidean')]
    combination = params.get('combination', 'Nearest')
    
    for z in range(size):
        for y in range(size):
            for x in range(size):
                distances = sorted([dist_func((x, y, z), point) for point in points])
                if combination == 'Nearest':
                    world[z, y, x] = 1 - distances[0]
                elif combination == 'Second Nearest':
                    world[z, y, x] = 1 - distances[1]
                else:  # Difference
                    world[z, y, x] = 1 - (distances[1] - distances[0])
    
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
    worley = generate_worley_2d(params)
    return (perlin + worley) / 2

def generate_perlin_worley_3d(params):
    perlin = generate_perlin_3d(params)
    worley = generate_worley_3d(params)
    return (perlin + worley) / 2

@st.cache_data
def cached_noise_generation(noise_type, is_3d, seed_val, size_val, channel_id=None, **params):
    """Cached wrapper for noise generation with channel-specific caching"""
    if noise_type == "None":
        return np.zeros((size_val, size_val, size_val) if is_3d else (size_val, size_val))
    
    noise_generators = {
        "Perlin": (generate_perlin_3d, generate_perlin_2d),
        "Worley": (generate_worley_3d, generate_worley_2d),
        "Curl": (generate_curl_3d, generate_curl_2d),
        "Perlin-Worley": (generate_perlin_worley_3d, generate_perlin_worley_2d)
    }
    
    if noise_type not in noise_generators:
        return np.zeros((size_val, size_val, size_val) if is_3d else (size_val, size_val))
        
    generator_3d, generator_2d = noise_generators[noise_type]
    return generator_3d(params) if is_3d else generator_2d(params)

def generate_channel_noise(noise_type, channel_params, is_3d=False, channel_id=None):
    params_dict = dict(channel_params) if channel_params else {}
    return cached_noise_generation(
        noise_type=noise_type,
        is_3d=is_3d,
        seed_val=seed,
        size_val=size,
        channel_id=channel_id,
        **params_dict
    )

# Generate noise for each channel
is_3d = dimensionality == "3D"
noise_data = {
    'red': generate_channel_noise(red_noise, params.get('red', {}), is_3d, 'red'),
    'green': generate_channel_noise(green_noise, params.get('green', {}), is_3d, 'green'),
    'blue': generate_channel_noise(blue_noise, params.get('blue', {}), is_3d, 'blue'),
    'alpha': generate_channel_noise(alpha_noise, params.get('alpha', {}), is_3d, 'alpha')
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


# Export status
if "export_status" in st.session_state:
    st.info(st.session_state["export_status"])

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


# Export controls in columns
col1, col2, col3 = st.columns([2, 1, 1])
with col1:
    save_path = st.text_input("📁 Export file path", "noise_exports/noise.png")
with col2:
    if st.button("📤 Export as PNG") and save_path.lower().endswith(".png"):
        st.session_state["export_status"] = "Initializing..."
        threading.Thread(target=export_noise, args=(save_path,), daemon=True).start()
with col3:
    if os.path.exists(save_path):
        with open(save_path, "rb") as file:
            st.download_button(
                label="⬇️ Download",
                data=file,
                file_name=os.path.basename(save_path),
                mime="image/png"
            )