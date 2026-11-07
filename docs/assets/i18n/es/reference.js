/* Español: the heads, headings and introductions of the reference pages (archicad-addon/, api-coverage/,
   gh-coverage/), which stay mostly English (data-i18n-partial). See ../README.md. */
(window.TAPIR_I18N = window.TAPIR_I18N || []).push ({
    // The command reference (archicad-addon/)
    'Tapir commands - Tapir documentation': 'Comandos de Tapir - Documentación de Tapir',
    'Every JSON command of the Tapir add-on for Archicad, with its parameters, its response and an example.':
        'Cada comando JSON del add-on Tapir para Archicad, con sus parámetros, su respuesta y un ejemplo.',
    '<a href="../">Tapir</a> / <a href="../docs/">Docs</a> / Commands': '<a href="../">Tapir</a> / <a href="../docs/">Docs</a> / Comandos',
    'Tapir <em>commands.</em>': 'Comandos de <em>Tapir.</em>',
    'The JSON commands the Tapir add-on registers in Archicad, in the <code>TapirCommand</code> namespace, next to Graphisoft\'s official commands. Each command shows its parameters, its response and an example.':
        'Los comandos JSON que el add-on Tapir registra en Archicad, en el espacio de nombres <code>TapirCommand</code>, junto a los comandos oficiales de Graphisoft. Cada comando muestra sus parámetros, su respuesta y un ejemplo.',
    'New to Tapir? <a href="../start/">Get started</a> shows how to install it and run a first script.':
        '¿Es nuevo en Tapir? <a href="../start/">Empezar</a> muestra cómo instalarlo y ejecutar un primer script.',
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
    'Archicad API coverage - Tapir documentation': 'Cobertura de la API de Archicad - Documentación de Tapir',
    'Which parts of the Archicad API the Tapir add-on uses, kept up to date from every issue, pull request and push.':
        'Qué partes de la API de Archicad usa el add-on Tapir, actualizado con cada issue, pull request y push.',
    '<a href="../">Tapir</a> / <a href="../docs/">Docs</a> / API coverage': '<a href="../">Tapir</a> / <a href="../docs/">Docs</a> / Cobertura de la API',
    'Archicad API <em>coverage.</em>': 'Cobertura de la <em>API de Archicad.</em>',
    'What Tapir covers of Graphisoft\'s Archicad API, item by item.': 'Lo que Tapir cubre de la API de Archicad de Graphisoft, elemento por elemento.',
    'Which parts of the Archicad API (<span id="archicad-range">every supported Archicad version</span>: C functions with their older names and command ids, and the C++ API) the <a href="https://github.com/ENZYME-APD/tapir-archicad-automation">Tapir Add-On</a> uses, updated within minutes of every issue and push to main, and within the hour of every pull request. See also the <a href="../archicad-addon/">command reference</a>.':
        'Qué partes de la API de Archicad (<span id="archicad-range">cada versión de Archicad compatible</span>: las funciones C con sus nombres antiguos e identificadores de comando, y la API C++) usa el <a href="https://github.com/ENZYME-APD/tapir-archicad-automation">Add-On Tapir</a>, actualizado a los pocos minutos de cada issue y push a main, y en menos de una hora tras cada pull request. Vea también la <a href="../archicad-addon/">referencia de comandos</a>.',
    'By part of the API': 'Por parte de la API',

    // The Grasshopper coverage (gh-coverage/)
    'Grasshopper coverage - Tapir documentation': 'Cobertura de Grasshopper - Documentación de Tapir',
    'Which Tapir commands the components of the Tapir Grasshopper plugin call, and whether they cover the commands\' parameters.':
        'Qué comandos de Tapir llaman los componentes del plugin de Grasshopper de Tapir, y si cubren los parámetros de los comandos.',
    '<a href="../">Tapir</a> / <a href="../docs/">Docs</a> / Grasshopper coverage': '<a href="../">Tapir</a> / <a href="../docs/">Docs</a> / Cobertura de Grasshopper',
    'Grasshopper <em>coverage.</em>': 'Cobertura de <em>Grasshopper.</em>',
    'Which Tapir commands the Grasshopper components call, and the parameters they still miss.':
        'Qué comandos de Tapir llaman los componentes de Grasshopper, y qué parámetros les faltan todavía.',
    'Which JSON commands of the <a href="https://github.com/ENZYME-APD/tapir-archicad-automation">Tapir Add-On</a> a component of the Tapir Grasshopper plugin calls, and whether its components cover the command\'s parameters, updated within minutes of every issue and push to main, and within the hour of every pull request. This is a static check of the plugin\'s code: it shows which parameters the components name, not that each one works. See also the <a href="../archicad-addon/">command reference</a> and the <a href="../api-coverage/">Archicad API coverage</a>.':
        'Qué comandos JSON del <a href="https://github.com/ENZYME-APD/tapir-archicad-automation">Add-On Tapir</a> llama un componente del plugin de Grasshopper de Tapir, y si sus componentes cubren los parámetros del comando, actualizado a los pocos minutos de cada issue y push a main, y en menos de una hora tras cada pull request. Es una comprobación estática del código del plugin: muestra qué parámetros nombran los componentes, no que cada uno funcione. Vea también la <a href="../archicad-addon/">referencia de comandos</a> y la <a href="../api-coverage/">cobertura de la API de Archicad</a>.',
    'Parameters of the used commands': 'Parámetros de los comandos usados'
});
