/* Deutsch: the API limits page (limits/index.html). The Evidence column (headers and function names)
   stays English. See ../README.md. */
(window.TAPIR_I18N = window.TAPIR_I18N || []).push ({
    'What the Archicad API can\'t do | Tapir': 'Was die Archicad-API nicht kann | Tapir',
    'What people ask for that Graphisoft\'s Archicad API does not offer, such as undo or the clipboard, each with what to do instead and the DevKit evidence.':
        'Was Nutzer wünschen, die Archicad-API von Graphisoft aber nicht bietet, etwa Rückgängig oder die Zwischenablage, jeweils mit dem, was Sie stattdessen tun können, und dem Nachweis im DevKit.',
    '<a href="../">Tapir</a> / <a href="../docs/">Docs</a> / API limits': '<a href="../">Tapir</a> / <a href="../docs/">Doku</a> / API-Grenzen',
    'What the Archicad API <em>can\'t do.</em>': 'Was die Archicad-API <em>nicht kann.</em>',
    'Tapir uses the Archicad API that Graphisoft publishes for add-ons, and covers all of it that can be a JSON command. What that API does not offer, Tapir cannot offer either. This page lists what people ask for that is not in it, with what to do instead.':
        'Tapir nutzt die Archicad-API, die Graphisoft für Add-Ons veröffentlicht, und deckt alles davon ab, was ein JSON-Befehl sein kann. Was diese API nicht bietet, kann auch Tapir nicht bieten. Diese Seite listet, was gewünscht wird, aber darin fehlt, und was Sie stattdessen tun können.',

    // The list
    'The <em>list.</em>': 'Die <em>Liste.</em>',
    'It is also what Tapir would like the Archicad API to offer, and the list Tapir sends to Graphisoft. Every row was checked in the headers of the Archicad API DevKits of Archicad 25 to 30, the versions Tapir is built for; the Evidence column names the headers and functions.':
        'Sie ist zugleich das, was Tapir sich von der Archicad-API wünscht, und die Liste, die Tapir an Graphisoft schickt. Jede Zeile wurde in den Headern der DevKits der Archicad-API von Archicad 25 bis 30 geprüft, den Versionen, für die Tapir gebaut wird; die Spalte Nachweis nennt die Header und Funktionen.',
    'What people want': 'Was gewünscht wird',
    'Status': 'Status',
    'What to do instead': 'Was Sie stattdessen tun können',
    'Evidence': 'Nachweis',
    'Issue': 'Issue',
    'Not possible': 'Nicht möglich',
    'Partly possible': 'Teilweise möglich',
    'Workaround': 'Ausweg',
    'Nothing yet': 'Noch nichts',
    'Not documented': 'Nicht dokumentiert',

    // Elements
    'Elements': 'Elemente',
    'Explode an element into its parts, as Edit &gt; Reshape &gt; Explode into Current View does':
        'Ein Element in seine Teile auflösen, wie es Edit &gt; Reshape &gt; Explode into Current View tut',
    'A camera with a name of its own, or an axonometric camera': 'Eine Kamera mit eigenem Namen oder eine axonometrische Kamera',
    'Name the Camera Set the camera is in. For an axonometric view, set the 3D window with <a href="../archicad-addon/#Set3DProjectionSettings">Set3DProjectionSettings</a>.':
        'Benennen Sie das Camera Set, in dem die Kamera liegt. Für eine axonometrische Ansicht stellen Sie das 3D-Fenster mit <a href="../archicad-addon/#Set3DProjectionSettings">Set3DProjectionSettings</a> ein.',
    'Remove one direction of a trim between two roofs or shells that trim each other':
        'Eine Richtung eines Trimms zwischen zwei Dächern oder Schalen entfernen, die sich gegenseitig trimmen',
    'Remove the trim with <a href="../archicad-addon/#RemoveElementTrims">RemoveElementTrims</a>, which removes both directions, then trim the other direction again with <a href="../archicad-addon/#TrimElements">TrimElements</a>.':
        'Entfernen Sie den Trimm mit <a href="../archicad-addon/#RemoveElementTrims">RemoveElementTrims</a>, das beide Richtungen entfernt, und trimmen Sie die andere Richtung danach erneut mit <a href="../archicad-addon/#TrimElements">TrimElements</a>.',
    'Read which slab or stair edge a railing is linked to, or link a railing to one':
        'Auslesen, mit welcher Decken- oder Treppenkante ein Geländer verknüpft ist, oder ein Geländer mit einer verknüpfen',
    'Turn off the automatic story visibility of a railing, so the stories it is shown on can be set':
        'Die automatische Geschosssichtbarkeit eines Geländers ausschalten, damit sich die Geschosse festlegen lassen, auf denen es gezeigt wird',
    'Set the Floor Plan Display, Show Projection, uncut and overhead lines or the cut line type of a mesh':
        'Floor Plan Display, Show Projection, die ungeschnittenen und darüberliegenden Linien oder den Linientyp des Schnitts eines Netzes festlegen',
    'Read or set the floor plan symbol of a railing: its break mark, which symbol components it shows, their 2D attributes and the symbol overrides':
        'Das Grundrisssymbol eines Geländers lesen oder einstellen: seine Bruchlinie, welche Symbolkomponenten es zeigt, deren 2D-Attribute und die Symbolüberschreibungen',
    'Read the slab\'s outline with <a href="../archicad-addon/#GetDetailsOfElements">GetDetailsOfElements</a> and create a static railing along the edge with <a href="../archicad-addon/#CreateRailings">CreateRailings</a>. When the slab changes, delete the railing and create it again.':
        'Lesen Sie den Umriss der Decke mit <a href="../archicad-addon/#GetDetailsOfElements">GetDetailsOfElements</a> aus und erstellen Sie mit <a href="../archicad-addon/#CreateRailings">CreateRailings</a> ein statisches Geländer entlang der Kante. Wenn sich die Decke ändert, löschen Sie das Geländer und erstellen Sie es neu.',
    'Move a window or door into another wall': 'Ein Fenster oder eine Tür in eine andere Wand verschieben',
    'Delete it and create it in the other wall with <a href="../archicad-addon/#CreateWindows">CreateWindows</a> or <a href="../archicad-addon/#CreateDoors">CreateDoors</a>; a favorite (<code>favoriteName</code>) gives it the same settings.':
        'Löschen Sie das Element und erstellen Sie es mit <a href="../archicad-addon/#CreateWindows">CreateWindows</a> oder <a href="../archicad-addon/#CreateDoors">CreateDoors</a> in der anderen Wand neu; ein Favorit (<code>favoriteName</code>) gibt ihm dieselben Einstellungen.',
    'Move a skylight onto another roof or shell': 'Ein Dachfenster auf ein anderes Dach oder eine andere Schale verschieben',
    'Delete it and create it on the other roof or shell with <a href="../archicad-addon/#CreateSkylights">CreateSkylights</a>.':
        'Löschen Sie es und erstellen Sie es mit <a href="../archicad-addon/#CreateSkylights">CreateSkylights</a> auf dem anderen Dach oder der anderen Schale neu.',
    'Turn an automatic zone into a manual one': 'Eine automatische Zone in eine manuelle umwandeln',
    'Delete the zone and create it again as a manual zone, from its polygon, with <a href="../archicad-addon/#CreateZones">CreateZones</a>.':
        'Löschen Sie die Zone und erstellen Sie sie mit <a href="../archicad-addon/#CreateZones">CreateZones</a> aus ihrem Polygon als manuelle Zone neu.',
    'Change the height of a slanted curtain wall': 'Die Höhe einer geneigten Vorhangfassade ändern',
    'Create it again with <a href="../archicad-addon/#CreateCurtainWalls">CreateCurtainWalls</a>, with its <code>angle</code> and the new <code>height</code>.':
        'Erstellen Sie sie mit <a href="../archicad-addon/#CreateCurtainWalls">CreateCurtainWalls</a> neu, mit ihrem <code>angle</code> und der neuen <code>height</code>.',
    'Move the lines or the arc of an angle dimension': 'Die Linien oder den Bogen einer Winkelbemaßung verschieben',
    'Delete it and create it again with <a href="../archicad-addon/#CreateAngleDimensions">CreateAngleDimensions</a>.':
        'Löschen Sie sie und erstellen Sie sie mit <a href="../archicad-addon/#CreateAngleDimensions">CreateAngleDimensions</a> neu.',
    'Set the position or the offset of a stair\'s walking line': 'Die Lage oder den Versatz der Lauflinie einer Treppe festlegen',
    'Anchor the sill or header of a window or door to the project zero or to a reference level':
        'Die Brüstung oder den Sturz eines Fensters oder einer Tür an den Projektnullpunkt oder an eine Bezugsebene binden',
    'Anchor it to a story with <code>verticalLink</code> <code>SillToStory</code> or <code>HeaderToStory</code> in <a href="../archicad-addon/#ModifyWindows">ModifyWindows</a> or <a href="../archicad-addon/#ModifyDoors">ModifyDoors</a>, and work out the height from the story level (<a href="../archicad-addon/#GetStories">GetStories</a>).':
        'Binden Sie sie mit <code>verticalLink</code> <code>SillToStory</code> oder <code>HeaderToStory</code> in <a href="../archicad-addon/#ModifyWindows">ModifyWindows</a> oder <a href="../archicad-addon/#ModifyDoors">ModifyDoors</a> an ein Geschoss und berechnen Sie die Höhe aus dem Niveau des Geschosses (<a href="../archicad-addon/#GetStories">GetStories</a>).',
    'Read or set the joint type (Vertical Cut and the others) and the dx offset of a stair structure':
        'Den Verbindungstyp (Vertical Cut und die anderen) und den dx-Versatz einer Treppenstruktur lesen oder einstellen',
    'Change the baseline of an existing stair (move a flight end, a landing or a winder) and keep its GUID':
        'Die Basislinie einer bestehenden Treppe ändern (ein Laufende, ein Podest oder eine Wendelung verschieben) und ihre GUID behalten',
    'Recreate the stair on the new baseline with <a href="../archicad-addon/#RecreateStairs">RecreateStairs</a>, which keeps its settings, Element ID, classifications, categories, properties and solid element operations, and returns the old and the new GUID. Its labels are lost.':
        'Erstellen Sie die Treppe mit <a href="../archicad-addon/#RecreateStairs">RecreateStairs</a> auf der neuen Basislinie neu. Der Befehl behält ihre Einstellungen, Element-ID, Klassifizierungen, Kategorien, Eigenschaften und Solid Element Operations und gibt die alte und die neue GUID zurück. Ihre Beschriftungen gehen verloren.',
    'Set the left and right boundaries or the walking line of a stair (a custom outline, or a walking line off the middle of the flight)':
        'Die linke und rechte Begrenzung oder die Lauflinie einer Treppe festlegen (ein eigener Umriss oder eine Lauflinie außerhalb der Laufmitte)',
    'Archicad lays them out from the baseline and the flight width: change the baseline with <a href="../archicad-addon/#RecreateStairs">RecreateStairs</a> or the flight width with <a href="../archicad-addon/#ModifyStairs">ModifyStairs</a>. The walking line stays in the middle of the flight. <a href="../archicad-addon/#GetDetailsOfElements">GetDetailsOfElements</a> reads them.':
        'Archicad legt sie aus der Basislinie und der Laufbreite an: Ändern Sie die Basislinie mit <a href="../archicad-addon/#RecreateStairs">RecreateStairs</a> oder die Laufbreite mit <a href="../archicad-addon/#ModifyStairs">ModifyStairs</a>. Die Lauflinie bleibt in der Mitte des Laufs. <a href="../archicad-addon/#GetDetailsOfElements">GetDetailsOfElements</a> liest sie.',
    'Set a label’s optimised position, “use text pen as uniform pen” of a symbol label, or a perpendicular text direction':
        'Die optimierte Position einer Beschriftung, „use text pen as uniform pen“ einer Symbolbeschriftung oder eine senkrechte Textrichtung einstellen',
    'Set the text direction (parallel, horizontal, vertical or a custom angle) with textWay in <a href="../archicad-addon/#CreateLabels">CreateLabels</a> and <a href="../archicad-addon/#ModifyLabels">ModifyLabels</a>. When the symbol label’s library part has a parameter for the other two, set it with <a href="../archicad-addon/#SetGDLParametersOfElements">SetGDLParametersOfElements</a>; otherwise set them in the Label tool settings or a favorite, and create the labels with favoriteName.':
        'Die Textrichtung (parallel, horizontal, vertikal oder ein eigener Winkel) setzen Sie mit textWay in <a href="../archicad-addon/#CreateLabels">CreateLabels</a> und <a href="../archicad-addon/#ModifyLabels">ModifyLabels</a>. Hat das Bibliothekselement der Symbolbeschriftung einen Parameter für die beiden anderen, setzen Sie ihn mit <a href="../archicad-addon/#SetGDLParametersOfElements">SetGDLParametersOfElements</a>; sonst stellen Sie sie in den Einstellungen des Beschriftungswerkzeugs oder in einem Favoriten ein und erstellen die Beschriftungen mit favoriteName.',
    'Create a radial dimension of an arc, a circle, a curved wall or a curved slab edge':
        'Eine Radiusbemaßung eines Bogens, eines Kreises, einer gebogenen Wand oder einer gebogenen Deckenkante erstellen',
    'Place the radial dimension in Archicad and read it with <a href="../archicad-addon/#GetDetailsOfElements">GetDetailsOfElements</a>, which returns how Archicad stored it. Those readings are what a create command needs.':
        'Platzieren Sie die Radiusbemaßung in Archicad und lesen Sie sie mit <a href="../archicad-addon/#GetDetailsOfElements">GetDetailsOfElements</a> aus, das zurückgibt, wie Archicad sie gespeichert hat. Diese Werte braucht ein Befehl zum Erstellen.',

    // Design options
    'Design options': 'Varianten',
    'Read which design option, or the Main Model, is the default that new elements go to':
        'Auslesen, welche Variante oder ob das Hauptmodell die Vorgabe ist, in die neue Elemente kommen',
    'Create a temporary element, read its design option with <a href="../archicad-addon/#GetDesignOptionForElements">GetDesignOptionForElements</a>, then move it to the Main Model with <a href="../archicad-addon/#MoveElementsToDesignOptions">MoveElementsToDesignOptions</a> (a null GUID as <code>designOptionId</code>) and delete it with <a href="../archicad-addon/#DeleteElements">DeleteElements</a>; an element in an option that is not shown cannot be deleted. Set the default with <a href="../archicad-addon/#SetDefaultDesignOption">SetDefaultDesignOption</a>.':
        'Erstellen Sie ein temporäres Element, lesen Sie seine Variante mit <a href="../archicad-addon/#GetDesignOptionForElements">GetDesignOptionForElements</a> aus, verschieben Sie es mit <a href="../archicad-addon/#MoveElementsToDesignOptions">MoveElementsToDesignOptions</a> (eine Null-GUID als <code>designOptionId</code>) ins Hauptmodell und löschen Sie es dann mit <a href="../archicad-addon/#DeleteElements">DeleteElements</a>; ein Element in einer nicht angezeigten Variante lässt sich nicht löschen. Die Vorgabe setzen Sie mit <a href="../archicad-addon/#SetDefaultDesignOption">SetDefaultDesignOption</a>.',
    'Read or change which design options the current window shows, as the eye icons of the Design Options palette do':
        'Auslesen oder ändern, welche Varianten das aktuelle Fenster zeigt, wie es die Augen-Symbole der Varianten-Palette tun',
    'Work with saved views: read and set the combination a View Map view shows with <a href="../archicad-addon/#GetDesignOptionCombinationOfViews">GetDesignOptionCombinationOfViews</a> and <a href="../archicad-addon/#SetDesignOptionCombinationOfViews">SetDesignOptionCombinationOfViews</a>, and the options of a combination with <a href="../archicad-addon/#SetActiveDesignOptionsInCombinations">SetActiveDesignOptionsInCombinations</a>.':
        'Arbeiten Sie mit gespeicherten Ausschnitten: Lesen und setzen Sie die Kombination, die ein Ausschnitt der Ausschnittmappe zeigt, mit <a href="../archicad-addon/#GetDesignOptionCombinationOfViews">GetDesignOptionCombinationOfViews</a> und <a href="../archicad-addon/#SetDesignOptionCombinationOfViews">SetDesignOptionCombinationOfViews</a>, und die Varianten einer Kombination mit <a href="../archicad-addon/#SetActiveDesignOptionsInCombinations">SetActiveDesignOptionsInCombinations</a>.',
    'Duplicate a design option or a design option set, as the duplicate button of the Design Options palette does':
        'Eine Variante oder ein Variantenset duplizieren, wie es die Schaltfläche Duplizieren der Varianten-Palette tut',
    'Create the set and its options with <a href="../archicad-addon/#CreateDesignOptionSets">CreateDesignOptionSets</a> and <a href="../archicad-addon/#CreateDesignOptions">CreateDesignOptions</a>. For the elements, list them with <a href="../archicad-addon/#GetElementsOfDesignOptions">GetElementsOfDesignOptions</a>, copy them with <a href="../archicad-addon/#CopyElements">CopyElements</a> and move the copies to the new option with <a href="../archicad-addon/#MoveElementsToDesignOptions">MoveElementsToDesignOptions</a>.':
        'Erstellen Sie das Set und seine Varianten mit <a href="../archicad-addon/#CreateDesignOptionSets">CreateDesignOptionSets</a> und <a href="../archicad-addon/#CreateDesignOptions">CreateDesignOptions</a>. Für die Elemente listen Sie sie mit <a href="../archicad-addon/#GetElementsOfDesignOptions">GetElementsOfDesignOptions</a> auf, kopieren sie mit <a href="../archicad-addon/#CopyElements">CopyElements</a> und verschieben die Kopien mit <a href="../archicad-addon/#MoveElementsToDesignOptions">MoveElementsToDesignOptions</a> in die neue Variante.',
    'Lock or colour the passive elements of design options, as the Design Options palette does':
        'Die passiven Elemente von Varianten sperren oder einfärben, wie es die Varianten-Palette tut',

    // Navigator and views
    'Navigator and views': 'Navigator und Ansichten',
    'Add an item to a publisher set': 'Einem Publisher Set ein Element hinzufügen',
    'Add the items in Archicad\'s Organizer, then publish the set with <a href="../archicad-addon/#PublishPublisherSet">PublishPublisherSet</a>.':
        'Fügen Sie die Elemente im Organizer von Archicad hinzu und veröffentlichen Sie das Set dann mit <a href="../archicad-addon/#PublishPublisherSet">PublishPublisherSet</a>.',
    'Rename a publisher set': 'Ein Publisher Set umbenennen',
    'Rename the set in Archicad\'s Organizer. <a href="../archicad-addon/#GetPublisherSets">GetPublisherSets</a> reads the names, and <a href="../archicad-addon/#PublishPublisherSet">PublishPublisherSet</a> publishes a set by its name.':
        'Benennen Sie das Set im Organizer von Archicad um. <a href="../archicad-addon/#GetPublisherSets">GetPublisherSets</a> liest die Namen, und <a href="../archicad-addon/#PublishPublisherSet">PublishPublisherSet</a> veröffentlicht ein Set über seinen Namen.',
    'Change the settings of an existing Layout Book subset (prefix, numbering style, start at, ...)':
        'Die Einstellungen eines bestehenden Teilsets im Planbuch ändern (Präfix, Nummerierungsstil, Beginnen mit, ...)',
    'Read the subset with <a href="../archicad-addon/#GetLayoutSubsetSettings">GetLayoutSubsetSettings</a>, create a subset with the new settings with <a href="../archicad-addon/#CreateLayoutSubset">CreateLayoutSubset</a> and move the layouts into it with <a href="../archicad-addon/#MoveNavigatorItem">MoveNavigatorItem</a>. The old, empty subset is deleted in Archicad.':
        'Lesen Sie das Teilset mit <a href="../archicad-addon/#GetLayoutSubsetSettings">GetLayoutSubsetSettings</a> aus, legen Sie mit <a href="../archicad-addon/#CreateLayoutSubset">CreateLayoutSubset</a> ein Teilset mit den neuen Einstellungen an und verschieben Sie die Layouts mit <a href="../archicad-addon/#MoveNavigatorItem">MoveNavigatorItem</a> hinein. Das alte, leere Teilset löschen Sie in Archicad.',
    'Change, rename or create a stored Model View Options combination':'Eine gespeicherte Kombination der Model View Options ändern, umbenennen oder anlegen',
    'Read the combinations with <a href="../archicad-addon/#GetModelViewOptionDetails">GetModelViewOptionDetails</a>; change the options of the current window with <a href="../archicad-addon/#SetViewOptions">SetViewOptions</a>.':
        'Lesen Sie die Kombinationen mit <a href="../archicad-addon/#GetModelViewOptionDetails">GetModelViewOptionDetails</a>; ändern Sie die Optionen des aktuellen Fensters mit <a href="../archicad-addon/#SetViewOptions">SetViewOptions</a>.',
    'Make a 3D style the current one': 'Einen 3D-Stil zum aktuellen machen',
    'Give a view the 3D style with <a href="../archicad-addon/#SetViewSettings">SetViewSettings</a> (<code>d3styleName</code>) and open that view with <a href="../archicad-addon/#ChangeWindow">ChangeWindow</a> (<code>navigatorItemId</code>, from Archicad 27).':
        'Geben Sie einem Ausschnitt den 3D-Stil mit <a href="../archicad-addon/#SetViewSettings">SetViewSettings</a> (<code>d3styleName</code>) und öffnen Sie diesen Ausschnitt mit <a href="../archicad-addon/#ChangeWindow">ChangeWindow</a> (<code>navigatorItemId</code>, ab Archicad 27).',
    'Read or set the floor plan cut plane or the structural analytical model settings saved in a view':
        'Die Grundriss-Schnittebene oder die Einstellungen des statischen Analysemodells lesen oder einstellen, die in einem Ausschnitt gespeichert sind',
    'Set them in the view\'s View Settings in Archicad. The project\'s floor plan cut plane can be set with <a href="../archicad-addon/#SetProjectPreferences">SetProjectPreferences</a>.':
        'Stellen Sie sie in Archicad in den View Settings des Ausschnitts ein. Die Grundriss-Schnittebene des Projekts lässt sich mit <a href="../archicad-addon/#SetProjectPreferences">SetProjectPreferences</a> einstellen.',
    'Tell a clone folder of the View Map from a normal folder':
        'Einen Klonordner der View Map von einem normalen Ordner unterscheiden',
    'Give the clone folders a name or ID of their own and find them by it in <a href="../archicad-addon/#GetNavigatorItemTree">GetNavigatorItemTree</a>.':
        'Geben Sie den Klonordnern einen eigenen Namen oder eine eigene ID und finden Sie sie daran in <a href="../archicad-addon/#GetNavigatorItemTree">GetNavigatorItemTree</a>.',

    // Project and files
    'Project and files': 'Projekt und Dateien',
    'Undo or redo, as Edit &gt; Undo does': 'Rückgängig machen oder wiederherstellen, wie Edit &gt; Undo',
    'Most Tapir commands change the model in an undo step, so the user can undo them in Archicad; the command reference says which cannot be undone. Save the project with <a href="../archicad-addon/#SaveProject">SaveProject</a> before a script that may need to be taken back.':
        'Die meisten Tapir-Befehle ändern das Modell in einem Rückgängig-Schritt, sodass der Nutzer sie in Archicad rückgängig machen kann; die Befehlsreferenz nennt die, bei denen das nicht geht. Sichern Sie das Projekt mit <a href="../archicad-addon/#SaveProject">SaveProject</a> vor einem Skript, das Sie vielleicht zurücknehmen müssen.',
    'Copy and paste through the clipboard': 'Über die Zwischenablage kopieren und einfügen',
    'In a project, copy 2D elements to another view with <a href="../archicad-addon/#CopyElementsToDatabase">CopyElementsToDatabase</a>. Between projects, save the elements as a module with <a href="../archicad-addon/#SaveAsModuleFile">SaveAsModuleFile</a>, place it in the other project with <a href="../archicad-addon/#CreateHotlinkNodes">CreateHotlinkNodes</a> and <a href="../archicad-addon/#CreateHotlinkInstances">CreateHotlinkInstances</a>, and, to keep plain elements, <a href="../archicad-addon/#BreakHotlinkNodes">BreakHotlinkNodes</a>.':
        'Innerhalb eines Projekts kopieren Sie 2D-Elemente mit <a href="../archicad-addon/#CopyElementsToDatabase">CopyElementsToDatabase</a> in eine andere Ansicht. Zwischen Projekten sichern Sie die Elemente mit <a href="../archicad-addon/#SaveAsModuleFile">SaveAsModuleFile</a> als Modul, platzieren es im anderen Projekt mit <a href="../archicad-addon/#CreateHotlinkNodes">CreateHotlinkNodes</a> und <a href="../archicad-addon/#CreateHotlinkInstances">CreateHotlinkInstances</a> und lösen es, um einfache Elemente zu behalten, mit <a href="../archicad-addon/#BreakHotlinkNodes">BreakHotlinkNodes</a> auf.',
    'Merge a file into the project, as File &gt; Interoperability &gt; Merge does': 'Eine Datei in das Projekt einfügen, wie es File &gt; Interoperability &gt; Merge tut',
    'Place the file as a hotlink with <a href="../archicad-addon/#CreateHotlinkNodes">CreateHotlinkNodes</a> and <a href="../archicad-addon/#CreateHotlinkInstances">CreateHotlinkInstances</a>, then <a href="../archicad-addon/#BreakHotlinkNodes">BreakHotlinkNodes</a>.':
        'Platzieren Sie die Datei als Hotlink mit <a href="../archicad-addon/#CreateHotlinkNodes">CreateHotlinkNodes</a> und <a href="../archicad-addon/#CreateHotlinkInstances">CreateHotlinkInstances</a>, dann <a href="../archicad-addon/#BreakHotlinkNodes">BreakHotlinkNodes</a>.',
    'Delete one offset modifier of a profile': 'Einen einzelnen Versatz-Modifikator eines Profils löschen',
    'Rebuild the profile with <a href="../archicad-addon/#CreateProfiles">CreateProfiles</a>\' <code>replaceSkins</code> and <code>newSkins</code>, which drops every modifier, then add the ones to keep again with <code>newProfileModifiers</code> (Archicad 27 and later).':
        'Bauen Sie das Profil mit <code>replaceSkins</code> und <code>newSkins</code> von <a href="../archicad-addon/#CreateProfiles">CreateProfiles</a> neu auf, was alle Modifikatoren entfernt, und fügen Sie die zu behaltenden danach mit <code>newProfileModifiers</code> wieder hinzu (ab Archicad 27).',
    'Import an IFC translator from a file into the project, or export one to a file':
        'Einen IFC-Übersetzer aus einer Datei ins Projekt importieren oder einen in eine Datei exportieren',
    'Keep the office translator in the project template, so new projects have it, and bring it into older projects once with Import… in the IFC Translators dialog. A script can check that a project has it with <a href="../archicad-addon/#GetIFCExportTranslators">GetIFCExportTranslators</a> and export with it by name with <a href="../archicad-addon/#IFCFileOperation">IFCFileOperation</a>.':
        'Legen Sie den Übersetzer des Büros in der Projektvorlage ab, damit neue Projekte ihn haben, und holen Sie ihn einmal mit Import… im Dialog IFC Translators in ältere Projekte. Ein Skript kann mit <a href="../archicad-addon/#GetIFCExportTranslators">GetIFCExportTranslators</a> prüfen, ob ein Projekt ihn hat, und mit <a href="../archicad-addon/#IFCFileOperation">IFCFileOperation</a> über seinen Namen damit exportieren.',

    // Documents and publishing
    'Documents and publishing': 'Dokumente und Veröffentlichen',
    'Read the rows of an Interactive Schedule': 'Die Zeilen einer Interactive Schedule lesen',
    'Build the list yourself: find the elements with <a href="../archicad-addon/#GetElementsByType">GetElementsByType</a> and read their values with <a href="../archicad-addon/#GetPropertyValuesOfElements">GetPropertyValuesOfElements</a> or <a href="../archicad-addon/#GetQuantitiesOfElements">GetQuantitiesOfElements</a>. The <a href="../scripts/">built-in scripts</a> Room Finishes Schedule and Material Take-Off build such lists.':
        'Erstellen Sie die Liste selbst: Finden Sie die Elemente mit <a href="../archicad-addon/#GetElementsByType">GetElementsByType</a> und lesen Sie ihre Werte mit <a href="../archicad-addon/#GetPropertyValuesOfElements">GetPropertyValuesOfElements</a> oder <a href="../archicad-addon/#GetQuantitiesOfElements">GetQuantitiesOfElements</a>. Die <a href="../scripts/">mitgelieferten Skripte</a> Room Finishes Schedule und Material Take-Off erstellen solche Listen.',
    'Export an Interactive Schedule': 'Eine Interactive Schedule exportieren',
    'Put the schedule in a publisher set in Archicad once, then publish that set with <a href="../archicad-addon/#PublishPublisherSet">PublishPublisherSet</a>.':
        'Legen Sie die Liste einmal in Archicad in ein Publisher Set und veröffentlichen Sie dieses Set dann mit <a href="../archicad-addon/#PublishPublisherSet">PublishPublisherSet</a>.',
    'Choose the output format (PDF, DWG, DXF …) of a publisher set per call':
        'Das Ausgabeformat (PDF, DWG, DXF …) eines Publisher Sets pro Aufruf wählen',
    'Make one publisher set per format in Archicad’s Organizer once, then publish the one you need with <a href="../archicad-addon/#PublishPublisherSet">PublishPublisherSet</a>; its answer lists the files written (<code>writtenFiles</code>).':
        'Legen Sie einmal im Organizer von Archicad ein Publisher Set pro Format an und veröffentlichen Sie dann das benötigte mit <a href="../archicad-addon/#PublishPublisherSet">PublishPublisherSet</a>; seine Antwort listet die geschriebenen Dateien auf (<code>writtenFiles</code>).',
    'Create a revision Change without placing a Change Marker, or change the ID or description of an existing Change':
        'Einen Change der Revision anlegen, ohne einen Change Marker zu platzieren, oder die ID oder Beschreibung eines bestehenden Change ändern',
    'Create the Change together with its first marker with <a href="../archicad-addon/#CreateChangeMarkers">CreateChangeMarkers</a> (createChange). To change an existing Change, use Archicad’s Change Manager.':
        'Legen Sie den Change zusammen mit seinem ersten Marker mit <a href="../archicad-addon/#CreateChangeMarkers">CreateChangeMarkers</a> (createChange) an. Um einen bestehenden Change zu ändern, nutzen Sie den Change Manager von Archicad.',
    'Set the custom scheme fields of a revision Change': 'Die benutzerdefinierten Schemafelder eines Change der Revision setzen',
    'Fill them in Archicad’s Change Manager; <a href="../archicad-addon/#GetRevisionChanges">GetRevisionChanges</a> reads them back.':
        'Füllen Sie sie im Change Manager von Archicad aus; <a href="../archicad-addon/#GetRevisionChanges">GetRevisionChanges</a> liest sie wieder aus.',
    'Add elements to a revision Change, or remove them from it':
        'Elemente zu einem Change der Revision hinzufügen oder aus ihm entfernen',
    'Add them in Archicad’s Change Manager; <a href="../archicad-addon/#GetRevisionChangesOfElements">GetRevisionChangesOfElements</a> reads which Changes an element belongs to.':
        'Fügen Sie sie im Change Manager von Archicad hinzu; <a href="../archicad-addon/#GetRevisionChangesOfElements">GetRevisionChangesOfElements</a> liest aus, zu welchen Changes ein Element gehört.',

    // Library parts and GDL
    'Library parts and GDL': 'Bibliotheksteile und GDL',
    'Run a 2D GDL script given as text, as <a href="../archicad-addon/#InterpretGDLScript3D">InterpretGDLScript3D</a> does with a 3D script':
        'Ein als Text gegebenes 2D-GDL-Skript ausführen, wie es <a href="../archicad-addon/#InterpretGDLScript3D">InterpretGDLScript3D</a> mit einem 3D-Skript tut',
    'Save the script in a library part with <a href="../archicad-addon/#CreateLibraryParts">CreateLibraryParts</a> (or <a href="../archicad-addon/#SetLibraryPartScripts">SetLibraryPartScripts</a>), then draw its 2D symbol with <a href="../archicad-addon/#GetLibraryPart2DPrimitives">GetLibraryPart2DPrimitives</a>.':
        'Speichern Sie das Skript mit <a href="../archicad-addon/#CreateLibraryParts">CreateLibraryParts</a> (oder <a href="../archicad-addon/#SetLibraryPartScripts">SetLibraryPartScripts</a>) in einem Bibliotheksteil und zeichnen Sie dann sein 2D-Symbol mit <a href="../archicad-addon/#GetLibraryPart2DPrimitives">GetLibraryPart2DPrimitives</a>.',
    'Get the error message and the line number when a GDL script fails':
        'Die Fehlermeldung und die Zeilennummer erhalten, wenn ein GDL-Skript fehlschlägt',
    'Check whether the script runs with <a href="../archicad-addon/#InterpretGDLScript3D">InterpretGDLScript3D</a> (3D) or <a href="../archicad-addon/#GetLibraryPart2DPrimitives">GetLibraryPart2DPrimitives</a> (2D). To see the error, open the library part in Archicad’s GDL editor with <a href="../archicad-addon/#OpenLibraryPartInEditor">OpenLibraryPartInEditor</a> (from Archicad 27).':
        'Prüfen Sie mit <a href="../archicad-addon/#InterpretGDLScript3D">InterpretGDLScript3D</a> (3D) oder <a href="../archicad-addon/#GetLibraryPart2DPrimitives">GetLibraryPart2DPrimitives</a> (2D), ob das Skript läuft. Um den Fehler zu sehen, öffnen Sie das Bibliotheksteil mit <a href="../archicad-addon/#OpenLibraryPartInEditor">OpenLibraryPartInEditor</a> im GDL-Editor von Archicad (ab Archicad 27).',

    // Archicad's interface
    'Archicad\'s interface': 'Die Oberfläche von Archicad',
    'Read the text of the Session Report window': 'Den Text des Fensters Session Report lesen',
    'Open the window for the user with <a href="../archicad-addon/#ChangeWindow">ChangeWindow</a> and windowType \'Report\' (from Archicad 26).':
        'Öffnen Sie das Fenster für den Nutzer mit <a href="../archicad-addon/#ChangeWindow">ChangeWindow</a> und windowType \'Report\' (ab Archicad 26).',
    'Get the outcome of an Action Center check': 'Das Ergebnis einer Prüfung im Action Center erhalten',
    'Run the check with <a href="../archicad-addon/#RunActionCenterChecks">RunActionCenterChecks</a>; Archicad shows the Action Center when it found an issue. For the hotlink check, read the state of each source with <a href="../archicad-addon/#GetHotlinkSourceStatus">GetHotlinkSourceStatus</a> (from Archicad 27).':
        'Starten Sie die Prüfung mit <a href="../archicad-addon/#RunActionCenterChecks">RunActionCenterChecks</a>; Archicad zeigt das Action Center, wenn es ein Problem gefunden hat. Für die Hotlink-Prüfung lesen Sie den Zustand jeder Quelle mit <a href="../archicad-addon/#GetHotlinkSourceStatus">GetHotlinkSourceStatus</a> (ab Archicad 27).',
    'Load a new add-on (.apx or .bundle) while Archicad runs, from an add-on or a script':
        'Ein neues Add-On (.apx oder .bundle) laden, während Archicad läuft, aus einem Add-On oder einem Skript heraus',
    'Add it in Archicad\'s Add-On Manager (Options &gt; Add-On Manager &gt; Edit List of Available Add-Ons &gt; Add). An add-on that is already loaded is replaced only after Archicad quits, as the Tapir Installer does when it updates Tapir.':
        'Fügen Sie es im Add-On Manager von Archicad hinzu (Options &gt; Add-On Manager &gt; Edit List of Available Add-Ons &gt; Add). Ein bereits geladenes Add-On wird erst ersetzt, nachdem Archicad beendet wurde, so wie es der Tapir Installer tut, wenn er Tapir aktualisiert.',

    'Set the profile modifier values, or the stretched width and height, of a wall, beam or column that uses a profile':
        'Die Werte der Profil-Modifikatoren oder die gestreckte Breite und Höhe einer Wand, eines Unterzugs oder einer Stütze mit Profil einstellen',
    'Read them with <a href="../archicad-addon/#GetProfileValuesOfElements">GetProfileValuesOfElements</a>. To change them, change the profile with <a href="../archicad-addon/#CreateProfiles">CreateProfiles</a> (a copy for these elements, or the profile itself), or set them in the settings of the element in Archicad.':
        'Lesen Sie sie mit <a href="../archicad-addon/#GetProfileValuesOfElements">GetProfileValuesOfElements</a> aus. Um sie zu ändern, ändern Sie das Profil mit <a href="../archicad-addon/#CreateProfiles">CreateProfiles</a> (eine Kopie für diese Elemente oder das Profil selbst) oder stellen Sie sie in den Einstellungen des Elements in Archicad ein.',
    'Read or set how a grid element is shown in sections, elevations and interior elevations: its vertical extent and its markers at the top and bottom':
        'Lesen oder einstellen, wie ein Rasterelement in Schnitten, Ansichten und Innenansichten gezeigt wird: seine vertikale Ausdehnung und seine Marken oben und unten',
    'Select the grid elements in Archicad and set it for all of them at once in their settings dialog.':
        'Wählen Sie die Rasterelemente in Archicad aus und stellen Sie es in ihrem Einstellungsdialog für alle auf einmal ein.',
    'Read the settings of an IFC translator, such as its type mapping and property mapping, for example to check them as the IFC conformity check of the Missing BIM Data report does':
        'Die Einstellungen eines IFC-Übersetzers lesen, etwa seine Typ-Zuordnung und Eigenschaften-Zuordnung, zum Beispiel um sie zu prüfen, wie es die IFC-Konformitätsprüfung des Berichts über fehlende BIM-Daten tut',
    'Check what the translator gives the elements instead: their IFC type with <a href="../archicad-addon/#GetIFCTypeOfElements">GetIFCTypeOfElements</a>, and from Archicad 28 the properties and classification references the preview translator maps to them with <a href="../archicad-addon/#GetIFCPropertiesOfElements">GetIFCPropertiesOfElements</a> and <a href="../archicad-addon/#GetIFCClassificationReferencesOfElements">GetIFCClassificationReferencesOfElements</a>. <a href="../archicad-addon/#GetAllProperties">GetAllProperties</a> shows whether the properties a mapping needs exist in the project.':
        'Prüfen Sie stattdessen, was der Übersetzer den Elementen gibt: ihren IFC-Typ mit <a href="../archicad-addon/#GetIFCTypeOfElements">GetIFCTypeOfElements</a> und ab Archicad 28 die Eigenschaften und Klassifizierungsreferenzen, die der Vorschau-Übersetzer ihnen zuordnet, mit <a href="../archicad-addon/#GetIFCPropertiesOfElements">GetIFCPropertiesOfElements</a> und <a href="../archicad-addon/#GetIFCClassificationReferencesOfElements">GetIFCClassificationReferencesOfElements</a>. Ob die Eigenschaften, die eine Zuordnung braucht, im Projekt vorhanden sind, zeigt <a href="../archicad-addon/#GetAllProperties">GetAllProperties</a>.',
    'Find the graphic override rules, Interactive Schedules and SAF translators that refer to a property or classification that no longer exists, as the Missing BIM Data report does':
        'Die Regeln für grafische Überschreibungen, die interaktiven Auswertungen und die SAF-Übersetzer finden, die auf eine Eigenschaft oder Klassifizierung verweisen, die es nicht mehr gibt, wie es der Bericht über fehlende BIM-Daten tut',
    'For graphic override rules (from Archicad 27): read the criterion of each rule with <a href="../archicad-addon/#GetGraphicOverrideRules">GetGraphicOverrideRules</a> (includeCriterion) and compare the properties and classifications it tests with <a href="../archicad-addon/#GetAllProperties">GetAllProperties</a> and <a href="../archicad-addon/#GetAllClassificationSystems">GetAllClassificationSystems</a>. For Interactive Schedules and SAF translators: nothing yet.':
        'Für Regeln für grafische Überschreibungen (ab Archicad 27): Lesen Sie das Kriterium jeder Regel mit <a href="../archicad-addon/#GetGraphicOverrideRules">GetGraphicOverrideRules</a> (includeCriterion) und vergleichen Sie die Eigenschaften und Klassifizierungen, die es prüft, mit <a href="../archicad-addon/#GetAllProperties">GetAllProperties</a> und <a href="../archicad-addon/#GetAllClassificationSystems">GetAllClassificationSystems</a>. Für interaktive Auswertungen und SAF-Übersetzer: noch nichts.',
    'Export and import, all at once, the settings that the dialogs of Archicad save as XML (properties, classifications, the Layout Book, graphic overrides, Model View Options, IFC and DWG translators), to keep several projects in sync':
        'Die Einstellungen, die die Dialoge von Archicad als XML speichern (Eigenschaften, Klassifizierungen, das Planbuch, grafische Überschreibungen, Modellansicht-Optionen, IFC- und DWG-Übersetzer), alle auf einmal exportieren und importieren, um mehrere Projekte abzugleichen',
    'Copy what the API can write, one kind at a time, in one script run against each project: properties and classifications from their XML files with <a href="../archicad-addon/#ImportPropertiesXml">ImportPropertiesXml</a> and <a href="../archicad-addon/#ImportClassificationsXml">ImportClassificationsXml</a>; attributes from another project with <a href="../archicad-addon/#ImportAttributesFromProject">ImportAttributesFromProject</a>; graphic override rules with <a href="../archicad-addon/#GetGraphicOverrideRules">GetGraphicOverrideRules</a> and <a href="../archicad-addon/#CreateGraphicOverrideRules">CreateGraphicOverrideRules</a>; Project Info with <a href="../archicad-addon/#GetProjectInfoFields">GetProjectInfoFields</a> and <a href="../archicad-addon/#SetProjectInfoField">SetProjectInfoField</a>; Favorites with <a href="../archicad-addon/#ExportFavorites">ExportFavorites</a> and <a href="../archicad-addon/#ImportFavorites">ImportFavorites</a>. The Layout Book, Model View Options and the translators: nothing yet.':
        'Kopieren Sie, was die API schreiben kann, eine Art nach der anderen, in einem Skript, das Sie für jedes Projekt ausführen: Eigenschaften und Klassifizierungen aus ihren XML-Dateien mit <a href="../archicad-addon/#ImportPropertiesXml">ImportPropertiesXml</a> und <a href="../archicad-addon/#ImportClassificationsXml">ImportClassificationsXml</a>; Attribute aus einem anderen Projekt mit <a href="../archicad-addon/#ImportAttributesFromProject">ImportAttributesFromProject</a>; Regeln für grafische Überschreibungen mit <a href="../archicad-addon/#GetGraphicOverrideRules">GetGraphicOverrideRules</a> und <a href="../archicad-addon/#CreateGraphicOverrideRules">CreateGraphicOverrideRules</a>; die Projektinfo mit <a href="../archicad-addon/#GetProjectInfoFields">GetProjectInfoFields</a> und <a href="../archicad-addon/#SetProjectInfoField">SetProjectInfoField</a>; Favoriten mit <a href="../archicad-addon/#ExportFavorites">ExportFavorites</a> und <a href="../archicad-addon/#ImportFavorites">ImportFavorites</a>. Das Planbuch, die Modellansicht-Optionen und die Übersetzer: noch nichts.',


    'Found something else the Archicad API cannot do? Say so in an issue, with the DevKit headers you checked, and it is added here. <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/issues/new/choose">Open an issue</a>':
        'Etwas anderes gefunden, das die Archicad-API nicht kann? Schreiben Sie es in ein Issue, mit den DevKit-Headern, die Sie geprüft haben, und es kommt auf diese Seite. <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/issues/new/choose">Ein Issue öffnen</a>'
});
