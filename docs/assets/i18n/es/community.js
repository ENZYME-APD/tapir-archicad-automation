/* Español: the Community page (community/index.html, with the texts community/community.js reads
   from it). See ../README.md. */
(window.TAPIR_I18N = window.TAPIR_I18N || []).push ({
    'Community – Archicad automation with Tapir': 'Comunidad – automatización de Archicad con Tapir',
    'Ask on Discord, report a bug, test a fix in your Archicad: how Tapir\'s community and its bots turn a report into a release, and how the project grew.':
        'Pregunte en Discord, informe de un error, pruebe una corrección en su Archicad: cómo la comunidad de Tapir y sus bots convierten un informe en una nueva versión, y cómo ha crecido el proyecto.',
    '<a href="../">Tapir</a> / Community': '<a href="../">Tapir</a> / Comunidad',
    'Ask. Report. <em>Test.</em>': 'Pregunte. Informe. <em>Pruebe.</em>',
    'Architects, developers and the maintainers in one place. Bugs and ideas become fixes that someone with your Archicad version tests before they ship.':
        'Arquitectos, desarrolladores y los mantenedores en un solo lugar. Los errores y las ideas se convierten en correcciones que alguien con su versión de Archicad prueba antes de publicarlas.',

    // Where to find us
    'Where to find us': 'Dónde encontrarnos',
    'Ask anything': 'Pregunte lo que quiera',
    'Architects, developers and the maintainers, in one server. Show what you made and talk with the people who build Tapir.':
        'Arquitectos, desarrolladores y los mantenedores, en un solo servidor. Muestre lo que ha hecho y hable con las personas que desarrollan Tapir.',
    'Join Discord': 'Unirse a Discord',
    'Learn the API': 'Aprenda la API',
    'A video series from the first command to full workflows.': 'Una serie de vídeos, desde el primer comando hasta flujos de trabajo completos.',
    'Watch the tutorials': 'Ver los tutoriales',
    'AI assistants': 'Asistentes de IA',
    'Talk to your model': 'Hable con su modelo',
    'MCP servers let Claude and other assistants read and change a project through Tapir.':
        'Los servidores MCP permiten que Claude y otros asistentes lean y modifiquen un proyecto a través de Tapir.',

    // How the bots work
    'From report to release': 'Del informe a la versión',
    'How the bots <em>work.</em>': 'Cómo trabajan <em>los bots.</em>',
    'Most fixes are written by bots and checked by people. The maintainers set the rules and can stop any change; you decide whether a fix works in your Archicad.':
        'La mayoría de las correcciones las escriben bots y las revisan personas. Los mantenedores fijan las reglas y pueden detener cualquier cambio; usted decide si una corrección funciona en su Archicad.',
    '<b>01</b><strong>Post it</strong><span>Describe the bug or idea on Discord or GitHub. A GitHub account is not needed on Discord.</span>':
        '<b>01</b><strong>Publíquelo</strong><span>Describa el error o la idea en Discord o GitHub. En Discord no necesita una cuenta de GitHub.</span>',
    '<b>02</b><strong>An issue</strong><span>Every half hour, Discord posts that read like a bug report or a feature request become GitHub issues, and the bot replies with the link.</span>':
        '<b>02</b><strong>Un issue</strong><span>Cada media hora, los mensajes de Discord que parecen un informe de error o una petición de función se convierten en issues de GitHub, y el bot responde con el enlace.</span>',
    '<b>03</b><strong>A draft fix</strong><span>The issue bot answers, asks for what it is missing, or writes the fix and opens a pull request. It is built for every Archicad version and reviewed.</span>':
        '<b>03</b><strong>Un borrador de corrección</strong><span>El bot de issues responde, pide lo que le falta, o escribe la corrección y abre una pull request. Se compila para cada versión de Archicad y se revisa.</span>',
    '<b>04</b><strong>You test</strong><span>A comment on the pull request links the built add-on and asks the person who reported it to try it.</span>':
        '<b>04</b><strong>Usted prueba</strong><span>Un comentario en la pull request enlaza el add-on compilado y pide a la persona que informó del error que lo pruebe.</span>',
    '<b>05</b><strong>Released</strong><span>Green and reviewed, it is merged three days later unless a maintainer stops it, and ships in Monday\'s <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/releases">release</a>.</span>':
        '<b>05</b><strong>Publicada</strong><span>En verde y revisada, se fusiona tres días después salvo que un mantenedor la detenga, y sale en la <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/releases">versión</a> del lunes.</span>',

    // Report and test
    'Report and test': 'Informar y probar',
    'Report': 'Informar',
    'How to report a bug': 'Cómo informar de un error',
    'Post it on Discord or open an issue on GitHub; both reach the same place. A report that can be fixed in one go says:':
        'Publíquelo en Discord o abra un issue en GitHub; ambos llegan al mismo sitio. Un informe que se puede corregir de una vez indica:',
    'your Archicad version and build, and its language (for example 29.0.0 (3100) INT);':
        'su versión y build de Archicad, y su idioma (por ejemplo 29.0.0 (3100) INT);',
    'your Tapir version (the <em>About Tapir</em> dialog shows it);':
        'su versión de Tapir (el cuadro de diálogo <em>About Tapir</em> la muestra);',
    'what you did: the script or Grasshopper definition, or the command and the JSON you sent;':
        'lo que hizo: el script o la definición de Grasshopper, o el comando y el JSON que envió;',
    'what you expected and what happened instead, with the error message;':
        'lo que esperaba y lo que ocurrió en su lugar, con el mensaje de error;',
    'whether the project is a Teamwork project, and which libraries are loaded, when it matters.':
        'si el proyecto es un proyecto Teamwork y qué bibliotecas están cargadas, cuando importe.',
    '<strong class="callout-label"><span aria-hidden="true">⚠</span> Public</strong> Issues and Discord channels are public. Leave out client names, project names and file paths you don\'t want to share.':
        '<strong class="callout-label"><span aria-hidden="true">⚠</span> Público</strong> Los issues y los canales de Discord son públicos. No incluya nombres de clientes, nombres de proyectos ni rutas de archivos que no quiera compartir.',
    'Open an issue on GitHub': 'Abrir un issue en GitHub',
    'Post on Discord': 'Publicar en Discord',
    'Test': 'Probar',
    'How to test a fix': 'Cómo probar una corrección',
    'A fix is only known to work once someone has run it in Archicad. The bots can build Tapir but cannot run Archicad, so your test is what matters most.':
        'Solo se sabe que una corrección funciona cuando alguien la ha ejecutado en Archicad. Los bots pueden compilar Tapir pero no ejecutar Archicad, así que su prueba es lo que más importa.',
    'Open the pull request linked from your issue. A comment on it lists the downloads, one per Archicad version and platform. Downloading needs a GitHub login.':
        'Abra la pull request enlazada desde su issue. Un comentario en ella enumera las descargas, una por versión de Archicad y plataforma. Para descargar hay que iniciar sesión en GitHub.',
    'Unzip the download for your Archicad version. On macOS it holds another zip with the <code>.bundle</code>.':
        'Descomprima la descarga para su versión de Archicad. En macOS contiene otro zip con el <code>.bundle</code>.',
    'In Archicad, open <em>Options &gt; Add-On Manager &gt; Edit List of Available Add-Ons</em>, remove or disable the installed Tapir, then <em>Add</em> the <code>.apx</code> (Windows) or <code>.bundle</code> (macOS) and click OK.':
        'En Archicad, abra <em>Options &gt; Add-On Manager &gt; Edit List of Available Add-Ons</em>, quite o desactive el Tapir instalado, luego añada con <em>Add</em> el <code>.apx</code> (Windows) o el <code>.bundle</code> (macOS) y haga clic en OK.',
    'Do what went wrong before, in a copy of your project.':
        'Repita lo que fallaba antes, en una copia de su proyecto.',
    'Say on the pull request whether it works and fixes your case. Afterwards, put your installed Tapir back the same way.':
        'Diga en la pull request si funciona y soluciona su caso. Después, vuelva a poner su Tapir instalado de la misma manera.',
    'Testing often? The <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/blob/main/tools/community-test-runner/GUIDE.md">community test runner</a> tests open pull requests on your own Archicad while it is closed, leaves your Tapir in place and posts the result. It runs on Windows and needs Python and a GitHub account.':
        '¿Prueba a menudo? El <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/blob/main/tools/community-test-runner/GUIDE.md">community test runner</a> prueba las pull requests abiertas en su propio Archicad mientras está cerrado, deja su Tapir en su sitio y publica el resultado. Funciona en Windows y necesita Python y una cuenta de GitHub.',

    // Momentum (community.js fills in the numbers)
    '{sign}{diff} in {days} days': '{sign}{diff} en {days} días',
    'Momentum': 'Impulso',
    'Commits': 'Commits',
    'Discord messages': 'Mensajes de Discord',
    'Discord members <span class="change"></span>': 'Miembros de Discord <span class="change"></span>',
    'People taking part, last 12 months': 'Personas que participan, últimos 12 meses',
    'Top contributors': 'Principales colaboradores',
    'Counted: commits (co-authors included), issues they reported that a merged fix closed, and pull requests they tested in Archicad (passed or failed).':
        'Se cuentan los commits (coautores incluidos), los issues que informaron y que cerró una corrección fusionada, y las pull requests que probaron en Archicad (con éxito o no).',
    '{count} commit': '{count} commit',
    '{count} commits': '{count} commits',
    '{count} report': '{count} informe',
    '{count} reports': '{count} informes',
    '{count} test': '{count} prueba',
    '{count} tests': '{count} pruebas',

    // Growth
    'Commits, issues and pull requests, and Discord messages per month, stacked, with the number of Discord members as a line, {first} to {last}. The table below lists every number.':
        'Commits, issues y pull requests, y mensajes de Discord por mes, apilados, con el número de miembros de Discord como línea, de {first} a {last}. La tabla de abajo recoge todos los números.',
    'Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec': 'ene feb mar abr may jun jul ago sept oct nov dic',
    'Growth': 'Crecimiento',
    'Growth since <em>2022.</em>': 'Crecimiento desde <em>2022.</em>',
    'Everything that happens around Tapir each month, from the first commit at Enzyme to today. Only numbers are collected, never the text of a message.':
        'Todo lo que ocurre alrededor de Tapir cada mes, desde el primer commit en Enzyme hasta hoy. Solo se recogen números, nunca el texto de un mensaje.',
    '<i class="swatch layer-github"></i>Issues and pull requests': '<i class="swatch layer-github"></i>Issues y pull requests',
    '<i class="swatch layer-discordMessages"></i>Discord messages': '<i class="swatch layer-discordMessages"></i>Mensajes de Discord',
    '<i class="swatch swatch-line"></i>Discord members, right scale': '<i class="swatch swatch-line"></i>Miembros de Discord, escala derecha',
    'Per month, stacked, on a square-root scale so the early years stay visible. Commits by people, without merges and bots; Discord messages include threads and forum posts. Discord members are the people in the server at the end of each month, counted by when they joined; people who later left are not included.':
        'Por mes, apilados, en una escala de raíz cuadrada para que los primeros años sigan siendo visibles. Commits de personas, sin merges ni bots; los mensajes de Discord incluyen hilos y publicaciones de foro. Los miembros de Discord son las personas en el servidor al final de cada mes, contadas según cuándo se unieron; no se incluye a quienes se fueron después.',
    'Show the numbers as a table': 'Mostrar los números en una tabla',
    'Month': 'Mes',
    'Issues opened': 'Issues abiertos',
    'Pull requests merged': 'Pull requests fusionadas',
    'Discord members': 'Miembros de Discord',

    // Help build it
    'Help build it': 'Ayude a construirlo',
    'Made a script your office uses? Share it, and any office can install it from the palette.':
        '¿Ha hecho un script que usa su estudio? Compártalo, y cualquier estudio podrá instalarlo desde la paleta.',
    'How to share': 'Cómo compartir',
    'Ask for a feature': 'Pedir una función',
    'How features are requested, and what is being asked for now.': 'Cómo se piden las funciones y qué se está pidiendo ahora.',
    'See the roadmap': 'Ver la hoja de ruta',
    'Code': 'Código',
    'Write a command': 'Escribir un comando',
    'The add-on is C++ on the Archicad API. The development guide shows how to add a command.':
        'El add-on está escrito en C++ sobre la API de Archicad. La guía de desarrollo muestra cómo añadir un comando.',
    'Read the guide': 'Leer la guía',
    'Get involved': 'Participe',
    'We\'re building a motivated, diverse community around Archicad automation.':
        'Estamos construyendo una comunidad motivada y diversa en torno a la automatización de Archicad.',
    'Say hello on Discord, and tell us what you automate in Archicad.':
        'Salude en Discord y cuéntenos qué automatiza en Archicad.',
    'Join the Discord': 'Unirse al Discord'
});
