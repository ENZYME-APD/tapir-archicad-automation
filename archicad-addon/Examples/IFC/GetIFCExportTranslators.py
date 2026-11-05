"""Lists the IFC export translators of the project with GetIFCExportTranslators, in the order of Archicad's translator list (preview marks the first one)."""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetIFCExportTranslators'))
