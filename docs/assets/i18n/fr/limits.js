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
    'Not documented': 'Non documenté',

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
    'Turn off the automatic story visibility of a railing, so the stories it is shown on can be set':
        'Désactiver la visibilité automatique par niveau d\'un garde-corps, pour pouvoir choisir les niveaux où il est affiché',
    'Set the Floor Plan Display, Show Projection, uncut and overhead lines or the cut line type of a mesh':
        'Définir le Floor Plan Display, le Show Projection, les lignes non coupées et au-dessus ou le type de ligne de la coupe d\'un maillage',
    'Read or set the floor plan symbol of a railing: its break mark, which symbol components it shows, their 2D attributes and the symbol overrides':
        'Lire ou régler le symbole en plan d\'un garde-corps : sa ligne de coupure, les composants du symbole qu\'il affiche, leurs attributs 2D et les remplacements du symbole',
    'Read the slab\'s outline with <a href="../archicad-addon/#GetDetailsOfElements">GetDetailsOfElements</a> and create a static railing along the edge with <a href="../archicad-addon/#CreateRailings">CreateRailings</a>. When the slab changes, delete the railing and create it again.':
        'Lisez le contour de la dalle avec <a href="../archicad-addon/#GetDetailsOfElements">GetDetailsOfElements</a> et créez un garde-corps statique le long du bord avec <a href="../archicad-addon/#CreateRailings">CreateRailings</a>. Quand la dalle change, supprimez le garde-corps et créez-le à nouveau.',
    'Move a window or door into another wall': 'Déplacer une fenêtre ou une porte dans un autre mur',
    'Delete it and create it in the other wall with <a href="../archicad-addon/#CreateWindows">CreateWindows</a> or <a href="../archicad-addon/#CreateDoors">CreateDoors</a>; a favorite (<code>favoriteName</code>) gives it the same settings.':
        'Supprimez-la et créez-la dans l’autre mur avec <a href="../archicad-addon/#CreateWindows">CreateWindows</a> ou <a href="../archicad-addon/#CreateDoors">CreateDoors</a> ; un favori (<code>favoriteName</code>) lui donne les mêmes réglages.',
    'Move a skylight onto another roof or shell': 'Déplacer une fenêtre de toit sur un autre toit ou une autre coque',
    'Delete it and create it on the other roof or shell with <a href="../archicad-addon/#CreateSkylights">CreateSkylights</a>.':
        'Supprimez-la et créez-la sur l’autre toit ou l’autre coque avec <a href="../archicad-addon/#CreateSkylights">CreateSkylights</a>.',
    'Turn an automatic zone into a manual one': 'Transformer une zone automatique en zone manuelle',
    'Delete the zone and create it again as a manual zone, from its polygon, with <a href="../archicad-addon/#CreateZones">CreateZones</a>.':
        'Supprimez la zone et créez-la à nouveau comme zone manuelle, à partir de son polygone, avec <a href="../archicad-addon/#CreateZones">CreateZones</a>.',
    'Change the height of a slanted curtain wall': 'Changer la hauteur d’un mur-rideau incliné',
    'Create it again with <a href="../archicad-addon/#CreateCurtainWalls">CreateCurtainWalls</a>, with its <code>angle</code> and the new <code>height</code>.':
        'Créez-le à nouveau avec <a href="../archicad-addon/#CreateCurtainWalls">CreateCurtainWalls</a>, avec son <code>angle</code> et la nouvelle <code>height</code>.',
    'Move the lines or the arc of an angle dimension': 'Déplacer les lignes ou l’arc d’une cote angulaire',
    'Delete it and create it again with <a href="../archicad-addon/#CreateAngleDimensions">CreateAngleDimensions</a>.':
        'Supprimez-la et créez-la à nouveau avec <a href="../archicad-addon/#CreateAngleDimensions">CreateAngleDimensions</a>.',
    'Set the position or the offset of a stair\'s walking line': 'Définir la position ou le décalage de la ligne de foulée d’un escalier',
    'Anchor the sill or header of a window or door to the project zero or to a reference level':
        'Ancrer l’allège ou le linteau d’une fenêtre ou d’une porte au zéro du projet ou à un niveau de référence',
    'Anchor it to a story with <code>verticalLink</code> <code>SillToStory</code> or <code>HeaderToStory</code> in <a href="../archicad-addon/#ModifyWindows">ModifyWindows</a> or <a href="../archicad-addon/#ModifyDoors">ModifyDoors</a>, and work out the height from the story level (<a href="../archicad-addon/#GetStories">GetStories</a>).':
        'Ancrez-le à un niveau avec <code>verticalLink</code> <code>SillToStory</code> ou <code>HeaderToStory</code> dans <a href="../archicad-addon/#ModifyWindows">ModifyWindows</a> ou <a href="../archicad-addon/#ModifyDoors">ModifyDoors</a>, et calculez la hauteur à partir de la cote du niveau (<a href="../archicad-addon/#GetStories">GetStories</a>).',
    'Read or set the joint type (Vertical Cut and the others) and the dx offset of a stair structure':
        'Lire ou régler le type de jonction (Vertical Cut et les autres) et le décalage dx d’une structure d’escalier',
    'Change the baseline of an existing stair (move a flight end, a landing or a winder) and keep its GUID':
        'Modifier la ligne de base d’un escalier existant (déplacer l’extrémité d’une volée, un palier ou un tournant) en gardant son GUID',
    'Recreate the stair on the new baseline with <a href="../archicad-addon/#RecreateStairs">RecreateStairs</a>, which keeps its settings, Element ID, classifications, categories, properties and solid element operations, and returns the old and the new GUID. Its labels are lost.':
        'Recréez l’escalier sur la nouvelle ligne de base avec <a href="../archicad-addon/#RecreateStairs">RecreateStairs</a>, qui garde ses paramètres, son ID d’élément, ses classifications, ses catégories, ses propriétés et ses opérations sur éléments solides, et renvoie l’ancien et le nouveau GUID. Ses étiquettes sont perdues.',
    'Set the left and right boundaries or the walking line of a stair (a custom outline, or a walking line off the middle of the flight)':
        'Régler les contours gauche et droit ou la ligne de foulée d’un escalier (un contour personnalisé, ou une ligne de foulée hors du milieu de la volée)',
    'Archicad lays them out from the baseline and the flight width: change the baseline with <a href="../archicad-addon/#RecreateStairs">RecreateStairs</a> or the flight width with <a href="../archicad-addon/#ModifyStairs">ModifyStairs</a>. The walking line stays in the middle of the flight. <a href="../archicad-addon/#GetDetailsOfElements">GetDetailsOfElements</a> reads them.':
        'Archicad les trace d’après la ligne de base et la largeur de volée : changez la ligne de base avec <a href="../archicad-addon/#RecreateStairs">RecreateStairs</a> ou la largeur de volée avec <a href="../archicad-addon/#ModifyStairs">ModifyStairs</a>. La ligne de foulée reste au milieu de la volée. <a href="../archicad-addon/#GetDetailsOfElements">GetDetailsOfElements</a> les lit.',
    'Set a label’s optimised position, “use text pen as uniform pen” of a symbol label, or a perpendicular text direction':
        'Régler la position optimisée d’une étiquette, « use text pen as uniform pen » d’une étiquette symbole, ou une direction de texte perpendiculaire',
    'Set the text direction (parallel, horizontal, vertical or a custom angle) with textWay in <a href="../archicad-addon/#CreateLabels">CreateLabels</a> and <a href="../archicad-addon/#ModifyLabels">ModifyLabels</a>. When the symbol label’s library part has a parameter for the other two, set it with <a href="../archicad-addon/#SetGDLParametersOfElements">SetGDLParametersOfElements</a>; otherwise set them in the Label tool settings or a favorite, and create the labels with favoriteName.':
        'Réglez la direction du texte (parallèle, horizontale, verticale ou un angle libre) avec textWay dans <a href="../archicad-addon/#CreateLabels">CreateLabels</a> et <a href="../archicad-addon/#ModifyLabels">ModifyLabels</a>. Quand l’élément de bibliothèque de l’étiquette symbole a un paramètre pour les deux autres, réglez-le avec <a href="../archicad-addon/#SetGDLParametersOfElements">SetGDLParametersOfElements</a> ; sinon réglez-les dans les paramètres de l’outil Étiquette ou dans un favori, et créez les étiquettes avec favoriteName.',
    'Create a radial dimension of an arc, a circle, a curved wall or a curved slab edge':
        'Créer une cote de rayon d’un arc, d’un cercle, d’un mur courbe ou d’un bord de dalle courbe',
    'Place the radial dimension in Archicad and read it with <a href="../archicad-addon/#GetDetailsOfElements">GetDetailsOfElements</a>, which returns how Archicad stored it. Those readings are what a create command needs.':
        'Placez la cote de rayon dans Archicad et lisez-la avec <a href="../archicad-addon/#GetDetailsOfElements">GetDetailsOfElements</a>, qui renvoie la façon dont Archicad l’a enregistrée. Ce sont ces valeurs dont une commande de création a besoin.',

    // Design options
    'Design options': 'Options de conception',
    'Read which design option, or the Main Model, is the default that new elements go to':
        'Lire quelle option de conception, ou le Main Model, est celle par défaut dans laquelle vont les nouveaux éléments',
    'Create a temporary element, read its design option with <a href="../archicad-addon/#GetDesignOptionForElements">GetDesignOptionForElements</a>, then move it to the Main Model with <a href="../archicad-addon/#MoveElementsToDesignOptions">MoveElementsToDesignOptions</a> (a null GUID as <code>designOptionId</code>) and delete it with <a href="../archicad-addon/#DeleteElements">DeleteElements</a>; an element in an option that is not shown cannot be deleted. Set the default with <a href="../archicad-addon/#SetDefaultDesignOption">SetDefaultDesignOption</a>.':
        'Créez un élément temporaire, lisez son option de conception avec <a href="../archicad-addon/#GetDesignOptionForElements">GetDesignOptionForElements</a>, puis déplacez-le dans le Main Model avec <a href="../archicad-addon/#MoveElementsToDesignOptions">MoveElementsToDesignOptions</a> (un GUID nul comme <code>designOptionId</code>) et supprimez-le avec <a href="../archicad-addon/#DeleteElements">DeleteElements</a> ; un élément d’une option qui n’est pas affichée ne peut pas être supprimé. Définissez l’option par défaut avec <a href="../archicad-addon/#SetDefaultDesignOption">SetDefaultDesignOption</a>.',
    'Read or change which design options the current window shows, as the eye icons of the Design Options palette do':
        'Lire ou changer les options de conception que la fenêtre active affiche, comme le font les icônes en forme d’œil de la palette Design Options',
    'Work with saved views: read and set the combination a View Map view shows with <a href="../archicad-addon/#GetDesignOptionCombinationOfViews">GetDesignOptionCombinationOfViews</a> and <a href="../archicad-addon/#SetDesignOptionCombinationOfViews">SetDesignOptionCombinationOfViews</a>, and the options of a combination with <a href="../archicad-addon/#SetActiveDesignOptionsInCombinations">SetActiveDesignOptionsInCombinations</a>.':
        'Travaillez avec des vues enregistrées : lisez et définissez la combinaison qu’affiche une vue de la View Map avec <a href="../archicad-addon/#GetDesignOptionCombinationOfViews">GetDesignOptionCombinationOfViews</a> et <a href="../archicad-addon/#SetDesignOptionCombinationOfViews">SetDesignOptionCombinationOfViews</a>, et les options d’une combinaison avec <a href="../archicad-addon/#SetActiveDesignOptionsInCombinations">SetActiveDesignOptionsInCombinations</a>.',
    'Duplicate a design option or a design option set, as the duplicate button of the Design Options palette does':
        'Dupliquer une option de conception ou un jeu d’options de conception, comme le fait le bouton de duplication de la palette Design Options',
    'Create the set and its options with <a href="../archicad-addon/#CreateDesignOptionSets">CreateDesignOptionSets</a> and <a href="../archicad-addon/#CreateDesignOptions">CreateDesignOptions</a>. For the elements, list them with <a href="../archicad-addon/#GetElementsOfDesignOptions">GetElementsOfDesignOptions</a>, copy them with <a href="../archicad-addon/#CopyElements">CopyElements</a> and move the copies to the new option with <a href="../archicad-addon/#MoveElementsToDesignOptions">MoveElementsToDesignOptions</a>.':
        'Créez le jeu et ses options avec <a href="../archicad-addon/#CreateDesignOptionSets">CreateDesignOptionSets</a> et <a href="../archicad-addon/#CreateDesignOptions">CreateDesignOptions</a>. Pour les éléments, listez-les avec <a href="../archicad-addon/#GetElementsOfDesignOptions">GetElementsOfDesignOptions</a>, copiez-les avec <a href="../archicad-addon/#CopyElements">CopyElements</a> et déplacez les copies dans la nouvelle option avec <a href="../archicad-addon/#MoveElementsToDesignOptions">MoveElementsToDesignOptions</a>.',
    'Lock or colour the passive elements of design options, as the Design Options palette does':
        'Verrouiller ou colorer les éléments passifs des options de conception, comme le fait la palette Design Options',

    // Navigator and views
    'Navigator and views': 'Navigateur et vues',
    'Add an item to a publisher set': 'Ajouter un élément à un publisher set',
    'Add the items in Archicad\'s Organizer, then publish the set with <a href="../archicad-addon/#PublishPublisherSet">PublishPublisherSet</a>.':
        'Ajoutez les éléments dans l’Organizer d’Archicad, puis publiez le set avec <a href="../archicad-addon/#PublishPublisherSet">PublishPublisherSet</a>.',
    'Rename a publisher set': 'Renommer un publisher set',
    'Rename the set in Archicad\'s Organizer. <a href="../archicad-addon/#GetPublisherSets">GetPublisherSets</a> reads the names, and <a href="../archicad-addon/#PublishPublisherSet">PublishPublisherSet</a> publishes a set by its name.':
        'Renommez le set dans l’Organizer d’Archicad. <a href="../archicad-addon/#GetPublisherSets">GetPublisherSets</a> lit les noms, et <a href="../archicad-addon/#PublishPublisherSet">PublishPublisherSet</a> publie un set par son nom.',
    'Change, rename or create a stored Model View Options combination': 'Modifier, renommer ou créer une combinaison enregistrée de Model View Options',
    'Read the combinations with <a href="../archicad-addon/#GetModelViewOptionDetails">GetModelViewOptionDetails</a>; change the options of the current window with <a href="../archicad-addon/#SetViewOptions">SetViewOptions</a>.':
        'Lisez les combinaisons avec <a href="../archicad-addon/#GetModelViewOptionDetails">GetModelViewOptionDetails</a> ; modifiez les options de la fenêtre active avec <a href="../archicad-addon/#SetViewOptions">SetViewOptions</a>.',
    'Make a 3D style the current one': 'Faire d’un style 3D le style courant',
    'Give a view the 3D style with <a href="../archicad-addon/#SetViewSettings">SetViewSettings</a> (<code>d3styleName</code>) and open that view with <a href="../archicad-addon/#ChangeWindow">ChangeWindow</a> (<code>navigatorItemId</code>, from Archicad 27).':
        'Donnez le style 3D à une vue avec <a href="../archicad-addon/#SetViewSettings">SetViewSettings</a> (<code>d3styleName</code>) et ouvrez cette vue avec <a href="../archicad-addon/#ChangeWindow">ChangeWindow</a> (<code>navigatorItemId</code>, à partir d’Archicad 27).',
    'Read or set the floor plan cut plane or the structural analytical model settings saved in a view':
        'Lire ou régler le plan de coupe du plan d’étage ou les paramètres du modèle analytique structurel enregistrés dans une vue',
    'Set them in the view\'s View Settings in Archicad. The project\'s floor plan cut plane can be set with <a href="../archicad-addon/#SetProjectPreferences">SetProjectPreferences</a>.':
        'Réglez-les dans les View Settings de la vue dans Archicad. Le plan de coupe du plan d’étage du projet peut être réglé avec <a href="../archicad-addon/#SetProjectPreferences">SetProjectPreferences</a>.',
    'Tell a clone folder of the View Map from a normal folder': 'Distinguer un dossier clone de la View Map d’un dossier normal',
    'Give the clone folders a name or ID of their own and find them by it in <a href="../archicad-addon/#GetNavigatorItemTree">GetNavigatorItemTree</a>.':
        'Donnez aux dossiers clones un nom ou un ID qui leur est propre et retrouvez-les ainsi dans <a href="../archicad-addon/#GetNavigatorItemTree">GetNavigatorItemTree</a>.',
    'Change the settings of an existing Layout Book subset (prefix, numbering style, start at, ...)':
        'Modifier les paramètres d’un sous-ensemble existant du Layout Book (préfixe, style de numérotation, début, ...)',
    'Read the subset with <a href="../archicad-addon/#GetLayoutSubsetSettings">GetLayoutSubsetSettings</a>, create a subset with the new settings with <a href="../archicad-addon/#CreateLayoutSubset">CreateLayoutSubset</a> and move the layouts into it with <a href="../archicad-addon/#MoveNavigatorItem">MoveNavigatorItem</a>. The old, empty subset is deleted in Archicad.':
        'Lisez le sous-ensemble avec <a href="../archicad-addon/#GetLayoutSubsetSettings">GetLayoutSubsetSettings</a>, créez un sous-ensemble avec les nouveaux paramètres avec <a href="../archicad-addon/#CreateLayoutSubset">CreateLayoutSubset</a> et déplacez-y les layouts avec <a href="../archicad-addon/#MoveNavigatorItem">MoveNavigatorItem</a>. L’ancien sous-ensemble, désormais vide, se supprime dans Archicad.',

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
    'Delete one offset modifier of a profile': 'Supprimer un seul modificateur de décalage d’un profil',
    'Rebuild the profile with <a href="../archicad-addon/#CreateProfiles">CreateProfiles</a>\' <code>replaceSkins</code> and <code>newSkins</code>, which drops every modifier, then add the ones to keep again with <code>newProfileModifiers</code> (Archicad 27 and later).':
        'Reconstruisez le profil avec <code>replaceSkins</code> et <code>newSkins</code> de <a href="../archicad-addon/#CreateProfiles">CreateProfiles</a>, ce qui supprime tous les modificateurs, puis ajoutez à nouveau ceux à garder avec <code>newProfileModifiers</code> (Archicad 27 et suivants).',
    'Import an IFC translator from a file into the project, or export one to a file':
        'Importer un traducteur IFC d’un fichier dans le projet, ou en exporter un vers un fichier',
    'Keep the office translator in the project template, so new projects have it, and bring it into older projects once with Import… in the IFC Translators dialog. A script can check that a project has it with <a href="../archicad-addon/#GetIFCExportTranslators">GetIFCExportTranslators</a> and export with it by name with <a href="../archicad-addon/#IFCFileOperation">IFCFileOperation</a>.':
        'Gardez le traducteur du bureau dans le gabarit de projet, pour que les nouveaux projets l’aient, et importez-le une fois dans les projets plus anciens avec Import… dans la boîte de dialogue IFC Translators. Un script peut vérifier qu’un projet l’a avec <a href="../archicad-addon/#GetIFCExportTranslators">GetIFCExportTranslators</a> et exporter avec lui, par son nom, avec <a href="../archicad-addon/#IFCFileOperation">IFCFileOperation</a>.',

    // Documents and publishing
    'Documents and publishing': 'Documents et publication',
    'Read the rows of an Interactive Schedule': 'Lire les lignes d’un Interactive Schedule',
    'Build the list yourself: find the elements with <a href="../archicad-addon/#GetElementsByType">GetElementsByType</a> and read their values with <a href="../archicad-addon/#GetPropertyValuesOfElements">GetPropertyValuesOfElements</a> or <a href="../archicad-addon/#GetQuantitiesOfElements">GetQuantitiesOfElements</a>. The <a href="../scripts/">built-in scripts</a> Room Finishes Schedule and Material Take-Off build such lists.':
        'Construisez la liste vous-même : trouvez les éléments avec <a href="../archicad-addon/#GetElementsByType">GetElementsByType</a> et lisez leurs valeurs avec <a href="../archicad-addon/#GetPropertyValuesOfElements">GetPropertyValuesOfElements</a> ou <a href="../archicad-addon/#GetQuantitiesOfElements">GetQuantitiesOfElements</a>. Les <a href="../scripts/">scripts intégrés</a> Room Finishes Schedule et Material Take-Off construisent de telles listes.',
    'Export an Interactive Schedule': 'Exporter un Interactive Schedule',
    'Put the schedule in a publisher set in Archicad once, then publish that set with <a href="../archicad-addon/#PublishPublisherSet">PublishPublisherSet</a>.':
        'Placez une fois le tableau dans un publisher set dans Archicad, puis publiez ce set avec <a href="../archicad-addon/#PublishPublisherSet">PublishPublisherSet</a>.',
    'Choose the output format (PDF, DWG, DXF …) of a publisher set per call':
        'Choisir le format de sortie (PDF, DWG, DXF …) d’un publisher set à chaque appel',
    'Make one publisher set per format in Archicad’s Organizer once, then publish the one you need with <a href="../archicad-addon/#PublishPublisherSet">PublishPublisherSet</a>; its answer lists the files written (<code>writtenFiles</code>).':
        'Créez une fois un publisher set par format dans l’Organizer d’Archicad, puis publiez celui dont vous avez besoin avec <a href="../archicad-addon/#PublishPublisherSet">PublishPublisherSet</a> ; sa réponse liste les fichiers écrits (<code>writtenFiles</code>).',
    'Create a revision Change without placing a Change Marker, or change the ID or description of an existing Change':
        'Créer un Change de révision sans placer de Change Marker, ou modifier l’ID ou la description d’un Change existant',
    'Create the Change together with its first marker with <a href="../archicad-addon/#CreateChangeMarkers">CreateChangeMarkers</a> (createChange). To change an existing Change, use Archicad’s Change Manager.':
        'Créez le Change avec son premier marqueur grâce à <a href="../archicad-addon/#CreateChangeMarkers">CreateChangeMarkers</a> (createChange). Pour modifier un Change existant, utilisez le Change Manager d’Archicad.',
    'Set the custom scheme fields of a revision Change': 'Renseigner les champs de schéma personnalisés d’un Change de révision',
    'Fill them in Archicad’s Change Manager; <a href="../archicad-addon/#GetRevisionChanges">GetRevisionChanges</a> reads them back.':
        'Remplissez-les dans le Change Manager d’Archicad ; <a href="../archicad-addon/#GetRevisionChanges">GetRevisionChanges</a> les relit.',
    'Add elements to a revision Change, or remove them from it':
        'Ajouter des éléments à un Change de révision, ou les en retirer',
    'Add them in Archicad’s Change Manager; <a href="../archicad-addon/#GetRevisionChangesOfElements">GetRevisionChangesOfElements</a> reads which Changes an element belongs to.':
        'Ajoutez-les dans le Change Manager d’Archicad ; <a href="../archicad-addon/#GetRevisionChangesOfElements">GetRevisionChangesOfElements</a> lit à quels Changes appartient un élément.',

    // Library parts and GDL
    'Library parts and GDL': 'Objets de bibliothèque et GDL',
    'Run a 2D GDL script given as text, as <a href="../archicad-addon/#InterpretGDLScript3D">InterpretGDLScript3D</a> does with a 3D script':
        'Exécuter un script GDL 2D donné sous forme de texte, comme <a href="../archicad-addon/#InterpretGDLScript3D">InterpretGDLScript3D</a> le fait pour un script 3D',
    'Save the script in a library part with <a href="../archicad-addon/#CreateLibraryParts">CreateLibraryParts</a> (or <a href="../archicad-addon/#SetLibraryPartScripts">SetLibraryPartScripts</a>), then draw its 2D symbol with <a href="../archicad-addon/#GetLibraryPart2DPrimitives">GetLibraryPart2DPrimitives</a>.':
        'Enregistrez le script dans un objet de bibliothèque avec <a href="../archicad-addon/#CreateLibraryParts">CreateLibraryParts</a> (ou <a href="../archicad-addon/#SetLibraryPartScripts">SetLibraryPartScripts</a>), puis dessinez son symbole 2D avec <a href="../archicad-addon/#GetLibraryPart2DPrimitives">GetLibraryPart2DPrimitives</a>.',
    'Get the error message and the line number when a GDL script fails':
        'Obtenir le message d’erreur et le numéro de ligne quand un script GDL échoue',
    'Check whether the script runs with <a href="../archicad-addon/#InterpretGDLScript3D">InterpretGDLScript3D</a> (3D) or <a href="../archicad-addon/#GetLibraryPart2DPrimitives">GetLibraryPart2DPrimitives</a> (2D). To see the error, open the library part in Archicad’s GDL editor with <a href="../archicad-addon/#OpenLibraryPartInEditor">OpenLibraryPartInEditor</a> (from Archicad 27).':
        'Vérifiez que le script s’exécute avec <a href="../archicad-addon/#InterpretGDLScript3D">InterpretGDLScript3D</a> (3D) ou <a href="../archicad-addon/#GetLibraryPart2DPrimitives">GetLibraryPart2DPrimitives</a> (2D). Pour voir l’erreur, ouvrez l’objet dans l’éditeur GDL d’Archicad avec <a href="../archicad-addon/#OpenLibraryPartInEditor">OpenLibraryPartInEditor</a> (à partir d’Archicad 27).',

    // Archicad's interface
    'Archicad\'s interface': 'L’interface d’Archicad',
    'Read the text of the Session Report window': 'Lire le texte de la fenêtre Session Report',
    'Open the window for the user with <a href="../archicad-addon/#ChangeWindow">ChangeWindow</a> and windowType \'Report\' (from Archicad 26).':
        'Ouvrez la fenêtre pour l’utilisateur avec <a href="../archicad-addon/#ChangeWindow">ChangeWindow</a> et windowType \'Report\' (à partir d’Archicad 26).',
    'Get the outcome of an Action Center check': 'Obtenir le résultat d’une vérification de l’Action Center',
    'Run the check with <a href="../archicad-addon/#RunActionCenterChecks">RunActionCenterChecks</a>; Archicad shows the Action Center when it found an issue. For the hotlink check, read the state of each source with <a href="../archicad-addon/#GetHotlinkSourceStatus">GetHotlinkSourceStatus</a> (from Archicad 27).':
        'Lancez la vérification avec <a href="../archicad-addon/#RunActionCenterChecks">RunActionCenterChecks</a> ; Archicad affiche l’Action Center s’il a trouvé un problème. Pour la vérification des hotlinks, lisez l’état de chaque source avec <a href="../archicad-addon/#GetHotlinkSourceStatus">GetHotlinkSourceStatus</a> (à partir d’Archicad 27).',
    'Load a new add-on (.apx or .bundle) while Archicad runs, from an add-on or a script':
        'Charger un nouvel add-on (.apx ou .bundle) pendant qu’Archicad tourne, depuis un add-on ou un script',
    'Add it in Archicad\'s Add-On Manager (Options &gt; Add-On Manager &gt; Edit List of Available Add-Ons &gt; Add). An add-on that is already loaded is replaced only after Archicad quits, as the Tapir Installer does when it updates Tapir.':
        'Ajoutez-le dans l’Add-On Manager d’Archicad (Options &gt; Add-On Manager &gt; Edit List of Available Add-Ons &gt; Add). Un add-on déjà chargé n’est remplacé qu’après la fermeture d’Archicad, comme le fait le Tapir Installer quand il met Tapir à jour.',

    'Set the profile modifier values, or the stretched width and height, of a wall, beam or column that uses a profile':
        'Définir les valeurs des modificateurs de profil, ou la largeur et la hauteur étirées, d\'un mur, d\'une poutre ou d\'un poteau qui utilise un profil',
    'Read them with <a href="../archicad-addon/#GetProfileValuesOfElements">GetProfileValuesOfElements</a>. To change them, change the profile with <a href="../archicad-addon/#CreateProfiles">CreateProfiles</a> (a copy for these elements, or the profile itself), or set them in the settings of the element in Archicad.':
        'Lisez-les avec <a href="../archicad-addon/#GetProfileValuesOfElements">GetProfileValuesOfElements</a>. Pour les changer, modifiez le profil avec <a href="../archicad-addon/#CreateProfiles">CreateProfiles</a> (une copie pour ces éléments, ou le profil lui-même), ou réglez-les dans les paramètres de l\'élément dans Archicad.',
    'Read or set how a grid element is shown in sections, elevations and interior elevations: its vertical extent and its markers at the top and bottom':
        'Lire ou définir comment un élément de grille est affiché dans les coupes, les façades et les élévations intérieures : son étendue verticale et ses marqueurs en haut et en bas',
    'Select the grid elements in Archicad and set it for all of them at once in their settings dialog.':
        'Sélectionnez les éléments de grille dans Archicad et réglez-le pour tous à la fois dans leur boîte de dialogue de paramètres.',
    'Read the settings of an IFC translator, such as its type mapping and property mapping, for example to check them as the IFC conformity check of the Missing BIM Data report does':
        'Lire les réglages d\'un traducteur IFC, comme son mappage de types et son mappage de propriétés, par exemple pour les vérifier comme le fait le contrôle de conformité IFC du rapport sur les données BIM manquantes',
    'Check what the translator gives the elements instead: their IFC type with <a href="../archicad-addon/#GetIFCTypeOfElements">GetIFCTypeOfElements</a>, and from Archicad 28 the properties and classification references the preview translator maps to them with <a href="../archicad-addon/#GetIFCPropertiesOfElements">GetIFCPropertiesOfElements</a> and <a href="../archicad-addon/#GetIFCClassificationReferencesOfElements">GetIFCClassificationReferencesOfElements</a>. <a href="../archicad-addon/#GetAllProperties">GetAllProperties</a> shows whether the properties a mapping needs exist in the project.':
        'Vérifiez plutôt ce que le traducteur donne aux éléments : leur type IFC avec <a href="../archicad-addon/#GetIFCTypeOfElements">GetIFCTypeOfElements</a>, et à partir d\'Archicad 28 les propriétés et les références de classification que le traducteur d\'aperçu leur associe avec <a href="../archicad-addon/#GetIFCPropertiesOfElements">GetIFCPropertiesOfElements</a> et <a href="../archicad-addon/#GetIFCClassificationReferencesOfElements">GetIFCClassificationReferencesOfElements</a>. <a href="../archicad-addon/#GetAllProperties">GetAllProperties</a> montre si les propriétés dont un mappage a besoin existent dans le projet.',
    'Find the graphic override rules, Interactive Schedules and SAF translators that refer to a property or classification that no longer exists, as the Missing BIM Data report does':
        'Trouver les règles de remplacement graphique, les nomenclatures interactives et les traducteurs SAF qui renvoient à une propriété ou une classification qui n\'existe plus, comme le fait le rapport sur les données BIM manquantes',
    'For graphic override rules (from Archicad 27): read the criterion of each rule with <a href="../archicad-addon/#GetGraphicOverrideRules">GetGraphicOverrideRules</a> (includeCriterion) and compare the properties and classifications it tests with <a href="../archicad-addon/#GetAllProperties">GetAllProperties</a> and <a href="../archicad-addon/#GetAllClassificationSystems">GetAllClassificationSystems</a>. For Interactive Schedules and SAF translators: nothing yet.':
        'Pour les règles de remplacement graphique (à partir d\'Archicad 27) : lisez le critère de chaque règle avec <a href="../archicad-addon/#GetGraphicOverrideRules">GetGraphicOverrideRules</a> (includeCriterion) et comparez les propriétés et les classifications qu\'il teste avec <a href="../archicad-addon/#GetAllProperties">GetAllProperties</a> et <a href="../archicad-addon/#GetAllClassificationSystems">GetAllClassificationSystems</a>. Pour les nomenclatures interactives et les traducteurs SAF : rien pour l\'instant.',
    'Export and import, all at once, the settings that the dialogs of Archicad save as XML (properties, classifications, the Layout Book, graphic overrides, Model View Options, IFC and DWG translators), to keep several projects in sync':
        'Exporter et importer en une fois les réglages que les dialogues d\'Archicad enregistrent en XML (propriétés, classifications, le Classeur de mises en page, remplacements graphiques, options de vue du modèle, traducteurs IFC et DWG), pour garder plusieurs projets synchronisés',
    'Copy what the API can write, one kind at a time, in one script run against each project: properties and classifications from their XML files with <a href="../archicad-addon/#ImportPropertiesXml">ImportPropertiesXml</a> and <a href="../archicad-addon/#ImportClassificationsXml">ImportClassificationsXml</a>; attributes from another project with <a href="../archicad-addon/#ImportAttributesFromProject">ImportAttributesFromProject</a>; graphic override rules with <a href="../archicad-addon/#GetGraphicOverrideRules">GetGraphicOverrideRules</a> and <a href="../archicad-addon/#CreateGraphicOverrideRules">CreateGraphicOverrideRules</a>; Project Info with <a href="../archicad-addon/#GetProjectInfoFields">GetProjectInfoFields</a> and <a href="../archicad-addon/#SetProjectInfoField">SetProjectInfoField</a>; Favorites with <a href="../archicad-addon/#ExportFavorites">ExportFavorites</a> and <a href="../archicad-addon/#ImportFavorites">ImportFavorites</a>. The Layout Book, Model View Options and the translators: nothing yet.':
        'Copiez ce que l\'API peut écrire, un type à la fois, dans un script lancé sur chaque projet : les propriétés et les classifications depuis leurs fichiers XML avec <a href="../archicad-addon/#ImportPropertiesXml">ImportPropertiesXml</a> et <a href="../archicad-addon/#ImportClassificationsXml">ImportClassificationsXml</a> ; les attributs d\'un autre projet avec <a href="../archicad-addon/#ImportAttributesFromProject">ImportAttributesFromProject</a> ; les règles de remplacement graphique avec <a href="../archicad-addon/#GetGraphicOverrideRules">GetGraphicOverrideRules</a> et <a href="../archicad-addon/#CreateGraphicOverrideRules">CreateGraphicOverrideRules</a> ; les infos du projet avec <a href="../archicad-addon/#GetProjectInfoFields">GetProjectInfoFields</a> et <a href="../archicad-addon/#SetProjectInfoField">SetProjectInfoField</a> ; les favoris avec <a href="../archicad-addon/#ExportFavorites">ExportFavorites</a> et <a href="../archicad-addon/#ImportFavorites">ImportFavorites</a>. Le Classeur de mises en page, les options de vue du modèle et les traducteurs : rien pour l\'instant.',


    'Found something else the Archicad API cannot do? Say so in an issue, with the DevKit headers you checked, and it is added here. <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/issues/new/choose">Open an issue</a>':
        'Vous avez trouvé autre chose que l’API Archicad ne sait pas faire ? Signalez-le dans une issue, avec les en-têtes du DevKit que vous avez vérifiés, et ce sera ajouté ici. <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/issues/new/choose">Ouvrir une issue</a>'
});
