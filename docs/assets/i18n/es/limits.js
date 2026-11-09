/* Español: the API limits page (limits/index.html). The Evidence column (headers and function names)
   stays English. See ../README.md. */
(window.TAPIR_I18N = window.TAPIR_I18N || []).push ({
    'What the Archicad API can\'t do | Tapir': 'Lo que la API de Archicad no puede hacer | Tapir',
    'What people ask for that Graphisoft\'s Archicad API does not offer, such as undo or the clipboard, each with what to do instead and the DevKit evidence.':
        'Lo que se pide y la API de Archicad de Graphisoft no ofrece, como deshacer o el portapapeles, cada uno con lo que puede hacer en su lugar y la prueba en el DevKit.',
    '<a href="../">Tapir</a> / <a href="../docs/">Docs</a> / API limits': '<a href="../">Tapir</a> / <a href="../docs/">Docs</a> / Límites de la API',
    'What the Archicad API <em>can\'t do.</em>': 'Lo que la API de Archicad <em>no puede hacer.</em>',
    'Tapir uses the Archicad API that Graphisoft publishes for add-ons, and covers all of it that can be a JSON command. What that API does not offer, Tapir cannot offer either. This page lists what people ask for that is not in it, with what to do instead.':
        'Tapir usa la API de Archicad que Graphisoft publica para los add-ons, y cubre todo lo que de ella puede ser un comando JSON. Lo que esa API no ofrece, Tapir tampoco puede ofrecerlo. Esta página enumera lo que se pide y no está en ella, con lo que puede hacer en su lugar.',

    // The list
    'The <em>list.</em>': 'La <em>lista.</em>',
    'It is also what Tapir would like the Archicad API to offer, and the list Tapir sends to Graphisoft. Every row was checked in the headers of the Archicad API DevKits of Archicad 25 to 30, the versions Tapir is built for; the Evidence column names the headers and functions.':
        'Es también lo que a Tapir le gustaría que ofreciera la API de Archicad, y la lista que Tapir envía a Graphisoft. Cada fila se comprobó en las cabeceras de los DevKits de la API de Archicad 25 a 30, las versiones para las que se compila Tapir; la columna Prueba nombra las cabeceras y funciones.',
    'What people want': 'Lo que se pide',
    'Status': 'Estado',
    'What to do instead': 'Qué hacer en su lugar',
    'Evidence': 'Prueba',
    'Issue': 'Issue',
    'Not possible': 'No es posible',
    'Partly possible': 'Posible en parte',
    'Workaround': 'Alternativa',
    'Nothing yet': 'Nada por ahora',

    // Elements
    'Elements': 'Elementos',
    'Explode an element into its parts, as Edit &gt; Reshape &gt; Explode into Current View does':
        'Descomponer un elemento en sus partes, como hace Edit &gt; Reshape &gt; Explode into Current View',
    'A camera with a name of its own, or an axonometric camera': 'Una cámara con nombre propio, o una cámara axonométrica',
    'Name the Camera Set the camera is in. For an axonometric view, set the 3D window with <a href="../archicad-addon/#Set3DProjectionSettings">Set3DProjectionSettings</a>.':
        'Ponga nombre al Camera Set en el que está la cámara. Para una vista axonométrica, ajuste la ventana 3D con <a href="../archicad-addon/#Set3DProjectionSettings">Set3DProjectionSettings</a>.',
    'Remove one direction of a trim between two roofs or shells that trim each other':
        'Quitar una sola dirección de un recorte entre dos cubiertas o láminas que se recortan mutuamente',
    'Remove the trim with <a href="../archicad-addon/#RemoveElementTrims">RemoveElementTrims</a>, which removes both directions, then trim the other direction again with <a href="../archicad-addon/#TrimElements">TrimElements</a>.':
        'Quite el recorte con <a href="../archicad-addon/#RemoveElementTrims">RemoveElementTrims</a>, que quita ambas direcciones, y luego vuelva a recortar la otra dirección con <a href="../archicad-addon/#TrimElements">TrimElements</a>.',
    'Read which slab or stair edge a railing is linked to, or link a railing to one':
        'Leer a qué borde de forjado o escalera está vinculada una barandilla, o vincular una barandilla a uno',
    'Read the slab\'s outline with <a href="../archicad-addon/#GetDetailsOfElements">GetDetailsOfElements</a> and create a static railing along the edge with <a href="../archicad-addon/#CreateRailings">CreateRailings</a>. When the slab changes, delete the railing and create it again.':
        'Lea el contorno del forjado con <a href="../archicad-addon/#GetDetailsOfElements">GetDetailsOfElements</a> y cree una barandilla estática a lo largo del borde con <a href="../archicad-addon/#CreateRailings">CreateRailings</a>. Cuando cambie el forjado, borre la barandilla y vuelva a crearla.',

    // Navigator and views
    'Navigator and views': 'Navegador y vistas',
    'Add an item to a publisher set': 'Añadir un elemento a un publisher set',
    'Add the items in Archicad\'s Organizer, then publish the set with <a href="../archicad-addon/#PublishPublisherSet">PublishPublisherSet</a>.':
        'Añada los elementos en el Organizer de Archicad y luego publique el set con <a href="../archicad-addon/#PublishPublisherSet">PublishPublisherSet</a>.',
    'Change, rename or create a stored Model View Options combination': 'Cambiar, renombrar o crear una combinación guardada de Model View Options',
    'Read the combinations with <a href="../archicad-addon/#GetModelViewOptionDetails">GetModelViewOptionDetails</a>; change the options of the current window with <a href="../archicad-addon/#SetViewOptions">SetViewOptions</a>.':
        'Lea las combinaciones con <a href="../archicad-addon/#GetModelViewOptionDetails">GetModelViewOptionDetails</a>; cambie las opciones de la ventana actual con <a href="../archicad-addon/#SetViewOptions">SetViewOptions</a>.',

    // Project and files
    'Project and files': 'Proyecto y archivos',
    'Undo or redo, as Edit &gt; Undo does': 'Deshacer o rehacer, como Edit &gt; Undo',
    'Most Tapir commands change the model in an undo step, so the user can undo them in Archicad; the command reference says which cannot be undone. Save the project with <a href="../archicad-addon/#SaveProject">SaveProject</a> before a script that may need to be taken back.':
        'La mayoría de los comandos de Tapir cambian el modelo en un paso de deshacer, así que el usuario puede deshacerlos en Archicad; la referencia de comandos indica cuáles no se pueden deshacer. Guarde el proyecto con <a href="../archicad-addon/#SaveProject">SaveProject</a> antes de un script que quizá tenga que revertir.',
    'Copy and paste through the clipboard': 'Copiar y pegar mediante el portapapeles',
    'In a project, copy 2D elements to another view with <a href="../archicad-addon/#CopyElementsToDatabase">CopyElementsToDatabase</a>. Between projects, save the elements as a module with <a href="../archicad-addon/#SaveAsModuleFile">SaveAsModuleFile</a>, place it in the other project with <a href="../archicad-addon/#CreateHotlinkNodes">CreateHotlinkNodes</a> and <a href="../archicad-addon/#CreateHotlinkInstances">CreateHotlinkInstances</a>, and, to keep plain elements, <a href="../archicad-addon/#BreakHotlinkNodes">BreakHotlinkNodes</a>.':
        'Dentro de un proyecto, copie elementos 2D a otra vista con <a href="../archicad-addon/#CopyElementsToDatabase">CopyElementsToDatabase</a>. Entre proyectos, guarde los elementos como módulo con <a href="../archicad-addon/#SaveAsModuleFile">SaveAsModuleFile</a>, colóquelo en el otro proyecto con <a href="../archicad-addon/#CreateHotlinkNodes">CreateHotlinkNodes</a> y <a href="../archicad-addon/#CreateHotlinkInstances">CreateHotlinkInstances</a> y, para quedarse con elementos simples, <a href="../archicad-addon/#BreakHotlinkNodes">BreakHotlinkNodes</a>.',
    'Merge a file into the project, as File &gt; Interoperability &gt; Merge does': 'Fusionar un archivo en el proyecto, como hace File &gt; Interoperability &gt; Merge',
    'Place the file as a hotlink with <a href="../archicad-addon/#CreateHotlinkNodes">CreateHotlinkNodes</a> and <a href="../archicad-addon/#CreateHotlinkInstances">CreateHotlinkInstances</a>, then <a href="../archicad-addon/#BreakHotlinkNodes">BreakHotlinkNodes</a>.':
        'Coloque el archivo como hotlink con <a href="../archicad-addon/#CreateHotlinkNodes">CreateHotlinkNodes</a> y <a href="../archicad-addon/#CreateHotlinkInstances">CreateHotlinkInstances</a>, y luego <a href="../archicad-addon/#BreakHotlinkNodes">BreakHotlinkNodes</a>.',

    // Documents and publishing
    'Documents and publishing': 'Documentos y publicación',
    'Read the rows of an Interactive Schedule': 'Leer las filas de un Interactive Schedule',
    'Build the list yourself: find the elements with <a href="../archicad-addon/#GetElementsByType">GetElementsByType</a> and read their values with <a href="../archicad-addon/#GetPropertyValuesOfElements">GetPropertyValuesOfElements</a> or <a href="../archicad-addon/#GetQuantitiesOfElements">GetQuantitiesOfElements</a>. The <a href="../scripts/">built-in scripts</a> Room Finishes Schedule and Material Take-Off build such lists.':
        'Construya la lista usted mismo: encuentre los elementos con <a href="../archicad-addon/#GetElementsByType">GetElementsByType</a> y lea sus valores con <a href="../archicad-addon/#GetPropertyValuesOfElements">GetPropertyValuesOfElements</a> o <a href="../archicad-addon/#GetQuantitiesOfElements">GetQuantitiesOfElements</a>. Los <a href="../scripts/">scripts incluidos</a> Room Finishes Schedule y Material Take-Off construyen listas así.',
    'Export an Interactive Schedule': 'Exportar un Interactive Schedule',
    'Put the schedule in a publisher set in Archicad once, then publish that set with <a href="../archicad-addon/#PublishPublisherSet">PublishPublisherSet</a>.':
        'Ponga una vez el listado en un publisher set en Archicad y luego publique ese set con <a href="../archicad-addon/#PublishPublisherSet">PublishPublisherSet</a>.',

    // Archicad's interface
    'Archicad\'s interface': 'La interfaz de Archicad',
    'Read the text of the Session Report window': 'Leer el texto de la ventana Session Report',
    'Open the window for the user with <a href="../archicad-addon/#ChangeWindow">ChangeWindow</a> and windowType \'Report\' (from Archicad 26).':
        'Abra la ventana para el usuario con <a href="../archicad-addon/#ChangeWindow">ChangeWindow</a> y windowType \'Report\' (desde Archicad 26).',
    'Get the outcome of an Action Center check': 'Obtener el resultado de una comprobación del Action Center',
    'Run the check with <a href="../archicad-addon/#RunActionCenterChecks">RunActionCenterChecks</a>; Archicad shows the Action Center when it found an issue. For the hotlink check, read the state of each source with <a href="../archicad-addon/#GetHotlinkSourceStatus">GetHotlinkSourceStatus</a> (from Archicad 27).':
        'Ejecute la comprobación con <a href="../archicad-addon/#RunActionCenterChecks">RunActionCenterChecks</a>; Archicad muestra el Action Center cuando encuentra un problema. Para la comprobación de hotlinks, lea el estado de cada origen con <a href="../archicad-addon/#GetHotlinkSourceStatus">GetHotlinkSourceStatus</a> (desde Archicad 27).',

    'Found something else the Archicad API cannot do? Say so in an issue, with the DevKit headers you checked, and it is added here. <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/issues/new/choose">Open an issue</a>':
        '¿Ha encontrado algo más que la API de Archicad no puede hacer? Dígalo en un issue, con las cabeceras del DevKit que comprobó, y se añadirá aquí. <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/issues/new/choose">Abrir un issue</a>'
});
