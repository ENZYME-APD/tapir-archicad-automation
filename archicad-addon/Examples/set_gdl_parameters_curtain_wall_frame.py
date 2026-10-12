import aclib

# On a curtain wall frame, A, B, ac_frameWidth and ac_frameDepth mirror the frame's contour
# (frameContour in GetDetailsOfElements), which Archicad derives from the frame's own settings:
# a value written through the GDL parameters is overwritten from the contour, so
# SetGDLParametersOfElements rejects them instead of reporting a change that did not happen.
# The other parameters of a frame are set as on any element. A frame without a library part
# (an invisible Boundary frame, for example) has no GDL parameters at all.

def SetGDLParameters (frame, gdlParameters):
    return aclib.RunTapirCommand ('SetGDLParametersOfElements', {
        'elementsWithGDLParameters': [{ 'elementId': frame['elementId'], 'gdlParameters': gdlParameters }]
    }, debug = False)['executionResults'][0]

curtainWalls = aclib.RunTapirCommand ('GetElementsByType', { 'elementType': 'CurtainWall' }, debug = False)['elements']
subelementsOfCurtainWalls = aclib.RunTapirCommand ('GetSubelementsOfHierarchicalElements', { 'elements': curtainWalls }, debug = False)['subelements']
frames = [frame for subelements in subelementsOfCurtainWalls for frame in subelements['cWallFrames']]
print ('Project contains {} Curtain Wall Frame(s)'.format (len (frames)))

gdlParametersOfFrames = aclib.RunTapirCommand ('GetGDLParametersOfElements', { 'elements': frames }, debug = False)['gdlParametersOfElements']
framesWithParameters = [(frame, gdlParameters['parameters']) for frame, gdlParameters in zip (frames, gdlParametersOfFrames) if 'parameters' in gdlParameters]
framesWithoutParameters = [frame for frame, gdlParameters in zip (frames, gdlParametersOfFrames) if 'parameters' not in gdlParameters]
print ('{} of them have GDL parameters'.format (len (framesWithParameters)))

if framesWithParameters:
    frame, parameters = framesWithParameters[0]
    parametersByName = { parameter['name']: parameter for parameter in parameters }

    # The mirrored size parameters are rejected, whether addressed by name or by index.
    for name in ('A', 'B', 'ac_frameWidth', 'ac_frameDepth'):
        if name in parametersByName:
            result = SetGDLParameters (frame, [{ 'name': name, 'value': parametersByName[name]['value'] + 0.1 }])
            print ('Setting {} by name: {}'.format (name, result))
    if 'A' in parametersByName:
        result = SetGDLParameters (frame, [{ 'index': parametersByName['A']['index'], 'value': parametersByName['A']['value'] + 0.1 }])
        print ('Setting A by index: {}'.format (result))

    # Any other parameter, here a pen written back with its own value, is set as on any element.
    penParameter = next ((parameter for parameter in parameters
                          if parameter['type'] == 'PenColor' and 'dimension1' not in parameter and not parameter['isLocked']), None)
    if penParameter is not None:
        result = SetGDLParameters (frame, [{ 'name': penParameter['name'], 'value': penParameter['value'] }])
        print ('Setting {}: {}'.format (penParameter['name'], result))

if framesWithoutParameters:
    result = SetGDLParameters (framesWithoutParameters[0], [{ 'name': 'A', 'value': 0.1 }])
    print ('Setting A of a frame without library part: {}'.format (result))
