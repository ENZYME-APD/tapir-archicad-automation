"""Gets the layer that new Walls, Slabs and Columns are placed on with GetLayerOfElementDefaults."""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetLayerOfElementDefaults', {'elementTypes': ['Wall', 'Slab', 'Column']}))
