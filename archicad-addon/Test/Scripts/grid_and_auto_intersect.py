import aclib

# Auto-Intersection (Options > Auto-Intersection) decides whether the walls and beams
# a script creates get joined at their intersections. It is a setting of the running
# Archicad, not of the project, so a script that needs a known result sets the state
# it needs and restores the previous one afterwards.
originalAutoIntersect = aclib.RunTapirCommand ('GetAutoIntersect', debug = False)['isOn']

for isOn in [True, False]:
    aclib.RunTapirCommand ('SetAutoIntersect', {'isOn': isOn})
    autoIntersect = aclib.RunTapirCommand ('GetAutoIntersect', debug = False)
    print ('Auto-Intersection is now {}'.format ('on' if autoIntersect['isOn'] else 'off'))

aclib.RunTapirCommand ('SetAutoIntersect', {'isOn': originalAutoIntersect}, debug = False)
