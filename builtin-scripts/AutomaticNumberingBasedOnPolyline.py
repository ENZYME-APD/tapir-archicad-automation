##############################################
# SCRIPT AUTOMATIC NUMBERING BASED ON POLYLINE
# For ARCHICAD 27 with Python 3.X and Tapir addon
# Works with any selected elements - Improved error handling
#
# Created by Mathias Jonathan
###############################################

import tkinter as tk
from tkinter import messagebox, simpledialog
from archicad import ACConnection
import aclib
import math

# Establish connection with ArchiCAD
conn = ACConnection.connect()
assert conn

# Create shortcuts for commands, types and utilities
acc = conn.commands
act = conn.types
acu = conn.utilities

# Function to show a popup message
def show_popup(message, title="Script Message", message_type="error"):
    root = tk.Tk()
    root.withdraw()  # Hide the main window
    print(message)
    if message_type == "error":
        messagebox.showerror(title, message)
    elif message_type == "info":
        messagebox.showinfo(title, message)
    elif message_type == "warning":
        messagebox.showwarning(title, message)
    root.destroy()

# Function to show an input dialog
def show_input_dialog(prompt, title="Input Needed", default="1"):
    root = tk.Tk()
    root.withdraw()  # Hide the main window
    response = simpledialog.askstring(title, prompt, initialvalue=default)
    root.destroy()
    return response

################################
# CONFIGURATION PARAMETERS
################################


# Property ID for the General_ElementID that will be numbered
propertyId = acu.GetBuiltInPropertyId('General_ElementID')

################################
# UTILITY FUNCTIONS
################################

# Generate the property value string
def generatePropertyValueString(elemIndex: int) -> str:
    return f"{elemIndex:03d}"  # Format as 001, 002, etc.

# Create NormalStringPropertyValue from the string
def generatePropertyValue(elemIndex: int) -> act.NormalStringPropertyValue:
    return act.NormalStringPropertyValue(generatePropertyValueString(elemIndex))

# Calculate distance between two points
def distance(x1, y1, x2, y2):
    return math.sqrt((x2 - x1) ** 2 + (y2 - y1) ** 2)

# Find the closest point on a line segment to a given point
def closest_point_on_segment(px, py, x1, y1, x2, y2):
    # Calculate the length of the segment squared
    segment_length_squared = (x2 - x1) ** 2 + (y2 - y1) ** 2

    # If the segment is just a point, return that point
    if segment_length_squared == 0:
        return (x1, y1), distance(px, py, x1, y1), 0

    # Calculate the projection of point P onto the segment
    t = max(0, min(1, ((px - x1) * (x2 - x1) + (py - y1)
            * (y2 - y1)) / segment_length_squared))

    # Find the closest point on the segment
    closest_x = x1 + t * (x2 - x1)
    closest_y = y1 + t * (y2 - y1)

    return (closest_x, closest_y), distance(px, py, closest_x, closest_y), t

# Find the closest point on a polyline to a given point
def closest_point_on_polyline(polyline_points, point):
    min_distance = float("inf")
    closest_point = None
    segment_start_index = 0
    segment_relative_pos = 0

    # Check each segment of the polyline
    for i in range(len(polyline_points) - 1):
        p1 = polyline_points[i]
        p2 = polyline_points[i + 1]

        # Calculate distance to the segment
        segment_point, segment_distance, rel_position = closest_point_on_segment(
            point[0], point[1],
            p1[0], p1[1],
            p2[0], p2[1]
        )

        if segment_distance < min_distance:
            min_distance = segment_distance
            closest_point = segment_point
            segment_start_index = i
            segment_relative_pos = rel_position

    # Calculate the total position along the polyline
    total_position = 0
    for i in range(segment_start_index):
        p1 = polyline_points[i]
        p2 = polyline_points[i + 1]
        total_position += distance(p1[0], p1[1], p2[0], p2[1])

    # Add the position along the current segment
    p1 = polyline_points[segment_start_index]
    p2 = polyline_points[segment_start_index + 1]
    segment_length = distance(p1[0], p1[1], p2[0], p2[1])
    total_position += segment_length * segment_relative_pos

    return closest_point, min_distance, total_position

################################
# MAIN PROCESS
################################


# Get all selected elements
selectedElementsResponse = aclib.RunTapirCommand('GetSelectedElements', {})
selectedElements = selectedElementsResponse.get('elements', [])

if not selectedElements:
    show_popup("No elements selected! Please select the reference polyline and the elements to number.",
               "Selection Error", "error")
    exit(1)

print(f"Found {len(selectedElements)} selected elements")

detailsOfElementsResponse = aclib.RunTapirCommand(
    'GetDetailsOfElements', {'elements': selectedElements})
detailsOfElements = detailsOfElementsResponse.get('detailsOfElements', [])

if not detailsOfElements:
    show_popup("No elements selected! Please select the reference polyline and the elements to number.",
               "Selection Error", "error")
    exit(1)

# Find the reference polyline object among selected elements
referencePolylines = []

for i in range(len(selectedElements)):
    if detailsOfElements[i]['type'] == 'PolyLine':
        referencePolylines.append((selectedElements[i], detailsOfElements[i]))

# Check if we found exactly one reference polyline in the selection
if len(referencePolylines) == 0:
    show_popup("No reference polyline found in the selection!\n\nPlease select exactly ONE polyline along with the elements you want to number.",
               "Polyline Error", "error")
    exit(1)
elif len(referencePolylines) > 1:
    show_popup("Multiple reference polylines found in the selection!\n\nPlease select exactly ONE polyline for this script to work properly.",
               "Polyline Error", "error")
    exit(1)

# If we get here, we have exactly one reference polyline in the selection
referencePolyline = referencePolylines[0][0]
detailsOfReferencePolyline = referencePolylines[0][1]

polyline_points = [(coord['x'], coord['y'])
                   for coord in detailsOfReferencePolyline['details']['coordinates']]

print(f"Reference polyline has {len(polyline_points)} points")

if len(polyline_points) < 2:
    show_popup("Error: Reference polyline needs at least 2 points for proper numbering!",
               "Polyline Error", "error")
    exit(1)

# Ask user for starting number
try:
    starting_number = show_input_dialog(
        "Enter the starting number for the first element:", "Numbering Configuration", "1")
    if starting_number is None:  # User clicked Cancel
        print("Script cancelled by user")
        exit(0)

    # Validate input: must be a positive integer
    starting_number = int(starting_number)
    if starting_number < 1:
        show_popup(
            "Please enter a positive integer as the starting number.\nScript will now exit.",
            "Invalid Input", "error")
        exit(1)
except ValueError:
    show_popup(
        "Please enter a valid integer number.\nScript will now exit.",
        "Invalid Input", "error")
    exit(1)

print(f"Using starting number: {starting_number}")

# Exclude the reference polyline from the elements to number
elementsToNumber = []
for elem in selectedElements:
    if referencePolyline and elem['elementId']['guid'] != referencePolyline['elementId']['guid']:
        elementsToNumber.append(elem)

if not elementsToNumber:
    show_popup("Error: No elements to number! Please select elements other than the reference polyline.",
               "Selection Error", "error")
    exit(1)

print(f"Found {len(elementsToNumber)} elements to number")

# Get 3D bounding boxes for all elements to number
try:
    boundingBoxesResponse = acc.Get3DBoundingBoxes(elementsToNumber)
    boundingBoxes = boundingBoxesResponse
except Exception as e:
    show_popup(
        f"Error getting bounding boxes: {e}", "Processing Error", "error")
    exit(1)

# Calculate center points of each bounding box
element_centers = []
for i, bb in enumerate(boundingBoxes):
    try:
        # Calculate the center of the bounding box
        x_center = (bb.boundingBox3D.xMin + bb.boundingBox3D.xMax) / 2
        y_center = (bb.boundingBox3D.yMin + bb.boundingBox3D.yMax) / 2

        element_centers.append({
            'element': elementsToNumber[i],
            'center': (x_center, y_center)
        })
    except Exception as e:
        print(f"Error processing bounding box for element {i}: {e}")
        continue

if not element_centers:
    show_popup("Error: Could not calculate centers for any elements!",
               "Processing Error", "error")
    exit(1)

# Find closest points and sort elements
element_positions = []

for elem in element_centers:
    center = elem['center']

    # Find closest point on the polyline
    closest_point, min_distance, position_on_polyline = closest_point_on_polyline(
        polyline_points, center)

    element_positions.append({
        'element': elem['element'],
        'center': center,
        'closest_point': closest_point,
        'distance': min_distance,
        'position': position_on_polyline
    })

# Sort elements by their position along the polyline
sorted_elements = sorted(element_positions, key=lambda elem: elem['position'])

# Number the elements
elemPropertyValues = []

for index, elem in enumerate(sorted_elements, start=starting_number):
    # Create element property value
    elemPropertyValues.append(act.ElementPropertyValue(
        elem['element']['elementId'], propertyId, generatePropertyValue(index)))

# Set the new property values for all elements
try:
    acc.SetPropertyValuesOfElements(elemPropertyValues)
    print(
        f"Successfully numbered {len(sorted_elements)} elements starting from {starting_number}")
except Exception as e:
    show_popup(
        f"Error setting property values: {e}", "Processing Error", "error")
    exit(1)

# Show success message to user
show_popup(f"Successfully numbered {len(sorted_elements)} elements starting from {starting_number}.",
           "Operation Complete", "info")
