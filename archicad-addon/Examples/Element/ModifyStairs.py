"""Creates a straight stair, then changes its width, its walking line, its tread thickness, its structure and its floor plan display with ModifyStairs.

Makes the flights monolithic with a smooth underside, numbers the steps on the risers, shows the
stair on its home story only in the floor plan without the walking line, with the full symbol and
no break mark on its bottom story, a break line 0.1 m wide at the floor plan cut plane and pen 7 for
its lines, and on its home story and one story up in the reflected ceiling plan without the
structure. Draws the hidden lines of the structure under the stair with pen 3 in the floor plan, and
the contour of the hidden part of the monolithic structure with pen 3 too, and hides the hidden
monolithic structure on the stories above the stair in the floor plan. Reads the stair back with
GetDetailsOfElements, then deletes the stair it created.
"""
import aclib

stairs = aclib.RunTapirCommand ('CreateStairs', {'stairsData': [{
    'baseLinePoints': [{'x': 0.0, 'y': 0.0}, {'x': 4.0, 'y': 0.0}],
    'zCoordinate': 0.0,
    'totalHeight': 3.0,
    'flightWidth': 1.2
}]})['elements']

aclib.PrintJson (aclib.RunTapirCommand ('ModifyStairs', {'stairsWithDetails': [{
    'elementId': stairs[0]['elementId'],
    'flightWidth': 1.4,
    'walkingLinePosition': 'Center',
    'numberingType': 'Risers',
    'treads': [{'role': 'Run', 'visible': True, 'thickness': 0.04}],
    'structures': [{'role': 'Run', 'side': 'UnderSide', 'visible': True, 'structureType': 'Monolith', 'bottomType': 'Smooth', 'thickness': 0.18}],
    'stairDisplayType': 'Custom',
    'floorPlanLayout': {
        'storyVisibility': 'HomeStoryOnly', 'showWalkingLine': False, 'showStructure': True,
        'gridTypeBottom': 'NoBreaklineFull',
        'breakLinePositionType': 'FloorPlanCutPlaneHeight', 'breakLineDistance': 0.1,
        'overridePen': True, 'pen': 7
    },
    'reflectedCeilingPlanLayout': {'storyVisibility': 'HomeAndOneStoryUp', 'showWalkingLine': True, 'showStructure': False},
    'structureSymbols': [{'structureType': 'UnderStructure', 'view': 'FloorPlan', 'hiddenLinePen': 3}],
    'monolithStructureSymbols': [{'view': 'FloorPlan', 'part': 'Hidden', 'contourLinePen': 3}],
    'structureVisibilityOnStories': [{'view': 'FloorPlan', 'stories': [
        {'relevantStories': 'AboveRelevant', 'structures': [{'part': 'Hidden', 'structure': 'Monolith', 'visible': False}]}
    ]}]
}]}))

aclib.PrintJson (aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': stairs}))

aclib.RunTapirCommand ('DeleteElements', {'elements': stairs})
