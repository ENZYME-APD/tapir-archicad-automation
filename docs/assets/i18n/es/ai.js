/* Español: the Tapir and AI page (ai/index.html). See ../README.md.
   The two prompts are translated too (an AI tool understands them in any language), the Python
   example stays as it is. */
(window.TAPIR_I18N = window.TAPIR_I18N || []).push ({
    'Control Archicad with AI assistants (MCP) | Tapir': 'Controlar Archicad con asistentes de IA (MCP) | Tapir',
    'Control Archicad with Claude and other AI assistants through Tapir: the Tapir MCP servers, a prompt to start with, and how to let an AI write a script.':
        'Controla Archicad con Claude y otros asistentes de IA a través de Tapir: los servidores MCP de Tapir, un prompt para empezar y cómo dejar que una IA escriba un script.',
    '<a href="../">Tapir</a> / <a href="../docs/">Docs</a> / AI': '<a href="../">Tapir</a> / <a href="../docs/">Documentación</a> / IA',
    'Tapir and <em>AI.</em>': 'Tapir e <em>IA.</em>',
    'An AI assistant can read your Archicad project and change it through Tapir. You ask in plain words; it sends the commands to the Archicad running on your computer.':
        'Un asistente de IA puede leer tu proyecto de Archicad y cambiarlo a través de Tapir. Tú lo pides con palabras sencillas; él envía los comandos al Archicad que tienes abierto en tu ordenador.',

    // Two ways
    'Two ways to <em>connect.</em>': 'Dos formas de <em>conectar.</em>',
    'Both talk to the same Tapir commands on port 19723, on your own computer.':
        'Las dos usan los mismos comandos de Tapir en el puerto 19723, en tu propio ordenador.',
    'While you chat': 'Mientras chateas',
    'An MCP server': 'Un servidor MCP',
    'The AI tool calls Tapir\'s commands itself, while you chat. Works with tools that support the Model Context Protocol (MCP), such as Claude Desktop.':
        'La herramienta de IA llama ella misma a los comandos de Tapir mientras chateas. Funciona con herramientas compatibles con el Model Context Protocol (MCP), como Claude Desktop.',
    'Tapir MCP servers': 'Servidores MCP para Tapir',
    'Any AI tool': 'Cualquier herramienta de IA',
    'A script the AI writes': 'Un script que escribe la IA',
    'Any AI tool can write a short Python script that sends Tapir commands. You read it, then run it from the Tapir palette or a terminal.':
        'Cualquier herramienta de IA puede escribir un script corto en Python que envíe comandos de Tapir. Tú lo lees y luego lo ejecutas desde la paleta de Tapir o un terminal.',
    'How it works': 'Cómo funciona',

    // MCP servers
    'MCP servers': 'Servidores MCP',
    'Tapir MCP <em>servers.</em>': 'Servidores MCP <em>para Tapir.</em>',
    'These are community projects, not part of Tapir. Each has its own install steps in its README. Which one Tapir recommends is still being decided in <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/issues/2070">issue #2070</a>, so they are listed here without a recommendation.':
        'Son proyectos de la comunidad, no forman parte de Tapir. Cada uno explica su instalación en su README. Cuál recomienda Tapir aún se está decidiendo en la <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/issues/2070">issue #2070</a>, así que aquí aparecen sin recomendación.',
    'By SzamosiMate': 'De SzamosiMate',
    'The MCP server for Tapir that the others started from. It is also published on PyPI as the <span class="mono">tapir-archicad-mcp</span> package.':
        'El servidor MCP para Tapir del que partieron los demás. También está publicado en PyPI como el paquete <span class="mono">tapir-archicad-mcp</span>.',
    'Its repository': 'Su repositorio',
    'The package on PyPI': 'El paquete en PyPI',
    'By the community': 'De la comunidad',
    'Its forks': 'Sus forks',
    'Versions of it that others changed and published. Check when each was last updated.':
        'Versiones que otros han modificado y publicado. Comprueba cuándo se actualizó cada una por última vez.',
    'See the forks': 'Ver los forks',
    '<strong class="callout-label"><span aria-hidden="true">⚠</span> Careful</strong> An MCP server can change your model. Try it on a copy of your project first, and read what the AI is about to do before you allow it.':
        '<strong class="callout-label"><span aria-hidden="true">⚠</span> Cuidado</strong> Un servidor MCP puede cambiar tu modelo. Pruébalo primero en una copia de tu proyecto y lee lo que la IA va a hacer antes de permitirlo.',

    // A prompt to start with
    'A prompt to start with': 'Un prompt para empezar',
    'Start by <em>reading.</em>': 'Empieza por <em>leer.</em>',
    'Open a project in Archicad, with the Tapir Add-On loaded, and give your AI tool this prompt. It only reads, so it is safe to try on any project.':
        'Abre un proyecto en Archicad, con el Add-On de Tapir cargado, y dale este prompt a tu herramienta de IA. Solo lee, así que puedes probarlo sin riesgo en cualquier proyecto.',
    'When the answer looks right, ask for a change in the same plain words, and say exactly which elements it may change: "Set the property X to Y on the selected walls only."':
        'Cuando la respuesta parezca correcta, pide un cambio con las mismas palabras sencillas y di exactamente qué elementos puede cambiar: «Pon la propiedad X a Y solo en los muros seleccionados».',
    'prompt': 'prompt',
    'read only': 'solo lectura',
    'Use Tapir to look at the Archicad project I have open. Do not change anything; only read. 1. List the stories with their names and levels. 2. Count the walls, slabs, columns, doors and windows on each story. 3. Show the result as one table, one row per story. Tell me which Tapir commands you used.':
        'Usa Tapir para revisar el proyecto de Archicad que tengo abierto.\nNo cambies nada; solo lee.\n1. Enumera las plantas con sus nombres y niveles.\n2. Cuenta los muros, forjados, pilares, puertas y ventanas de cada planta.\n3. Muestra el resultado en una sola tabla, una fila por planta.\nDime qué comandos de Tapir has usado.',

    // Check the names first
    'Check the names first': 'Comprueba primero los nombres',
    'Check names, <em>don\'t guess.</em>': 'Comprueba los nombres, <em>no adivines.</em>',
    'Your words rarely match the names in the project: "fire rating" may be a property called <span class="mono">FireRating</span> in one template and <span class="mono">Feuerwiderstand</span> in another, and "level 2" may be the story "02 First Floor". An AI that guesses the name still gets an answer, only a wrong one: no values found, so "no doors have a fire rating".':
        'Tus palabras rara vez coinciden con los nombres del proyecto: «resistencia al fuego» puede ser una propiedad llamada <span class="mono">FireRating</span> en una plantilla y <span class="mono">Feuerwiderstand</span> en otra, y «nivel 2» puede ser la planta «02 First Floor». Una IA que adivina el nombre obtiene igualmente una respuesta, solo que equivocada: no encuentra valores, así que «ninguna puerta tiene resistencia al fuego».',
    '<b aria-hidden="true">1</b><strong>Look the words up first</strong><span>Before a command whose parameters come from your words (a story, property, classification, layer, building material, composite, surface, zone category or element type), the AI sends those words to <a href="../archicad-addon/#ResolveNames">ResolveNames</a>, with the kind of each when it knows it. It only reads.</span>':
        '<b aria-hidden="true">1</b><strong>Busca primero las palabras</strong><span>Antes de un comando cuyos parámetros salen de tus palabras (una planta, propiedad, clasificación, capa, material de construcción, compuesto, superficie, categoría de zona o tipo de elemento), la IA envía esas palabras a <a href="../archicad-addon/#ResolveNames">ResolveNames</a>, con el tipo de cada una cuando lo sabe. Este comando solo lee.</span>',
    '<b aria-hidden="true">2</b><strong>Use what it returns</strong><span><span class="mono">found</span>: the item named exactly so, with the ids the other commands take (story index, attribute id, elementType, propertyId, classification ids). <span class="mono">closeMatches</span>: only similar names, best first; the AI asks you which one you meant instead of picking one itself.</span>':
        '<b aria-hidden="true">2</b><strong>Usa lo que devuelve</strong><span><span class="mono">found</span>: el elemento que se llama exactamente así, con los ids que usan los demás comandos (índice de planta, id de atributo, elementType, propertyId, ids de clasificación). <span class="mono">closeMatches</span>: solo nombres parecidos, los mejores primero; la IA te pregunta a cuál te referías en lugar de elegir uno por su cuenta.</span>',
    '<b aria-hidden="true">3</b><strong>Say "not in this project"</strong><span><span class="mono">notFound</span>: nothing in the project is close, so the project has no such item. The AI says "this project has no X" instead of guessing a name or reporting an empty result as the answer.</span>':
        '<b aria-hidden="true">3</b><strong>Di «no está en este proyecto»</strong><span><span class="mono">notFound</span>: nada en el proyecto se le parece, así que el proyecto no tiene ese elemento. La IA dice «este proyecto no tiene X» en lugar de adivinar un nombre o dar un resultado vacío como respuesta.</span>',
    'Add these lines to your prompt, or to the instructions of your MCP server, so the AI checks the names every time. They work the same when the AI writes a script: the script sends <span class="mono">ResolveNames</span> first and stops with a message on <span class="mono">notFound</span>.':
        'Añade estas líneas a tu prompt, o a las instrucciones de tu servidor MCP, para que la IA compruebe los nombres cada vez. Funcionan igual cuando la IA escribe un script: el script envía primero <span class="mono">ResolveNames</span> y se detiene con un mensaje ante <span class="mono">notFound</span>.',
    'Before a change, ask for a preview. The commands that delete or replace, such as <a href="../archicad-addon/#DeleteElements">DeleteElements</a>, <a href="../archicad-addon/#DeleteAttributes">DeleteAttributes</a> and <a href="../archicad-addon/#DeleteAndReplaceAttributes">DeleteAndReplaceAttributes</a>, take <span class="mono">"dryRun": true</span>: nothing changes, and the response lists what would. The command reference says which commands have it.':
        'Antes de un cambio, pide una vista previa. Los comandos que borran o sustituyen, como <a href="../archicad-addon/#DeleteElements">DeleteElements</a>, <a href="../archicad-addon/#DeleteAttributes">DeleteAttributes</a> y <a href="../archicad-addon/#DeleteAndReplaceAttributes">DeleteAndReplaceAttributes</a>, aceptan <span class="mono">"dryRun": true</span>: no cambia nada, y la respuesta enumera lo que cambiaría. La referencia de comandos indica cuáles lo tienen.',
    'add to any request': 'añádelo a cualquier petición',
    'Before you use a name taken from my words (a story, property, classification, layer, building material, composite, surface, zone category or element type), check it with the Tapir command ResolveNames. - found: use the ids it returns. - closeMatches: ask me which one I mean. - notFound: tell me this project has no such item. Never guess a name. Before a command that deletes or replaces, run it with "dryRun": true if it has that input, and show me what it would change.':
        'Antes de usar un nombre sacado de mis palabras (una planta,\npropiedad, clasificación, capa, material de construcción,\ncompuesto, superficie, categoría de zona o tipo de elemento),\ncompruébalo con el comando de Tapir ResolveNames.\n- found: usa los ids que devuelve.\n- closeMatches: pregúntame a cuál me refiero.\n- notFound: dime que este proyecto no tiene ese elemento.\n  Nunca adivines un nombre.\nAntes de un comando que borra o sustituye, ejecútalo con\n"dryRun": true si tiene esa entrada, y muéstrame qué\ncambiaría.',

    // When the AI writes a script
    'When the AI writes a script': 'Cuando la IA escribe un script',
    'Describe it, <em>run it.</em>': 'Descríbelo, <em>ejecútalo.</em>',
    'Without an MCP server, describe the task, give the AI the command list, and run the script it writes.':
        'Sin servidor MCP, describe la tarea, dale a la IA la lista de comandos y ejecuta el script que escriba.',
    '<b aria-hidden="true">1</b><strong>Describe what you want</strong><span>For example: "Extract all composite exterior walls on level 2 and calculate their net volume."</span>':
        '<b aria-hidden="true">1</b><strong>Describe lo que quieres</strong><span>Por ejemplo: «Extrae todos los muros exteriores compuestos de la planta 2 y calcula su volumen neto».</span>',
    '<b aria-hidden="true">2</b><strong>Share the command list</strong><span>Give the AI the <a href="../archicad-addon/">command reference</a> and the working example below. Ask for Python\'s standard library only, on port 19723.</span>':
        '<b aria-hidden="true">2</b><strong>Comparte la lista de comandos</strong><span>Dale a la IA la <a href="../archicad-addon/">referencia de comandos</a> y el ejemplo funcional de abajo. Pide que use solo la biblioteca estándar de Python, en el puerto 19723.</span>',
    '<b aria-hidden="true">3</b><strong>Read it, then run it</strong><span>Read the script before you run it, from the Tapir palette or a terminal. Try one that changes the model on a copy of your project first.</span>':
        '<b aria-hidden="true">3</b><strong>Léelo y luego ejecútalo</strong><span>Lee el script antes de ejecutarlo desde la paleta de Tapir o un terminal. Prueba primero en una copia de tu proyecto un script que cambie el modelo.</span>',
    'for Claude or any AI tool': 'para Claude o cualquier herramienta de IA',
    'Write a Python script that uses only the standard library and sends Tapir commands to Archicad on http://127.0.0.1:19723, like the example I attach. Use the Tapir command reference I attach. Extract all composite exterior walls on level 2 and calculate their net volume. Only read the model; change nothing.':
        'Escribe un script de Python que use solo la\nbiblioteca estándar y envíe comandos de Tapir a\nArchicad en http://127.0.0.1:19723, como el\nejemplo que adjunto.\nUsa la referencia de comandos de Tapir que adjunto.\n\nExtrae todos los muros exteriores compuestos de la\nplanta 2 y calcula su volumen neto.\n\nSolo lee el modelo; no cambies nada.',

    // Next step
    'Next step': 'Siguiente paso',
    'Install Tapir': 'Instala Tapir',
    'Install Tapir, then give your AI tool the prompt above.': 'Instala Tapir y luego dale a tu herramienta de IA el prompt de arriba.',
    'Get started': 'Empezar',
    'The command reference': 'La referencia de comandos',
    'The list to give the AI: every command with its schema.': 'La lista que darle a la IA: cada comando con su esquema.',
    'Open the reference': 'Abrir la referencia',
    'Show what you built': 'Enseña lo que has hecho',
    'Ask about MCP servers and share your prompts.': 'Pregunta por los servidores MCP y comparte tus prompts.',
    'Join Discord': 'Únete a Discord'
});
