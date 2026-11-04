"""Reads the marquee of the current window with GetMarquee: its shape, whether it is multi-story, and its outline.

The type is 'none' when there is no marquee.
"""
import aclib

marquee = aclib.RunTapirCommand ('GetMarquee', {})
print ('Marquee type: {}'.format (marquee['type']))
