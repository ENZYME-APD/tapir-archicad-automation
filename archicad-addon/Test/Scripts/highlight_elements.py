import aclib
import time

allElements = aclib.RunCommand ('API.GetAllElements', {})

commandName = 'HighlightElements'
commandParameters = {
    'elements' : allElements['elements'],
    'highlightedColors' : [[(i*30) % 255, 50, 50, 255] for i in range(len(allElements['elements']))],
    'wireframe3D' : True,
    'nonHighlightedColor' : [0, 0, 255, 128]
}

response = aclib.RunTapirCommand (commandName, commandParameters)

time.sleep (5)

# Clear highlight
response = aclib.RunTapirCommand ('HighlightElements', {
    'elements' : [],
    'highlightedColors' : [],
})