import aclib

# Both matrices are 3x4, row by row (12 numbers):
# x' = m[0]*x + m[1]*y + m[2]*z + m[3]
# y' = m[4]*x + m[5]*y + m[6]*z + m[7]
# z' = m[8]*x + m[9]*y + m[10]*z + m[11]
transformation = aclib.RunTapirCommand ('GetSurveyPointTransformation')

def Transform (m, point):
    x, y, z = point['x'], point['y'], point['z']
    return {
        'x': m[0] * x + m[1] * y + m[2] * z + m[3],
        'y': m[4] * x + m[5] * y + m[6] * z + m[7],
        'z': m[8] * x + m[9] * y + m[10] * z + m[11]
    }

# A point in Project Origin coordinates, transformed into Survey Point coordinates
projectPoint = {'x': 10.0, 'y': 5.0, 'z': 0.0}
surveyPoint = Transform (transformation['projectToSurvey'], projectPoint)
print ('Project point in survey coordinates:')
print (aclib.JsonDumpDictionary (surveyPoint))

# ...and back, with the inverse matrix
if 'surveyToProject' in transformation:
    print ('Transformed back into project coordinates:')
    print (aclib.JsonDumpDictionary (Transform (transformation['surveyToProject'], surveyPoint)))
