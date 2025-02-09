import io
import numpy as np
import streamlit as st
from PIL import Image

def create_gradient(width, height, direction='horizontal', intensity_stops=None):
    try:
        if intensity_stops is None or len(intensity_stops) < 2:
            return None
            
        # Sort stops by position
        intensity_stops.sort(key=lambda x: x[1])
        positions = np.array([stop[1] for stop in intensity_stops])
        intensities = np.array([stop[0] for stop in intensity_stops])
        
        # Create position array based on direction
        if direction == 'horizontal':
            x = np.linspace(0, 1, width)
            gradient = np.interp(x, positions, intensities)
            gradient = np.tile(gradient, (height, 1))
        else:  # vertical
            y = np.linspace(0, 1, height)
            gradient = np.interp(y, positions, intensities)
            gradient = np.tile(gradient.reshape(-1, 1), (1, width))
        
        return gradient.astype(np.uint8)
    except Exception as e:
        st.error(f"Error creating gradient: {str(e)}")
        return None

st.title("Black & White Gradient Creator")

# Sidebar controls
st.sidebar.header("Gradient Settings")
width = st.sidebar.slider("Width", min_value=8, max_value=512, value=64, step=8)
height = st.sidebar.slider("Height", min_value=8, max_value=512, value=64, step=8)
direction = st.sidebar.radio("Direction", ['vertical', 'horizontal'])

# Multiple intensity stops
st.sidebar.subheader("Intensity Stops")
num_stops = st.sidebar.number_input("Number of stops", min_value=2, max_value=10, value=2)
intensity_stops = []

for i in range(num_stops):
    col1, col2 = st.sidebar.columns(2)
    with col1:
        intensity = st.number_input(f"Intensity {i+1}", min_value=0, max_value=255, value=255 if i==0 else 0)
    with col2:
        position = st.number_input(f"Position {i+1}", min_value=0.0, max_value=1.0, value=0.0 if i==0 else 1.0)
    intensity_stops.append((intensity, position))

# Create and display gradient
gradient = create_gradient(width, height, direction, intensity_stops)
if gradient is not None:
    gradient_image = Image.fromarray(gradient)
    st.image(gradient_image, caption="Gradient Preview", use_container_width=True)

    # Export functionality
    export_filename = st.text_input("Export filename (with .png extension)", "gradient.png")
    if st.button("Export Gradient"):
        if not export_filename.endswith('.png'):
            st.error("Filename must end with .png")
        else:
            try:
                gradient_image.save(export_filename)
                st.success(f"Gradient saved as {export_filename}")
            except Exception as e:
                st.error(f"Error saving file: {str(e)}")

    # Add download button
    try:
        buf = io.BytesIO()
        gradient_image.save(buf, format='PNG')
        btn = st.download_button(
            label="Download Gradient",
            data=buf.getvalue(),
            file_name=export_filename,
            mime="image/png"
        )
    except Exception as e:
        st.error(f"Error preparing download: {str(e)}")