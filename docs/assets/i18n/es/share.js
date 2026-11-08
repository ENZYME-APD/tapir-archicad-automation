/* Español: the Share page (scripts/share/index.html). See ../README.md.
   Channel, label, file and code names, and the palette's Share command, stay English. */
(window.TAPIR_I18N = window.TAPIR_I18N || []).push ({
    'Share an Archicad script | Tapir': 'Compartir un script de Archicad | Tapir',
    'Share an Archicad script you wrote: send it on Discord or as a pull request, and it is checked, adapted to work in any office and listed with your name.':
        'Comparte un script de Archicad que hayas escrito: envíalo por Discord o como pull request, y se revisa, se adapta para funcionar en cualquier estudio y se publica con tu nombre.',
    '<a href="../../">Tapir</a> / <a href="../">Scripts</a> / Share': '<a href="../../">Tapir</a> / <a href="../">Scripts</a> / Compartir',
    'Share a script <em>from Archicad.</em>': 'Comparte un script <em>desde Archicad.</em>',
    'No GitHub account, no setup. Your name stays on it, and everyone who uses Tapir can install it.':
        'Sin cuenta de GitHub, sin configurar nada. Tu nombre se queda en él, y cualquiera que use Tapir puede instalarlo.',

    // What happens
    'What happens': 'Qué pasa',
    '<b aria-hidden="true">1</b><strong>Send it</strong><span>Post the script in <code>#share-a-script</code> on Discord, with a few words on what it does. The Tapir palette is getting a Share button that does the same: you see the whole file before anything is sent. It arrives as a GitHub issue labelled <code>script-share</code>.</span>':
        '<b aria-hidden="true">1</b><strong>Envíalo</strong><span>Publica el script en <code>#share-a-script</code> en Discord, con unas palabras sobre lo que hace. La paleta de Tapir va a tener un botón Share que hace lo mismo: ves el archivo completo antes de que se envíe nada. Llega como un issue de GitHub con la etiqueta <code>script-share</code>.</span>',
    '<b aria-hidden="true">2</b><strong>Adapted for any office</strong><span>Tapir\'s bot, an AI, opens a pull request: hard-coded layers, properties and paths become settings, risky code is removed, and it works in every Archicad language, in metric and imperial.</span>':
        '<b aria-hidden="true">2</b><strong>Adaptado a cualquier estudio</strong><span>El bot de Tapir, una IA, abre un pull request: las capas, propiedades y rutas escritas a mano pasan a ser ajustes, se elimina el código arriesgado, y funciona en todos los idiomas de Archicad, en sistema métrico e imperial.</span>',
    '<b aria-hidden="true">3</b><strong>You confirm, it\'s listed</strong><span>Try the adapted version and say it still does what you meant. Once the pull request is reviewed and merged, it appears on the <a href="../">Scripts</a> page with your name.</span>':
        '<b aria-hidden="true">3</b><strong>Lo confirmas y se publica</strong><span>Prueba la versión adaptada y di si sigue haciendo lo que querías. Cuando el pull request se revisa y se fusiona, aparece en la página <a href="../">Scripts</a> con tu nombre.</span>',

    // Three ways
    'Three ways to send it': 'Tres formas de enviarlo',
    'Coming to the palette': 'Pronto en la paleta',
    'From the Tapir palette': 'Desde la paleta de Tapir',
    'Right-click your script and choose Share: add a few words about it and send. Until your palette has it, use Discord.':
        'Haz clic derecho en tu script y elige Share: añade unas palabras y envíalo. Mientras tu paleta no lo tenga, usa Discord.',
    'Easiest today': 'Lo más fácil hoy',
    'On Discord': 'En Discord',
    'Post the script in the <code>#share-a-script</code> channel, with a few words on what it does and which Archicad version you use.':
        'Publica el script en el canal <code>#share-a-script</code>, con unas palabras sobre lo que hace y qué versión de Archicad usas.',
    'Join Discord': 'Únete a Discord',
    'If you use GitHub': 'Si usas GitHub',
    'As a pull request': 'Como pull request',
    'Add a folder <code>marketplace/&lt;name&gt;/</code> with the script, a <code>listing.json</code> and a README, as the marketplace README describes.':
        'Añade una carpeta <code>marketplace/&lt;name&gt;/</code> con el script, un <code>listing.json</code> y un README, como describe el README del marketplace.',
    'The marketplace folder': 'La carpeta del marketplace',

    // The small print
    'The small print': 'La letra pequeña',
    'What is sent': 'Qué se envía',
    'Only the script': 'Solo el script',
    'The file, your one-line description, the name to credit, and your Archicad and Tapir versions. Nothing from the open project.':
        'El archivo, tu descripción de una línea, el nombre con el que se te acredita y tus versiones de Archicad y Tapir. Nada del proyecto abierto.',
    'Free for everyone': 'Libre para todos',
    'Every shared script is free and open source. Unless you choose another open-source licence, yours is published under the <a class="inline" href="https://opensource.org/license/mit">MIT licence</a>: anyone may use, change and share it, as long as they keep your name and the licence with it. It comes without warranty. You stay its author, and its page credits you.':
        'Todo script compartido es gratuito y de código abierto. Salvo que elijas otra licencia de código abierto, el tuyo se publica bajo la <a class="inline" href="https://opensource.org/license/mit">licencia MIT</a>: cualquiera puede usarlo, modificarlo y compartirlo, siempre que conserve tu nombre y la licencia. Se ofrece sin garantía. Sigues siendo su autor, y su página te acredita.',
    'Only share a script you wrote, or one whose author agreed to share it.':
        'Comparte solo un script que hayas escrito tú, o uno cuyo autor haya aceptado compartirlo.',

    // What the AI changes
    'What the AI changes': 'Qué cambia la IA',
    'What the AI <em>changes.</em>': 'Qué cambia <em>la IA.</em>',
    'It keeps what your script does. It changes how, so the script is safe and works in other offices, and lists every change in plain words in the pull request.':
        'Mantiene lo que hace tu script. Cambia el cómo, para que el script sea seguro y funcione en otros estudios, y enumera cada cambio con palabras sencillas en el pull request.',
    'Made safe': 'Hecho seguro',
    'Removed, or flagged for a maintainer': 'Se elimina, o se señala a un mantenedor',
    'connections to anything other than Archicad on your own computer;':
        'conexiones a cualquier cosa que no sea Archicad en tu propio ordenador;',
    'running other programs or code built from text (<code>subprocess</code>, <code>os.system</code>, <code>eval</code>, <code>exec</code>);':
        'ejecutar otros programas o código construido a partir de texto (<code>subprocess</code>, <code>os.system</code>, <code>eval</code>, <code>exec</code>);',
    'installing packages while it runs (<code>pip install</code>);':
        'instalar paquetes mientras se ejecuta (<code>pip install</code>);',
    'deleting or overwriting files, and writing anywhere but the project\'s folder and <code>Documents/Tapir</code>;':
        'borrar o sobrescribir archivos, y escribir en otro sitio que no sea la carpeta del proyecto y <code>Documents/Tapir</code>;',
    'passwords, tokens and keys;': 'contraseñas, tokens y claves;',
    'user names, <code>C:\\Users\\…</code> paths, and project, client and office names, in the code and its comments.':
        'nombres de usuario, rutas <code>C:\\Users\\…</code>, y nombres de proyectos, clientes y estudios, en el código y sus comentarios.',
    'Made to fit other offices': 'Adaptado a otros estudios',
    'Settings instead of your names': 'Ajustes en lugar de tus nombres',
    'Property names, layers, stories, paths and classification codes you typed in become settings at the top of the script, or fields of its window.':
        'Los nombres de propiedades, capas, plantas, rutas y códigos de clasificación que escribiste a mano pasan a ser ajustes al principio del script, o campos de su ventana.',
    'Built-in properties are found by their ID, so the script works in every language of Archicad.':
        'Las propiedades integradas se buscan por su ID, así que el script funciona en todos los idiomas de Archicad.',
    'Lengths and areas work in metric and imperial units; paths work on Windows and macOS.':
        'Las longitudes y superficies funcionan en unidades métricas e imperiales; las rutas, en Windows y macOS.',
    'Made safe to run': 'Seguro de ejecutar',
    'No surprises': 'Sin sorpresas',
    'It says what it is about to change, and does nothing, saying so, when nothing fits.':
        'Dice lo que va a cambiar, y no hace nada, avisándolo, cuando nada encaja.',
    'It makes its changes in one Undo step where it can.':
        'Hace sus cambios en un solo paso de Deshacer cuando puede.',
    'When a property or element type it needs is missing, it shows a clear message instead of crashing.':
        'Cuando falta una propiedad o un tipo de elemento que necesita, muestra un mensaje claro en lugar de fallar.',
    'Described': 'Descrito',
    'A README in plain words': 'Un README con palabras sencillas',
    'What it does, how to use it, its settings, what it changes, its limits, and <em>Original by</em> you.':
        'Qué hace, cómo usarlo, sus ajustes, qué cambia, sus límites, y <em>Original by</em> con tu nombre.',
    'Its category, the Archicad and Tapir versions it works with, and whether it changes the model, writes files or only reads.':
        'Su categoría, las versiones de Archicad y Tapir con las que funciona, y si cambia el modelo, escribe archivos o solo lee.',

    // Review
    'Review': 'Revisión',
    'Before it is merged': 'Antes de fusionarse',
    'What the review checks': 'Qué comprueba la revisión',
    'Every pull request is reviewed before it is merged, like any other code in Tapir. The review checks that:':
        'Cada pull request se revisa antes de fusionarse, como cualquier otro código de Tapir. La revisión comprueba que:',
    'nothing on the <em>Made safe</em> list is left in, unless a maintainer agreed to it;':
        'no queda nada de la lista <em>Hecho seguro</em>, salvo que un mantenedor lo haya aceptado;',
    'the script still does what you wrote it for, and you confirmed that;':
        'el script sigue haciendo aquello para lo que lo escribiste, y lo has confirmado;',
    'the folder has the files and fields the <a class="inline" href="https://github.com/ENZYME-APD/tapir-archicad-automation/blob/main/marketplace/README.md">marketplace format</a> asks for;':
        'la carpeta tiene los archivos y campos que pide el <a class="inline" href="https://github.com/ENZYME-APD/tapir-archicad-automation/blob/main/marketplace/README.md">formato del marketplace</a>;',
    'the README is accurate, and the screenshots show no client, project or people.':
        'el README es correcto, y las capturas no muestran clientes, proyectos ni personas.',
    'Next step': 'Siguiente paso',
    'Post your script': 'Publica tu script',
    'Share it in <code>#share-a-script</code> on Discord, and we take it from there.':
        'Compártelo en <code>#share-a-script</code> en Discord, y nosotros nos encargamos del resto.',
    'Browse scripts': 'Ver los scripts'
});
