/* Français: the Grasshopper page (grasshopper/index.html). See ../README.md.
   Component, input and button names stay English, as Grasshopper shows them. */
(window.TAPIR_I18N = window.TAPIR_I18N || []).push ({
    'Archicad Grasshopper live connection | Tapir': 'Connexion en direct entre Archicad et Grasshopper | Tapir',
    'Connect Grasshopper live to a running Archicad with the Tapir plugin: read, change and create Archicad elements from Rhino geometry, without writing code.':
        'Reliez Grasshopper en direct à un Archicad ouvert avec le plugin Tapir : lisez, modifiez et créez des éléments Archicad à partir de géométrie Rhino, sans écrire de code.',
    '<a href="../">Tapir</a> / <a href="../docs/">Docs</a> / Grasshopper':
        '<a href="../">Tapir</a> / <a href="../docs/">Docs</a> / Grasshopper',
    'Grasshopper, <em>visually.</em>': 'Grasshopper, <em>visuellement.</em>',
    'The Tapir plugin connects Grasshopper to a running Archicad: read elements and their settings, change them, and create new ones from Rhino geometry, without writing code.':
        'Le plugin Tapir relie Grasshopper à un Archicad ouvert : lisez les éléments et leurs paramètres, modifiez-les et créez-en de nouveaux à partir de géométrie Rhino, sans écrire de code.',

    // What the components do
    'What the components <em>do.</em>': 'Ce que font les <em>composants.</em>',
    'Each component sends a Tapir command to Archicad and gives you the answer as Grasshopper data. You find them on the <strong>Tapir</strong> tab, grouped as in Archicad.':
        'Chaque composant envoie une commande Tapir à Archicad et vous rend la réponse sous forme de données Grasshopper. Vous les trouvez dans l\'onglet <strong>Tapir</strong>, groupés comme dans Archicad.',
    '<b>READ</b><strong>Elements and their data</strong><span>Elements by type, story or selection; their properties, classifications, settings and geometry.</span>':
        '<b>LIRE</b><strong>Les éléments et leurs données</strong><span>Les éléments par type, étage ou sélection ; leurs propriétés, classifications, paramètres et géométrie.</span>',
    '<b>CHANGE</b><strong>What you pass in</strong><span>Property values, classifications, layers, IDs and tool settings of the elements you pass in.</span>':
        '<b>MODIFIER</b><strong>Ce que vous transmettez</strong><span>Valeurs de propriétés, classifications, calques, ID et paramètres d\'outil des éléments transmis.</span>',
    '<b>CREATE</b><strong>From Rhino geometry</strong><span>Walls, doors, windows, railings, level dimensions and more, from points and curves.</span>':
        '<b>CRÉER</b><strong>À partir de géométrie Rhino</strong><span>Murs, portes, fenêtres, garde-corps, cotes de niveau et plus, à partir de points et de courbes.</span>',
    '<b>PROJECT</b><strong>The rest of the file</strong><span>Stories, attributes, navigator views, layouts, issues, favorites and libraries.</span>':
        '<b>PROJET</b><strong>Le reste du fichier</strong><span>Étages, attributs, vues du navigateur, mises en page, annotations, favoris et bibliothèques.</span>',
    'Which Tapir commands have a component, and which settings each covers, is on the <a href="../gh-coverage/">Grasshopper coverage</a> page.':
        'Les commandes Tapir qui ont un composant, et les paramètres que couvre chacun, sont sur la page <a href="../gh-coverage/">Couverture Grasshopper</a>.',

    // Tool components
    'Tool components': 'Composants d\'outil',
    'One per Archicad <em>tool.</em>': 'Un par <em>outil</em> Archicad.',
    'A tool component works like the Archicad tool of the same name. One JSON input and one JSON output per panel of its settings dialog, keyed by the labels you see there, such as <code>Wall Height</code>.':
        'Un composant d\'outil fonctionne comme l\'outil Archicad du même nom. Une entrée JSON et une sortie JSON par panneau de sa boîte de dialogue de paramètres, avec pour clés les libellés anglais que vous y voyez, comme <code>Wall Height</code>.',
    'Placed by <code>StartPoint</code> and <code>EndPoint</code>.': 'Placé par <code>StartPoint</code> et <code>EndPoint</code>.',
    'Placed in an <code>OwnerWall</code> at a <code>CenterOffset</code>.': 'Placée dans un <code>OwnerWall</code> à un <code>CenterOffset</code>.',
    'Placed along a <code>ReferenceLine</code>.': 'Placé le long d\'une <code>ReferenceLine</code>.',
    'Placed at a <code>Position</code>, optionally on a parent element.': 'Placée à une <code>Position</code>, éventuellement sur un élément parent.',
    '<b>REFRESH</b><strong>Reads</strong><span>Reads the elements in <code>ElementGuid</code> and fills every output.</span>':
        '<b>REFRESH</b><strong>Lit</strong><span>Lit les éléments de <code>ElementGuid</code> et remplit chaque sortie.</span>',
    '<b>EXECUTE</b><strong>Writes</strong><span>Writes the given settings to those elements. Only the keys you give are written.</span>':
        '<b>EXECUTE</b><strong>Écrit</strong><span>Écrit les paramètres donnés dans ces éléments. Seules les clés fournies sont écrites.</span>',
    '<b>CREATE</b><strong>Makes new elements</strong><span>From the geometry inputs, starting from the tool\'s defaults or from a <code>FavoriteName</code>.</span>':
        '<b>CREATE</b><strong>Crée de nouveaux éléments</strong><span>À partir des entrées de géométrie, en partant des valeurs par défaut de l\'outil ou d\'un <code>FavoriteName</code>.</span>',
    'Nothing is written to Archicad until you press Execute or Create. A JSON output read by Refresh can be fed straight back into the same input. Archicad has no command to change railings or level dimensions after they are placed, so for those Execute only writes the ID, layer and draw order.':
        'Rien n\'est écrit dans Archicad tant que vous n\'appuyez pas sur Execute ou Create. Une sortie JSON lue par Refresh peut être renvoyée telle quelle dans la même entrée. Archicad n\'a pas de commande pour modifier les garde-corps ou les cotes de niveau une fois placés ; pour eux, Execute n\'écrit donc que l\'ID, le calque et l\'ordre d\'affichage.',
    '<strong class="callout-label"><span aria-hidden="true">ⓘ</span> Note</strong> Walls remembers the walls its Create made, in the <code>.gh</code> file. The next Create updates them instead of making new ones. Turn this off, or forget them, in the component\'s menu.':
        '<strong class="callout-label"><span aria-hidden="true">ⓘ</span> Remarque</strong> Walls retient les murs créés par son Create, dans le fichier <code>.gh</code>. Le Create suivant les met à jour au lieu d\'en créer de nouveaux. Désactivez cela, ou faites-les oublier, dans le menu du composant.',

    // Install
    'Install': 'Installation',
    'Install the Tapir Add-On in Archicad': 'Installer l\'Add-On Tapir dans Archicad',
    'The components talk to Archicad through the Add-On. <a href="../start/#install">Get started</a> shows how.':
        'Les composants communiquent avec Archicad par l\'Add-On. <a href="../start/#install">Démarrer</a> montre comment l\'installer.',
    'Install the <span class="mono">tapir</span> package in Rhino': 'Installer le paquet <span class="mono">tapir</span> dans Rhino',
    'In Rhino 7 or 8, open <em>Tools &gt; Package Manager</em> (or type <code>PackageManager</code>), search for <strong>tapir</strong> and click Install. Restart Rhino.':
        'Dans Rhino 7 ou 8, ouvrez <em>Tools &gt; Package Manager</em> (ou tapez <code>PackageManager</code>), cherchez <strong>tapir</strong> et cliquez sur Install. Redémarrez Rhino.',
    'Open Archicad, then Grasshopper': 'Ouvrir Archicad, puis Grasshopper',
    'With a project open in Archicad, start Grasshopper. The <strong>Tapir</strong> tab appears in its toolbar. With more than one Archicad open, give the port of the one you want to the <strong>ConnectArchicad</strong> component (<em>Tapir &gt; General</em>); the first Archicad uses 19723.':
        'Avec un projet ouvert dans Archicad, lancez Grasshopper. L\'onglet <strong>Tapir</strong> apparaît dans sa barre d\'outils. Si plusieurs Archicad sont ouverts, donnez le port de celui que vous voulez au composant <strong>ConnectArchicad</strong> (<em>Tapir &gt; General</em>) ; le premier Archicad utilise 19723.',

    // A first definition
    'A first definition': 'Une première définition',
    'One <em>wall.</em>': 'Un <em>mur.</em>',
    'Build this in an empty Grasshopper canvas, with a project open in Archicad:':
        'Construisez ceci dans un canevas Grasshopper vide, avec un projet ouvert dans Archicad :',
    'Place two <strong>Point</strong> parameters and set them to 0,0,0 and 5,0,0. Archicad reads the numbers as metres.':
        'Placez deux paramètres <strong>Point</strong> et réglez-les sur 0,0,0 et 5,0,0. Archicad lit les nombres en mètres.',
    'Place the <strong>Walls</strong> component from <em>Tapir &gt; Design Tools</em>. Connect the points to <code>StartPoint</code> and <code>EndPoint</code>.':
        'Placez le composant <strong>Walls</strong> de <em>Tapir &gt; Design Tools</em>. Reliez les points à <code>StartPoint</code> et <code>EndPoint</code>.',
    'Place a <strong>Panel</strong> with the text on the right and connect it to <code>GeometryAndPositioning_JSON</code>.':
        'Placez un <strong>Panel</strong> contenant le texte de droite et reliez-le à <code>GeometryAndPositioning_JSON</code>.',
    'Press <strong>Create</strong>. A 5 m wall appears in Archicad, 3 m high and 30 cm thick.':
        'Appuyez sur <strong>Create</strong>. Un mur de 5 m apparaît dans Archicad, haut de 3 m et épais de 30 cm.',
    'Move a point and press Create again: the same wall moves. Change the panel and press Create: the same wall changes.':
        'Déplacez un point et appuyez de nouveau sur Create : le même mur se déplace. Modifiez le panel et appuyez sur Create : le même mur change.',
    'Press <strong>Refresh</strong> to see every setting of the wall in the JSON outputs. Copy a key from there to set it the next time.':
        'Appuyez sur <strong>Refresh</strong> pour voir chaque paramètre du mur dans les sorties JSON. Copiez-y une clé pour la définir la fois suivante.',

    // Next step
    'Next step': 'Étape suivante',
    'Install the Add-On': 'Installer l\'Add-On',
    'Install Tapir and the <span class="mono">tapir</span> package, then try the wall above.':
        'Installez Tapir et le paquet <span class="mono">tapir</span>, puis essayez le mur ci-dessus.',
    'Get started': 'Démarrer',
    'Coverage': 'Couverture',
    'Which commands have a component, and which inputs each still lacks.':
        'Les commandes qui ont un composant, et les entrées qui manquent encore à chacun.',
    'See the coverage': 'Voir la couverture',
    'Videos': 'Vidéos',
    'Watch it work': 'Le voir à l\'œuvre',
    'The Archicad and Grasshopper tutorials.': 'Les tutoriels Archicad et Grasshopper.',
    'Watch the tutorials': 'Voir les tutoriels'
});
