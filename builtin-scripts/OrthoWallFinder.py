'''
Non-orthogonal wall finder script
by Daniel Alexander Kovacs for tapir
https://kadsolutions.co.uk/
Modified by Lucas Becker (@runxel)
'''

import aclib
import math
import tkinter as tk

def round_angle(angle, eps=1e-7):
    """Round angle to nearest integer if within tolerance for angles who are
    not close to 90° or 0° – those are usually indeed errors.
    """
    if abs(angle) > 0.1 or 90.1 < abs(angle) < 89.9:
        rounded = round(angle)
        if abs(angle - rounded) < eps:
            return float(rounded)
    return angle

def command_refresh():

    filter_list = ['IsVisibleByLayer']
    panel_tag = "Number of non-ortho walls in project:"

    if bool_current_story.get():
        filter_list.append('OnActualFloor')
        panel_tag = 'Number of non-ortho walls on current floor:'

    curr_active_view_rot = round_angle(math.degrees(aclib.RunTapirCommand('GetView2DTransformations')['transformations'][0]['rotation']))

    wall_IDs = aclib.RunTapirCommand('GetElementsByType', {'elementType': 'Wall', 'filters': filter_list})
    wall_details = aclib.RunTapirCommand('GetDetailsOfElements', wall_IDs)

    wall_angles = []

    for i in range(len(wall_IDs["elements"])):
        wall_angles.append(wall_IDs["elements"][i])
        x = wall_details['detailsOfElements'][i]['details']['endCoordinate']['x'] - wall_details['detailsOfElements'][i]['details']['begCoordinate']['x']
        y = wall_details['detailsOfElements'][i]['details']['endCoordinate']['y'] - wall_details['detailsOfElements'][i]['details']['begCoordinate']['y']
        atnd = math.degrees(math.atan2(y, x))
        angle = atnd
        # when using "strict mode" we can get false positives because of floating point
        if not strict_mode.get():
            angle = round_angle(atnd)

        if use_view_rot.get():
            angle = round_angle(angle - (360 - curr_active_view_rot))
        
        wall_angles[i]['angle'] = angle

    faulty_walls = []

    for wall in wall_angles:
        if (wall['angle'] % 45) > 0:
            faulty_walls.append(wall)

    # update the shown text
    msg["text"] = f"{panel_tag}\n{len(faulty_walls)}"
    window.update()

    selection = []
    for wall in faulty_walls:
        selection.append({'elementId': wall['elementId']})
    
    aclib.RunTapirCommand('HighlightElements', {'elements': selection, 'highlightedColors': [[255, 0, 0, 255] for _ in selection], 'wireframe3D': False, 'nonHighlightedColor': [200, 200, 200, 200]})

def on_closing():
    aclib.RunTapirCommand('HighlightElements', {'elements': [], 'highlightedColors': []})
    window.destroy()


# initialize TKinter window
window = tk.Tk()
window.title('Non-orthogonal wall finder')

window.minsize(300, 210)

window.grid_columnconfigure([0], minsize = 5)
window.grid_columnconfigure([2], weight = 1)

window.grid_rowconfigure([0, 2], minsize = 5)
window.grid_rowconfigure([4], minsize = 5, weight = 1)

# initialize Refresh button
button_refresh = tk.Button(master=window, text="Refresh", command=command_refresh, padx=5, pady=5)
button_refresh.grid(row=4, column=1, sticky="nsew")

# initialize Current story checkbox
bool_current_story = tk.BooleanVar()
checkbox_current_story = tk.Checkbutton(window, text = 'Current story only', variable = bool_current_story, padx=5, pady=5)
checkbox_current_story.grid(row=1, column=1,  sticky="nsew")
checkbox_current_story.select()

# initialize strict mode checkbox
strict_mode = tk.BooleanVar()
checkbox_strict_mode = tk.Checkbutton(window, text='Strict Mode', variable=strict_mode, padx=5, pady=5)
checkbox_strict_mode.grid(row=2, column=1, sticky="nsew")

# initialize use viewport rotation checkbox
use_view_rot = tk.BooleanVar()
checkbox_use_view_rot = tk.Checkbutton(window, text='Use current floorplan rotation', variable=use_view_rot, padx=5, pady=5)
checkbox_use_view_rot.grid(row=3, column=1, sticky="nsew")

# initialize info message
msg = tk.Message(window, text="", width=280)
msg.grid(row=0, column=1, padx=5, pady=10, sticky="nsew")

# set what happens when app is closed
window.protocol("WM_DELETE_WINDOW", on_closing)

# keep the window on top at all times
window.lift()
window.attributes('-topmost', True)

# run check once at startup
command_refresh()

window.mainloop()
