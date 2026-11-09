/* Français: the API limits page (limits/index.html). The Evidence column (headers and function names)
   stays English. See ../README.md. */
(window.TAPIR_I18N = window.TAPIR_I18N || []).push ({
    'What the Archicad API can\'t do | Tapir': 'Ce que l’API Archicad ne sait pas faire | Tapir',
    'What people ask for that Graphisoft\'s Archicad API does not offer, such as undo or the clipboard, each with what to do instead and the DevKit evidence.':
        'Ce que l’on demande et que l’API Archicad de Graphisoft n’offre pas, comme annuler ou le presse-papiers, chacun avec ce qu’il faut faire à la place et la preuve dans le DevKit.',
    '<a href="../">Tapir</a> / <a href="../docs/">Docs</a> / API limits': '<a href="../">Tapir</a> / <a href="../docs/">Docs</a> / Limites de l’API',
    'What the Archicad API <em>can\'t do.</em>': 'Ce que l’API Archicad <em>ne sait pas faire.</em>',
    'Tapir uses the Archicad API that Graphisoft publishes for add-ons, and covers all of it that can be a JSON command. What that API does not offer, Tapir cannot offer either. This page lists what people ask for that is not in it, with what to do instead.':
        'Tapir utilise l’API Archicad que Graphisoft publie pour les add-ons, et en couvre tout ce qui peut devenir une commande JSON. Ce que cette API n’offre pas, Tapir ne peut pas l’offrir non plus. Cette page liste ce que l’on demande et qui n’y figure pas, avec ce qu’il faut faire à la place.',

    // The list
    'The <em>list.</em>': 'La <em>liste.</em>',
    'It is also what Tapir would like the Archicad API to offer, and the list Tapir sends to Graphisoft. Every row was checked in the headers of the Archicad API DevKits of Archicad 25 to 30, the versions Tapir is built for; the Evidence column names the headers and functions.':
        'C’est aussi ce que Tapir aimerait que l’API Archicad offre, et la liste que Tapir envoie à Graphisoft. Chaque ligne a été vérifiée dans les en-têtes des DevKits de l’API Archicad d’Archicad 25 à 30, les versions pour lesquelles Tapir est compilé ; la colonne Preuve nomme les en-têtes et les fonctions.',
    'What people want': 'Ce que l’on veut',
    'Status': 'Statut',
    'What to do instead': 'Que faire à la place',
    'Evidence': 'Preuve',
    'Issue': 'Issue',
    'Not possible': 'Impossible',
    'Partly possible': 'Possible en partie',
    'Workaround': 'Contournement',
    'Nothing yet': 'Rien pour l’instant',

    // Elements
    'Elements': 'Éléments',
    'Explode an element into its parts, as Edit &gt; Reshape &gt; Explode into Current View does':
        'Décomposer un élément en ses parties, comme le fait Edit &gt; Reshape &gt; Explode into Current View',
    'A camera with a name of its own, or an axonometric camera': 'Une caméra avec son propre nom, ou une caméra axonométrique',
    'Name the Camera Set the camera is in. For an axonometric view, set the 3D window with <a href="../archicad-addon/#Set3DProjectionSettings">Set3DProjectionSettings</a>.':
        'Nommez le Camera Set qui contient la caméra. Pour une vue axonométrique, réglez la fenêtre 3D avec <a href="../archicad-addon/#Set3DProjectionSettings">Set3DProjectionSettings</a>.',
    'Remove one direction of a trim between two roofs or shells that trim each other':
        'Supprimer une seule direction d\'un ajustement entre deux toits ou coques qui s\'ajustent l\'un l\'autre',
    'Remove the trim with <a href="../archicad-addon/#RemoveElementTrims">RemoveElementTrims</a>, which removes both directions, then trim the other direction again with <a href="../archicad-addon/#TrimElements">TrimElements</a>.':
        'Supprimez l\'ajustement avec <a href="../archicad-addon/#RemoveElementTrims">RemoveElementTrims</a>, qui supprime les deux directions, puis ajustez à nouveau l\'autre direction avec <a href="../archicad-addon/#TrimElements">TrimElements</a>.',
    'Read which slab or stair edge a railing is linked to, or link a railing to one':
        'Lire à quel bord de dalle ou d\'escalier un garde-corps est lié, ou lier un garde-corps à l\'un d\'eux',
    'Read the slab\'s outline with <a href="../archicad-addon/#GetDetailsOfElements">GetDetailsOfElements</a> and create a static railing along the edge with <a href="../archicad-addon/#CreateRailings">CreateRailings</a>. When the slab changes, delete the railing and create it again.':
        'Lisez le contour de la dalle avec <a href="../archicad-addon/#GetDetailsOfElements">GetDetailsOfElements</a> et créez un garde-corps statique le long du bord avec <a href="../archicad-addon/#CreateRailings">CreateRailings</a>. Quand la dalle change, supprimez le garde-corps et créez-le à nouveau.',

    // Navigator and views
    'Navigator and views': 'Navigateur et vues',
    'Add an item to a publisher set': 'Ajouter un élément à un publisher set',
    'Add the items in Archicad\'s Organizer, then publish the set with <a href="../archicad-addon/#PublishPublisherSet">PublishPublisherSet</a>.':
        'Ajoutez les éléments dans l’Organizer d’Archicad, puis publiez le set avec <a href="../archicad-addon/#PublishPublisherSet">PublishPublisherSet</a>.',
    'Change, rename or create a stored Model View Options combination': 'Modifier, renommer ou créer une combinaison enregistrée de Model View Options',
    'Read the combinations with <a href="../archicad-addon/#GetModelViewOptionDetails">GetModelViewOptionDetails</a>; change the options of the current window with <a href="../archicad-addon/#SetViewOptions">SetViewOptions</a>.':
        'Lisez les combinaisons avec <a href="../archicad-addon/#GetModelViewOptionDetails">GetModelViewOptionDetails</a> ; modifiez les options de la fenêtre active avec <a href="../archicad-addon/#SetViewOptions">SetViewOptions</a>.',

    // Project and files
    'Project and files': 'Projet et fichiers',
    'Undo or redo, as Edit &gt; Undo does': 'Annuler ou rétablir, comme Edit &gt; Undo',
    'Most Tapir commands change the model in an undo step, so the user can undo them in Archicad; the command reference says which cannot be undone. Save the project with <a href="../archicad-addon/#SaveProject">SaveProject</a> before a script that may need to be taken back.':
        'La plupart des commandes Tapir modifient le modèle en une étape d’annulation, que l’utilisateur peut donc annuler dans Archicad ; la référence des commandes indique celles qui ne peuvent pas l’être. Enregistrez le projet avec <a href="../archicad-addon/#SaveProject">SaveProject</a> avant un script que vous pourriez devoir défaire.',
    'Copy and paste through the clipboard': 'Copier et coller par le presse-papiers',
    'In a project, copy 2D elements to another view with <a href="../archicad-addon/#CopyElementsToDatabase">CopyElementsToDatabase</a>. Between projects, save the elements as a module with <a href="../archicad-addon/#SaveAsModuleFile">SaveAsModuleFile</a>, place it in the other project with <a href="../archicad-addon/#CreateHotlinkNodes">CreateHotlinkNodes</a> and <a href="../archicad-addon/#CreateHotlinkInstances">CreateHotlinkInstances</a>, and, to keep plain elements, <a href="../archicad-addon/#BreakHotlinkNodes">BreakHotlinkNodes</a>.':
        'Dans un projet, copiez des éléments 2D vers une autre vue avec <a href="../archicad-addon/#CopyElementsToDatabase">CopyElementsToDatabase</a>. Entre projets, enregistrez les éléments comme module avec <a href="../archicad-addon/#SaveAsModuleFile">SaveAsModuleFile</a>, placez-le dans l’autre projet avec <a href="../archicad-addon/#CreateHotlinkNodes">CreateHotlinkNodes</a> et <a href="../archicad-addon/#CreateHotlinkInstances">CreateHotlinkInstances</a> puis, pour garder de simples éléments, <a href="../archicad-addon/#BreakHotlinkNodes">BreakHotlinkNodes</a>.',
    'Merge a file into the project, as File &gt; Interoperability &gt; Merge does': 'Fusionner un fichier dans le projet, comme le fait File &gt; Interoperability &gt; Merge',
    'Place the file as a hotlink with <a href="../archicad-addon/#CreateHotlinkNodes">CreateHotlinkNodes</a> and <a href="../archicad-addon/#CreateHotlinkInstances">CreateHotlinkInstances</a>, then <a href="../archicad-addon/#BreakHotlinkNodes">BreakHotlinkNodes</a>.':
        'Placez le fichier comme hotlink avec <a href="../archicad-addon/#CreateHotlinkNodes">CreateHotlinkNodes</a> et <a href="../archicad-addon/#CreateHotlinkInstances">CreateHotlinkInstances</a>, puis <a href="../archicad-addon/#BreakHotlinkNodes">BreakHotlinkNodes</a>.',

    // Documents and publishing
    'Documents and publishing': 'Documents et publication',
    'Read the rows of an Interactive Schedule': 'Lire les lignes d’un Interactive Schedule',
    'Build the list yourself: find the elements with <a href="../archicad-addon/#GetElementsByType">GetElementsByType</a> and read their values with <a href="../archicad-addon/#GetPropertyValuesOfElements">GetPropertyValuesOfElements</a> or <a href="../archicad-addon/#GetQuantitiesOfElements">GetQuantitiesOfElements</a>. The <a href="../scripts/">built-in scripts</a> Room Finishes Schedule and Material Take-Off build such lists.':
        'Construisez la liste vous-même : trouvez les éléments avec <a href="../archicad-addon/#GetElementsByType">GetElementsByType</a> et lisez leurs valeurs avec <a href="../archicad-addon/#GetPropertyValuesOfElements">GetPropertyValuesOfElements</a> ou <a href="../archicad-addon/#GetQuantitiesOfElements">GetQuantitiesOfElements</a>. Les <a href="../scripts/">scripts intégrés</a> Room Finishes Schedule et Material Take-Off construisent de telles listes.',
    'Export an Interactive Schedule': 'Exporter un Interactive Schedule',
    'Put the schedule in a publisher set in Archicad once, then publish that set with <a href="../archicad-addon/#PublishPublisherSet">PublishPublisherSet</a>.':
        'Placez une fois le tableau dans un publisher set dans Archicad, puis publiez ce set avec <a href="../archicad-addon/#PublishPublisherSet">PublishPublisherSet</a>.',

    // Archicad's interface
    'Archicad\'s interface': 'L’interface d’Archicad',
    'Read the text of the Session Report window': 'Lire le texte de la fenêtre Session Report',
    'Open the window for the user with <a href="../archicad-addon/#ChangeWindow">ChangeWindow</a> and windowType \'Report\' (from Archicad 26).':
        'Ouvrez la fenêtre pour l’utilisateur avec <a href="../archicad-addon/#ChangeWindow">ChangeWindow</a> et windowType \'Report\' (à partir d’Archicad 26).',
    'Get the outcome of an Action Center check': 'Obtenir le résultat d’une vérification de l’Action Center',
    'Run the check with <a href="../archicad-addon/#RunActionCenterChecks">RunActionCenterChecks</a>; Archicad shows the Action Center when it found an issue. For the hotlink check, read the state of each source with <a href="../archicad-addon/#GetHotlinkSourceStatus">GetHotlinkSourceStatus</a> (from Archicad 27).':
        'Lancez la vérification avec <a href="../archicad-addon/#RunActionCenterChecks">RunActionCenterChecks</a> ; Archicad affiche l’Action Center s’il a trouvé un problème. Pour la vérification des hotlinks, lisez l’état de chaque source avec <a href="../archicad-addon/#GetHotlinkSourceStatus">GetHotlinkSourceStatus</a> (à partir d’Archicad 27).',

    'Found something else the Archicad API cannot do? Say so in an issue, with the DevKit headers you checked, and it is added here. <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/issues/new/choose">Open an issue</a>':
        'Vous avez trouvé autre chose que l’API Archicad ne sait pas faire ? Signalez-le dans une issue, avec les en-têtes du DevKit que vous avez vérifiés, et ce sera ajouté ici. <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/issues/new/choose">Ouvrir une issue</a>'
});
