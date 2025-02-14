from PIL import Image

# Load the image
image_path = "vhdl_logo.png"
image = Image.open(image_path).convert("RGBA")

# Get pixel data
pixels = image.load()

# Define target colors
black = (0, 0, 0)
replacement = (26, 56, 32, 255)
threshold = 80  # Adjust sensitivity (lower = stricter, higher = more inclusive)

def is_close_to_black(pixel, threshold):
    """Check if a pixel is close to black within a given threshold."""
    r, g, b, _ = pixel  # Extract RGB values (ignore alpha)
    return (r ** 2 + g ** 2 + b ** 2) ** 0.5 < threshold

# Replace close-to-black pixels
for y in range(image.height):
    for x in range(image.width):
        if is_close_to_black(pixels[x, y], threshold):
            pixels[x, y] = replacement

# Save the modified image
output_path = "vhdl_logo2.png"
image.save(output_path)

print(f"Image saved as {output_path}")
