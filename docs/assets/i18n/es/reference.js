/* Español: the heads, headings and introductions of the reference pages (archicad-addon/, api-coverage/,
   gh-coverage/), which stay mostly English (data-i18n-partial). See ../README.md. */
(window.TAPIR_I18N = window.TAPIR_I18N || []).push ({
    // The command reference (archicad-addon/)
    'Archicad JSON API commands reference | Tapir': 'Referencia de comandos de la JSON API de Archicad | Tapir',
    'Every JSON command the Tapir add-on adds to Archicad, with its parameters, its response and a Python example, and how to call one from Python.':
        'Cada comando JSON que el add-on Tapir añade a Archicad, con sus parámetros, su respuesta y un ejemplo en Python, y cómo llamar a uno desde Python.',
    '<a href="../">Tapir</a> / <a href="../docs/">Docs</a> / Commands': '<a href="../">Tapir</a> / <a href="../docs/">Docs</a> / Comandos',
    'Tapir <em>commands.</em>': 'Comandos de <em>Tapir.</em>',
    'The JSON commands the Tapir add-on registers in Archicad, in the <code>TapirCommand</code> namespace, next to Graphisoft\'s official commands. Each command shows its parameters, its response and an example.':
        'Los comandos JSON que el add-on Tapir registra en Archicad, en el espacio de nombres <code>TapirCommand</code>, junto a los comandos oficiales de Graphisoft. Cada comando muestra sus parámetros, su respuesta y un ejemplo.',
    'New to Tapir? <a href="../start/">Get started</a> shows how to install it and run a first script. Looking for something no command does? <a href="../limits/">What the Archicad API can\'t do</a> lists what Graphisoft\'s API does not offer, with what to do instead.':
        '¿Es nuevo en Tapir? <a href="../start/">Empezar</a> muestra cómo instalarlo y ejecutar un primer script. ¿Busca algo que ningún comando hace? <a href="../limits/">Lo que la API de Archicad no puede hacer</a> enumera lo que la API de Graphisoft no ofrece, con lo que puede hacer en su lugar.',
    'Calling a <em>command.</em>': 'Llamar a un <em>comando.</em>',
    'Archicad answers JSON commands on <code>http://127.0.0.1:19723</code> while a project is open (the next Archicads opened use 19724, 19725 and so on). A Tapir command is sent through Archicad\'s <code>API.ExecuteAddOnCommand</code> command.':
        'Archicad responde a comandos JSON en <code>http://127.0.0.1:19723</code> mientras hay un proyecto abierto (los siguientes Archicad que se abran usan 19724, 19725, etc.). Un comando de Tapir se envía a través del comando <code>API.ExecuteAddOnCommand</code> de Archicad.',
    'Python standard library': 'Biblioteca estándar de Python',
    'Graphisoft\'s <a href="https://pypi.org/project/archicad/"><code>archicad</code> package</a>':
        'El <a href="https://pypi.org/project/archicad/">paquete <code>archicad</code></a> de Graphisoft',
    'The examples of the commands below use <code>aclib</code>, the small helper in <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/tree/main/archicad-addon/Examples">archicad-addon/Examples</a> that sends commands the same way as the standard-library script.':
        'Los ejemplos de los comandos de abajo usan <code>aclib</code>, el pequeño módulo auxiliar de <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/tree/main/archicad-addon/Examples">archicad-addon/Examples</a>, que envía los comandos igual que el script con la biblioteca estándar.',
    'All <em>commands.</em>': 'Todos los <em>comandos.</em>',

    // The two coverage pages
    'Overview': 'Resumen',
    'By group': 'Por grupo',
    'Checklist': 'Lista de verificación',

    // The Archicad API coverage (api-coverage/)
    'Archicad API coverage checklist | Tapir': 'Lista de cobertura de la API de Archicad | Tapir',
    'Which parts of Graphisoft\'s Archicad API the Tapir add-on uses, item by item, kept up to date from every issue, pull request and push to the repository.':
        'Qué partes de la API de Archicad de Graphisoft usa el add-on Tapir, elemento por elemento, actualizado con cada issue, pull request y push al repositorio.',
    '<a href="../">Tapir</a> / <a href="../docs/">Docs</a> / API coverage': '<a href="../">Tapir</a> / <a href="../docs/">Docs</a> / Cobertura de la API',
    'Archicad API <em>coverage.</em>': 'Cobertura de la <em>API de Archicad.</em>',
    'What Tapir covers of Graphisoft\'s Archicad API, item by item.': 'Lo que Tapir cubre de la API de Archicad de Graphisoft, elemento por elemento.',
    'Which parts of the Archicad API (<span id="archicad-range">every supported Archicad version</span>: C functions with their older names and command ids, and the C++ API) the <a href="https://github.com/ENZYME-APD/tapir-archicad-automation">Tapir Add-On</a> uses, updated within minutes of every issue and push to main, and within the hour of every pull request. See also the <a href="../archicad-addon/">command reference</a>.':
        'Qué partes de la API de Archicad (<span id="archicad-range">cada versión de Archicad compatible</span>: las funciones C con sus nombres antiguos e identificadores de comando, y la API C++) usa el <a href="https://github.com/ENZYME-APD/tapir-archicad-automation">Add-On Tapir</a>, actualizado a los pocos minutos de cada issue y push a main, y en menos de una hora tras cada pull request. Vea también la <a href="../archicad-addon/">referencia de comandos</a>.',
    'By part of the API': 'Por parte de la API',

    // The Grasshopper coverage (gh-coverage/)
    'Grasshopper coverage of the Tapir commands | Tapir': 'Cobertura en Grasshopper de los comandos de Tapir | Tapir',
    'Which Tapir commands for Archicad the components of the Tapir Grasshopper plugin call, and whether they cover each command\'s parameters or still miss some.':
        'Qué comandos de Tapir para Archicad llaman los componentes del plugin de Grasshopper de Tapir, y si cubren los parámetros de cada comando o aún les falta alguno.',
    '<a href="../">Tapir</a> / <a href="../docs/">Docs</a> / Grasshopper coverage': '<a href="../">Tapir</a> / <a href="../docs/">Docs</a> / Cobertura de Grasshopper',
    'Grasshopper <em>coverage.</em>': 'Cobertura de <em>Grasshopper.</em>',
    'Which Tapir commands the Grasshopper components call, and the parameters they still miss.':
        'Qué comandos de Tapir llaman los componentes de Grasshopper, y qué parámetros les faltan todavía.',
    'Which JSON commands of the <a href="https://github.com/ENZYME-APD/tapir-archicad-automation">Tapir Add-On</a> a component of the Tapir Grasshopper plugin calls, and whether its components cover the command\'s parameters, updated within minutes of every issue and push to main, and within the hour of every pull request. This is a static check of the plugin\'s code: it shows which parameters the components name, not that each one works. See also the <a href="../archicad-addon/">command reference</a> and the <a href="../api-coverage/">Archicad API coverage</a>.':
        'Qué comandos JSON del <a href="https://github.com/ENZYME-APD/tapir-archicad-automation">Add-On Tapir</a> llama un componente del plugin de Grasshopper de Tapir, y si sus componentes cubren los parámetros del comando, actualizado a los pocos minutos de cada issue y push a main, y en menos de una hora tras cada pull request. Es una comprobación estática del código del plugin: muestra qué parámetros nombran los componentes, no que cada uno funcione. Vea también la <a href="../archicad-addon/">referencia de comandos</a> y la <a href="../api-coverage/">cobertura de la API de Archicad</a>.',
    'Parameters of the used commands': 'Parámetros de los comandos usados'
});
