"""Creates a Building Material with its cut fill, cut surface and physical properties with CreateBuildingMaterials.

Overwrites the Building Material of the same name when the script is run again.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('CreateBuildingMaterials', {
    'buildingMaterialDataArray': [{
        'name': 'Tapir Example Building Material',
        'id': 'TAP-01',
        'manufacturer': 'Tapir',
        'description': 'Created by a Tapir example',
        'connPriority': 50,
        'cutFillIndex': 1,
        'cutFillPen': 2,
        'cutFillBackgroundPen': 0,
        'cutSurfaceIndex': 1,
        'thermalConductivity': 0.8,
        'density': 1800.0,
        'heatCapacity': 900.0
    }],
    'overwriteExisting': True
}))
