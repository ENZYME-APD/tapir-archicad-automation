/* Español: the Get started page (start/index.html). See ../README.md.
   The menu, palette and button names of Archicad and Tapir stay English, as the add-on shows them. */
(window.TAPIR_I18N = window.TAPIR_I18N || []).push ({
    'Automate Archicad with Python – get started | Tapir': 'Automatizar Archicad con Python – primeros pasos | Tapir',
    'Install Tapir, run a script from the Tapir palette in Archicad, add more scripts, write a first Python script and install the Grasshopper plugin.':
        'Instalar Tapir, ejecutar un script desde la paleta de Tapir en Archicad, añadir más scripts, escribir un primer script en Python e instalar el plugin de Grasshopper.',
    '<a href="../">Tapir</a> / Get started': '<a href="../">Tapir</a> / Empezar',
    'Get <em>started.</em>': 'Para <em>empezar.</em>',
    'From download to your first script in about ten minutes. Steps 1 to 3 need no programming.':
        'De la descarga a tu primer script en unos diez minutos. Los pasos 1 a 3 no requieren programar.',
    '<strong class="callout-label"><span aria-hidden="true">▶</span> Video</strong> Prefer watching? The <a href="../tutorials/">Tapir video tutorials</a> walk through installing Tapir and using it.':
        '<strong class="callout-label"><span aria-hidden="true">▶</span> Vídeo</strong> ¿Prefieres verlo? Los <a href="../tutorials/">vídeos tutoriales de Tapir</a> (en inglés) muestran cómo instalar Tapir y usarlo.',

    // 1. Install
    'Install Tapir': 'Instalar Tapir',
    '<strong>With the installer (recommended).</strong> It finds every Archicad version on the computer and installs the matching add-on into each, so Archicad loads it on its next start.':
        '<strong>Con el instalador (recomendado).</strong> Encuentra todas las versiones de Archicad del ordenador e instala en cada una el add-on correspondiente, para que Archicad lo cargue en su próximo inicio.',
    'Download <span class="os">WINDOWS</span>': 'Descargar <span class="os">WINDOWS</span>',
    'Download <span class="os">MACOS</span>': 'Descargar <span class="os">MACOS</span>',
    '<strong>Windows:</strong> the installer asks for administrator rights, because it copies the add-on into the Archicad folder under Program Files.':
        '<strong>Windows:</strong> el instalador pide derechos de administrador, porque copia el add-on en la carpeta de Archicad dentro de Program Files.',
    '<strong>macOS:</strong> unzip the download and open the app.':
        '<strong>macOS:</strong> descomprime la descarga y abre la aplicación.',
    '<strong>Prefer to install by hand?</strong> Download the <code>TapirAddOn_AC&lt;version&gt;_Win.apx</code> or <code>TapirAddOn_AC&lt;version&gt;_Mac.zip</code> file for your Archicad version from the <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/releases/latest">latest release</a>, then in Archicad: <em>Options &gt; Add-On Manager &gt; Edit List of Available Add-Ons &gt; Add</em>, browse to the file and click <em>OK</em>.':
        '<strong>¿Prefieres instalarlo a mano?</strong> Descarga el archivo <code>TapirAddOn_AC&lt;version&gt;_Win.apx</code> o <code>TapirAddOn_AC&lt;version&gt;_Mac.zip</code> de tu versión de Archicad desde la <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/releases/latest">última versión publicada</a>, y luego en Archicad: <em>Options &gt; Add-On Manager &gt; Edit List of Available Add-Ons &gt; Add</em>, busca el archivo y haz clic en <em>OK</em>.',
    'Tapir keeps itself up to date: it tells you when a new version is out and installs it for you.':
        'Tapir se mantiene actualizado solo: te avisa cuando sale una nueva versión y la instala por ti.',

    // 2. Run a script
    'Run your first script': 'Ejecuta tu primer script',
    'Tapir adds a <em>Tapir</em> menu to Archicad. Choose <em>Tapir Palette</em> from it, pick a script in the palette\'s list and click <em>Run Script</em>. The built-in scripts are there from the start: try <em>ProjectHealthCheck</em>, which lists elements without an ID, zones with the same number, unclassified elements and more. It only reads the model.':
        'Tapir añade un menú <em>Tapir</em> a Archicad. Elige en él <em>Tapir Palette</em>, selecciona un script en la lista de la paleta y haz clic en <em>Run Script</em>. Los scripts incluidos están ahí desde el principio: prueba <em>ProjectHealthCheck</em>, que lista los elementos sin ID, las zonas con el mismo número, los elementos sin clasificar y más. Solo lee el modelo.',
    'Tapir runs the scripts with <a href="https://docs.astral.sh/uv/">uv</a>, which brings the Python and the packages each script needs. If uv is not installed yet, Tapir offers to install it the first time you run a script.':
        'Tapir ejecuta los scripts con <a href="https://docs.astral.sh/uv/">uv</a>, que trae el Python y los paquetes que necesita cada script. Si uv aún no está instalado, Tapir ofrece instalarlo la primera vez que ejecutas un script.',
    '<em>Manage Shortcuts</em> on the palette puts up to six scripts in the Tapir menu, as <em>Run Script Shortcut 1</em> to <em>6</em>.':
        '<em>Manage Shortcuts</em> en la paleta pone hasta seis scripts en el menú Tapir, como <em>Run Script Shortcut 1</em> a <em>6</em>.',

    // 3. More scripts
    'Add more scripts': 'Añade más scripts',
    'The <a href="../scripts/">Scripts</a> page collects scripts shared by the community, each one reviewed before it is listed. The Tapir palette is getting a window to find and install them in one click.':
        'La página <a href="../scripts/">Scripts</a> reúne scripts compartidos por la comunidad, cada uno revisado antes de publicarse. La paleta de Tapir va a tener una ventana para encontrarlos e instalarlos con un clic.',
    '<strong class="callout-label"><span aria-hidden="true">ⓘ</span> Until then</strong> If your palette has no such window yet, add a script by hand, as below.':
        '<strong class="callout-label"><span aria-hidden="true">ⓘ</span> Mientras tanto</strong> Si tu paleta aún no tiene esa ventana, añade un script a mano, como se indica abajo.',
    'Download the scripts you want into a folder, then tell Tapir about that folder in its configuration file, <code>Documents/Tapir/config.json</code> in your user folder (Tapir writes it the first time it runs). Add the folder to its <code>localFolders</code> list, keeping the rest of the file as it is:':
        'Descarga los scripts que quieras en una carpeta y luego indica esa carpeta a Tapir en su archivo de configuración, <code>Documents/Tapir/config.json</code> en tu carpeta de usuario (Tapir lo crea la primera vez que se ejecuta). Añade la carpeta a su lista <code>localFolders</code>, dejando el resto del archivo como está:',
    '<code>path</code> is the folder (on macOS for example <code>/Users/me/Documents/Tapir scripts</code>). In JSON, a Windows path\'s backslashes are doubled.':
        '<code>path</code> es la carpeta (en macOS, por ejemplo, <code>/Users/me/Documents/Tapir scripts</code>). En JSON, las barras invertidas de una ruta de Windows se escriben dobles.',
    '<code>displayName</code> is the name the palette shows for the folder\'s scripts; without it, the folder\'s name is used.':
        '<code>displayName</code> es el nombre que muestra la paleta para los scripts de la carpeta; sin él, se usa el nombre de la carpeta.',
    'Optionally, <code>includePattern</code> and <code>excludePattern</code> are regular expressions that choose which files of the folder the palette lists.':
        'Opcionalmente, <code>includePattern</code> y <code>excludePattern</code> son expresiones regulares que eligen qué archivos de la carpeta lista la paleta.',
    'The palette lists the folder\'s scripts next to the built-in ones the next time it loads its scripts.':
        'La paleta lista los scripts de la carpeta junto a los incluidos la próxima vez que cargue sus scripts.',

    // 4. Python
    'Write a script of your own': 'Escribe tu propio script',
    'Archicad answers JSON commands on <code>http://127.0.0.1:19723</code> while a project is open (with several Archicads open, the next ones use 19724, 19725 and so on). Tapir\'s commands are called through Archicad\'s <code>API.ExecuteAddOnCommand</code> command, in the <code>TapirCommand</code> namespace. Python\'s standard library is enough; there are no packages to install.':
        'Archicad responde a comandos JSON en <code>http://127.0.0.1:19723</code> mientras hay un proyecto abierto (con varios Archicad abiertos, los siguientes usan 19724, 19725, etc.). Los comandos de Tapir se llaman mediante el comando <code>API.ExecuteAddOnCommand</code> de Archicad, en el espacio de nombres <code>TapirCommand</code>. Basta con la biblioteca estándar de Python; no hay paquetes que instalar.',
    'Graphisoft\'s <a href="https://pypi.org/project/archicad/"><code>archicad</code> package</a> sends the same commands and finds the port by itself, if you prefer it. Put your script in a <a href="#marketplace">local folder</a> to run it from the Tapir palette.':
        'El <a href="https://pypi.org/project/archicad/">paquete <code>archicad</code></a> de Graphisoft envía los mismos comandos y encuentra el puerto solo, si lo prefieres. Pon tu script en una <a href="#marketplace">carpeta local</a> para ejecutarlo desde la paleta de Tapir.',
    '<a href="../archicad-addon/">All commands, with schemas and examples</a>':
        '<a href="../archicad-addon/">Todos los comandos, con esquemas y ejemplos</a>',

    // 5. Grasshopper
    'Use it from Grasshopper': 'Úsalo desde Grasshopper',
    'In Rhino, open the Package Manager (the <code>PackageManager</code> command, or <em>Tools &gt; Package Manager</em>), search for <span class="mono">tapir</span>, install it and restart Rhino. Its components are on the <em>Tapir</em> tab in Grasshopper and work with the Archicad that runs Tapir. The Walls, Doors, Windows and Railings components read and change Archicad elements with the names you know from Archicad\'s settings dialogs.':
        'En Rhino, abre el Package Manager (el comando <code>PackageManager</code>, o <em>Tools &gt; Package Manager</em>), busca <span class="mono">tapir</span>, instálalo y reinicia Rhino. Sus componentes están en la pestaña <em>Tapir</em> de Grasshopper y trabajan con el Archicad en el que se ejecuta Tapir. Los componentes Walls, Doors, Windows y Railings leen y cambian elementos de Archicad con los nombres que conoces de los cuadros de diálogo de configuración de Archicad.',
    '<a href="../grasshopper/">The Grasshopper guide</a>': '<a href="../grasshopper/">La guía de Grasshopper</a>',

    // Next step
    'Next step': 'Siguiente paso',
    'Find a script': 'Encuentra un script',
    'Scripts shared by architects, checked and adapted for any office.':
        'Scripts compartidos por arquitectos, revisados y adaptados para cualquier estudio.',
    'Browse scripts': 'Ver los scripts',
    'The command reference': 'La referencia de comandos',
    'Every Tapir command with its parameters, its response and a Python example.':
        'Cada comando de Tapir con sus parámetros, su respuesta y un ejemplo en Python.',
    'Open the reference': 'Abrir la referencia',
    'Stuck?': '¿Atascado?',
    'Ask on Discord': 'Pregunta en Discord',
    'Architects, developers and the maintainers answer there. Or open an issue on GitHub.':
        'Allí responden arquitectos, desarrolladores y los mantenedores. O abre un issue en GitHub.',
    'Join Discord': 'Únete a Discord'
});
