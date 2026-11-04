"""Changes the color and width of pen 1 of the Pen Table of the model views with ModifyPens, then restores it.

In a Teamwork project the PenTables lockable object set must be reserved first.
"""
import aclib

penTables = aclib.RunTapirCommand ('GetAttributesByType', {'attributeType': 'PenTable'})['attributes']
penTables = aclib.RunTapirCommand ('GetPenTables', {
    'attributeIds': [{'attributeId': penTable['attributeId']} for penTable in penTables],
    'fields': ['isActiveForModel', 'pens']
})['penTables']
modelPenTable = next ((penTable for penTable in penTables if penTable.get ('isActiveForModel')), None)
if modelPenTable is None:
    raise SystemExit ('No Pen Table is active for the model views.')
oldPen = next (pen for pen in modelPenTable['pens'] if pen['index'] == 1)

aclib.PrintJson (aclib.RunTapirCommand ('ModifyPens', {
    'pens': [{'index': 1, 'color': {'red': 1.0, 'green': 0.0, 'blue': 0.0}, 'width': 0.5}],
    'target': 'model'
}))

aclib.PrintJson (aclib.RunTapirCommand ('ModifyPens', {
    'pens': [{'index': 1, 'color': oldPen['color'], 'width': oldPen['width']}],
    'target': 'model'
}))
