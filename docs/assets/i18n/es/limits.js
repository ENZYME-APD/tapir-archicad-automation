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
    'Not documented': 'No documentado',

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
    'Turn off the automatic story visibility of a railing, so the stories it is shown on can be set':
        'Desactivar la visibilidad automática por plantas de una barandilla, para poder fijar las plantas en las que se muestra',
    'Set the Floor Plan Display, Show Projection, uncut and overhead lines or the cut line type of a mesh':
        'Fijar el Floor Plan Display, el Show Projection, las líneas no cortadas y superiores o el tipo de línea del corte de una malla',
    'Read or set the floor plan symbol of a railing: its break mark, which symbol components it shows, their 2D attributes and the symbol overrides':
        'Leer o fijar el símbolo en planta de una barandilla: su línea de corte, qué componentes del símbolo muestra, sus atributos 2D y las sustituciones del símbolo',
    'Read the slab\'s outline with <a href="../archicad-addon/#GetDetailsOfElements">GetDetailsOfElements</a> and create a static railing along the edge with <a href="../archicad-addon/#CreateRailings">CreateRailings</a>. When the slab changes, delete the railing and create it again.':
        'Lea el contorno del forjado con <a href="../archicad-addon/#GetDetailsOfElements">GetDetailsOfElements</a> y cree una barandilla estática a lo largo del borde con <a href="../archicad-addon/#CreateRailings">CreateRailings</a>. Cuando cambie el forjado, borre la barandilla y vuelva a crearla.',
    'Move a window or door into another wall': 'Mover una ventana o una puerta a otro muro',
    'Delete it and create it in the other wall with <a href="../archicad-addon/#CreateWindows">CreateWindows</a> or <a href="../archicad-addon/#CreateDoors">CreateDoors</a>; a favorite (<code>favoriteName</code>) gives it the same settings.':
        'Bórrela y créela en el otro muro con <a href="../archicad-addon/#CreateWindows">CreateWindows</a> o <a href="../archicad-addon/#CreateDoors">CreateDoors</a>; un favorito (<code>favoriteName</code>) le da los mismos ajustes.',
    'Move a skylight onto another roof or shell': 'Mover un lucernario a otra cubierta o lámina',
    'Delete it and create it on the other roof or shell with <a href="../archicad-addon/#CreateSkylights">CreateSkylights</a>.':
        'Bórrelo y créelo en la otra cubierta o lámina con <a href="../archicad-addon/#CreateSkylights">CreateSkylights</a>.',
    'Turn an automatic zone into a manual one': 'Convertir una zona automática en una manual',
    'Delete the zone and create it again as a manual zone, from its polygon, with <a href="../archicad-addon/#CreateZones">CreateZones</a>.':
        'Borre la zona y vuelva a crearla como zona manual, a partir de su polígono, con <a href="../archicad-addon/#CreateZones">CreateZones</a>.',
    'Change the height of a slanted curtain wall': 'Cambiar la altura de un muro cortina inclinado',
    'Create it again with <a href="../archicad-addon/#CreateCurtainWalls">CreateCurtainWalls</a>, with its <code>angle</code> and the new <code>height</code>.':
        'Vuelva a crearlo con <a href="../archicad-addon/#CreateCurtainWalls">CreateCurtainWalls</a>, con su <code>angle</code> y la nueva <code>height</code>.',
    'Move the lines or the arc of an angle dimension': 'Mover las líneas o el arco de una cota angular',
    'Delete it and create it again with <a href="../archicad-addon/#CreateAngleDimensions">CreateAngleDimensions</a>.':
        'Bórrela y vuelva a crearla con <a href="../archicad-addon/#CreateAngleDimensions">CreateAngleDimensions</a>.',
    'Set the position or the offset of a stair\'s walking line': 'Definir la posición o el desfase de la línea de huella de una escalera',
    'Anchor the sill or header of a window or door to the project zero or to a reference level':
        'Anclar el antepecho o el dintel de una ventana o puerta al cero del proyecto o a un nivel de referencia',
    'Anchor it to a story with <code>verticalLink</code> <code>SillToStory</code> or <code>HeaderToStory</code> in <a href="../archicad-addon/#ModifyWindows">ModifyWindows</a> or <a href="../archicad-addon/#ModifyDoors">ModifyDoors</a>, and work out the height from the story level (<a href="../archicad-addon/#GetStories">GetStories</a>).':
        'Ánclelo a una planta con <code>verticalLink</code> <code>SillToStory</code> o <code>HeaderToStory</code> en <a href="../archicad-addon/#ModifyWindows">ModifyWindows</a> o <a href="../archicad-addon/#ModifyDoors">ModifyDoors</a>, y calcule la altura a partir de la cota de la planta (<a href="../archicad-addon/#GetStories">GetStories</a>).',
    'Read or set the joint type (Vertical Cut and the others) and the dx offset of a stair structure':
        'Leer o fijar el tipo de unión (Vertical Cut y los demás) y el desfase dx de una estructura de escalera',
    'Change the baseline of an existing stair (move a flight end, a landing or a winder) and keep its GUID':
        'Cambiar la línea base de una escalera existente (mover el extremo de un tramo, un rellano o unos peldaños compensados) y conservar su GUID',
    'Recreate the stair on the new baseline with <a href="../archicad-addon/#RecreateStairs">RecreateStairs</a>, which keeps its settings, Element ID, classifications, categories, properties and solid element operations, and returns the old and the new GUID. Its labels are lost.':
        'Vuelva a crear la escalera sobre la nueva línea base con <a href="../archicad-addon/#RecreateStairs">RecreateStairs</a>, que conserva sus ajustes, ID de elemento, clasificaciones, categorías, propiedades y operaciones de elementos sólidos, y devuelve el GUID antiguo y el nuevo. Sus etiquetas se pierden.',
    'Set the left and right boundaries or the walking line of a stair (a custom outline, or a walking line off the middle of the flight)':
        'Fijar los límites izquierdo y derecho o la línea de huella de una escalera (un contorno propio, o una línea de huella fuera del centro del tramo)',
    'Archicad lays them out from the baseline and the flight width: change the baseline with <a href="../archicad-addon/#RecreateStairs">RecreateStairs</a> or the flight width with <a href="../archicad-addon/#ModifyStairs">ModifyStairs</a>. The walking line stays in the middle of the flight. <a href="../archicad-addon/#GetDetailsOfElements">GetDetailsOfElements</a> reads them.':
        'Archicad los traza a partir de la línea base y del ancho del tramo: cambie la línea base con <a href="../archicad-addon/#RecreateStairs">RecreateStairs</a> o el ancho del tramo con <a href="../archicad-addon/#ModifyStairs">ModifyStairs</a>. La línea de huella se queda en el centro del tramo. <a href="../archicad-addon/#GetDetailsOfElements">GetDetailsOfElements</a> los lee.',
    'Set a label’s optimised position, “use text pen as uniform pen” of a symbol label, or a perpendicular text direction':
        'Fijar la posición optimizada de una etiqueta, «use text pen as uniform pen» de una etiqueta de símbolo, o una dirección de texto perpendicular',
    'Set the text direction (parallel, horizontal, vertical or a custom angle) with textWay in <a href="../archicad-addon/#CreateLabels">CreateLabels</a> and <a href="../archicad-addon/#ModifyLabels">ModifyLabels</a>. When the symbol label’s library part has a parameter for the other two, set it with <a href="../archicad-addon/#SetGDLParametersOfElements">SetGDLParametersOfElements</a>; otherwise set them in the Label tool settings or a favorite, and create the labels with favoriteName.':
        'Fije la dirección del texto (paralela, horizontal, vertical o un ángulo propio) con textWay en <a href="../archicad-addon/#CreateLabels">CreateLabels</a> y <a href="../archicad-addon/#ModifyLabels">ModifyLabels</a>. Cuando el objeto de biblioteca de la etiqueta de símbolo tenga un parámetro para las otras dos, fíjelo con <a href="../archicad-addon/#SetGDLParametersOfElements">SetGDLParametersOfElements</a>; si no, ajústelas en la configuración de la herramienta Etiqueta o en un favorito, y cree las etiquetas con favoriteName.',
    'Create a radial dimension of an arc, a circle, a curved wall or a curved slab edge':
        'Crear una cota de radio de un arco, un círculo, un muro curvo o un borde curvo de forjado',
    'Place the radial dimension in Archicad and read it with <a href="../archicad-addon/#GetDetailsOfElements">GetDetailsOfElements</a>, which returns how Archicad stored it. Those readings are what a create command needs.':
        'Coloque la cota de radio en Archicad y léala con <a href="../archicad-addon/#GetDetailsOfElements">GetDetailsOfElements</a>, que devuelve cómo la guardó Archicad. Esos valores son lo que necesita un comando de creación.',

    // Design options
    'Design options': 'Opciones de diseño',
    'Read which design option, or the Main Model, is the default that new elements go to':
        'Leer qué opción de diseño, o el Main Model, es la predeterminada a la que van los elementos nuevos',
    'Create a temporary element, read its design option with <a href="../archicad-addon/#GetDesignOptionForElements">GetDesignOptionForElements</a>, then move it to the Main Model with <a href="../archicad-addon/#MoveElementsToDesignOptions">MoveElementsToDesignOptions</a> (a null GUID as <code>designOptionId</code>) and delete it with <a href="../archicad-addon/#DeleteElements">DeleteElements</a>; an element in an option that is not shown cannot be deleted. Set the default with <a href="../archicad-addon/#SetDefaultDesignOption">SetDefaultDesignOption</a>.':
        'Cree un elemento temporal, lea su opción de diseño con <a href="../archicad-addon/#GetDesignOptionForElements">GetDesignOptionForElements</a>, luego muévalo al Main Model con <a href="../archicad-addon/#MoveElementsToDesignOptions">MoveElementsToDesignOptions</a> (un GUID nulo como <code>designOptionId</code>) y bórrelo con <a href="../archicad-addon/#DeleteElements">DeleteElements</a>; un elemento de una opción que no se muestra no se puede borrar. Fije la predeterminada con <a href="../archicad-addon/#SetDefaultDesignOption">SetDefaultDesignOption</a>.',
    'Read or change which design options the current window shows, as the eye icons of the Design Options palette do':
        'Leer o cambiar qué opciones de diseño muestra la ventana actual, como hacen los iconos de ojo de la paleta Design Options',
    'Work with saved views: read and set the combination a View Map view shows with <a href="../archicad-addon/#GetDesignOptionCombinationOfViews">GetDesignOptionCombinationOfViews</a> and <a href="../archicad-addon/#SetDesignOptionCombinationOfViews">SetDesignOptionCombinationOfViews</a>, and the options of a combination with <a href="../archicad-addon/#SetActiveDesignOptionsInCombinations">SetActiveDesignOptionsInCombinations</a>.':
        'Trabaje con vistas guardadas: lea y fije la combinación que muestra una vista del View Map con <a href="../archicad-addon/#GetDesignOptionCombinationOfViews">GetDesignOptionCombinationOfViews</a> y <a href="../archicad-addon/#SetDesignOptionCombinationOfViews">SetDesignOptionCombinationOfViews</a>, y las opciones de una combinación con <a href="../archicad-addon/#SetActiveDesignOptionsInCombinations">SetActiveDesignOptionsInCombinations</a>.',
    'Duplicate a design option or a design option set, as the duplicate button of the Design Options palette does':
        'Duplicar una opción de diseño o un conjunto de opciones de diseño, como hace el botón de duplicar de la paleta Design Options',
    'Create the set and its options with <a href="../archicad-addon/#CreateDesignOptionSets">CreateDesignOptionSets</a> and <a href="../archicad-addon/#CreateDesignOptions">CreateDesignOptions</a>. For the elements, list them with <a href="../archicad-addon/#GetElementsOfDesignOptions">GetElementsOfDesignOptions</a>, copy them with <a href="../archicad-addon/#CopyElements">CopyElements</a> and move the copies to the new option with <a href="../archicad-addon/#MoveElementsToDesignOptions">MoveElementsToDesignOptions</a>.':
        'Cree el conjunto y sus opciones con <a href="../archicad-addon/#CreateDesignOptionSets">CreateDesignOptionSets</a> y <a href="../archicad-addon/#CreateDesignOptions">CreateDesignOptions</a>. Para los elementos, enumérelos con <a href="../archicad-addon/#GetElementsOfDesignOptions">GetElementsOfDesignOptions</a>, cópielos con <a href="../archicad-addon/#CopyElements">CopyElements</a> y mueva las copias a la nueva opción con <a href="../archicad-addon/#MoveElementsToDesignOptions">MoveElementsToDesignOptions</a>.',
    'Lock or colour the passive elements of design options, as the Design Options palette does':
        'Bloquear o colorear los elementos pasivos de las opciones de diseño, como hace la paleta Design Options',

    // Navigator and views
    'Navigator and views': 'Navegador y vistas',
    'Add an item to a publisher set': 'Añadir un elemento a un publisher set',
    'Add the items in Archicad\'s Organizer, then publish the set with <a href="../archicad-addon/#PublishPublisherSet">PublishPublisherSet</a>.':
        'Añada los elementos en el Organizer de Archicad y luego publique el set con <a href="../archicad-addon/#PublishPublisherSet">PublishPublisherSet</a>.',
    'Change, rename or create a stored Model View Options combination': 'Cambiar, renombrar o crear una combinación guardada de Model View Options',
    'Read the combinations with <a href="../archicad-addon/#GetModelViewOptionDetails">GetModelViewOptionDetails</a>; change the options of the current window with <a href="../archicad-addon/#SetViewOptions">SetViewOptions</a>.':
        'Lea las combinaciones con <a href="../archicad-addon/#GetModelViewOptionDetails">GetModelViewOptionDetails</a>; cambie las opciones de la ventana actual con <a href="../archicad-addon/#SetViewOptions">SetViewOptions</a>.',
    'Make a 3D style the current one': 'Hacer que un estilo 3D sea el actual',
    'Give a view the 3D style with <a href="../archicad-addon/#SetViewSettings">SetViewSettings</a> (<code>d3styleName</code>) and open that view with <a href="../archicad-addon/#ChangeWindow">ChangeWindow</a> (<code>navigatorItemId</code>, from Archicad 27).':
        'Asigne el estilo 3D a una vista con <a href="../archicad-addon/#SetViewSettings">SetViewSettings</a> (<code>d3styleName</code>) y abra esa vista con <a href="../archicad-addon/#ChangeWindow">ChangeWindow</a> (<code>navigatorItemId</code>, desde Archicad 27).',
    'Read or set the floor plan cut plane or the structural analytical model settings saved in a view':
        'Leer o fijar el plano de corte de la planta o los ajustes del modelo analítico estructural guardados en una vista',
    'Set them in the view\'s View Settings in Archicad. The project\'s floor plan cut plane can be set with <a href="../archicad-addon/#SetProjectPreferences">SetProjectPreferences</a>.':
        'Ajústelos en los View Settings de la vista en Archicad. El plano de corte de la planta del proyecto se puede fijar con <a href="../archicad-addon/#SetProjectPreferences">SetProjectPreferences</a>.',
    'Tell a clone folder of the View Map from a normal folder': 'Distinguir una carpeta clon del View Map de una carpeta normal',
    'Give the clone folders a name or ID of their own and find them by it in <a href="../archicad-addon/#GetNavigatorItemTree">GetNavigatorItemTree</a>.':
        'Dé a las carpetas clon un nombre o un ID propio y encuéntrelas por él en <a href="../archicad-addon/#GetNavigatorItemTree">GetNavigatorItemTree</a>.',
    'Change the settings of an existing Layout Book subset (prefix, numbering style, start at, ...)':
        'Cambiar los ajustes de un subconjunto existente del Layout Book (prefijo, estilo de numeración, empezar en, ...)',
    'Read the subset with <a href="../archicad-addon/#GetLayoutSubsetSettings">GetLayoutSubsetSettings</a>, create a subset with the new settings with <a href="../archicad-addon/#CreateLayoutSubset">CreateLayoutSubset</a> and move the layouts into it with <a href="../archicad-addon/#MoveNavigatorItem">MoveNavigatorItem</a>. The old, empty subset is deleted in Archicad.':
        'Lea el subconjunto con <a href="../archicad-addon/#GetLayoutSubsetSettings">GetLayoutSubsetSettings</a>, cree un subconjunto con los nuevos ajustes con <a href="../archicad-addon/#CreateLayoutSubset">CreateLayoutSubset</a> y mueva a él los layouts con <a href="../archicad-addon/#MoveNavigatorItem">MoveNavigatorItem</a>. El subconjunto antiguo, ya vacío, se borra en Archicad.',

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
    'Delete one offset modifier of a profile': 'Borrar un solo modificador de desfase de un perfil',
    'Rebuild the profile with <a href="../archicad-addon/#CreateProfiles">CreateProfiles</a>\' <code>replaceSkins</code> and <code>newSkins</code>, which drops every modifier, then add the ones to keep again with <code>newProfileModifiers</code> (Archicad 27 and later).':
        'Reconstruya el perfil con <code>replaceSkins</code> y <code>newSkins</code> de <a href="../archicad-addon/#CreateProfiles">CreateProfiles</a>, lo que quita todos los modificadores, y luego vuelva a añadir los que quiera conservar con <code>newProfileModifiers</code> (Archicad 27 y posteriores).',
    'Import an IFC translator from a file into the project, or export one to a file':
        'Importar un traductor IFC desde un archivo al proyecto, o exportar uno a un archivo',
    'Keep the office translator in the project template, so new projects have it, and bring it into older projects once with Import… in the IFC Translators dialog. A script can check that a project has it with <a href="../archicad-addon/#GetIFCExportTranslators">GetIFCExportTranslators</a> and export with it by name with <a href="../archicad-addon/#IFCFileOperation">IFCFileOperation</a>.':
        'Guarde el traductor de la oficina en la plantilla de proyecto, para que los proyectos nuevos lo tengan, y llévelo una vez a los proyectos antiguos con Import… en el cuadro de diálogo IFC Translators. Un script puede comprobar que un proyecto lo tiene con <a href="../archicad-addon/#GetIFCExportTranslators">GetIFCExportTranslators</a> y exportar con él, por su nombre, con <a href="../archicad-addon/#IFCFileOperation">IFCFileOperation</a>.',

    // Documents and publishing
    'Documents and publishing': 'Documentos y publicación',
    'Read the rows of an Interactive Schedule': 'Leer las filas de un Interactive Schedule',
    'Build the list yourself: find the elements with <a href="../archicad-addon/#GetElementsByType">GetElementsByType</a> and read their values with <a href="../archicad-addon/#GetPropertyValuesOfElements">GetPropertyValuesOfElements</a> or <a href="../archicad-addon/#GetQuantitiesOfElements">GetQuantitiesOfElements</a>. The <a href="../scripts/">built-in scripts</a> Room Finishes Schedule and Material Take-Off build such lists.':
        'Construya la lista usted mismo: encuentre los elementos con <a href="../archicad-addon/#GetElementsByType">GetElementsByType</a> y lea sus valores con <a href="../archicad-addon/#GetPropertyValuesOfElements">GetPropertyValuesOfElements</a> o <a href="../archicad-addon/#GetQuantitiesOfElements">GetQuantitiesOfElements</a>. Los <a href="../scripts/">scripts incluidos</a> Room Finishes Schedule y Material Take-Off construyen listas así.',
    'Export an Interactive Schedule': 'Exportar un Interactive Schedule',
    'Put the schedule in a publisher set in Archicad once, then publish that set with <a href="../archicad-addon/#PublishPublisherSet">PublishPublisherSet</a>.':
        'Ponga una vez el listado en un publisher set en Archicad y luego publique ese set con <a href="../archicad-addon/#PublishPublisherSet">PublishPublisherSet</a>.',
    'Choose the output format (PDF, DWG, DXF …) of a publisher set per call':
        'Elegir el formato de salida (PDF, DWG, DXF …) de un publisher set en cada llamada',
    'Make one publisher set per format in Archicad’s Organizer once, then publish the one you need with <a href="../archicad-addon/#PublishPublisherSet">PublishPublisherSet</a>; its answer lists the files written (<code>writtenFiles</code>).':
        'Cree una vez un publisher set por formato en el Organizer de Archicad y luego publique el que necesite con <a href="../archicad-addon/#PublishPublisherSet">PublishPublisherSet</a>; su respuesta enumera los archivos escritos (<code>writtenFiles</code>).',
    'Create a revision Change without placing a Change Marker, or change the ID or description of an existing Change':
        'Crear un Change de revisión sin colocar un Change Marker, o cambiar el ID o la descripción de un Change existente',
    'Create the Change together with its first marker with <a href="../archicad-addon/#CreateChangeMarkers">CreateChangeMarkers</a> (createChange). To change an existing Change, use Archicad’s Change Manager.':
        'Cree el Change junto con su primer marcador con <a href="../archicad-addon/#CreateChangeMarkers">CreateChangeMarkers</a> (createChange). Para cambiar un Change existente, use el Change Manager de Archicad.',
    'Set the custom scheme fields of a revision Change': 'Fijar los campos de esquema personalizados de un Change de revisión',
    'Fill them in Archicad’s Change Manager; <a href="../archicad-addon/#GetRevisionChanges">GetRevisionChanges</a> reads them back.':
        'Rellénelos en el Change Manager de Archicad; <a href="../archicad-addon/#GetRevisionChanges">GetRevisionChanges</a> los vuelve a leer.',
    'Add elements to a revision Change, or remove them from it': 'Añadir elementos a un Change de revisión, o quitarlos de él',
    'Add them in Archicad’s Change Manager; <a href="../archicad-addon/#GetRevisionChangesOfElements">GetRevisionChangesOfElements</a> reads which Changes an element belongs to.':
        'Añádalos en el Change Manager de Archicad; <a href="../archicad-addon/#GetRevisionChangesOfElements">GetRevisionChangesOfElements</a> lee a qué Changes pertenece un elemento.',

    // Library parts and GDL
    'Library parts and GDL': 'Objetos de biblioteca y GDL',
    'Run a 2D GDL script given as text, as <a href="../archicad-addon/#InterpretGDLScript3D">InterpretGDLScript3D</a> does with a 3D script':
        'Ejecutar un script GDL 2D dado como texto, como hace <a href="../archicad-addon/#InterpretGDLScript3D">InterpretGDLScript3D</a> con un script 3D',
    'Save the script in a library part with <a href="../archicad-addon/#CreateLibraryParts">CreateLibraryParts</a> (or <a href="../archicad-addon/#SetLibraryPartScripts">SetLibraryPartScripts</a>), then draw its 2D symbol with <a href="../archicad-addon/#GetLibraryPart2DPrimitives">GetLibraryPart2DPrimitives</a>.':
        'Guarde el script en un objeto de biblioteca con <a href="../archicad-addon/#CreateLibraryParts">CreateLibraryParts</a> (o <a href="../archicad-addon/#SetLibraryPartScripts">SetLibraryPartScripts</a>) y luego dibuje su símbolo 2D con <a href="../archicad-addon/#GetLibraryPart2DPrimitives">GetLibraryPart2DPrimitives</a>.',
    'Get the error message and the line number when a GDL script fails':
        'Obtener el mensaje de error y el número de línea cuando falla un script GDL',
    'Check whether the script runs with <a href="../archicad-addon/#InterpretGDLScript3D">InterpretGDLScript3D</a> (3D) or <a href="../archicad-addon/#GetLibraryPart2DPrimitives">GetLibraryPart2DPrimitives</a> (2D). To see the error, open the library part in Archicad’s GDL editor with <a href="../archicad-addon/#OpenLibraryPartInEditor">OpenLibraryPartInEditor</a> (from Archicad 27).':
        'Compruebe si el script se ejecuta con <a href="../archicad-addon/#InterpretGDLScript3D">InterpretGDLScript3D</a> (3D) o <a href="../archicad-addon/#GetLibraryPart2DPrimitives">GetLibraryPart2DPrimitives</a> (2D). Para ver el error, abra el objeto de biblioteca en el editor GDL de Archicad con <a href="../archicad-addon/#OpenLibraryPartInEditor">OpenLibraryPartInEditor</a> (desde Archicad 27).',

    // Archicad's interface
    'Archicad\'s interface': 'La interfaz de Archicad',
    'Read the text of the Session Report window': 'Leer el texto de la ventana Session Report',
    'Open the window for the user with <a href="../archicad-addon/#ChangeWindow">ChangeWindow</a> and windowType \'Report\' (from Archicad 26).':
        'Abra la ventana para el usuario con <a href="../archicad-addon/#ChangeWindow">ChangeWindow</a> y windowType \'Report\' (desde Archicad 26).',
    'Get the outcome of an Action Center check': 'Obtener el resultado de una comprobación del Action Center',
    'Run the check with <a href="../archicad-addon/#RunActionCenterChecks">RunActionCenterChecks</a>; Archicad shows the Action Center when it found an issue. For the hotlink check, read the state of each source with <a href="../archicad-addon/#GetHotlinkSourceStatus">GetHotlinkSourceStatus</a> (from Archicad 27).':
        'Ejecute la comprobación con <a href="../archicad-addon/#RunActionCenterChecks">RunActionCenterChecks</a>; Archicad muestra el Action Center cuando encuentra un problema. Para la comprobación de hotlinks, lea el estado de cada origen con <a href="../archicad-addon/#GetHotlinkSourceStatus">GetHotlinkSourceStatus</a> (desde Archicad 27).',
    'Load a new add-on (.apx or .bundle) while Archicad runs, from an add-on or a script':
        'Cargar un add-on nuevo (.apx o .bundle) mientras Archicad está abierto, desde un add-on o un script',
    'Add it in Archicad\'s Add-On Manager (Options &gt; Add-On Manager &gt; Edit List of Available Add-Ons &gt; Add). An add-on that is already loaded is replaced only after Archicad quits, as the Tapir Installer does when it updates Tapir.':
        'Añádalo en el Add-On Manager de Archicad (Options &gt; Add-On Manager &gt; Edit List of Available Add-Ons &gt; Add). Un add-on que ya está cargado solo se sustituye después de cerrar Archicad, como hace el Tapir Installer cuando actualiza Tapir.',

    'Set the profile modifier values, or the stretched width and height, of a wall, beam or column that uses a profile':
        'Establecer los valores de los modificadores de perfil, o el ancho y la altura estirados, de un muro, una viga o un pilar que usa un perfil',
    'Read them with <a href="../archicad-addon/#GetProfileValuesOfElements">GetProfileValuesOfElements</a>. To change them, change the profile with <a href="../archicad-addon/#CreateProfiles">CreateProfiles</a> (a copy for these elements, or the profile itself), or set them in the settings of the element in Archicad.':
        'Léalos con <a href="../archicad-addon/#GetProfileValuesOfElements">GetProfileValuesOfElements</a>. Para cambiarlos, cambie el perfil con <a href="../archicad-addon/#CreateProfiles">CreateProfiles</a> (una copia para estos elementos, o el propio perfil), o ajústelos en la configuración del elemento en Archicad.',
    'Read or set how a grid element is shown in sections, elevations and interior elevations: its vertical extent and its markers at the top and bottom':
        'Leer o establecer cómo se muestra un elemento de rejilla en secciones, alzados y alzados interiores: su extensión vertical y sus marcadores arriba y abajo',
    'Select the grid elements in Archicad and set it for all of them at once in their settings dialog.':
        'Seleccione los elementos de rejilla en Archicad y ajústelo para todos a la vez en su cuadro de diálogo de configuración.',
    'Read the settings of an IFC translator, such as its type mapping and property mapping, for example to check them as the IFC conformity check of the Missing BIM Data report does':
        'Leer la configuración de un traductor IFC, como su asignación de tipos y su asignación de propiedades, por ejemplo para comprobarla como lo hace la comprobación de conformidad IFC del informe de datos BIM que faltan',
    'Check what the translator gives the elements instead: their IFC type with <a href="../archicad-addon/#GetIFCTypeOfElements">GetIFCTypeOfElements</a>, and from Archicad 28 the properties and classification references the preview translator maps to them with <a href="../archicad-addon/#GetIFCPropertiesOfElements">GetIFCPropertiesOfElements</a> and <a href="../archicad-addon/#GetIFCClassificationReferencesOfElements">GetIFCClassificationReferencesOfElements</a>. <a href="../archicad-addon/#GetAllProperties">GetAllProperties</a> shows whether the properties a mapping needs exist in the project.':
        'Compruebe en su lugar lo que el traductor da a los elementos: su tipo IFC con <a href="../archicad-addon/#GetIFCTypeOfElements">GetIFCTypeOfElements</a> y, desde Archicad 28, las propiedades y referencias de clasificación que el traductor de vista previa les asigna con <a href="../archicad-addon/#GetIFCPropertiesOfElements">GetIFCPropertiesOfElements</a> y <a href="../archicad-addon/#GetIFCClassificationReferencesOfElements">GetIFCClassificationReferencesOfElements</a>. <a href="../archicad-addon/#GetAllProperties">GetAllProperties</a> muestra si existen en el proyecto las propiedades que necesita una asignación.',
    'Find the graphic override rules, Interactive Schedules and SAF translators that refer to a property or classification that no longer exists, as the Missing BIM Data report does':
        'Encontrar las reglas de sustitución gráfica, las tablas interactivas y los traductores SAF que remiten a una propiedad o clasificación que ya no existe, como lo hace el informe de datos BIM que faltan',
    'For graphic override rules (from Archicad 27): read the criterion of each rule with <a href="../archicad-addon/#GetGraphicOverrideRules">GetGraphicOverrideRules</a> (includeCriterion) and compare the properties and classifications it tests with <a href="../archicad-addon/#GetAllProperties">GetAllProperties</a> and <a href="../archicad-addon/#GetAllClassificationSystems">GetAllClassificationSystems</a>. For Interactive Schedules and SAF translators: nothing yet.':
        'Para las reglas de sustitución gráfica (desde Archicad 27): lea el criterio de cada regla con <a href="../archicad-addon/#GetGraphicOverrideRules">GetGraphicOverrideRules</a> (includeCriterion) y compare las propiedades y clasificaciones que comprueba con <a href="../archicad-addon/#GetAllProperties">GetAllProperties</a> y <a href="../archicad-addon/#GetAllClassificationSystems">GetAllClassificationSystems</a>. Para las tablas interactivas y los traductores SAF: nada todavía.',
    'Export and import, all at once, the settings that the dialogs of Archicad save as XML (properties, classifications, the Layout Book, graphic overrides, Model View Options, IFC and DWG translators), to keep several projects in sync':
        'Exportar e importar de una vez la configuración que los diálogos de Archicad guardan como XML (propiedades, clasificaciones, el Libro de presentaciones, sustituciones gráficas, opciones de vista del modelo, traductores IFC y DWG), para mantener varios proyectos sincronizados',
    'Copy what the API can write, one kind at a time, in one script run against each project: properties and classifications from their XML files with <a href="../archicad-addon/#ImportPropertiesXml">ImportPropertiesXml</a> and <a href="../archicad-addon/#ImportClassificationsXml">ImportClassificationsXml</a>; attributes from another project with <a href="../archicad-addon/#ImportAttributesFromProject">ImportAttributesFromProject</a>; graphic override rules with <a href="../archicad-addon/#GetGraphicOverrideRules">GetGraphicOverrideRules</a> and <a href="../archicad-addon/#CreateGraphicOverrideRules">CreateGraphicOverrideRules</a>; Project Info with <a href="../archicad-addon/#GetProjectInfoFields">GetProjectInfoFields</a> and <a href="../archicad-addon/#SetProjectInfoField">SetProjectInfoField</a>; Favorites with <a href="../archicad-addon/#ExportFavorites">ExportFavorites</a> and <a href="../archicad-addon/#ImportFavorites">ImportFavorites</a>. The Layout Book, Model View Options and the translators: nothing yet.':
        'Copie lo que la API puede escribir, un tipo cada vez, en un script que ejecute en cada proyecto: propiedades y clasificaciones desde sus archivos XML con <a href="../archicad-addon/#ImportPropertiesXml">ImportPropertiesXml</a> y <a href="../archicad-addon/#ImportClassificationsXml">ImportClassificationsXml</a>; atributos de otro proyecto con <a href="../archicad-addon/#ImportAttributesFromProject">ImportAttributesFromProject</a>; reglas de sustitución gráfica con <a href="../archicad-addon/#GetGraphicOverrideRules">GetGraphicOverrideRules</a> y <a href="../archicad-addon/#CreateGraphicOverrideRules">CreateGraphicOverrideRules</a>; la información del proyecto con <a href="../archicad-addon/#GetProjectInfoFields">GetProjectInfoFields</a> y <a href="../archicad-addon/#SetProjectInfoField">SetProjectInfoField</a>; favoritos con <a href="../archicad-addon/#ExportFavorites">ExportFavorites</a> y <a href="../archicad-addon/#ImportFavorites">ImportFavorites</a>. El Libro de presentaciones, las opciones de vista del modelo y los traductores: nada todavía.',


    'Found something else the Archicad API cannot do? Say so in an issue, with the DevKit headers you checked, and it is added here. <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/issues/new/choose">Open an issue</a>':
        '¿Ha encontrado algo más que la API de Archicad no puede hacer? Dígalo en un issue, con las cabeceras del DevKit que comprobó, y se añadirá aquí. <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/issues/new/choose">Abrir un issue</a>'
});
