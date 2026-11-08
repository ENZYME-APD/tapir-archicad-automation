/* Español: the Docs page (docs/index.html). See ../README.md. */
(window.TAPIR_I18N = window.TAPIR_I18N || []).push ({
    'Archicad automation docs | Tapir': 'Documentación de automatización de Archicad | Tapir',
    'Tapir\'s documentation: every JSON command, the Grasshopper guide, the Archicad API coverage and the Grasshopper coverage, and how a call to Tapir works.':
        'La documentación de Tapir: cada comando JSON, la guía de Grasshopper, la cobertura de la API de Archicad y la de Grasshopper, y cómo funciona una llamada a Tapir.',
    '<a href="../">Tapir</a> / Docs': '<a href="../">Tapir</a> / Docs',
    'The <em>reference.</em>': 'La <em>referencia.</em>',
    'Every command, what Tapir covers of the Archicad API, and how the Grasshopper components map to it.':
        'Cada comando, lo que Tapir cubre de la API de Archicad y cómo se corresponden con ella los componentes de Grasshopper.',
    'The docs': 'La documentación',

    // The four docs
    'Tapir commands': 'Comandos de Tapir',
    'Search by name, filter by the version a command came in, and open any command for its input and output schema and a Python example.':
        'Busca por nombre, filtra por la versión en la que llegó un comando y abre cualquier comando para ver su esquema de entrada y salida y un ejemplo en Python.',
    'Open the reference': 'Abrir la referencia',
    'Grasshopper guide': 'Guía de Grasshopper',
    'The tool components, one per Archicad tool, with inputs named after the settings dialog. Install, a first definition, and what the buttons do.':
        'Los componentes de herramienta, uno por herramienta de Archicad, con entradas nombradas como en el cuadro de configuración. Instalación, una primera definición y qué hacen los botones.',
    'Read the guide': 'Leer la guía',
    'Archicad API': 'API de Archicad',
    '· <span data-live-value="coverage"></span> covered': '· <span data-live-value="coverage"></span> cubierto',
    'Every function of Graphisoft\'s Archicad API and whether Tapir uses it, is working on it, or skipped it, updated every hour.':
        'Cada función de la API de Archicad de Graphisoft y si Tapir la usa, está trabajando en ella o la ha descartado, actualizado cada hora.',
    'See the coverage': 'Ver la cobertura',
    'Components': 'Componentes',
    'Which Tapir commands have a Grasshopper component, and which inputs each component still lacks.':
        'Qué comandos de Tapir tienen un componente de Grasshopper y qué entradas le faltan aún a cada componente.',

    // The API limits teaser (its title is in common.js)
    'What the Archicad API can\'t do, such as undo, the clipboard or the rows of a schedule, with what to do instead and the DevKit evidence.':
        'Lo que la API de Archicad no puede hacer, como deshacer, el portapapeles o las filas de un listado, con lo que puede hacer en su lugar y la prueba en el DevKit.',
    'See the limits': 'Ver los límites',

    // How it works
    'How it works': 'Cómo funciona',
    'How it <em>works.</em>': 'Cómo <em>funciona.</em>',
    'Tapir is an Archicad add-on that turns native API functions into a stateless JSON server on port 19723. Anything that can send an HTTP request can drive Archicad.':
        'Tapir es un add-on de Archicad que convierte funciones nativas de la API en un servidor JSON sin estado en el puerto 19723. Cualquier cosa que pueda enviar una petición HTTP puede controlar Archicad.',
    'Grasshopper, Python scripts and AI tools send JSON over HTTP to the Tapir Add-On, which calls Archicad\'s C++ API on the BIM model.':
        'Grasshopper, los scripts de Python y las herramientas de IA envían JSON por HTTP al add-on Tapir, que llama a la API C++ de Archicad sobre el modelo BIM.',
    '<strong>Grasshopper</strong><span>The tapir components</span>': '<strong>Grasshopper</strong><span>Los componentes tapir</span>',
    '<strong>Python scripts</strong><span>The palette, or your own</span>': '<strong>Scripts de Python</strong><span>Los de la paleta o los tuyos</span>',
    '<strong>AI and MCP</strong><span>Assistants and MCP servers</span>': '<strong>IA y MCP</strong><span>Asistentes y servidores MCP</span>',
    '↓ JSON over HTTP ↓': '↓ JSON por HTTP ↓',
    '<strong>Tapir Add-On</strong><span>A stateless JSON layer: one command per request</span>':
        '<strong>Add-on Tapir</strong><span>Una capa JSON sin estado: un comando por petición</span>',
    '<strong>Archicad</strong><span>The BIM model and its C++ API</span>': '<strong>Archicad</strong><span>El modelo BIM y su API C++</span>',

    // Anatomy of a call
    'Anatomy of a call': 'Anatomía de una llamada',
    'Every Tapir command is sent the same way; only <code>commandName</code> and <code>addOnCommandParameters</code> change. <a href="../start/#python">Your first Python script</a>':
        'Todos los comandos de Tapir se envían igual; solo cambian <code>commandName</code> y <code>addOnCommandParameters</code>. <a href="../start/#python">Tu primer script en Python</a>',

    // More guides
    'More guides': 'Más guías',
    'Install and run a script': 'Instalar y ejecutar un script',
    'Install Tapir, run a script from the palette and write your first Python script.':
        'Instala Tapir, ejecuta un script desde la paleta y escribe tu primer script en Python.',
    'Get started': 'Empezar',
    'AI': 'IA',
    'MCP servers and scripts an AI assistant writes for you.': 'Servidores MCP y scripts que un asistente de IA escribe por ti.',
    'Use Tapir with AI': 'Usar Tapir con IA',
    'Learn by watching': 'Aprende viendo',
    'Six videos, from installing Tapir to whole workflows.': 'Seis vídeos, desde la instalación de Tapir hasta flujos de trabajo completos.',
    'Watch the tutorials': 'Ver los tutoriales'
});
