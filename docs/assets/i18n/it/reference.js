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
    'Overview': 'Panoramica',
    'By group': 'Per gruppo',
    'Checklist': 'Checklist',

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
    'Parameters of the used commands': 'Parametri dei comandi usati'
});
