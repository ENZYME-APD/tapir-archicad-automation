import aclib

for type in [
            "Layer",
            "Line",
            "Fill",
            "Composite",
            "Surface",
            "LayerCombination",
            "ZoneCategory",
            "Profile",
            "PenTable",
            "MEPSystem",
            "OperationProfile",
            "BuildingMaterial"
            ]:
    aclib.RunTapirCommand ('GetAttributesByType', {'attributeType': type})