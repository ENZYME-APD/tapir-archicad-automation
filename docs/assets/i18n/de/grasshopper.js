/* Deutsch: the Grasshopper page (grasshopper/index.html). See ../README.md.
   Component, input and button names stay English, as Grasshopper shows them. */
(window.TAPIR_I18N = window.TAPIR_I18N || []).push ({
    'Archicad Grasshopper live connection | Tapir': 'Live-Verbindung zwischen Archicad und Grasshopper | Tapir',
    'Connect Grasshopper live to a running Archicad with the Tapir plugin: read, change and create Archicad elements from Rhino geometry, without writing code.':
        'Grasshopper mit dem Tapir-Plugin live mit einem laufenden Archicad verbinden: Archicad-Elemente aus Rhino-Geometrie lesen, ändern und erstellen, ohne Code zu schreiben.',
    '<a href="../">Tapir</a> / <a href="../docs/">Docs</a> / Grasshopper':
        '<a href="../">Tapir</a> / <a href="../docs/">Doku</a> / Grasshopper',
    'Grasshopper, <em>visually.</em>': 'Grasshopper, <em>visuell.</em>',
    'The Tapir plugin connects Grasshopper to a running Archicad: read elements and their settings, change them, and create new ones from Rhino geometry, without writing code.':
        'Das Tapir-Plugin verbindet Grasshopper mit einem laufenden Archicad: Elemente und ihre Einstellungen lesen, sie ändern und neue aus Rhino-Geometrie erstellen, ohne Code zu schreiben.',

    // What the components do
    'What the components <em>do.</em>': 'Was die Komponenten <em>tun.</em>',
    'Each component sends a Tapir command to Archicad and gives you the answer as Grasshopper data. You find them on the <strong>Tapir</strong> tab, grouped as in Archicad.':
        'Jede Komponente sendet einen Tapir-Befehl an Archicad und gibt Ihnen die Antwort als Grasshopper-Daten. Sie finden sie im Reiter <strong>Tapir</strong>, gruppiert wie in Archicad.',
    '<b>READ</b><strong>Elements and their data</strong><span>Elements by type, story or selection; their properties, classifications, settings and geometry.</span>':
        '<b>LESEN</b><strong>Elemente und ihre Daten</strong><span>Elemente nach Typ, Geschoss oder Auswahl; ihre Eigenschaften, Klassifizierungen, Einstellungen und Geometrie.</span>',
    '<b>CHANGE</b><strong>What you pass in</strong><span>Property values, classifications, layers, IDs and tool settings of the elements you pass in.</span>':
        '<b>ÄNDERN</b><strong>Was Sie übergeben</strong><span>Eigenschaftswerte, Klassifizierungen, Ebenen, IDs und Werkzeugeinstellungen der übergebenen Elemente.</span>',
    '<b>CREATE</b><strong>From Rhino geometry</strong><span>Walls, doors, windows, railings, level dimensions and more, from points and curves.</span>':
        '<b>ERSTELLEN</b><strong>Aus Rhino-Geometrie</strong><span>Wände, Türen, Fenster, Geländer, Höhenkoten und mehr, aus Punkten und Kurven.</span>',
    '<b>PROJECT</b><strong>The rest of the file</strong><span>Stories, attributes, navigator views, layouts, issues, favorites and libraries.</span>':
        '<b>PROJEKT</b><strong>Der Rest der Datei</strong><span>Geschosse, Attribute, Navigator-Ausschnitte, Layouts, Anmerkungen, Favoriten und Bibliotheken.</span>',
    'Which Tapir commands have a component, and which settings each covers, is on the <a href="../gh-coverage/">Grasshopper coverage</a> page.':
        'Welche Tapir-Befehle eine Komponente haben und welche Einstellungen jede abdeckt, steht auf der Seite <a href="../gh-coverage/">Grasshopper-Abdeckung</a>.',

    // Tool components
    'Tool components': 'Werkzeug-Komponenten',
    'One per Archicad <em>tool.</em>': 'Eine je Archicad-<em>Werkzeug.</em>',
    'A tool component works like the Archicad tool of the same name. One JSON input and one JSON output per panel of its settings dialog, keyed by the labels you see there, such as <code>Wall Height</code>.':
        'Eine Werkzeug-Komponente arbeitet wie das gleichnamige Archicad-Werkzeug. Ein JSON-Eingang und ein JSON-Ausgang je Bereich seines Einstellungsdialogs, mit den englischen Beschriftungen, die Sie dort sehen, als Schlüssel, etwa <code>Wall Height</code>.',
    'Placed by <code>StartPoint</code> and <code>EndPoint</code>.': 'Platziert über <code>StartPoint</code> und <code>EndPoint</code>.',
    'Placed in an <code>OwnerWall</code> at a <code>CenterOffset</code>.': 'Platziert in einer <code>OwnerWall</code> bei einem <code>CenterOffset</code>.',
    'Placed along a <code>ReferenceLine</code>.': 'Platziert entlang einer <code>ReferenceLine</code>.',
    'Placed at a <code>Position</code>, optionally on a parent element.': 'Platziert an einer <code>Position</code>, wahlweise auf einem übergeordneten Element.',
    '<b>REFRESH</b><strong>Reads</strong><span>Reads the elements in <code>ElementGuid</code> and fills every output.</span>':
        '<b>REFRESH</b><strong>Liest</strong><span>Liest die Elemente in <code>ElementGuid</code> und füllt jeden Ausgang.</span>',
    '<b>EXECUTE</b><strong>Writes</strong><span>Writes the given settings to those elements. Only the keys you give are written.</span>':
        '<b>EXECUTE</b><strong>Schreibt</strong><span>Schreibt die angegebenen Einstellungen in diese Elemente. Nur die angegebenen Schlüssel werden geschrieben.</span>',
    '<b>CREATE</b><strong>Makes new elements</strong><span>From the geometry inputs, starting from the tool\'s defaults or from a <code>FavoriteName</code>.</span>':
        '<b>CREATE</b><strong>Erstellt neue Elemente</strong><span>Aus den Geometrie-Eingängen, ausgehend von den Standardwerten des Werkzeugs oder von einem <code>FavoriteName</code>.</span>',
    'Nothing is written to Archicad until you press Execute or Create. A JSON output read by Refresh can be fed straight back into the same input. Archicad has no command to change railings or level dimensions after they are placed, so for those Execute only writes the ID, layer and draw order.':
        'Nichts wird nach Archicad geschrieben, bevor Sie Execute oder Create drücken. Ein von Refresh gelesener JSON-Ausgang kann direkt wieder in denselben Eingang gegeben werden. Archicad hat keinen Befehl, Geländer oder Höhenkoten nach dem Platzieren zu ändern; für sie schreibt Execute daher nur die ID, die Ebene und die Anzeigereihenfolge.',
    '<strong class="callout-label"><span aria-hidden="true">ⓘ</span> Note</strong> Walls remembers the walls its Create made, in the <code>.gh</code> file. The next Create updates them instead of making new ones. Turn this off, or forget them, in the component\'s menu.':
        '<strong class="callout-label"><span aria-hidden="true">ⓘ</span> Hinweis</strong> Walls merkt sich die Wände, die sein Create erstellt hat, in der <code>.gh</code>-Datei. Das nächste Create aktualisiert sie, statt neue zu erstellen. Im Menü der Komponente schalten Sie das aus oder lassen sie vergessen.',

    // Install
    'Install': 'Installation',
    'Install the Tapir Add-On in Archicad': 'Das Tapir-Add-On in Archicad installieren',
    'The components talk to Archicad through the Add-On. <a href="../start/#install">Get started</a> shows how.':
        'Die Komponenten sprechen über das Add-On mit Archicad. <a href="../start/#install">Loslegen</a> zeigt, wie es geht.',
    'Install the <span class="mono">tapir</span> package in Rhino': 'Das Paket <span class="mono">tapir</span> in Rhino installieren',
    'In Rhino 7 or 8, open <em>Tools &gt; Package Manager</em> (or type <code>PackageManager</code>), search for <strong>tapir</strong> and click Install. Restart Rhino.':
        'Öffnen Sie in Rhino 7 oder 8 <em>Tools &gt; Package Manager</em> (oder geben Sie <code>PackageManager</code> ein), suchen Sie nach <strong>tapir</strong> und klicken Sie auf Install. Starten Sie Rhino neu.',
    'Open Archicad, then Grasshopper': 'Erst Archicad, dann Grasshopper öffnen',
    'With a project open in Archicad, start Grasshopper. The <strong>Tapir</strong> tab appears in its toolbar. With more than one Archicad open, give the port of the one you want to the <strong>ConnectArchicad</strong> component (<em>Tapir &gt; General</em>); the first Archicad uses 19723.':
        'Starten Sie Grasshopper, während in Archicad ein Projekt geöffnet ist. Der Reiter <strong>Tapir</strong> erscheint in seiner Werkzeugleiste. Sind mehrere Archicad geöffnet, geben Sie den Port des gewünschten an die Komponente <strong>ConnectArchicad</strong> (<em>Tapir &gt; General</em>); das erste Archicad nutzt 19723.',

    // A first definition
    'A first definition': 'Eine erste Definition',
    'One <em>wall.</em>': 'Eine <em>Wand.</em>',
    'Build this in an empty Grasshopper canvas, with a project open in Archicad:':
        'Bauen Sie dies auf einer leeren Grasshopper-Arbeitsfläche, während in Archicad ein Projekt geöffnet ist:',
    'Place two <strong>Point</strong> parameters and set them to 0,0,0 and 5,0,0. Archicad reads the numbers as metres.':
        'Platzieren Sie zwei <strong>Point</strong>-Parameter und setzen Sie sie auf 0,0,0 und 5,0,0. Archicad liest die Zahlen als Meter.',
    'Place the <strong>Walls</strong> component from <em>Tapir &gt; Design Tools</em>. Connect the points to <code>StartPoint</code> and <code>EndPoint</code>.':
        'Platzieren Sie die Komponente <strong>Walls</strong> aus <em>Tapir &gt; Design Tools</em>. Verbinden Sie die Punkte mit <code>StartPoint</code> und <code>EndPoint</code>.',
    'Place a <strong>Panel</strong> with the text on the right and connect it to <code>GeometryAndPositioning_JSON</code>.':
        'Platzieren Sie ein <strong>Panel</strong> mit dem Text rechts und verbinden Sie es mit <code>GeometryAndPositioning_JSON</code>.',
    'Press <strong>Create</strong>. A 5 m wall appears in Archicad, 3 m high and 30 cm thick.':
        'Drücken Sie <strong>Create</strong>. In Archicad erscheint eine 5 m lange Wand, 3 m hoch und 30 cm dick.',
    'Move a point and press Create again: the same wall moves. Change the panel and press Create: the same wall changes.':
        'Verschieben Sie einen Punkt und drücken Sie erneut Create: Dieselbe Wand bewegt sich. Ändern Sie das Panel und drücken Sie Create: Dieselbe Wand ändert sich.',
    'Press <strong>Refresh</strong> to see every setting of the wall in the JSON outputs. Copy a key from there to set it the next time.':
        'Drücken Sie <strong>Refresh</strong>, um jede Einstellung der Wand in den JSON-Ausgängen zu sehen. Kopieren Sie einen Schlüssel von dort, um ihn beim nächsten Mal zu setzen.',

    // Next step
    'Next step': 'Nächster Schritt',
    'Install the Add-On': 'Das Add-On installieren',
    'Install Tapir and the <span class="mono">tapir</span> package, then try the wall above.':
        'Installieren Sie Tapir und das Paket <span class="mono">tapir</span>, dann probieren Sie die Wand oben aus.',
    'Get started': 'Loslegen',
    'Coverage': 'Abdeckung',
    'Which commands have a component, and which inputs each still lacks.':
        'Welche Befehle eine Komponente haben und welche Eingänge jeweils noch fehlen.',
    'See the coverage': 'Zur Abdeckung',
    'Videos': 'Videos',
    'Watch it work': 'Sehen, wie es funktioniert',
    'The Archicad and Grasshopper tutorials.': 'Die Tutorials zu Archicad und Grasshopper.',
    'Watch the tutorials': 'Zu den Tutorials'
});
