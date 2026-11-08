/* Français: the Get started page (start/index.html). See ../README.md.
   The menu, palette and button names of Archicad and Tapir stay English, as the add-on shows them. */
(window.TAPIR_I18N = window.TAPIR_I18N || []).push ({
    'Automate Archicad with Python – get started | Tapir': 'Automatiser Archicad avec Python – bien démarrer | Tapir',
    'Install Tapir, run a script from the Tapir palette in Archicad, add more scripts, write a first Python script and install the Grasshopper plugin.':
        'Installer Tapir, lancer un script depuis la palette Tapir dans Archicad, ajouter d\'autres scripts, écrire un premier script Python et installer le plugin Grasshopper.',
    '<a href="../">Tapir</a> / Get started': '<a href="../">Tapir</a> / Démarrer',
    'Get <em>started.</em>': 'Bien <em>démarrer.</em>',
    'From download to your first script in about ten minutes. Steps 1 to 3 need no programming.':
        'Du téléchargement à votre premier script en dix minutes environ. Les étapes 1 à 3 ne demandent aucune programmation.',
    '<strong class="callout-label"><span aria-hidden="true">▶</span> Video</strong> Prefer watching? The <a href="../tutorials/">Tapir video tutorials</a> walk through installing Tapir and using it.':
        '<strong class="callout-label"><span aria-hidden="true">▶</span> Vidéo</strong> Vous préférez regarder ? Les <a href="../tutorials/">tutoriels vidéo de Tapir</a> (en anglais) montrent comment installer Tapir et s\'en servir.',

    // 1. Install
    'Install Tapir': 'Installer Tapir',
    '<strong>With the installer (recommended).</strong> It finds every Archicad version on the computer and installs the matching add-on into each, so Archicad loads it on its next start.':
        '<strong>Avec l\'installateur (recommandé).</strong> Il trouve chaque version d\'Archicad sur l\'ordinateur et installe dans chacune l\'add-on correspondant, qu\'Archicad charge à son prochain démarrage.',
    'Download <span class="os">WINDOWS</span>': 'Télécharger <span class="os">WINDOWS</span>',
    'Download <span class="os">MACOS</span>': 'Télécharger <span class="os">MACOS</span>',
    '<strong>Windows:</strong> the installer asks for administrator rights, because it copies the add-on into the Archicad folder under Program Files.':
        '<strong>Windows :</strong> l\'installateur demande les droits d\'administrateur, car il copie l\'add-on dans le dossier d\'Archicad sous Program Files.',
    '<strong>macOS:</strong> unzip the download and open the app.':
        '<strong>macOS :</strong> décompressez le téléchargement et ouvrez l\'application.',
    '<strong>Prefer to install by hand?</strong> Download the <code>TapirAddOn_AC&lt;version&gt;_Win.apx</code> or <code>TapirAddOn_AC&lt;version&gt;_Mac.zip</code> file for your Archicad version from the <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/releases/latest">latest release</a>, then in Archicad: <em>Options &gt; Add-On Manager &gt; Edit List of Available Add-Ons &gt; Add</em>, browse to the file and click <em>OK</em>.':
        '<strong>Vous préférez installer à la main ?</strong> Téléchargez le fichier <code>TapirAddOn_AC&lt;version&gt;_Win.apx</code> ou <code>TapirAddOn_AC&lt;version&gt;_Mac.zip</code> de votre version d\'Archicad depuis la <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/releases/latest">dernière version publiée</a>, puis dans Archicad : <em>Options &gt; Add-On Manager &gt; Edit List of Available Add-Ons &gt; Add</em>, choisissez le fichier et cliquez sur <em>OK</em>.',
    'Tapir keeps itself up to date: it tells you when a new version is out and installs it for you.':
        'Tapir se tient à jour tout seul : il vous prévient quand une nouvelle version sort et l\'installe pour vous.',

    // 2. Run a script
    'Run your first script': 'Lancer votre premier script',
    'Tapir adds a <em>Tapir</em> menu to Archicad. Choose <em>Tapir Palette</em> from it, pick a script in the palette\'s list and click <em>Run Script</em>. The built-in scripts are there from the start: try <em>ProjectHealthCheck</em>, which lists elements without an ID, zones with the same number, unclassified elements and more. It only reads the model.':
        'Tapir ajoute un menu <em>Tapir</em> à Archicad. Choisissez-y <em>Tapir Palette</em>, sélectionnez un script dans la liste de la palette et cliquez sur <em>Run Script</em>. Les scripts intégrés sont là dès le départ : essayez <em>ProjectHealthCheck</em>, qui liste les éléments sans ID, les zones portant le même numéro, les éléments non classés et plus encore. Il ne fait que lire le modèle.',
    'Tapir runs the scripts with <a href="https://docs.astral.sh/uv/">uv</a>, which brings the Python and the packages each script needs. If uv is not installed yet, Tapir offers to install it the first time you run a script.':
        'Tapir exécute les scripts avec <a href="https://docs.astral.sh/uv/">uv</a>, qui apporte le Python et les paquets dont chaque script a besoin. Si uv n\'est pas encore installé, Tapir propose de l\'installer la première fois que vous lancez un script.',
    '<em>Manage Shortcuts</em> on the palette puts up to six scripts in the Tapir menu, as <em>Run Script Shortcut 1</em> to <em>6</em>.':
        '<em>Manage Shortcuts</em> dans la palette place jusqu\'à six scripts dans le menu Tapir, sous <em>Run Script Shortcut 1</em> à <em>6</em>.',

    // 3. More scripts
    'Add more scripts': 'Ajouter d\'autres scripts',
    'The <a href="../scripts/">Scripts</a> page collects scripts shared by the community, each one reviewed before it is listed. The Tapir palette is getting a window to find and install them in one click.':
        'La page <a href="../scripts/">Scripts</a> rassemble les scripts partagés par la communauté, chacun vérifié avant d\'être publié. La palette Tapir va recevoir une fenêtre pour les trouver et les installer en un clic.',
    '<strong class="callout-label"><span aria-hidden="true">ⓘ</span> Until then</strong> If your palette has no such window yet, add a script by hand, as below.':
        '<strong class="callout-label"><span aria-hidden="true">ⓘ</span> En attendant</strong> Si votre palette n\'a pas encore cette fenêtre, ajoutez un script à la main, comme ci-dessous.',
    'Download the scripts you want into a folder, then tell Tapir about that folder in its configuration file, <code>Documents/Tapir/config.json</code> in your user folder (Tapir writes it the first time it runs). Add the folder to its <code>localFolders</code> list, keeping the rest of the file as it is:':
        'Téléchargez les scripts voulus dans un dossier, puis indiquez ce dossier à Tapir dans son fichier de configuration, <code>Documents/Tapir/config.json</code> dans votre dossier utilisateur (Tapir le crée à son premier lancement). Ajoutez le dossier à sa liste <code>localFolders</code>, sans toucher au reste du fichier :',
    '<code>path</code> is the folder (on macOS for example <code>/Users/me/Documents/Tapir scripts</code>). In JSON, a Windows path\'s backslashes are doubled.':
        '<code>path</code> est le dossier (sur macOS par exemple <code>/Users/me/Documents/Tapir scripts</code>). En JSON, les barres obliques inverses d\'un chemin Windows sont doublées.',
    '<code>displayName</code> is the name the palette shows for the folder\'s scripts; without it, the folder\'s name is used.':
        '<code>displayName</code> est le nom que la palette affiche pour les scripts du dossier ; sans lui, c\'est le nom du dossier qui est utilisé.',
    'Optionally, <code>includePattern</code> and <code>excludePattern</code> are regular expressions that choose which files of the folder the palette lists.':
        'En option, <code>includePattern</code> et <code>excludePattern</code> sont des expressions régulières qui choisissent les fichiers du dossier que la palette liste.',
    'The palette lists the folder\'s scripts next to the built-in ones the next time it loads its scripts.':
        'La palette liste les scripts du dossier à côté des scripts intégrés la prochaine fois qu\'elle charge ses scripts.',

    // 4. Python
    'Write a script of your own': 'Écrire votre propre script',
    'Archicad answers JSON commands on <code>http://127.0.0.1:19723</code> while a project is open (with several Archicads open, the next ones use 19724, 19725 and so on). Tapir\'s commands are called through Archicad\'s <code>API.ExecuteAddOnCommand</code> command, in the <code>TapirCommand</code> namespace. Python\'s standard library is enough; there are no packages to install.':
        'Archicad répond aux commandes JSON sur <code>http://127.0.0.1:19723</code> tant qu\'un projet est ouvert (avec plusieurs Archicad ouverts, les suivants utilisent 19724, 19725, etc.). Les commandes de Tapir sont appelées via la commande <code>API.ExecuteAddOnCommand</code> d\'Archicad, dans l\'espace de noms <code>TapirCommand</code>. La bibliothèque standard de Python suffit ; aucun paquet à installer.',
    'Graphisoft\'s <a href="https://pypi.org/project/archicad/"><code>archicad</code> package</a> sends the same commands and finds the port by itself, if you prefer it. Put your script in a <a href="#marketplace">local folder</a> to run it from the Tapir palette.':
        'Le <a href="https://pypi.org/project/archicad/">paquet <code>archicad</code></a> de Graphisoft envoie les mêmes commandes et trouve le port tout seul, si vous le préférez. Placez votre script dans un <a href="#marketplace">dossier local</a> pour le lancer depuis la palette Tapir.',
    '<a href="../archicad-addon/">All commands, with schemas and examples</a>':
        '<a href="../archicad-addon/">Toutes les commandes, avec schémas et exemples</a>',

    // 5. Grasshopper
    'Use it from Grasshopper': 'L\'utiliser depuis Grasshopper',
    'In Rhino, open the Package Manager (the <code>PackageManager</code> command, or <em>Tools &gt; Package Manager</em>), search for <span class="mono">tapir</span>, install it and restart Rhino. Its components are on the <em>Tapir</em> tab in Grasshopper and work with the Archicad that runs Tapir. The Walls, Doors, Windows and Railings components read and change Archicad elements with the names you know from Archicad\'s settings dialogs.':
        'Dans Rhino, ouvrez le Package Manager (la commande <code>PackageManager</code>, ou <em>Tools &gt; Package Manager</em>), cherchez <span class="mono">tapir</span>, installez-le et redémarrez Rhino. Ses composants sont dans l\'onglet <em>Tapir</em> de Grasshopper et travaillent avec l\'Archicad où tourne Tapir. Les composants Walls, Doors, Windows et Railings lisent et modifient les éléments d\'Archicad avec les noms que vous connaissez de ses dialogues de paramètres.',
    '<a href="../grasshopper/">The Grasshopper guide</a>': '<a href="../grasshopper/">Le guide Grasshopper</a>',

    // Next step
    'Next step': 'Étape suivante',
    'Find a script': 'Trouver un script',
    'Scripts shared by architects, checked and adapted for any office.':
        'Des scripts partagés par des architectes, vérifiés et adaptés à toute agence.',
    'Browse scripts': 'Parcourir les scripts',
    'The command reference': 'La référence des commandes',
    'Every Tapir command with its parameters, its response and a Python example.':
        'Chaque commande Tapir avec ses paramètres, sa réponse et un exemple Python.',
    'Open the reference': 'Ouvrir la référence',
    'Stuck?': 'Bloqué ?',
    'Ask on Discord': 'Demander sur Discord',
    'Architects, developers and the maintainers answer there. Or open an issue on GitHub.':
        'Architectes, développeurs et mainteneurs y répondent. Ou ouvrez une issue sur GitHub.',
    'Join Discord': 'Rejoindre Discord'
});
