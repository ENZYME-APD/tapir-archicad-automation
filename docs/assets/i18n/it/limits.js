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
    'Read the slab\'s outline with <a href="../archicad-addon/#GetDetailsOfElements">GetDetailsOfElements</a> and create a static railing along the edge with <a href="../archicad-addon/#CreateRailings">CreateRailings</a>. When the slab changes, delete the railing and create it again.':
        'Leggete il contorno del solaio con <a href="../archicad-addon/#GetDetailsOfElements">GetDetailsOfElements</a> e create un parapetto statico lungo il bordo con <a href="../archicad-addon/#CreateRailings">CreateRailings</a>. Quando il solaio cambia, eliminate il parapetto e createlo di nuovo.',

    // Navigator and views
    'Navigator and views': 'Navigatore e viste',
    'Add an item to a publisher set': 'Aggiungere un elemento a un publisher set',
    'Add the items in Archicad\'s Organizer, then publish the set with <a href="../archicad-addon/#PublishPublisherSet">PublishPublisherSet</a>.':
        'Aggiungete gli elementi nell’Organizer di Archicad, poi pubblicate il set con <a href="../archicad-addon/#PublishPublisherSet">PublishPublisherSet</a>.',
    'Change, rename or create a stored Model View Options combination': 'Modificare, rinominare o creare una combinazione salvata di Model View Options',
    'Read the combinations with <a href="../archicad-addon/#GetModelViewOptionDetails">GetModelViewOptionDetails</a>; change the options of the current window with <a href="../archicad-addon/#SetViewOptions">SetViewOptions</a>.':
        'Leggete le combinazioni con <a href="../archicad-addon/#GetModelViewOptionDetails">GetModelViewOptionDetails</a>; modificate le opzioni della finestra corrente con <a href="../archicad-addon/#SetViewOptions">SetViewOptions</a>.',

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

    // Documents and publishing
    'Documents and publishing': 'Documenti e pubblicazione',
    'Read the rows of an Interactive Schedule': 'Leggere le righe di un Interactive Schedule',
    'Build the list yourself: find the elements with <a href="../archicad-addon/#GetElementsByType">GetElementsByType</a> and read their values with <a href="../archicad-addon/#GetPropertyValuesOfElements">GetPropertyValuesOfElements</a> or <a href="../archicad-addon/#GetQuantitiesOfElements">GetQuantitiesOfElements</a>. The <a href="../scripts/">built-in scripts</a> Room Finishes Schedule and Material Take-Off build such lists.':
        'Costruite l’elenco da soli: trovate gli elementi con <a href="../archicad-addon/#GetElementsByType">GetElementsByType</a> e leggetene i valori con <a href="../archicad-addon/#GetPropertyValuesOfElements">GetPropertyValuesOfElements</a> o <a href="../archicad-addon/#GetQuantitiesOfElements">GetQuantitiesOfElements</a>. Gli <a href="../scripts/">script inclusi</a> Room Finishes Schedule e Material Take-Off costruiscono elenchi di questo tipo.',
    'Export an Interactive Schedule': 'Esportare un Interactive Schedule',
    'Put the schedule in a publisher set in Archicad once, then publish that set with <a href="../archicad-addon/#PublishPublisherSet">PublishPublisherSet</a>.':
        'Mettete una volta l’abaco in un publisher set in Archicad, poi pubblicate quel set con <a href="../archicad-addon/#PublishPublisherSet">PublishPublisherSet</a>.',

    // Archicad's interface
    'Archicad\'s interface': 'L’interfaccia di Archicad',
    'Read the text of the Session Report window': 'Leggere il testo della finestra Session Report',
    'Open the window for the user with <a href="../archicad-addon/#ChangeWindow">ChangeWindow</a> and windowType \'Report\' (from Archicad 26).':
        'Aprite la finestra per l’utente con <a href="../archicad-addon/#ChangeWindow">ChangeWindow</a> e windowType \'Report\' (da Archicad 26).',
    'Get the outcome of an Action Center check': 'Ottenere l’esito di un controllo dell’Action Center',
    'Run the check with <a href="../archicad-addon/#RunActionCenterChecks">RunActionCenterChecks</a>; Archicad shows the Action Center when it found an issue. For the hotlink check, read the state of each source with <a href="../archicad-addon/#GetHotlinkSourceStatus">GetHotlinkSourceStatus</a> (from Archicad 27).':
        'Eseguite il controllo con <a href="../archicad-addon/#RunActionCenterChecks">RunActionCenterChecks</a>; Archicad mostra l’Action Center quando ha trovato un problema. Per il controllo degli hotlink, leggete lo stato di ogni sorgente con <a href="../archicad-addon/#GetHotlinkSourceStatus">GetHotlinkSourceStatus</a> (da Archicad 27).',

    'Found something else the Archicad API cannot do? Say so in an issue, with the DevKit headers you checked, and it is added here. <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/issues/new/choose">Open an issue</a>':
        'Avete trovato altro che l’API di Archicad non può fare? Segnalatelo in una issue, con gli header del DevKit che avete verificato, e verrà aggiunto qui. <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/issues/new/choose">Apri una issue</a>'
});
