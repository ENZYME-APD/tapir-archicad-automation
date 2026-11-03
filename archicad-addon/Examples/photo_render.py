import os
import tempfile
import aclib

# Lists the rendering scenes of the project (the Scene list of the PhotoRendering
# Settings palette) with the current one, makes one of them current, then renders the
# 3D model with a scene into an image file, as Document > Creative Imaging > PhotoRender
# Projection does. The size, resolution and quality of the image come from the scene;
# a width, height or resolution given to PhotoRender is used for that render only, the
# scene keeps its own settings. The model is rendered as the 3D window shows it, so set
# up the 3D window with the view to render first. The format comes from the extension
# when it is not given.

scenes = aclib.RunTapirCommand ('GetRenderingScenes')
sceneNames = scenes['sceneNames']

if len (sceneNames) > 0:
    aclib.RunTapirCommand ('SetCurrentRenderingScene', {
        'sceneName': sceneNames[0]
    })

renderParameters = {
    'imageFilePath': os.path.join (tempfile.gettempdir (), 'tapir_render.png'),
    'format': 'png',
    # A small image, rendered at this size instead of the size of the scene.
    'width': 320,
    'height': 240,
    'resolution': 72
}
if len (sceneNames) > 0:
    # The scene given here is made current first, and stays current afterwards.
    renderParameters['sceneName'] = sceneNames[-1]

aclib.RunTapirCommand ('PhotoRender', renderParameters)

# The scene rendered with is the current one now, so it comes first.
aclib.RunTapirCommand ('GetRenderingScenes')
