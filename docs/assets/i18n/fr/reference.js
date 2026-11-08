/* Français: the heads, headings and introductions of the reference pages (archicad-addon/, api-coverage/,
   gh-coverage/), which stay mostly English (data-i18n-partial). See ../README.md. */
(window.TAPIR_I18N = window.TAPIR_I18N || []).push ({
    // The command reference (archicad-addon/)
    'Archicad JSON API commands reference | Tapir': 'Référence des commandes de la JSON API d\'Archicad | Tapir',
    'Every JSON command the Tapir add-on adds to Archicad, with its parameters, its response and a Python example, and how to call one from Python.':
        'Chaque commande JSON que l\'add-on Tapir ajoute à Archicad, avec ses paramètres, sa réponse et un exemple Python, et comment en appeler une depuis Python.',
    '<a href="../">Tapir</a> / <a href="../docs/">Docs</a> / Commands': '<a href="../">Tapir</a> / <a href="../docs/">Docs</a> / Commandes',
    'Tapir <em>commands.</em>': 'Les commandes <em>Tapir.</em>',
    'The JSON commands the Tapir add-on registers in Archicad, in the <code>TapirCommand</code> namespace, next to Graphisoft\'s official commands. Each command shows its parameters, its response and an example.':
        'Les commandes JSON que l\'add-on Tapir enregistre dans Archicad, dans l\'espace de noms <code>TapirCommand</code>, à côté des commandes officielles de Graphisoft. Chaque commande montre ses paramètres, sa réponse et un exemple.',
    'New to Tapir? <a href="../start/">Get started</a> shows how to install it and run a first script. Looking for something no command does? <a href="../limits/">What the Archicad API can\'t do</a> lists what Graphisoft\'s API does not offer, with what to do instead.':
        'Vous découvrez Tapir ? <a href="../start/">Démarrer</a> montre comment l\'installer et lancer un premier script. Vous cherchez quelque chose qu’aucune commande ne fait ? <a href="../limits/">Ce que l’API Archicad ne sait pas faire</a> liste ce que l’API de Graphisoft n’offre pas, avec ce qu’il faut faire à la place.',
    'Calling a <em>command.</em>': 'Appeler une <em>commande.</em>',
    'Archicad answers JSON commands on <code>http://127.0.0.1:19723</code> while a project is open (the next Archicads opened use 19724, 19725 and so on). A Tapir command is sent through Archicad\'s <code>API.ExecuteAddOnCommand</code> command.':
        'Archicad répond aux commandes JSON sur <code>http://127.0.0.1:19723</code> tant qu\'un projet est ouvert (les Archicad ouverts ensuite utilisent 19724, 19725 et ainsi de suite). Une commande Tapir est envoyée par la commande <code>API.ExecuteAddOnCommand</code> d\'Archicad.',
    'Python standard library': 'Bibliothèque standard de Python',
    'Graphisoft\'s <a href="https://pypi.org/project/archicad/"><code>archicad</code> package</a>':
        'Le <a href="https://pypi.org/project/archicad/">package <code>archicad</code></a> de Graphisoft',
    'The examples of the commands below use <code>aclib</code>, the small helper in <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/tree/main/archicad-addon/Examples">archicad-addon/Examples</a> that sends commands the same way as the standard-library script.':
        'Les exemples des commandes ci-dessous utilisent <code>aclib</code>, le petit module d\'aide de <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/tree/main/archicad-addon/Examples">archicad-addon/Examples</a>, qui envoie les commandes de la même façon que le script avec la bibliothèque standard.',
    'All <em>commands.</em>': 'Toutes les <em>commandes.</em>',

    // The two coverage pages
    'Overview': 'Vue d\'ensemble',
    'By group': 'Par groupe',
    'Checklist': 'Liste de contrôle',

    // The Archicad API coverage (api-coverage/)
    'Archicad API coverage checklist | Tapir': 'Liste de contrôle de la couverture de l\'API Archicad | Tapir',
    'Which parts of Graphisoft\'s Archicad API the Tapir add-on uses, item by item, kept up to date from every issue, pull request and push to the repository.':
        'Les parties de l\'API Archicad de Graphisoft qu\'utilise l\'add-on Tapir, élément par élément, tenues à jour à chaque issue, pull request et push sur le dépôt.',
    '<a href="../">Tapir</a> / <a href="../docs/">Docs</a> / API coverage': '<a href="../">Tapir</a> / <a href="../docs/">Docs</a> / Couverture de l’API',
    'Archicad API <em>coverage.</em>': 'Couverture de l\'API <em>Archicad.</em>',
    'What Tapir covers of Graphisoft\'s Archicad API, item by item.': 'Ce que Tapir couvre de l\'API Archicad de Graphisoft, élément par élément.',
    'Which parts of the Archicad API (<span id="archicad-range">every supported Archicad version</span>: C functions with their older names and command ids, and the C++ API) the <a href="https://github.com/ENZYME-APD/tapir-archicad-automation">Tapir Add-On</a> uses, updated within minutes of every issue and push to main, and within the hour of every pull request. See also the <a href="../archicad-addon/">command reference</a>.':
        'Les parties de l\'API Archicad (<span id="archicad-range">chaque version d\'Archicad prise en charge</span> : les fonctions C avec leurs anciens noms et identifiants de commande, et l\'API C++) qu\'utilise l\'<a href="https://github.com/ENZYME-APD/tapir-archicad-automation">Add-On Tapir</a>, mises à jour quelques minutes après chaque issue et push sur main, et dans l\'heure après chaque pull request. Voir aussi la <a href="../archicad-addon/">référence des commandes</a>.',
    'By part of the API': 'Par partie de l\'API',

    // The Grasshopper coverage (gh-coverage/)
    'Grasshopper coverage of the Tapir commands | Tapir': 'Couverture Grasshopper des commandes Tapir | Tapir',
    'Which Tapir commands for Archicad the components of the Tapir Grasshopper plugin call, and whether they cover each command\'s parameters or still miss some.':
        'Les commandes Tapir pour Archicad qu\'appellent les composants du plugin Grasshopper de Tapir, et s\'ils couvrent tous les paramètres de chaque commande ou s\'il leur en manque encore.',
    '<a href="../">Tapir</a> / <a href="../docs/">Docs</a> / Grasshopper coverage': '<a href="../">Tapir</a> / <a href="../docs/">Docs</a> / Couverture Grasshopper',
    'Grasshopper <em>coverage.</em>': 'Couverture <em>Grasshopper.</em>',
    'Which Tapir commands the Grasshopper components call, and the parameters they still miss.':
        'Les commandes Tapir qu\'appellent les composants Grasshopper, et les paramètres qui leur manquent encore.',
    'Which JSON commands of the <a href="https://github.com/ENZYME-APD/tapir-archicad-automation">Tapir Add-On</a> a component of the Tapir Grasshopper plugin calls, and whether its components cover the command\'s parameters, updated within minutes of every issue and push to main, and within the hour of every pull request. This is a static check of the plugin\'s code: it shows which parameters the components name, not that each one works. See also the <a href="../archicad-addon/">command reference</a> and the <a href="../api-coverage/">Archicad API coverage</a>.':
        'Les commandes JSON de l\'<a href="https://github.com/ENZYME-APD/tapir-archicad-automation">Add-On Tapir</a> qu\'appelle un composant du plugin Grasshopper de Tapir, et si ses composants couvrent les paramètres de la commande, mises à jour quelques minutes après chaque issue et push sur main, et dans l\'heure après chaque pull request. C\'est une vérification statique du code du plugin : elle montre les paramètres que les composants nomment, pas que chacun fonctionne. Voir aussi la <a href="../archicad-addon/">référence des commandes</a> et la <a href="../api-coverage/">couverture de l\'API Archicad</a>.',
    'Parameters of the used commands': 'Paramètres des commandes utilisées'
});
