/* Italiano: the heads, headings and introductions of the reference pages (archicad-addon/, api-coverage/,
   gh-coverage/), which stay mostly English (data-i18n-partial). See ../README.md. */
(window.TAPIR_I18N = window.TAPIR_I18N || []).push ({
    // The command reference (archicad-addon/)
    'Archicad JSON API commands reference | Tapir': 'Riferimento dei comandi JSON API di Archicad | Tapir',
    'Every JSON command the Tapir add-on adds to Archicad, with its parameters, its response and a Python example, and how to call one from Python.':
        'Ogni comando JSON che l\'add-on Tapir aggiunge ad Archicad, con i suoi parametri, la sua risposta e un esempio Python, e come chiamarne uno da Python.',
    '<a href="../">Tapir</a> / <a href="../docs/">Docs</a> / Commands': '<a href="../">Tapir</a> / <a href="../docs/">Docs</a> / Comandi',
    'Tapir <em>commands.</em>': 'Comandi di <em>Tapir.</em>',
    'The JSON commands the Tapir add-on registers in Archicad, in the <code>TapirCommand</code> namespace, next to Graphisoft\'s official commands. Each command shows its parameters, its response and an example.':
        'I comandi JSON che l\'add-on Tapir registra in Archicad, nel namespace <code>TapirCommand</code>, accanto ai comandi ufficiali di Graphisoft. Ogni comando mostra i suoi parametri, la sua risposta e un esempio.',
    'New to Tapir? <a href="../start/">Get started</a> shows how to install it and run a first script. Looking for something no command does? <a href="../limits/">What the Archicad API can\'t do</a> lists what Graphisoft\'s API does not offer, with what to do instead.':
        'Nuovo su Tapir? <a href="../start/">Inizia</a> mostra come installarlo ed eseguire un primo script. Cerchi qualcosa che nessun comando fa? <a href="../limits/">Cosa non può fare l’API di Archicad</a> elenca ciò che l’API di Graphisoft non offre, con cosa fare invece.',
    'Calling a <em>command.</em>': 'Chiamare un <em>comando.</em>',
    'Archicad answers JSON commands on <code>http://127.0.0.1:19723</code> while a project is open (the next Archicads opened use 19724, 19725 and so on). A Tapir command is sent through Archicad\'s <code>API.ExecuteAddOnCommand</code> command.':
        'Archicad risponde ai comandi JSON su <code>http://127.0.0.1:19723</code> mentre un progetto è aperto (gli Archicad aperti dopo usano 19724, 19725 e così via). Un comando di Tapir si invia tramite il comando <code>API.ExecuteAddOnCommand</code> di Archicad.',
    'Python standard library': 'Libreria standard di Python',
    'Graphisoft\'s <a href="https://pypi.org/project/archicad/"><code>archicad</code> package</a>':
        'Il <a href="https://pypi.org/project/archicad/">pacchetto <code>archicad</code></a> di Graphisoft',
    'The examples of the commands below use <code>aclib</code>, the small helper in <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/tree/main/archicad-addon/Examples">archicad-addon/Examples</a> that sends commands the same way as the standard-library script.':
        'Gli esempi dei comandi qui sotto usano <code>aclib</code>, il piccolo modulo di supporto in <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/tree/main/archicad-addon/Examples">archicad-addon/Examples</a>, che invia i comandi nello stesso modo dello script con la libreria standard.',
    'All <em>commands.</em>': 'Tutti i <em>comandi.</em>',

    // The two coverage pages
    'Coverage at a <em>glance.</em>': 'La copertura in <em>sintesi.</em>',
    'By group': 'Per gruppo',
    'The whole <em>checklist.</em>': 'La checklist <em>completa.</em>',

    // The Archicad API coverage (api-coverage/)
    'Archicad API coverage checklist | Tapir': 'Checklist della copertura dell\'API di Archicad | Tapir',
    'Which parts of Graphisoft\'s Archicad API the Tapir add-on uses, item by item, kept up to date from every issue, pull request and push to the repository.':
        'Quali parti dell\'API di Archicad di Graphisoft usa l\'add-on Tapir, voce per voce, aggiornato a ogni issue, pull request e push nel repository.',
    '<a href="../">Tapir</a> / <a href="../docs/">Docs</a> / API coverage': '<a href="../">Tapir</a> / <a href="../docs/">Docs</a> / Copertura dell’API',
    'Archicad API <em>coverage.</em>': 'Copertura dell\'<em>API di Archicad.</em>',
    'What Tapir covers of Graphisoft\'s Archicad API, item by item.': 'Cosa copre Tapir dell\'API di Archicad di Graphisoft, voce per voce.',
    'Which parts of the Archicad API (<span id="archicad-range">every supported Archicad version</span>: C functions with their older names and command ids, and the C++ API) the <a href="https://github.com/ENZYME-APD/tapir-archicad-automation">Tapir Add-On</a> uses, updated within minutes of every issue and push to main, and within the hour of every pull request. See also the <a href="../archicad-addon/">command reference</a>.':
        'Quali parti dell\'API di Archicad (<span id="archicad-range">ogni versione di Archicad supportata</span>: le funzioni C con i loro nomi precedenti e gli id dei comandi, e l\'API C++) usa l\'<a href="https://github.com/ENZYME-APD/tapir-archicad-automation">Add-On Tapir</a>, aggiornato pochi minuti dopo ogni issue e push su main, ed entro un\'ora da ogni pull request. Vedi anche il <a href="../archicad-addon/">riferimento dei comandi</a>.',
    'By part of the API': 'Per parte dell\'API',

    // The Grasshopper coverage (gh-coverage/)
    'Grasshopper coverage of the Tapir commands | Tapir': 'Copertura Grasshopper dei comandi di Tapir | Tapir',
    'Which Tapir commands for Archicad the components of the Tapir Grasshopper plugin call, and whether they cover each command\'s parameters or still miss some.':
        'Quali comandi di Tapir per Archicad chiamano i componenti del plugin Grasshopper di Tapir, e se coprono i parametri di ogni comando o se ne manca ancora qualcuno.',
    '<a href="../">Tapir</a> / <a href="../docs/">Docs</a> / Grasshopper coverage': '<a href="../">Tapir</a> / <a href="../docs/">Docs</a> / Copertura Grasshopper',
    'Grasshopper <em>coverage.</em>': 'Copertura <em>Grasshopper.</em>',
    'Which Tapir commands the Grasshopper components call, and the parameters they still miss.':
        'Quali comandi di Tapir chiamano i componenti Grasshopper, e quali parametri mancano ancora.',
    'Which JSON commands of the <a href="https://github.com/ENZYME-APD/tapir-archicad-automation">Tapir Add-On</a> a component of the Tapir Grasshopper plugin calls, and whether its components cover the command\'s parameters, updated within minutes of every issue and push to main, and within the hour of every pull request. This is a static check of the plugin\'s code: it shows which parameters the components name, not that each one works. See also the <a href="../archicad-addon/">command reference</a> and the <a href="../api-coverage/">Archicad API coverage</a>.':
        'Quali comandi JSON dell\'<a href="https://github.com/ENZYME-APD/tapir-archicad-automation">Add-On Tapir</a> chiama un componente del plugin Grasshopper di Tapir, e se i suoi componenti coprono i parametri del comando, aggiornato pochi minuti dopo ogni issue e push su main, ed entro un\'ora da ogni pull request. È un controllo statico del codice del plugin: mostra quali parametri nominano i componenti, non che ognuno funzioni. Vedi anche il <a href="../archicad-addon/">riferimento dei comandi</a> e la <a href="../api-coverage/">copertura dell\'API di Archicad</a>.',
    'Parameters of the used commands': 'Parametri dei comandi usati',

    // The field coverage (field-coverage/), and its block on the API coverage page
    'Field coverage of the Archicad element types | Tapir':
        'Copertura dei campi dei tipi di elemento di Archicad | Tapir',
    'Which settings of every Archicad element type a Tapir command has a key of its own for, and which a script can reach only through GetElementFields and SetElementFields.':
        'Quali impostazioni di ogni tipo di elemento di Archicad hanno una chiave propria in un comando Tapir, e quali uno script può raggiungere solo tramite GetElementFields e SetElementFields.',
    '<a href="../">Tapir</a> / <a href="../docs/">Docs</a> / Field coverage':
        '<a href="../">Tapir</a> / <a href="../docs/">Docs</a> / Copertura dei campi',
    'Field <em>coverage.</em>':
        'Copertura dei <em>campi.</em>',
    'Which settings of each Archicad element type a Tapir command has a key for, and which a script reaches only through the generic field commands.':
        'Quali impostazioni di ogni tipo di elemento di Archicad hanno una chiave in un comando Tapir, e quali uno script raggiunge solo tramite i comandi generici dei campi.',
    'Every field of every element type of the Archicad API (<code>API_WallType.thickness</code>, <code>API_RoofType.shellBase.topMat</code>, …), and whether code of the <a href="https://github.com/ENZYME-APD/tapir-archicad-automation">Tapir Add-On</a> writes or reads it by name, so that a command most likely has a key of its own for it. The rest a script reaches only through <a href="../archicad-addon/#GetElementFields"><code>GetElementFields</code></a> and <a href="../archicad-addon/#SetElementFields"><code>SetElementFields</code></a>, on Archicad 29 and 30. This is a static estimate from the code, updated with every push to main: "written" does not prove that a command takes the field as input. See also the <a href="../archicad-addon/">command reference</a> and the <a href="../api-coverage/">Archicad API coverage</a>.':
        'Ogni campo di ogni tipo di elemento dell\'API di Archicad (<code>API_WallType.thickness</code>, <code>API_RoofType.shellBase.topMat</code>, …), e se il codice dell\'<a href="https://github.com/ENZYME-APD/tapir-archicad-automation">Add-On Tapir</a> lo scrive o lo legge per nome, così che un comando ha molto probabilmente una chiave propria per esso. Gli altri uno script li raggiunge solo tramite <a href="../archicad-addon/#GetElementFields"><code>GetElementFields</code></a> e <a href="../archicad-addon/#SetElementFields"><code>SetElementFields</code></a>, in Archicad 29 e 30. È una stima statica ricavata dal codice, aggiornata a ogni push su main: «scritto» non prova che un comando accetti il campo come input. Vedi anche il <a href="../archicad-addon/">riferimento dei comandi</a> e la <a href="../api-coverage/">copertura dell\'API di Archicad</a>.',
    'By element type':
        'Per tipo di elemento',
    'Element settings':
        'Impostazioni degli elementi',
    'A call such as <code>ACAPI_Element_Change</code> counts as one used item above, though a command may write only some of an element\'s settings with it. The <a href="../field-coverage/">field coverage</a> lists every setting of every element type, and whether a command has a key of its own for it.':
        'Una chiamata come <code>ACAPI_Element_Change</code> conta sopra come un elemento usato, anche se un comando può scrivere con essa solo alcune impostazioni di un elemento. La <a href="../field-coverage/">copertura dei campi</a> elenca ogni impostazione di ogni tipo di elemento, e se un comando ha una chiave propria per essa.'
});
