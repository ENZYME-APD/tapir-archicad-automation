/* Español: the Grasshopper page (grasshopper/index.html). See ../README.md.
   Component, input and button names stay English, as Grasshopper shows them. */
(window.TAPIR_I18N = window.TAPIR_I18N || []).push ({
    'Archicad Grasshopper live connection | Tapir': 'Conexión en vivo entre Archicad y Grasshopper | Tapir',
    'Connect Grasshopper live to a running Archicad with the Tapir plugin: read, change and create Archicad elements from Rhino geometry, without writing code.':
        'Conectar Grasshopper en vivo con un Archicad abierto mediante el plugin de Tapir: leer, cambiar y crear elementos de Archicad a partir de geometría de Rhino, sin escribir código.',
    '<a href="../">Tapir</a> / <a href="../docs/">Docs</a> / Grasshopper':
        '<a href="../">Tapir</a> / <a href="../docs/">Documentación</a> / Grasshopper',
    'Grasshopper, <em>visually.</em>': 'Grasshopper, <em>de forma visual.</em>',
    'The Tapir plugin connects Grasshopper to a running Archicad: read elements and their settings, change them, and create new ones from Rhino geometry, without writing code.':
        'El plugin de Tapir conecta Grasshopper con un Archicad abierto: lee elementos y sus parámetros, cámbialos y crea otros nuevos a partir de geometría de Rhino, sin escribir código.',

    // What the components do
    'What the components <em>do.</em>': 'Qué hacen los <em>componentes.</em>',
    'Each component sends a Tapir command to Archicad and gives you the answer as Grasshopper data. You find them on the <strong>Tapir</strong> tab, grouped as in Archicad.':
        'Cada componente envía un comando de Tapir a Archicad y te devuelve la respuesta como datos de Grasshopper. Los encontrarás en la pestaña <strong>Tapir</strong>, agrupados como en Archicad.',
    '<b>READ</b><strong>Elements and their data</strong><span>Elements by type, story or selection; their properties, classifications, settings and geometry.</span>':
        '<b>LEER</b><strong>Elementos y sus datos</strong><span>Elementos por tipo, planta o selección; sus propiedades, clasificaciones, parámetros y geometría.</span>',
    '<b>CHANGE</b><strong>What you pass in</strong><span>Property values, classifications, layers, IDs and tool settings of the elements you pass in.</span>':
        '<b>CAMBIAR</b><strong>Lo que le pasas</strong><span>Valores de propiedades, clasificaciones, capas, IDs y parámetros de herramienta de los elementos que le pasas.</span>',
    '<b>CREATE</b><strong>From Rhino geometry</strong><span>Walls, doors, windows, railings, level dimensions and more, from points and curves.</span>':
        '<b>CREAR</b><strong>A partir de geometría de Rhino</strong><span>Muros, puertas, ventanas, barandillas, cotas de nivel y más, a partir de puntos y curvas.</span>',
    '<b>PROJECT</b><strong>The rest of the file</strong><span>Stories, attributes, navigator views, layouts, issues, favorites and libraries.</span>':
        '<b>PROYECTO</b><strong>El resto del archivo</strong><span>Plantas, atributos, vistas del navegador, layouts, incidencias, favoritos y bibliotecas.</span>',
    'Which Tapir commands have a component, and which settings each covers, is on the <a href="../gh-coverage/">Grasshopper coverage</a> page.':
        'Qué comandos de Tapir tienen un componente, y qué parámetros cubre cada uno, está en la página de <a href="../gh-coverage/">cobertura de Grasshopper</a>.',

    // Tool components
    'Tool components': 'Componentes de herramienta',
    'One per Archicad <em>tool.</em>': 'Uno por <em>herramienta</em> de Archicad.',
    'A tool component works like the Archicad tool of the same name. One JSON input and one JSON output per panel of its settings dialog, keyed by the labels you see there, such as <code>Wall Height</code>.':
        'Un componente de herramienta funciona como la herramienta de Archicad del mismo nombre. Una entrada JSON y una salida JSON por cada panel de su cuadro de configuración, con las etiquetas en inglés que ves allí como claves, por ejemplo <code>Wall Height</code>.',
    'Placed by <code>StartPoint</code> and <code>EndPoint</code>.': 'Se coloca con <code>StartPoint</code> y <code>EndPoint</code>.',
    'Placed in an <code>OwnerWall</code> at a <code>CenterOffset</code>.': 'Se coloca en un <code>OwnerWall</code> a un <code>CenterOffset</code>.',
    'Placed along a <code>ReferenceLine</code>.': 'Se coloca a lo largo de una <code>ReferenceLine</code>.',
    'Placed at a <code>Position</code>, optionally on a parent element.': 'Se coloca en una <code>Position</code>, opcionalmente sobre un elemento padre.',
    '<b>REFRESH</b><strong>Reads</strong><span>Reads the elements in <code>ElementGuid</code> and fills every output.</span>':
        '<b>REFRESH</b><strong>Lee</strong><span>Lee los elementos de <code>ElementGuid</code> y rellena todas las salidas.</span>',
    '<b>EXECUTE</b><strong>Writes</strong><span>Writes the given settings to those elements. Only the keys you give are written.</span>':
        '<b>EXECUTE</b><strong>Escribe</strong><span>Escribe los parámetros dados en esos elementos. Solo se escriben las claves que indicas.</span>',
    '<b>CREATE</b><strong>Makes new elements</strong><span>From the geometry inputs, starting from the tool\'s defaults or from a <code>FavoriteName</code>.</span>':
        '<b>CREATE</b><strong>Crea elementos nuevos</strong><span>A partir de las entradas de geometría, partiendo de los valores por defecto de la herramienta o de un <code>FavoriteName</code>.</span>',
    'Nothing is written to Archicad until you press Execute or Create. A JSON output read by Refresh can be fed straight back into the same input. Archicad has no command to change railings or level dimensions after they are placed, so for those Execute only writes the ID, layer and draw order.':
        'No se escribe nada en Archicad hasta que pulsas Execute o Create. Una salida JSON leída por Refresh puede volver directamente a la misma entrada. Archicad no tiene ningún comando para cambiar barandillas o cotas de nivel una vez colocadas, así que para ellas Execute solo escribe el ID, la capa y el orden de visualización.',
    '<strong class="callout-label"><span aria-hidden="true">ⓘ</span> Note</strong> Walls remembers the walls its Create made, in the <code>.gh</code> file. The next Create updates them instead of making new ones. Turn this off, or forget them, in the component\'s menu.':
        '<strong class="callout-label"><span aria-hidden="true">ⓘ</span> Nota</strong> Walls recuerda los muros que creó su Create, en el archivo <code>.gh</code>. El siguiente Create los actualiza en lugar de crear otros nuevos. Puedes desactivarlo, u olvidarlos, en el menú del componente.',

    // Install
    'Install': 'Instalación',
    'Install the Tapir Add-On in Archicad': 'Instala el Add-On de Tapir en Archicad',
    'The components talk to Archicad through the Add-On. <a href="../start/#install">Get started</a> shows how.':
        'Los componentes se comunican con Archicad a través del Add-On. <a href="../start/#install">Empezar</a> muestra cómo instalarlo.',
    'Install the <span class="mono">tapir</span> package in Rhino': 'Instala el paquete <span class="mono">tapir</span> en Rhino',
    'In Rhino 7 or 8, open <em>Tools &gt; Package Manager</em> (or type <code>PackageManager</code>), search for <strong>tapir</strong> and click Install. Restart Rhino.':
        'En Rhino 7 u 8, abre <em>Tools &gt; Package Manager</em> (o escribe <code>PackageManager</code>), busca <strong>tapir</strong> y haz clic en Install. Reinicia Rhino.',
    'Open Archicad, then Grasshopper': 'Abre Archicad y luego Grasshopper',
    'With a project open in Archicad, start Grasshopper. The <strong>Tapir</strong> tab appears in its toolbar. With more than one Archicad open, give the port of the one you want to the <strong>ConnectArchicad</strong> component (<em>Tapir &gt; General</em>); the first Archicad uses 19723.':
        'Con un proyecto abierto en Archicad, inicia Grasshopper. La pestaña <strong>Tapir</strong> aparece en su barra de herramientas. Si hay más de un Archicad abierto, pasa el puerto del que quieras al componente <strong>ConnectArchicad</strong> (<em>Tapir &gt; General</em>); el primer Archicad usa el 19723.',

    // A first definition
    'A first definition': 'Una primera definición',
    'One <em>wall.</em>': 'Un <em>muro.</em>',
    'Build this in an empty Grasshopper canvas, with a project open in Archicad:':
        'Monta esto en un lienzo vacío de Grasshopper, con un proyecto abierto en Archicad:',
    'Place two <strong>Point</strong> parameters and set them to 0,0,0 and 5,0,0. Archicad reads the numbers as metres.':
        'Coloca dos parámetros <strong>Point</strong> y ponlos en 0,0,0 y 5,0,0. Archicad lee los números como metros.',
    'Place the <strong>Walls</strong> component from <em>Tapir &gt; Design Tools</em>. Connect the points to <code>StartPoint</code> and <code>EndPoint</code>.':
        'Coloca el componente <strong>Walls</strong> de <em>Tapir &gt; Design Tools</em>. Conecta los puntos a <code>StartPoint</code> y <code>EndPoint</code>.',
    'Place a <strong>Panel</strong> with the text on the right and connect it to <code>GeometryAndPositioning_JSON</code>.':
        'Coloca un <strong>Panel</strong> con el texto de la derecha y conéctalo a <code>GeometryAndPositioning_JSON</code>.',
    'Press <strong>Create</strong>. A 5 m wall appears in Archicad, 3 m high and 30 cm thick.':
        'Pulsa <strong>Create</strong>. Aparece en Archicad un muro de 5 m, de 3 m de alto y 30 cm de grosor.',
    'Move a point and press Create again: the same wall moves. Change the panel and press Create: the same wall changes.':
        'Mueve un punto y vuelve a pulsar Create: el mismo muro se mueve. Cambia el panel y pulsa Create: el mismo muro cambia.',
    'Press <strong>Refresh</strong> to see every setting of the wall in the JSON outputs. Copy a key from there to set it the next time.':
        'Pulsa <strong>Refresh</strong> para ver todos los parámetros del muro en las salidas JSON. Copia de ahí una clave para fijarla la próxima vez.',

    // Next step
    'Next step': 'Siguiente paso',
    'Install the Add-On': 'Instala el Add-On',
    'Install Tapir and the <span class="mono">tapir</span> package, then try the wall above.':
        'Instala Tapir y el paquete <span class="mono">tapir</span>, y luego prueba el muro de arriba.',
    'Get started': 'Empezar',
    'Coverage': 'Cobertura',
    'Which commands have a component, and which inputs each still lacks.':
        'Qué comandos tienen un componente, y qué entradas le faltan aún a cada uno.',
    'See the coverage': 'Ver la cobertura',
    'Videos': 'Vídeos',
    'Watch it work': 'Míralo en acción',
    'The Archicad and Grasshopper tutorials.': 'Los tutoriales de Archicad y Grasshopper.',
    'Watch the tutorials': 'Ver los tutoriales'
});
