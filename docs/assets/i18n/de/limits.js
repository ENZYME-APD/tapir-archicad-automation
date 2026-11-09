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
    'Read the slab\'s outline with <a href="../archicad-addon/#GetDetailsOfElements">GetDetailsOfElements</a> and create a static railing along the edge with <a href="../archicad-addon/#CreateRailings">CreateRailings</a>. When the slab changes, delete the railing and create it again.':
        'Lesen Sie den Umriss der Decke mit <a href="../archicad-addon/#GetDetailsOfElements">GetDetailsOfElements</a> aus und erstellen Sie mit <a href="../archicad-addon/#CreateRailings">CreateRailings</a> ein statisches Geländer entlang der Kante. Wenn sich die Decke ändert, löschen Sie das Geländer und erstellen Sie es neu.',

    // Navigator and views
    'Navigator and views': 'Navigator und Ansichten',
    'Add an item to a publisher set': 'Einem Publisher Set ein Element hinzufügen',
    'Add the items in Archicad\'s Organizer, then publish the set with <a href="../archicad-addon/#PublishPublisherSet">PublishPublisherSet</a>.':
        'Fügen Sie die Elemente im Organizer von Archicad hinzu und veröffentlichen Sie das Set dann mit <a href="../archicad-addon/#PublishPublisherSet">PublishPublisherSet</a>.',
    'Change, rename or create a stored Model View Options combination': 'Eine gespeicherte Kombination der Model View Options ändern, umbenennen oder anlegen',
    'Read the combinations with <a href="../archicad-addon/#GetModelViewOptionDetails">GetModelViewOptionDetails</a>; change the options of the current window with <a href="../archicad-addon/#SetViewOptions">SetViewOptions</a>.':
        'Lesen Sie die Kombinationen mit <a href="../archicad-addon/#GetModelViewOptionDetails">GetModelViewOptionDetails</a>; ändern Sie die Optionen des aktuellen Fensters mit <a href="../archicad-addon/#SetViewOptions">SetViewOptions</a>.',

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

    // Documents and publishing
    'Documents and publishing': 'Dokumente und Veröffentlichen',
    'Read the rows of an Interactive Schedule': 'Die Zeilen einer Interactive Schedule lesen',
    'Build the list yourself: find the elements with <a href="../archicad-addon/#GetElementsByType">GetElementsByType</a> and read their values with <a href="../archicad-addon/#GetPropertyValuesOfElements">GetPropertyValuesOfElements</a> or <a href="../archicad-addon/#GetQuantitiesOfElements">GetQuantitiesOfElements</a>. The <a href="../scripts/">built-in scripts</a> Room Finishes Schedule and Material Take-Off build such lists.':
        'Erstellen Sie die Liste selbst: Finden Sie die Elemente mit <a href="../archicad-addon/#GetElementsByType">GetElementsByType</a> und lesen Sie ihre Werte mit <a href="../archicad-addon/#GetPropertyValuesOfElements">GetPropertyValuesOfElements</a> oder <a href="../archicad-addon/#GetQuantitiesOfElements">GetQuantitiesOfElements</a>. Die <a href="../scripts/">mitgelieferten Skripte</a> Room Finishes Schedule und Material Take-Off erstellen solche Listen.',
    'Export an Interactive Schedule': 'Eine Interactive Schedule exportieren',
    'Put the schedule in a publisher set in Archicad once, then publish that set with <a href="../archicad-addon/#PublishPublisherSet">PublishPublisherSet</a>.':
        'Legen Sie die Liste einmal in Archicad in ein Publisher Set und veröffentlichen Sie dieses Set dann mit <a href="../archicad-addon/#PublishPublisherSet">PublishPublisherSet</a>.',

    // Archicad's interface
    'Archicad\'s interface': 'Die Oberfläche von Archicad',
    'Read the text of the Session Report window': 'Den Text des Fensters Session Report lesen',
    'Open the window for the user with <a href="../archicad-addon/#ChangeWindow">ChangeWindow</a> and windowType \'Report\' (from Archicad 26).':
        'Öffnen Sie das Fenster für den Nutzer mit <a href="../archicad-addon/#ChangeWindow">ChangeWindow</a> und windowType \'Report\' (ab Archicad 26).',
    'Get the outcome of an Action Center check': 'Das Ergebnis einer Prüfung im Action Center erhalten',
    'Run the check with <a href="../archicad-addon/#RunActionCenterChecks">RunActionCenterChecks</a>; Archicad shows the Action Center when it found an issue. For the hotlink check, read the state of each source with <a href="../archicad-addon/#GetHotlinkSourceStatus">GetHotlinkSourceStatus</a> (from Archicad 27).':
        'Starten Sie die Prüfung mit <a href="../archicad-addon/#RunActionCenterChecks">RunActionCenterChecks</a>; Archicad zeigt das Action Center, wenn es ein Problem gefunden hat. Für die Hotlink-Prüfung lesen Sie den Zustand jeder Quelle mit <a href="../archicad-addon/#GetHotlinkSourceStatus">GetHotlinkSourceStatus</a> (ab Archicad 27).',

    'Found something else the Archicad API cannot do? Say so in an issue, with the DevKit headers you checked, and it is added here. <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/issues/new/choose">Open an issue</a>':
        'Etwas anderes gefunden, das die Archicad-API nicht kann? Schreiben Sie es in ein Issue, mit den DevKit-Headern, die Sie geprüft haben, und es kommt auf diese Seite. <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/issues/new/choose">Ein Issue öffnen</a>'
});
