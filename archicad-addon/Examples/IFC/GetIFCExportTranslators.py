"""Lists the IFC export translators of the project with GetIFCExportTranslators, the preview translator first."""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetIFCExportTranslators'))
