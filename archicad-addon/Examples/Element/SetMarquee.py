"""Draws a 10 x 10 m rectangle marquee around the origin with SetMarquee, then reads it back.

The original marquee (or no marquee) is restored at the end.
"""
import aclib

original = aclib.RunTapirCommand ('GetMarquee', {})

aclib.PrintJson (aclib.RunTapirCommand ('SetMarquee', {
    'box': {'xMin': -5.0, 'yMin': -5.0, 'xMax': 5.0, 'yMax': 5.0},
    'multiStory': False
}))
aclib.RunTapirCommand ('GetMarquee', {})

# Restore the original marquee; without polygon and box SetMarquee removes it.
if original['type'] == 'none':
    aclib.RunTapirCommand ('SetMarquee', {})
elif original['type'] == 'polygon':
    aclib.RunTapirCommand ('SetMarquee', {'polygon': original['polygon'], 'multiStory': original['multiStory']})
else:
    aclib.RunTapirCommand ('SetMarquee', {
        'box': original['box'],
        'rotationAngle': original.get ('rotationAngle', 0.0),
        'multiStory': original['multiStory']
    })
