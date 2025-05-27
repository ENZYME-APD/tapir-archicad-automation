'''
Non-orthogonal wall finder script
by Daniel Alexander Kovacs for tapir
https://kadsolutions.co.uk/
'''

import aclib
import math
import tkinter as tk

default_tolerance = 2.0

def command_refresh():
    filter_list = ['IsVisibleByLayer']
    panel_tag = '# of non-ortho walls in project:'

    if bool_current_story.get():
        filter_list.append('OnActualFloor')
        panel_tag = '# of non-ortho walls on current floor:'

    try:
        tolerance = float(entry_tolerance.get())
    except Exception:
        tolerance = default_tolerance

    wall_IDs = aclib.RunTapirCommand('GetElementsByType', {'elementType': 'Wall', 'filters': filter_list})
    wall_details = aclib.RunTapirCommand('GetDetailsOfElements', wall_IDs)

    wall_angles = []
    details_list = wall_details.get('detailsOfElements', [])
    elements_list = wall_IDs.get('elements', [])

    for elem, details in zip(elements_list, details_list):
        coords = details['details']
        x = coords['endCoordinate']['x'] - coords['begCoordinate']['x']
        y = coords['endCoordinate']['y'] - coords['begCoordinate']['y']
        angle = math.degrees(math.atan2(y, x))
        elem['angle'] = angle
        wall_angles.append(elem)

    faulty_walls = [
        wall for wall in wall_angles
        if min(abs((wall['angle'] % 45)), abs(45 - (wall['angle'] % 45))) > tolerance
    ]

    panel_content = f'{panel_tag} {len(faulty_walls)} (tolerance {tolerance})\n'

    panel.insert(index = 1.0, chars = panel_content)

    selection = [{'elementId': wall['elementId']} for wall in faulty_walls]
    aclib.RunTapirCommand(
        'HighlightElements',
        {
            'elements': selection,
            'highlightedColors': [[255, 0, 0, 255]] * len(selection),
            'wireframe3D': False,
            'nonHighlightedColor': [200, 200, 200, 200]
        }
    )

def on_closing():
    aclib.RunTapirCommand('HighlightElements', {'elements': [], 'highlightedColors': []})
    window.destroy()

def validate_float(P):
    if P == "" or P == "." or P == "-":
        return True
    try:
        float(P)
        return True
    except ValueError:
        return False

# initialize TKinter window

window = tk.Tk()
window.title('Non-orthogonal wall finder')

window.minsize(800, 130)

window.grid_columnconfigure([0], minsize = 5)
window.grid_columnconfigure([3], weight = 1)

window.grid_rowconfigure([0], minsize = 5)
window.grid_rowconfigure([4], weight = 1)

# initialize Refresh button
button_refresh = tk.Button(master = window, text = "Refresh", command = command_refresh, padx=5, pady=5)
button_refresh.grid(row = 1, column = 1, columnspan = 2, sticky = "ew")

# initialize Current story checkbox
bool_current_story = tk.BooleanVar()
checkbox_current_story = tk.Checkbutton(window, text = 'Current story only', variable = bool_current_story, padx=5, pady=5)
checkbox_current_story.grid(row = 2, column = 1, columnspan = 2, sticky = "ew")
checkbox_current_story.select()

# initialize info panel
panel = tk.Text(master = window, state = 'normal', width = 40, height= 5)
panel.grid(row = 0, column = 3, sticky = "nsew", rowspan = 5, columnspan = 5, padx = 5, pady = 5)

panel_content = 'Click "Refresh" to highlight non-orthogonal walls\n'
panel.insert(index = 1.0, chars = panel_content)

# Add angle tolerance label and entry
label_tolerance = tk.Label(window, text="Angle tolerance (deg):", padx=5, pady=5)
label_tolerance.grid(row=3, column=1, sticky="ew")

vcmd = (window.register(validate_float), "%P")
entry_tolerance = tk.Entry(window, validate="key", validatecommand=vcmd)
entry_tolerance.grid(row=3, column=2, sticky="ew")
entry_tolerance.insert(0, str(default_tolerance))

# set what happens when app is closed
window.protocol("WM_DELETE_WINDOW", on_closing)

# keep the window on top at all times
window.attributes('-topmost', True)

# run check once at startup
command_refresh()

window.mainloop()