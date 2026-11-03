import aclib

# SolveStairBaseline runs the stair solver on the same input as CreateStairs, without
# creating anything, so a baseline can be checked (and replaced by the solved one) first.
stairsData = [
    {
        'baseLinePoints': [
            {'x': 0.0, 'y': 0.0},
            {'x': 0.0, 'y': 5.0}
        ],
        'zCoordinate': 0.0,
        'totalHeight': 3.0,
        'flightWidth': 1.2
    },
    {
        'baseLinePoints': [
            {'x': 10.0, 'y': 0.0},
            {'x': 10.0, 'y': 3.0},
            {'x': 13.0, 'y': 3.0}
        ],
        'zCoordinate': 0.0,
        'totalHeight': 3.0,
        'flightWidth': 1.2
    }
]

solveResult = aclib.RunTapirCommand (
    'SolveStairBaseline', {
        'stairsData': stairsData
    })

for data, solution in zip (stairsData, solveResult['solutions']):
    if 'error' in solution:
        continue
    if solution['solution'] == 'NotSolved':
        print ('The baseline could not be solved, the stair is not created.')
        continue
    # Create the stair from the solved baseline
    data['baseLinePoints'] = solution['baseLinePoints']
    aclib.RunTapirCommand (
        'CreateStairs', {
            'stairsData': [data]
        })
