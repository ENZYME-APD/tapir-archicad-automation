"""Creates an add-on viewpoint root with a leaf node in it, using CreateAddOnViewpoints.

The new items stay in the Project Map. Roots can't be created in a Teamwork project.
Requires Archicad 27 or later.
"""
import base64
import aclib

root = aclib.RunTapirCommand ('CreateAddOnViewpoints', {
    'viewpoints': [{'itemType': 'Root', 'displayId': 'TAPIR', 'displayName': 'Tapir Example Root'}]
})['viewpoints'][0]
aclib.PrintJson (root)

aclib.PrintJson (aclib.RunTapirCommand ('CreateAddOnViewpoints', {
    'viewpoints': [{
        'itemType': 'Leaf',
        'parentViewpointId': root['viewpointId'],
        'displayId': 'L1',
        'displayName': 'Tapir Example Leaf',
        'data': base64.b64encode (b'created by Tapir').decode ('ascii')
    }]
}))
