'''
Non-orthogonal wall finder script
by Daniel Alexander Kovacs for tapir
https://kadsolutions.co.uk/
'''

import aclib
import math
import tkinter as tk

def command_refresh():

    filter_list = ['IsVisibleByLayer']
    panel_tag = '# of non-ortho walls in project:'

    if bool_current_story.get():
        filter_list.append('OnActualFloor')
        panel_tag = '# of non-ortho walls on current floor:'


    wall_IDs = aclib.RunTapirCommand('GetElementsByType', {'elementType': 'Wall', 'filters': filter_list})
    wall_details = aclib.RunTapirCommand('GetDetailsOfElements', wall_IDs)

    wall_angles = []

    for i in range(len(wall_IDs["elements"])):
        wall_angles.append(wall_IDs["elements"][i])
        x = wall_details['detailsOfElements'][i]['details']['endCoordinate']['x'] - wall_details['detailsOfElements'][i]['details']['begCoordinate']['x']
        y = wall_details['detailsOfElements'][i]['details']['endCoordinate']['y'] - wall_details['detailsOfElements'][i]['details']['begCoordinate']['y']
        angle = math.degrees(math.atan2(y, x))

        wall_angles[i]['angle'] = angle

    faulty_walls = []

    for wall in wall_angles:
        if (wall['angle'] % 45) > 0:
            faulty_walls.append(wall)

    panel_content = f'{panel_tag} {len(faulty_walls)}\n'

    panel.insert(index = 1.0, chars = panel_content)

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

window.minsize(550, 130)

window.grid_columnconfigure([0], minsize = 5)
window.grid_columnconfigure([2], weight = 1)

window.grid_rowconfigure([0, 2], minsize = 5)
window.grid_rowconfigure([4], minsize = 5, weight = 1)

# initialize Refresh button
button_refresh = tk.Button(master = window, text = "Refresh", command = command_refresh, padx=5, pady=5)
button_refresh.grid(row = 1, column = 1, sticky = "nsew")

# initialize Current story checkbox
bool_current_story = tk.BooleanVar()
checkbox_current_story = tk.Checkbutton(window, text = 'Current story only', variable = bool_current_story, padx=5, pady=5)
checkbox_current_story.grid(row = 2, column = 1, sticky = "nsew")
checkbox_current_story.select()

# initialize info panel
panel = tk.Text(master = window, state = 'normal', width = 40, height= 5)
panel.grid(row = 0, column = 2, sticky = "nsew", rowspan = 5, padx = 5, pady = 5)

panel_content = 'Click "Refresh" to highlight non-orthogonal walls\n'
panel.insert(index = 1.0, chars = panel_content)

# set what happens when app is closed
window.protocol("WM_DELETE_WINDOW", on_closing)

# keep the window on top at all times
window.attributes('-topmost', True)

# run check once at startup
command_refresh()

window.mainloop()