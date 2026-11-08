/* Italiano: the Grasshopper page (grasshopper/index.html). See ../README.md.
   Component, input and button names stay English, as Grasshopper shows them. */
(window.TAPIR_I18N = window.TAPIR_I18N || []).push ({
    'Archicad Grasshopper live connection | Tapir': 'Connessione live tra Grasshopper e Archicad | Tapir',
    'Connect Grasshopper live to a running Archicad with the Tapir plugin: read, change and create Archicad elements from Rhino geometry, without writing code.':
        'Collega Grasshopper in tempo reale a un Archicad aperto con il plugin Tapir: leggi, modifica e crea elementi di Archicad dalla geometria di Rhino, senza scrivere codice.',
    '<a href="../">Tapir</a> / <a href="../docs/">Docs</a> / Grasshopper':
        '<a href="../">Tapir</a> / <a href="../docs/">Documentazione</a> / Grasshopper',
    'Grasshopper, <em>visually.</em>': 'Grasshopper, <em>visivamente.</em>',
    'The Tapir plugin connects Grasshopper to a running Archicad: read elements and their settings, change them, and create new ones from Rhino geometry, without writing code.':
        'Il plugin Tapir collega Grasshopper a un Archicad aperto: leggi gli elementi e le loro impostazioni, modificali e creane di nuovi dalla geometria di Rhino, senza scrivere codice.',

    // What the components do
    'What the components <em>do.</em>': 'Cosa fanno i <em>componenti.</em>',
    'Each component sends a Tapir command to Archicad and gives you the answer as Grasshopper data. You find them on the <strong>Tapir</strong> tab, grouped as in Archicad.':
        'Ogni componente invia un comando Tapir ad Archicad e ti restituisce la risposta come dati Grasshopper. Li trovi nella scheda <strong>Tapir</strong>, raggruppati come in Archicad.',
    '<b>READ</b><strong>Elements and their data</strong><span>Elements by type, story or selection; their properties, classifications, settings and geometry.</span>':
        '<b>LEGGERE</b><strong>Elementi e loro dati</strong><span>Elementi per tipo, piano o selezione; le loro proprietà, classificazioni, impostazioni e geometria.</span>',
    '<b>CHANGE</b><strong>What you pass in</strong><span>Property values, classifications, layers, IDs and tool settings of the elements you pass in.</span>':
        '<b>MODIFICARE</b><strong>Ciò che passi</strong><span>Valori di proprietà, classificazioni, layer, ID e impostazioni strumento degli elementi che passi.</span>',
    '<b>CREATE</b><strong>From Rhino geometry</strong><span>Walls, doors, windows, railings, level dimensions and more, from points and curves.</span>':
        '<b>CREARE</b><strong>Dalla geometria di Rhino</strong><span>Muri, porte, finestre, ringhiere, quote di livello e altro, da punti e curve.</span>',
    '<b>PROJECT</b><strong>The rest of the file</strong><span>Stories, attributes, navigator views, layouts, issues, favorites and libraries.</span>':
        '<b>PROGETTO</b><strong>Il resto del file</strong><span>Piani, attributi, viste del navigatore, layout, segnalazioni, preferiti e librerie.</span>',
    'Which Tapir commands have a component, and which settings each covers, is on the <a href="../gh-coverage/">Grasshopper coverage</a> page.':
        'Quali comandi Tapir hanno un componente, e quali impostazioni copre ciascuno, è nella pagina <a href="../gh-coverage/">Copertura Grasshopper</a>.',

    // Tool components
    'Tool components': 'Componenti strumento',
    'One per Archicad <em>tool.</em>': 'Uno per ogni <em>strumento</em> di Archicad.',
    'A tool component works like the Archicad tool of the same name. One JSON input and one JSON output per panel of its settings dialog, keyed by the labels you see there, such as <code>Wall Height</code>.':
        'Un componente strumento funziona come lo strumento di Archicad con lo stesso nome. Un ingresso JSON e un\'uscita JSON per ogni pannello della sua finestra di impostazioni, con come chiavi le etichette inglesi che vedi lì, ad esempio <code>Wall Height</code>.',
    'Placed by <code>StartPoint</code> and <code>EndPoint</code>.': 'Posizionato con <code>StartPoint</code> e <code>EndPoint</code>.',
    'Placed in an <code>OwnerWall</code> at a <code>CenterOffset</code>.': 'Posizionata in un <code>OwnerWall</code> a un <code>CenterOffset</code>.',
    'Placed along a <code>ReferenceLine</code>.': 'Posizionata lungo una <code>ReferenceLine</code>.',
    'Placed at a <code>Position</code>, optionally on a parent element.': 'Posizionata in una <code>Position</code>, facoltativamente su un elemento padre.',
    '<b>REFRESH</b><strong>Reads</strong><span>Reads the elements in <code>ElementGuid</code> and fills every output.</span>':
        '<b>REFRESH</b><strong>Legge</strong><span>Legge gli elementi in <code>ElementGuid</code> e riempie ogni uscita.</span>',
    '<b>EXECUTE</b><strong>Writes</strong><span>Writes the given settings to those elements. Only the keys you give are written.</span>':
        '<b>EXECUTE</b><strong>Scrive</strong><span>Scrive le impostazioni date in quegli elementi. Vengono scritte solo le chiavi che indichi.</span>',
    '<b>CREATE</b><strong>Makes new elements</strong><span>From the geometry inputs, starting from the tool\'s defaults or from a <code>FavoriteName</code>.</span>':
        '<b>CREATE</b><strong>Crea nuovi elementi</strong><span>Dagli ingressi di geometria, partendo dai valori predefiniti dello strumento o da un <code>FavoriteName</code>.</span>',
    'Nothing is written to Archicad until you press Execute or Create. A JSON output read by Refresh can be fed straight back into the same input. Archicad has no command to change railings or level dimensions after they are placed, so for those Execute only writes the ID, layer and draw order.':
        'Nulla viene scritto in Archicad finché non premi Execute o Create. Un\'uscita JSON letta da Refresh può tornare direttamente nello stesso ingresso. Archicad non ha un comando per modificare ringhiere o quote di livello dopo averle posizionate, quindi per queste Execute scrive solo l\'ID, il layer e l\'ordine di visualizzazione.',
    '<strong class="callout-label"><span aria-hidden="true">ⓘ</span> Note</strong> Walls remembers the walls its Create made, in the <code>.gh</code> file. The next Create updates them instead of making new ones. Turn this off, or forget them, in the component\'s menu.':
        '<strong class="callout-label"><span aria-hidden="true">ⓘ</span> Nota</strong> Walls ricorda i muri creati dal suo Create, nel file <code>.gh</code>. Il Create successivo li aggiorna invece di crearne di nuovi. Puoi disattivarlo, o farli dimenticare, nel menu del componente.',

    // Install
    'Install': 'Installazione',
    'Install the Tapir Add-On in Archicad': 'Installa l\'Add-On Tapir in Archicad',
    'The components talk to Archicad through the Add-On. <a href="../start/#install">Get started</a> shows how.':
        'I componenti comunicano con Archicad tramite l\'Add-On. <a href="../start/#install">Inizia</a> mostra come installarlo.',
    'Install the <span class="mono">tapir</span> package in Rhino': 'Installa il pacchetto <span class="mono">tapir</span> in Rhino',
    'In Rhino 7 or 8, open <em>Tools &gt; Package Manager</em> (or type <code>PackageManager</code>), search for <strong>tapir</strong> and click Install. Restart Rhino.':
        'In Rhino 7 o 8, apri <em>Tools &gt; Package Manager</em> (o digita <code>PackageManager</code>), cerca <strong>tapir</strong> e fai clic su Install. Riavvia Rhino.',
    'Open Archicad, then Grasshopper': 'Apri Archicad, poi Grasshopper',
    'With a project open in Archicad, start Grasshopper. The <strong>Tapir</strong> tab appears in its toolbar. With more than one Archicad open, give the port of the one you want to the <strong>ConnectArchicad</strong> component (<em>Tapir &gt; General</em>); the first Archicad uses 19723.':
        'Con un progetto aperto in Archicad, avvia Grasshopper. La scheda <strong>Tapir</strong> compare nella sua barra degli strumenti. Con più Archicad aperti, passa la porta di quello che vuoi al componente <strong>ConnectArchicad</strong> (<em>Tapir &gt; General</em>); il primo Archicad usa la 19723.',

    // A first definition
    'A first definition': 'Una prima definizione',
    'One <em>wall.</em>': 'Un <em>muro.</em>',
    'Build this in an empty Grasshopper canvas, with a project open in Archicad:':
        'Costruisci questo in un canvas vuoto di Grasshopper, con un progetto aperto in Archicad:',
    'Place two <strong>Point</strong> parameters and set them to 0,0,0 and 5,0,0. Archicad reads the numbers as metres.':
        'Inserisci due parametri <strong>Point</strong> e impostali su 0,0,0 e 5,0,0. Archicad legge i numeri come metri.',
    'Place the <strong>Walls</strong> component from <em>Tapir &gt; Design Tools</em>. Connect the points to <code>StartPoint</code> and <code>EndPoint</code>.':
        'Inserisci il componente <strong>Walls</strong> da <em>Tapir &gt; Design Tools</em>. Collega i punti a <code>StartPoint</code> e <code>EndPoint</code>.',
    'Place a <strong>Panel</strong> with the text on the right and connect it to <code>GeometryAndPositioning_JSON</code>.':
        'Inserisci un <strong>Panel</strong> con il testo a destra e collegalo a <code>GeometryAndPositioning_JSON</code>.',
    'Press <strong>Create</strong>. A 5 m wall appears in Archicad, 3 m high and 30 cm thick.':
        'Premi <strong>Create</strong>. In Archicad compare un muro di 5 m, alto 3 m e spesso 30 cm.',
    'Move a point and press Create again: the same wall moves. Change the panel and press Create: the same wall changes.':
        'Sposta un punto e premi di nuovo Create: lo stesso muro si sposta. Modifica il panel e premi Create: lo stesso muro cambia.',
    'Press <strong>Refresh</strong> to see every setting of the wall in the JSON outputs. Copy a key from there to set it the next time.':
        'Premi <strong>Refresh</strong> per vedere ogni impostazione del muro nelle uscite JSON. Copia da lì una chiave per impostarla la volta successiva.',

    // Next step
    'Next step': 'Passo successivo',
    'Install the Add-On': 'Installa l\'Add-On',
    'Install Tapir and the <span class="mono">tapir</span> package, then try the wall above.':
        'Installa Tapir e il pacchetto <span class="mono">tapir</span>, poi prova il muro qui sopra.',
    'Get started': 'Inizia',
    'Coverage': 'Copertura',
    'Which commands have a component, and which inputs each still lacks.':
        'Quali comandi hanno un componente, e quali ingressi mancano ancora a ciascuno.',
    'See the coverage': 'Vedi la copertura',
    'Videos': 'Video',
    'Watch it work': 'Guardalo all\'opera',
    'The Archicad and Grasshopper tutorials.': 'I tutorial su Archicad e Grasshopper.',
    'Watch the tutorials': 'Guarda i tutorial'
});
