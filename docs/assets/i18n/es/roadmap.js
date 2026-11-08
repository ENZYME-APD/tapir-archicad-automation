/* Español: the Roadmap page (roadmap/index.html). See ../README.md.
   Command names, the label enhancement and the names of the issue form and of the old list stay English. */
(window.TAPIR_I18N = window.TAPIR_I18N || []).push ({
    'Tapir roadmap – Archicad automation': 'Hoja de ruta de Tapir – automatización de Archicad',
    'How to ask for a feature in Tapir, the Archicad add-on, what people are asking for now, what shipped from the old list and which ideas are still open.':
        'Cómo pedir una función en Tapir, el add-on para Archicad, qué se está pidiendo ahora, qué ha salido de la lista antigua y qué ideas siguen abiertas.',
    '<a href="../">Tapir</a> / <a href="../community/">Community</a> / Roadmap':
        '<a href="../">Tapir</a> / <a href="../community/">Comunidad</a> / Hoja de ruta',
    'What\'s <em>next.</em>': 'Lo que <em>viene.</em>',
    'Tapir grows from requests. Most new commands, components and fixes start as a message from someone who needed them in their own work.':
        'Tapir crece a partir de peticiones. La mayoría de los nuevos comandos, componentes y correcciones empiezan como el mensaje de alguien que los necesitaba en su propio trabajo.',

    // How to ask for a feature
    'How to ask for a <em>feature.</em>': 'Cómo pedir una <em>función.</em>',
    'Four steps from "I need this" to a Monday release.': 'Cuatro pasos desde «Necesito esto» hasta una versión publicada un lunes.',
    'Say what you want to do': 'Diga qué quiere hacer',
    'Post it on Discord or open an issue on GitHub. Describe the task in Archicad, not only the command: "number the doors along a line", "read the area of every zone". A Discord message that reads like a feature request becomes a GitHub issue by itself, labelled <code>enhancement</code>.':
        'Escríbalo en Discord o abra un issue en GitHub. Describa la tarea en Archicad, no solo el comando: «numerar las puertas a lo largo de una línea», «leer la superficie de cada zona». Un mensaje de Discord que parece una petición de función se convierte por sí solo en un issue de GitHub, con la etiqueta <code>enhancement</code>.',
    'Asking for a part of the Archicad API?': '¿Quiere una parte de la API de Archicad?',
    'The <a href="../api-coverage/">API coverage</a> page lists every function of the Archicad API and whether Tapir has a command for it. Use the <em>Archicad API request</em> issue form to ask for one that is missing.':
        'La página <a href="../api-coverage/">Cobertura de la API</a> enumera cada función de la API de Archicad e indica si Tapir tiene un comando para ella. Use el formulario de issue <em>Archicad API request</em> para pedir una que falte.',
    'The issue bot picks it up': 'El bot de issues se encarga',
    'A clear, wanted request is built like a fix: the bot writes it and opens a pull request, and you are asked to try it. A large one is split into parts. When the design is open, the bot describes the choices and a maintainer decides.':
        'Una petición clara y deseada se desarrolla como una corrección: el bot la escribe y abre un pull request, y se le pide a usted que la pruebe. Una grande se divide en partes. Cuando el diseño está abierto, el bot describe las opciones y decide un mantenedor.',
    'It ships on a Monday': 'Sale un lunes',
    'Merged changes are released every Monday. The release notes list them, and the add-on offers to update itself.':
        'Los cambios fusionados se publican cada lunes. Las notas de la versión los enumeran, y el add-on ofrece actualizarse solo.',

    // What is being asked for now
    'What is being asked for now': 'Lo que se pide ahora',
    'Requests': 'Peticiones',
    'Open feature requests': 'Peticiones de funciones abiertas',
    'Every open issue labelled <code>enhancement</code>. Add a comment or a reaction to one you need too.':
        'Cada issue abierto con la etiqueta <code>enhancement</code>. Añada un comentario o una reacción al que usted también necesite.',
    'See the requests': 'Ver las peticiones',
    'Plan': 'Plan',
    'Project board': 'Tablero del proyecto',
    'The maintainers\' board of planned and ongoing work.': 'El tablero de los mantenedores, con el trabajo previsto y en curso.',
    'Open the board': 'Abrir el tablero',
    'In progress': 'En curso',
    'Open pull requests': 'Pull requests abiertos',
    'Changes being built, reviewed and tested right now.': 'Cambios que se están desarrollando, revisando y probando ahora mismo.',
    'See the pull requests': 'Ver los pull requests',
    'Shipped': 'Publicado',
    'Releases': 'Versiones',
    'Every release, with its notes and downloads. The <span class="nowrap">command reference</span> shows the version each command came in.':
        'Cada versión, con sus notas y descargas. La <span class="nowrap">referencia de comandos</span> muestra la versión en la que llegó cada comando.',
    'See the releases': 'Ver las versiones',

    // Recently shipped
    'Recently shipped': 'Publicado recientemente',
    'Already <em>here.</em>': 'Ya <em>está aquí.</em>',
    'Asked for on the old site\'s "What\'s cooking" list, and now part of Tapir.':
        'Pedido en la lista «What\'s cooking» del antiguo sitio, y ahora parte de Tapir.',
    'Stairs, roofs and curtain walls': 'Escaleras, cubiertas y muros cortina',
    'Read and change them: <a href="../archicad-addon/#ModifyStairs"><code>ModifyStairs</code></a>, <a href="../archicad-addon/#ModifyRoofs"><code>ModifyRoofs</code></a> and <a href="../archicad-addon/#ModifyCurtainWalls"><code>ModifyCurtainWalls</code></a>.':
        'Leerlos y modificarlos: <a href="../archicad-addon/#ModifyStairs"><code>ModifyStairs</code></a>, <a href="../archicad-addon/#ModifyRoofs"><code>ModifyRoofs</code></a> y <a href="../archicad-addon/#ModifyCurtainWalls"><code>ModifyCurtainWalls</code></a>.',
    'Building materials and composites': 'Materiales de construcción y compuestos',
    'Create them from a script: <a href="../archicad-addon/#CreateBuildingMaterials"><code>CreateBuildingMaterials</code></a> and <a href="../archicad-addon/#CreateComposites"><code>CreateComposites</code></a>.':
        'Crearlos desde un script: <a href="../archicad-addon/#CreateBuildingMaterials"><code>CreateBuildingMaterials</code></a> y <a href="../archicad-addon/#CreateComposites"><code>CreateComposites</code></a>.',
    'Dialogs from Python': 'Diálogos desde Python',
    'A script shows its own window in Archicad: <a href="../archicad-addon/#ShowScriptUI"><code>ShowScriptUI</code></a> and <a href="../archicad-addon/#GetScriptUIResult"><code>GetScriptUIResult</code></a>.':
        'Un script muestra su propia ventana en Archicad: <a href="../archicad-addon/#ShowScriptUI"><code>ShowScriptUI</code></a> y <a href="../archicad-addon/#GetScriptUIResult"><code>GetScriptUIResult</code></a>.',
    '2D detailing': 'Detalles 2D',
    'Lines, fills and text: <a href="../archicad-addon/#CreateLineElements"><code>CreateLineElements</code></a>, <a href="../archicad-addon/#CreateHatches"><code>CreateHatches</code></a> and <a href="../archicad-addon/#CreateTexts"><code>CreateTexts</code></a>.':
        'Líneas, tramas y textos: <a href="../archicad-addon/#CreateLineElements"><code>CreateLineElements</code></a>, <a href="../archicad-addon/#CreateHatches"><code>CreateHatches</code></a> y <a href="../archicad-addon/#CreateTexts"><code>CreateTexts</code></a>.',
    'Your own properties': 'Sus propias propiedades',
    'Add property definitions from a script: <a href="../archicad-addon/#CreatePropertyDefinitions"><code>CreatePropertyDefinitions</code></a>.':
        'Añada definiciones de propiedades desde un script: <a href="../archicad-addon/#CreatePropertyDefinitions"><code>CreatePropertyDefinitions</code></a>.',
    'Built-in scripts': 'Scripts incluidos',
    'Ready to run from the palette, from numbering to a project health check. <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/tree/main/builtin-scripts">See them on GitHub</a>.':
        'Listos para ejecutar desde la paleta, de la numeración a la revisión del estado del proyecto. <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/tree/main/builtin-scripts">Verlos en GitHub</a>.',

    // Ideas
    'Ideas': 'Ideas',
    'Still <em>open.</em>': 'Aún <em>abierto.</em>',
    'Two ideas from the old list that nobody has asked for in an issue yet. Want one? Open the issue.':
        'Dos ideas de la antigua lista que nadie ha pedido todavía en un issue. ¿Quiere una? Abra el issue.',
    'Idea': 'Idea',
    'Clearer error messages in Python': 'Mensajes de error más claros en Python',
    'Tracebacks and command errors that say what went wrong and how to fix it, so a script is easier to troubleshoot.':
        'Tracebacks y errores de comando que digan qué ha fallado y cómo arreglarlo, para que sea más fácil depurar un script.',
    'Ask for it': 'Pedirla',
    'Tapir scripts in CI': 'Scripts de Tapir en CI',
    'Running Tapir scripts automatically, for example in a GitHub Actions pipeline.':
        'Ejecutar scripts de Tapir automáticamente, por ejemplo en un pipeline de GitHub Actions.',

    // Next step
    'Next step': 'Siguiente paso',
    'Missing something in your work?': '¿Le falta algo en su trabajo?',
    'Tell us what you want to automate': 'Díganos qué quiere automatizar',
    'Open an issue, or describe it on Discord and the bot files it for you.':
        'Abra un issue, o descríbalo en Discord y el bot lo crea por usted.',
    'Request a feature on GitHub': 'Pedir una función en GitHub',
    'Ask on Discord': 'Preguntar en Discord'
});
