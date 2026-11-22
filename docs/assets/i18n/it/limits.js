/* Italiano: the API limits page (limits/index.html). The Evidence column (headers and function names)
   stays English. See ../README.md. */
(window.TAPIR_I18N = window.TAPIR_I18N || []).push ({
    'What the Archicad API can\'t do | Tapir': 'Cosa non può fare l’API di Archicad | Tapir',
    'What people ask for that Graphisoft\'s Archicad API does not offer, such as undo or the clipboard, each with what to do instead and the DevKit evidence.':
        'Ciò che si chiede e che l’API di Archicad di Graphisoft non offre, come annulla o gli appunti, ognuno con cosa fare invece e la prova nel DevKit.',
    '<a href="../">Tapir</a> / <a href="../docs/">Docs</a> / API limits': '<a href="../">Tapir</a> / <a href="../docs/">Docs</a> / Limiti dell’API',
    'What the Archicad API <em>can\'t do.</em>': 'Cosa l’API di Archicad <em>non può fare.</em>',
    'Tapir uses the Archicad API that Graphisoft publishes for add-ons, and covers all of it that can be a JSON command. What that API does not offer, Tapir cannot offer either. This page lists what people ask for that is not in it, with what to do instead.':
        'Tapir usa l’API di Archicad che Graphisoft pubblica per gli add-on, e ne copre tutto ciò che può diventare un comando JSON. Ciò che quell’API non offre, non può offrirlo nemmeno Tapir. Questa pagina elenca ciò che viene chiesto e che manca, con cosa fare invece.',

    // The list
    'The <em>list.</em>': 'L’<em>elenco.</em>',
    'It is also what Tapir would like the Archicad API to offer, and the list Tapir sends to Graphisoft. Every row was checked in the headers of the Archicad API DevKits of Archicad 25 to 30, the versions Tapir is built for; the Evidence column names the headers and functions.':
        'È anche ciò che Tapir vorrebbe dall’API di Archicad, e l’elenco che Tapir invia a Graphisoft. Ogni riga è stata verificata negli header dei DevKit dell’API di Archicad da Archicad 25 a 30, le versioni per cui Tapir viene compilato; la colonna Prova nomina gli header e le funzioni.',
    'What people want': 'Cosa si vuole',
    'Status': 'Stato',
    'What to do instead': 'Cosa fare invece',
    'Evidence': 'Prova',
    'Issue': 'Issue',
    'Not possible': 'Non possibile',
    'Partly possible': 'Possibile in parte',
    'Workaround': 'Alternativa',
    'Nothing yet': 'Ancora niente',
    'Not documented': 'Non documentato',

    // Elements
    'Elements': 'Elementi',
    'Explode an element into its parts, as Edit &gt; Reshape &gt; Explode into Current View does':
        'Esplodere un elemento nelle sue parti, come fa Edit &gt; Reshape &gt; Explode into Current View',
    'A camera with a name of its own, or an axonometric camera': 'Una camera con un nome proprio, o una camera assonometrica',
    'Name the Camera Set the camera is in. For an axonometric view, set the 3D window with <a href="../archicad-addon/#Set3DProjectionSettings">Set3DProjectionSettings</a>.':
        'Date un nome al Camera Set che contiene la camera. Per una vista assonometrica, impostate la finestra 3D con <a href="../archicad-addon/#Set3DProjectionSettings">Set3DProjectionSettings</a>.',
    'Remove one direction of a trim between two roofs or shells that trim each other':
        'Rimuovere una sola direzione di un taglio tra due coperture o gusci che si tagliano a vicenda',
    'Remove the trim with <a href="../archicad-addon/#RemoveElementTrims">RemoveElementTrims</a>, which removes both directions, then trim the other direction again with <a href="../archicad-addon/#TrimElements">TrimElements</a>.':
        'Rimuovete il taglio con <a href="../archicad-addon/#RemoveElementTrims">RemoveElementTrims</a>, che rimuove entrambe le direzioni, poi tagliate di nuovo l\'altra direzione con <a href="../archicad-addon/#TrimElements">TrimElements</a>.',
    'Read which slab or stair edge a railing is linked to, or link a railing to one':
        'Leggere a quale bordo di solaio o scala è collegato un parapetto, o collegare un parapetto a uno di essi',
    'Turn off the automatic story visibility of a railing, so the stories it is shown on can be set':
        'Disattivare la visibilità automatica sui piani di un parapetto, per poter scegliere i piani su cui è mostrato',
    'Set the Floor Plan Display, Show Projection, uncut and overhead lines or the cut line type of a mesh':
        'Impostare il Floor Plan Display, lo Show Projection, le linee non tagliate e sovrastanti o il tipo di linea del taglio di una mesh',
    'Read or set the floor plan symbol of a railing: its break mark, which symbol components it shows, their 2D attributes and the symbol overrides':
        'Leggere o impostare il simbolo in pianta di un parapetto: la linea di interruzione, quali componenti del simbolo mostra, i loro attributi 2D e le sostituzioni del simbolo',
    'Read the slab\'s outline with <a href="../archicad-addon/#GetDetailsOfElements">GetDetailsOfElements</a> and create a static railing along the edge with <a href="../archicad-addon/#CreateRailings">CreateRailings</a>. When the slab changes, delete the railing and create it again.':
        'Leggete il contorno del solaio con <a href="../archicad-addon/#GetDetailsOfElements">GetDetailsOfElements</a> e create un parapetto statico lungo il bordo con <a href="../archicad-addon/#CreateRailings">CreateRailings</a>. Quando il solaio cambia, eliminate il parapetto e createlo di nuovo.',
    'Move a window or door into another wall': 'Spostare una finestra o una porta in un altro muro',
    'Delete it and create it in the other wall with <a href="../archicad-addon/#CreateWindows">CreateWindows</a> or <a href="../archicad-addon/#CreateDoors">CreateDoors</a>; a favorite (<code>favoriteName</code>) gives it the same settings.':
        'Eliminatela e createla nell’altro muro con <a href="../archicad-addon/#CreateWindows">CreateWindows</a> o <a href="../archicad-addon/#CreateDoors">CreateDoors</a>; un preferito (<code>favoriteName</code>) le dà le stesse impostazioni.',
    'Move a skylight onto another roof or shell': 'Spostare un lucernario su un’altra copertura o un altro guscio',
    'Delete it and create it on the other roof or shell with <a href="../archicad-addon/#CreateSkylights">CreateSkylights</a>.':
        'Eliminatelo e createlo sull’altra copertura o sull’altro guscio con <a href="../archicad-addon/#CreateSkylights">CreateSkylights</a>.',
    'Turn an automatic zone into a manual one': 'Trasformare una zona automatica in una manuale',
    'Delete the zone and create it again as a manual zone, from its polygon, with <a href="../archicad-addon/#CreateZones">CreateZones</a>.':
        'Eliminate la zona e createla di nuovo come zona manuale, dal suo poligono, con <a href="../archicad-addon/#CreateZones">CreateZones</a>.',
    'Change the height of a slanted curtain wall': 'Cambiare l’altezza di una facciata continua inclinata',
    'Create it again with <a href="../archicad-addon/#CreateCurtainWalls">CreateCurtainWalls</a>, with its <code>angle</code> and the new <code>height</code>.':
        'Createla di nuovo con <a href="../archicad-addon/#CreateCurtainWalls">CreateCurtainWalls</a>, con il suo <code>angle</code> e la nuova <code>height</code>.',
    'Move the lines or the arc of an angle dimension': 'Spostare le linee o l’arco di una quota angolare',
    'Delete it and create it again with <a href="../archicad-addon/#CreateAngleDimensions">CreateAngleDimensions</a>.':
        'Eliminatela e createla di nuovo con <a href="../archicad-addon/#CreateAngleDimensions">CreateAngleDimensions</a>.',
    'Set the position or the offset of a stair\'s walking line':
        'Impostare la posizione o lo scostamento della linea di passo di una scala',
    'Anchor the sill or header of a window or door to the project zero or to a reference level':
        'Ancorare il davanzale o l’architrave di una finestra o di una porta allo zero di progetto o a un livello di riferimento',
    'Anchor it to a story with <code>verticalLink</code> <code>SillToStory</code> or <code>HeaderToStory</code> in <a href="../archicad-addon/#ModifyWindows">ModifyWindows</a> or <a href="../archicad-addon/#ModifyDoors">ModifyDoors</a>, and work out the height from the story level (<a href="../archicad-addon/#GetStories">GetStories</a>).':
        'Ancoratelo a un piano con <code>verticalLink</code> <code>SillToStory</code> o <code>HeaderToStory</code> in <a href="../archicad-addon/#ModifyWindows">ModifyWindows</a> o <a href="../archicad-addon/#ModifyDoors">ModifyDoors</a>, e ricavate l’altezza dalla quota del piano (<a href="../archicad-addon/#GetStories">GetStories</a>).',
    'Read or set the joint type (Vertical Cut and the others) and the dx offset of a stair structure':
        'Leggere o impostare il tipo di giunto (Vertical Cut e gli altri) e lo scostamento dx di una struttura della scala',
    'Change the baseline of an existing stair (move a flight end, a landing or a winder) and keep its GUID':
        'Cambiare la linea di base di una scala esistente (spostare l’estremità di una rampa, un pianerottolo o una parte a ventaglio) mantenendone il GUID',
    'Recreate the stair on the new baseline with <a href="../archicad-addon/#RecreateStairs">RecreateStairs</a>, which keeps its settings, Element ID, classifications, categories, properties and solid element operations, and returns the old and the new GUID. Its labels are lost.':
        'Ricreate la scala sulla nuova linea di base con <a href="../archicad-addon/#RecreateStairs">RecreateStairs</a>, che ne mantiene le impostazioni, l’ID elemento, le classificazioni, le categorie, le proprietà e le operazioni sugli elementi solidi, e restituisce il GUID vecchio e quello nuovo. Le sue etichette vanno perse.',
    'Set the left and right boundaries or the walking line of a stair (a custom outline, or a walking line off the middle of the flight)':
        'Impostare i contorni sinistro e destro o la linea di passo di una scala (un contorno personalizzato, o una linea di passo fuori dal centro della rampa)',
    'Archicad lays them out from the baseline and the flight width: change the baseline with <a href="../archicad-addon/#RecreateStairs">RecreateStairs</a> or the flight width with <a href="../archicad-addon/#ModifyStairs">ModifyStairs</a>. The walking line stays in the middle of the flight. <a href="../archicad-addon/#GetDetailsOfElements">GetDetailsOfElements</a> reads them.':
        'Archicad li traccia dalla linea di base e dalla larghezza della rampa: cambiate la linea di base con <a href="../archicad-addon/#RecreateStairs">RecreateStairs</a> o la larghezza della rampa con <a href="../archicad-addon/#ModifyStairs">ModifyStairs</a>. La linea di passo resta al centro della rampa. <a href="../archicad-addon/#GetDetailsOfElements">GetDetailsOfElements</a> li legge.',
    'Set a label’s optimised position, “use text pen as uniform pen” of a symbol label, or a perpendicular text direction':
        'Impostare la posizione ottimizzata di un’etichetta, “use text pen as uniform pen” di un’etichetta simbolo, o una direzione del testo perpendicolare',
    'Set the text direction (parallel, horizontal, vertical or a custom angle) with textWay in <a href="../archicad-addon/#CreateLabels">CreateLabels</a> and <a href="../archicad-addon/#ModifyLabels">ModifyLabels</a>. When the symbol label’s library part has a parameter for the other two, set it with <a href="../archicad-addon/#SetGDLParametersOfElements">SetGDLParametersOfElements</a>; otherwise set them in the Label tool settings or a favorite, and create the labels with favoriteName.':
        'Impostate la direzione del testo (parallela, orizzontale, verticale o un angolo a piacere) con textWay in <a href="../archicad-addon/#CreateLabels">CreateLabels</a> e <a href="../archicad-addon/#ModifyLabels">ModifyLabels</a>. Quando l’oggetto di libreria dell’etichetta simbolo ha un parametro per le altre due, impostatelo con <a href="../archicad-addon/#SetGDLParametersOfElements">SetGDLParametersOfElements</a>; altrimenti impostatele nelle impostazioni dello strumento Etichetta o in un preferito, e create le etichette con favoriteName.',
    'Create a radial dimension of an arc, a circle, a curved wall or a curved slab edge':
        'Creare una quota radiale di un arco, di un cerchio, di un muro curvo o del bordo curvo di un solaio',
    'Place the radial dimension in Archicad and read it with <a href="../archicad-addon/#GetDetailsOfElements">GetDetailsOfElements</a>, which returns how Archicad stored it. Those readings are what a create command needs.':
        'Posizionate la quota radiale in Archicad e leggetela con <a href="../archicad-addon/#GetDetailsOfElements">GetDetailsOfElements</a>, che restituisce come Archicad l’ha salvata. Quei valori sono ciò che serve a un comando di creazione.',

    // Design options
    'Design options': 'Opzioni di design',
    'Read which design option, or the Main Model, is the default that new elements go to':
        'Leggere quale opzione di design, o il Main Model, è quella predefinita in cui finiscono i nuovi elementi',
    'Create a temporary element, read its design option with <a href="../archicad-addon/#GetDesignOptionForElements">GetDesignOptionForElements</a>, then move it to the Main Model with <a href="../archicad-addon/#MoveElementsToDesignOptions">MoveElementsToDesignOptions</a> (a null GUID as <code>designOptionId</code>) and delete it with <a href="../archicad-addon/#DeleteElements">DeleteElements</a>; an element in an option that is not shown cannot be deleted. Set the default with <a href="../archicad-addon/#SetDefaultDesignOption">SetDefaultDesignOption</a>.':
        'Create un elemento temporaneo, leggetene l’opzione di design con <a href="../archicad-addon/#GetDesignOptionForElements">GetDesignOptionForElements</a>, poi spostatelo nel Main Model con <a href="../archicad-addon/#MoveElementsToDesignOptions">MoveElementsToDesignOptions</a> (un GUID nullo come <code>designOptionId</code>) ed eliminatelo con <a href="../archicad-addon/#DeleteElements">DeleteElements</a>; un elemento in un’opzione non mostrata non si può eliminare. Impostate quella predefinita con <a href="../archicad-addon/#SetDefaultDesignOption">SetDefaultDesignOption</a>.',
    'Read or change which design options the current window shows, as the eye icons of the Design Options palette do':
        'Leggere o cambiare quali opzioni di design mostra la finestra attuale, come fanno le icone a forma di occhio della palette Design Options',
    'Work with saved views: read and set the combination a View Map view shows with <a href="../archicad-addon/#GetDesignOptionCombinationOfViews">GetDesignOptionCombinationOfViews</a> and <a href="../archicad-addon/#SetDesignOptionCombinationOfViews">SetDesignOptionCombinationOfViews</a>, and the options of a combination with <a href="../archicad-addon/#SetActiveDesignOptionsInCombinations">SetActiveDesignOptionsInCombinations</a>.':
        'Lavorate con le viste salvate: leggete e impostate la combinazione che mostra una vista della View Map con <a href="../archicad-addon/#GetDesignOptionCombinationOfViews">GetDesignOptionCombinationOfViews</a> e <a href="../archicad-addon/#SetDesignOptionCombinationOfViews">SetDesignOptionCombinationOfViews</a>, e le opzioni di una combinazione con <a href="../archicad-addon/#SetActiveDesignOptionsInCombinations">SetActiveDesignOptionsInCombinations</a>.',
    'Duplicate a design option or a design option set, as the duplicate button of the Design Options palette does':
        'Duplicare un’opzione di design o un set di opzioni di design, come fa il pulsante Duplica della palette Design Options',
    'Create the set and its options with <a href="../archicad-addon/#CreateDesignOptionSets">CreateDesignOptionSets</a> and <a href="../archicad-addon/#CreateDesignOptions">CreateDesignOptions</a>. For the elements, list them with <a href="../archicad-addon/#GetElementsOfDesignOptions">GetElementsOfDesignOptions</a>, copy them with <a href="../archicad-addon/#CopyElements">CopyElements</a> and move the copies to the new option with <a href="../archicad-addon/#MoveElementsToDesignOptions">MoveElementsToDesignOptions</a>.':
        'Create il set e le sue opzioni con <a href="../archicad-addon/#CreateDesignOptionSets">CreateDesignOptionSets</a> e <a href="../archicad-addon/#CreateDesignOptions">CreateDesignOptions</a>. Per gli elementi, elencateli con <a href="../archicad-addon/#GetElementsOfDesignOptions">GetElementsOfDesignOptions</a>, copiateli con <a href="../archicad-addon/#CopyElements">CopyElements</a> e spostate le copie nella nuova opzione con <a href="../archicad-addon/#MoveElementsToDesignOptions">MoveElementsToDesignOptions</a>.',
    'Lock or colour the passive elements of design options, as the Design Options palette does':
        'Bloccare o colorare gli elementi passivi delle opzioni di design, come fa la palette Design Options',

    // Navigator and views
    'Navigator and views': 'Navigatore e viste',
    'Add an item to a publisher set': 'Aggiungere un elemento a un publisher set',
    'Add the items in Archicad\'s Organizer, then publish the set with <a href="../archicad-addon/#PublishPublisherSet">PublishPublisherSet</a>.':
        'Aggiungete gli elementi nell’Organizer di Archicad, poi pubblicate il set con <a href="../archicad-addon/#PublishPublisherSet">PublishPublisherSet</a>.',
    'Change, rename or create a stored Model View Options combination': 'Modificare, rinominare o creare una combinazione salvata di Model View Options',
    'Read the combinations with <a href="../archicad-addon/#GetModelViewOptionDetails">GetModelViewOptionDetails</a>; change the options of the current window with <a href="../archicad-addon/#SetViewOptions">SetViewOptions</a>.':
        'Leggete le combinazioni con <a href="../archicad-addon/#GetModelViewOptionDetails">GetModelViewOptionDetails</a>; modificate le opzioni della finestra corrente con <a href="../archicad-addon/#SetViewOptions">SetViewOptions</a>.',
    'Make a 3D style the current one': 'Rendere corrente uno stile 3D',
    'Give a view the 3D style with <a href="../archicad-addon/#SetViewSettings">SetViewSettings</a> (<code>d3styleName</code>) and open that view with <a href="../archicad-addon/#ChangeWindow">ChangeWindow</a> (<code>navigatorItemId</code>, from Archicad 27).':
        'Date lo stile 3D a una vista con <a href="../archicad-addon/#SetViewSettings">SetViewSettings</a> (<code>d3styleName</code>) e aprite quella vista con <a href="../archicad-addon/#ChangeWindow">ChangeWindow</a> (<code>navigatorItemId</code>, da Archicad 27).',
    'Read or set the floor plan cut plane or the structural analytical model settings saved in a view':
        'Leggere o impostare il piano di taglio della pianta o le impostazioni del modello analitico strutturale salvati in una vista',
    'Set them in the view\'s View Settings in Archicad. The project\'s floor plan cut plane can be set with <a href="../archicad-addon/#SetProjectPreferences">SetProjectPreferences</a>.':
        'Impostateli nelle View Settings della vista in Archicad. Il piano di taglio della pianta del progetto si può impostare con <a href="../archicad-addon/#SetProjectPreferences">SetProjectPreferences</a>.',
    'Tell a clone folder of the View Map from a normal folder':
        'Distinguere una cartella clone della View Map da una cartella normale',
    'Give the clone folders a name or ID of their own and find them by it in <a href="../archicad-addon/#GetNavigatorItemTree">GetNavigatorItemTree</a>.':
        'Date alle cartelle clone un nome o un ID proprio e trovatele in base a questo in <a href="../archicad-addon/#GetNavigatorItemTree">GetNavigatorItemTree</a>.',
    'Change the settings of an existing Layout Book subset (prefix, numbering style, start at, ...)':
        'Cambiare le impostazioni di un sottoinsieme esistente del Layout Book (prefisso, stile di numerazione, inizia da, ...)',
    'Read the subset with <a href="../archicad-addon/#GetLayoutSubsetSettings">GetLayoutSubsetSettings</a>, create a subset with the new settings with <a href="../archicad-addon/#CreateLayoutSubset">CreateLayoutSubset</a> and move the layouts into it with <a href="../archicad-addon/#MoveNavigatorItem">MoveNavigatorItem</a>. The old, empty subset is deleted in Archicad.':
        'Leggete il sottoinsieme con <a href="../archicad-addon/#GetLayoutSubsetSettings">GetLayoutSubsetSettings</a>, create un sottoinsieme con le nuove impostazioni con <a href="../archicad-addon/#CreateLayoutSubset">CreateLayoutSubset</a> e spostateci i layout con <a href="../archicad-addon/#MoveNavigatorItem">MoveNavigatorItem</a>. Il vecchio sottoinsieme, ormai vuoto, si elimina in Archicad.',

    // Project and files
    'Project and files': 'Progetto e file',
    'Undo or redo, as Edit &gt; Undo does': 'Annullare o ripetere, come Edit &gt; Undo',
    'Most Tapir commands change the model in an undo step, so the user can undo them in Archicad; the command reference says which cannot be undone. Save the project with <a href="../archicad-addon/#SaveProject">SaveProject</a> before a script that may need to be taken back.':
        'La maggior parte dei comandi di Tapir modifica il modello in un passo di annullamento, quindi l’utente può annullarli in Archicad; il riferimento dei comandi indica quali non si possono annullare. Salvate il progetto con <a href="../archicad-addon/#SaveProject">SaveProject</a> prima di uno script che potreste dover annullare.',
    'Copy and paste through the clipboard': 'Copiare e incollare tramite gli appunti',
    'In a project, copy 2D elements to another view with <a href="../archicad-addon/#CopyElementsToDatabase">CopyElementsToDatabase</a>. Between projects, save the elements as a module with <a href="../archicad-addon/#SaveAsModuleFile">SaveAsModuleFile</a>, place it in the other project with <a href="../archicad-addon/#CreateHotlinkNodes">CreateHotlinkNodes</a> and <a href="../archicad-addon/#CreateHotlinkInstances">CreateHotlinkInstances</a>, and, to keep plain elements, <a href="../archicad-addon/#BreakHotlinkNodes">BreakHotlinkNodes</a>.':
        'In un progetto, copiate elementi 2D in un’altra vista con <a href="../archicad-addon/#CopyElementsToDatabase">CopyElementsToDatabase</a>. Tra progetti, salvate gli elementi come modulo con <a href="../archicad-addon/#SaveAsModuleFile">SaveAsModuleFile</a>, posizionatelo nell’altro progetto con <a href="../archicad-addon/#CreateHotlinkNodes">CreateHotlinkNodes</a> e <a href="../archicad-addon/#CreateHotlinkInstances">CreateHotlinkInstances</a> e, per tenere elementi semplici, <a href="../archicad-addon/#BreakHotlinkNodes">BreakHotlinkNodes</a>.',
    'Merge a file into the project, as File &gt; Interoperability &gt; Merge does': 'Unire un file al progetto, come fa File &gt; Interoperability &gt; Merge',
    'Place the file as a hotlink with <a href="../archicad-addon/#CreateHotlinkNodes">CreateHotlinkNodes</a> and <a href="../archicad-addon/#CreateHotlinkInstances">CreateHotlinkInstances</a>, then <a href="../archicad-addon/#BreakHotlinkNodes">BreakHotlinkNodes</a>.':
        'Posizionate il file come hotlink con <a href="../archicad-addon/#CreateHotlinkNodes">CreateHotlinkNodes</a> e <a href="../archicad-addon/#CreateHotlinkInstances">CreateHotlinkInstances</a>, poi <a href="../archicad-addon/#BreakHotlinkNodes">BreakHotlinkNodes</a>.',
    'Delete one offset modifier of a profile': 'Eliminare un solo modificatore di scostamento di un profilo',
    'Rebuild the profile with <a href="../archicad-addon/#CreateProfiles">CreateProfiles</a>\' <code>replaceSkins</code> and <code>newSkins</code>, which drops every modifier, then add the ones to keep again with <code>newProfileModifiers</code> (Archicad 27 and later).':
        'Ricostruite il profilo con <code>replaceSkins</code> e <code>newSkins</code> di <a href="../archicad-addon/#CreateProfiles">CreateProfiles</a>, che elimina tutti i modificatori, poi aggiungete di nuovo quelli da mantenere con <code>newProfileModifiers</code> (Archicad 27 e successivi).',
    'Import an IFC translator from a file into the project, or export one to a file':
        'Importare un traduttore IFC da un file nel progetto, o esportarne uno in un file',
    'Keep the office translator in the project template, so new projects have it, and bring it into older projects once with Import… in the IFC Translators dialog. A script can check that a project has it with <a href="../archicad-addon/#GetIFCExportTranslators">GetIFCExportTranslators</a> and export with it by name with <a href="../archicad-addon/#IFCFileOperation">IFCFileOperation</a>.':
        'Tenete il traduttore dello studio nel modello di progetto, così i nuovi progetti lo hanno, e portatelo una volta nei progetti più vecchi con Import… nella finestra IFC Translators. Uno script può verificare che un progetto lo abbia con <a href="../archicad-addon/#GetIFCExportTranslators">GetIFCExportTranslators</a> ed esportare con esso, per nome, con <a href="../archicad-addon/#IFCFileOperation">IFCFileOperation</a>.',

    // Documents and publishing
    'Documents and publishing': 'Documenti e pubblicazione',
    'Read the rows of an Interactive Schedule': 'Leggere le righe di un Interactive Schedule',
    'Build the list yourself: find the elements with <a href="../archicad-addon/#GetElementsByType">GetElementsByType</a> and read their values with <a href="../archicad-addon/#GetPropertyValuesOfElements">GetPropertyValuesOfElements</a> or <a href="../archicad-addon/#GetQuantitiesOfElements">GetQuantitiesOfElements</a>. The <a href="../scripts/">built-in scripts</a> Room Finishes Schedule and Material Take-Off build such lists.':
        'Costruite l’elenco da soli: trovate gli elementi con <a href="../archicad-addon/#GetElementsByType">GetElementsByType</a> e leggetene i valori con <a href="../archicad-addon/#GetPropertyValuesOfElements">GetPropertyValuesOfElements</a> o <a href="../archicad-addon/#GetQuantitiesOfElements">GetQuantitiesOfElements</a>. Gli <a href="../scripts/">script inclusi</a> Room Finishes Schedule e Material Take-Off costruiscono elenchi di questo tipo.',
    'Export an Interactive Schedule': 'Esportare un Interactive Schedule',
    'Put the schedule in a publisher set in Archicad once, then publish that set with <a href="../archicad-addon/#PublishPublisherSet">PublishPublisherSet</a>.':
        'Mettete una volta l’abaco in un publisher set in Archicad, poi pubblicate quel set con <a href="../archicad-addon/#PublishPublisherSet">PublishPublisherSet</a>.',
    'Choose the output format (PDF, DWG, DXF …) of a publisher set per call':
        'Scegliere il formato di uscita (PDF, DWG, DXF …) di un publisher set a ogni chiamata',
    'Make one publisher set per format in Archicad’s Organizer once, then publish the one you need with <a href="../archicad-addon/#PublishPublisherSet">PublishPublisherSet</a>; its answer lists the files written (<code>writtenFiles</code>).':
        'Create una volta un publisher set per formato nell’Organizer di Archicad, poi pubblicate quello che vi serve con <a href="../archicad-addon/#PublishPublisherSet">PublishPublisherSet</a>; la sua risposta elenca i file scritti (<code>writtenFiles</code>).',
    'Create a revision Change without placing a Change Marker, or change the ID or description of an existing Change':
        'Creare un Change di revisione senza posizionare un Change Marker, o cambiare l’ID o la descrizione di un Change esistente',
    'Create the Change together with its first marker with <a href="../archicad-addon/#CreateChangeMarkers">CreateChangeMarkers</a> (createChange). To change an existing Change, use Archicad’s Change Manager.':
        'Create il Change insieme al suo primo marker con <a href="../archicad-addon/#CreateChangeMarkers">CreateChangeMarkers</a> (createChange). Per modificare un Change esistente, usate il Change Manager di Archicad.',
    'Set the custom scheme fields of a revision Change':
        'Impostare i campi personalizzati dello schema di un Change di revisione',
    'Fill them in Archicad’s Change Manager; <a href="../archicad-addon/#GetRevisionChanges">GetRevisionChanges</a> reads them back.':
        'Compilateli nel Change Manager di Archicad; <a href="../archicad-addon/#GetRevisionChanges">GetRevisionChanges</a> li rilegge.',
    'Add elements to a revision Change, or remove them from it': 'Aggiungere elementi a un Change di revisione, o toglierli',
    'Add them in Archicad’s Change Manager; <a href="../archicad-addon/#GetRevisionChangesOfElements">GetRevisionChangesOfElements</a> reads which Changes an element belongs to.':
        'Aggiungeteli nel Change Manager di Archicad; <a href="../archicad-addon/#GetRevisionChangesOfElements">GetRevisionChangesOfElements</a> legge a quali Change appartiene un elemento.',

    // Library parts and GDL
    'Library parts and GDL': 'Oggetti di libreria e GDL',
    'Run a 2D GDL script given as text, as <a href="../archicad-addon/#InterpretGDLScript3D">InterpretGDLScript3D</a> does with a 3D script':
        'Eseguire uno script GDL 2D dato come testo, come fa <a href="../archicad-addon/#InterpretGDLScript3D">InterpretGDLScript3D</a> con uno script 3D',
    'Save the script in a library part with <a href="../archicad-addon/#CreateLibraryParts">CreateLibraryParts</a> (or <a href="../archicad-addon/#SetLibraryPartScripts">SetLibraryPartScripts</a>), then draw its 2D symbol with <a href="../archicad-addon/#GetLibraryPart2DPrimitives">GetLibraryPart2DPrimitives</a>.':
        'Salvate lo script in un oggetto di libreria con <a href="../archicad-addon/#CreateLibraryParts">CreateLibraryParts</a> (o <a href="../archicad-addon/#SetLibraryPartScripts">SetLibraryPartScripts</a>), poi disegnatene il simbolo 2D con <a href="../archicad-addon/#GetLibraryPart2DPrimitives">GetLibraryPart2DPrimitives</a>.',
    'Get the error message and the line number when a GDL script fails':
        'Ottenere il messaggio di errore e il numero di riga quando uno script GDL fallisce',
    'Check whether the script runs with <a href="../archicad-addon/#InterpretGDLScript3D">InterpretGDLScript3D</a> (3D) or <a href="../archicad-addon/#GetLibraryPart2DPrimitives">GetLibraryPart2DPrimitives</a> (2D). To see the error, open the library part in Archicad’s GDL editor with <a href="../archicad-addon/#OpenLibraryPartInEditor">OpenLibraryPartInEditor</a> (from Archicad 27).':
        'Verificate se lo script viene eseguito con <a href="../archicad-addon/#InterpretGDLScript3D">InterpretGDLScript3D</a> (3D) o <a href="../archicad-addon/#GetLibraryPart2DPrimitives">GetLibraryPart2DPrimitives</a> (2D). Per vedere l’errore, aprite l’oggetto di libreria nell’editor GDL di Archicad con <a href="../archicad-addon/#OpenLibraryPartInEditor">OpenLibraryPartInEditor</a> (da Archicad 27).',

    // Archicad's interface
    'Archicad\'s interface': 'L’interfaccia di Archicad',
    'Read the text of the Session Report window': 'Leggere il testo della finestra Session Report',
    'Open the window for the user with <a href="../archicad-addon/#ChangeWindow">ChangeWindow</a> and windowType \'Report\' (from Archicad 26).':
        'Aprite la finestra per l’utente con <a href="../archicad-addon/#ChangeWindow">ChangeWindow</a> e windowType \'Report\' (da Archicad 26).',
    'Get the outcome of an Action Center check': 'Ottenere l’esito di un controllo dell’Action Center',
    'Run the check with <a href="../archicad-addon/#RunActionCenterChecks">RunActionCenterChecks</a>; Archicad shows the Action Center when it found an issue. For the hotlink check, read the state of each source with <a href="../archicad-addon/#GetHotlinkSourceStatus">GetHotlinkSourceStatus</a> (from Archicad 27).':
        'Eseguite il controllo con <a href="../archicad-addon/#RunActionCenterChecks">RunActionCenterChecks</a>; Archicad mostra l’Action Center quando ha trovato un problema. Per il controllo degli hotlink, leggete lo stato di ogni sorgente con <a href="../archicad-addon/#GetHotlinkSourceStatus">GetHotlinkSourceStatus</a> (da Archicad 27).',
    'Load a new add-on (.apx or .bundle) while Archicad runs, from an add-on or a script':
        'Caricare un nuovo add-on (.apx o .bundle) mentre Archicad è in esecuzione, da un add-on o da uno script',
    'Add it in Archicad\'s Add-On Manager (Options &gt; Add-On Manager &gt; Edit List of Available Add-Ons &gt; Add). An add-on that is already loaded is replaced only after Archicad quits, as the Tapir Installer does when it updates Tapir.':
        'Aggiungetelo nell’Add-On Manager di Archicad (Options &gt; Add-On Manager &gt; Edit List of Available Add-Ons &gt; Add). Un add-on già caricato viene sostituito solo dopo la chiusura di Archicad, come fa il Tapir Installer quando aggiorna Tapir.',

    'Set the profile modifier values, or the stretched width and height, of a wall, beam or column that uses a profile':
        'Impostare i valori dei modificatori del profilo, o la larghezza e l\'altezza allungate, di un muro, una trave o un pilastro che usa un profilo',
    'Read them with <a href="../archicad-addon/#GetProfileValuesOfElements">GetProfileValuesOfElements</a>. To change them, change the profile with <a href="../archicad-addon/#CreateProfiles">CreateProfiles</a> (a copy for these elements, or the profile itself), or set them in the settings of the element in Archicad.':
        'Leggeteli con <a href="../archicad-addon/#GetProfileValuesOfElements">GetProfileValuesOfElements</a>. Per cambiarli, modificate il profilo con <a href="../archicad-addon/#CreateProfiles">CreateProfiles</a> (una copia per questi elementi, o il profilo stesso), oppure impostateli nelle impostazioni dell\'elemento in Archicad.',
    'Read or set how a grid element is shown in sections, elevations and interior elevations: its vertical extent and its markers at the top and bottom':
        'Leggere o impostare come un elemento griglia è mostrato in sezioni, alzati e alzati interni: la sua estensione verticale e i suoi marcatori in alto e in basso',
    'Select the grid elements in Archicad and set it for all of them at once in their settings dialog.':
        'Selezionate gli elementi griglia in Archicad e impostatelo per tutti insieme nella loro finestra delle impostazioni.',
    'Read the settings of an IFC translator, such as its type mapping and property mapping, for example to check them as the IFC conformity check of the Missing BIM Data report does':
        'Leggere le impostazioni di un traduttore IFC, come la sua mappatura dei tipi e la mappatura delle proprietà, per esempio per controllarle come fa il controllo di conformità IFC del rapporto sui dati BIM mancanti',
    'Check what the translator gives the elements instead: their IFC type with <a href="../archicad-addon/#GetIFCTypeOfElements">GetIFCTypeOfElements</a>, and from Archicad 28 the properties and classification references the preview translator maps to them with <a href="../archicad-addon/#GetIFCPropertiesOfElements">GetIFCPropertiesOfElements</a> and <a href="../archicad-addon/#GetIFCClassificationReferencesOfElements">GetIFCClassificationReferencesOfElements</a>. <a href="../archicad-addon/#GetAllProperties">GetAllProperties</a> shows whether the properties a mapping needs exist in the project.':
        'Controlli invece cosa il traduttore dà agli elementi: il loro tipo IFC con <a href="../archicad-addon/#GetIFCTypeOfElements">GetIFCTypeOfElements</a> e, da Archicad 28, le proprietà e i riferimenti di classificazione che il traduttore di anteprima associa loro con <a href="../archicad-addon/#GetIFCPropertiesOfElements">GetIFCPropertiesOfElements</a> e <a href="../archicad-addon/#GetIFCClassificationReferencesOfElements">GetIFCClassificationReferencesOfElements</a>. <a href="../archicad-addon/#GetAllProperties">GetAllProperties</a> mostra se nel progetto esistono le proprietà di cui una mappatura ha bisogno.',
    'Find the graphic override rules, Interactive Schedules and SAF translators that refer to a property or classification that no longer exists, as the Missing BIM Data report does':
        'Trovare le regole di sostituzione grafica, gli abachi interattivi e i traduttori SAF che rimandano a una proprietà o a una classificazione che non esiste più, come fa il rapporto sui dati BIM mancanti',
    'For graphic override rules (from Archicad 27): read the criterion of each rule with <a href="../archicad-addon/#GetGraphicOverrideRules">GetGraphicOverrideRules</a> (includeCriterion) and compare the properties and classifications it tests with <a href="../archicad-addon/#GetAllProperties">GetAllProperties</a> and <a href="../archicad-addon/#GetAllClassificationSystems">GetAllClassificationSystems</a>. For Interactive Schedules and SAF translators: nothing yet.':
        'Per le regole di sostituzione grafica (da Archicad 27): legga il criterio di ogni regola con <a href="../archicad-addon/#GetGraphicOverrideRules">GetGraphicOverrideRules</a> (includeCriterion) e confronti le proprietà e le classificazioni che controlla con <a href="../archicad-addon/#GetAllProperties">GetAllProperties</a> e <a href="../archicad-addon/#GetAllClassificationSystems">GetAllClassificationSystems</a>. Per gli abachi interattivi e i traduttori SAF: ancora niente.',
    'Export and import, all at once, the settings that the dialogs of Archicad save as XML (properties, classifications, the Layout Book, graphic overrides, Model View Options, IFC and DWG translators), to keep several projects in sync':
        'Esportare e importare in una volta le impostazioni che le finestre di dialogo di Archicad salvano come XML (proprietà, classificazioni, il Book di impaginazione, sostituzioni grafiche, opzioni di vista del modello, traduttori IFC e DWG), per tenere sincronizzati più progetti',
    'Copy what the API can write, one kind at a time, in one script run against each project: properties and classifications from their XML files with <a href="../archicad-addon/#ImportPropertiesXml">ImportPropertiesXml</a> and <a href="../archicad-addon/#ImportClassificationsXml">ImportClassificationsXml</a>; attributes from another project with <a href="../archicad-addon/#ImportAttributesFromProject">ImportAttributesFromProject</a>; graphic override rules with <a href="../archicad-addon/#GetGraphicOverrideRules">GetGraphicOverrideRules</a> and <a href="../archicad-addon/#CreateGraphicOverrideRules">CreateGraphicOverrideRules</a>; Project Info with <a href="../archicad-addon/#GetProjectInfoFields">GetProjectInfoFields</a> and <a href="../archicad-addon/#SetProjectInfoField">SetProjectInfoField</a>; Favorites with <a href="../archicad-addon/#ExportFavorites">ExportFavorites</a> and <a href="../archicad-addon/#ImportFavorites">ImportFavorites</a>. The Layout Book, Model View Options and the translators: nothing yet.':
        'Copi ciò che l\'API può scrivere, un tipo alla volta, in uno script eseguito su ogni progetto: proprietà e classificazioni dai loro file XML con <a href="../archicad-addon/#ImportPropertiesXml">ImportPropertiesXml</a> e <a href="../archicad-addon/#ImportClassificationsXml">ImportClassificationsXml</a>; attributi da un altro progetto con <a href="../archicad-addon/#ImportAttributesFromProject">ImportAttributesFromProject</a>; regole di sostituzione grafica con <a href="../archicad-addon/#GetGraphicOverrideRules">GetGraphicOverrideRules</a> e <a href="../archicad-addon/#CreateGraphicOverrideRules">CreateGraphicOverrideRules</a>; le info del progetto con <a href="../archicad-addon/#GetProjectInfoFields">GetProjectInfoFields</a> e <a href="../archicad-addon/#SetProjectInfoField">SetProjectInfoField</a>; preferiti con <a href="../archicad-addon/#ExportFavorites">ExportFavorites</a> e <a href="../archicad-addon/#ImportFavorites">ImportFavorites</a>. Il Book di impaginazione, le opzioni di vista del modello e i traduttori: ancora niente.',


    'Found something else the Archicad API cannot do? Say so in an issue, with the DevKit headers you checked, and it is added here. <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/issues/new/choose">Open an issue</a>':
        'Avete trovato altro che l’API di Archicad non può fare? Segnalatelo in una issue, con gli header del DevKit che avete verificato, e verrà aggiunto qui. <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/issues/new/choose">Apri una issue</a>'
});
