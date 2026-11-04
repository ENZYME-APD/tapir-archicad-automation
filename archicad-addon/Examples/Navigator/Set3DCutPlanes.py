"""Sets one 3D cut plane with Set3DCutPlanes, then restores the original 3D cut.

An empty cutPlanes array switches the 3D cut off.
"""
import aclib

original = aclib.RunTapirCommand ('Get3DCutPlanes')

aclib.PrintJson (aclib.RunTapirCommand ('Set3DCutPlanes', {
    'cutPlanes': [{'pa': 1.0, 'pb': 0.0, 'pc': 0.0, 'pd': 10.0}]
}))

aclib.PrintJson (aclib.RunTapirCommand ('Set3DCutPlanes', {
    'cutPlanes': original['cutPlanes'] if original['isCutPlanes'] else []
}))
