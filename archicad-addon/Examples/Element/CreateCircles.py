"""Creates a circle with a 2 m radius with CreateCircles."""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('CreateCircles', {'circlesData': [{
    'origin': {'x': 0.0, 'y': 0.0},
    'radius': 2.0
}]}))
