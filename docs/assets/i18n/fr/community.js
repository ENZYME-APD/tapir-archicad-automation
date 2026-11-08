/* Français: the Community page (community/index.html, with the texts community/community.js reads
   from it). See ../README.md. */
(window.TAPIR_I18N = window.TAPIR_I18N || []).push ({
    'Community – Archicad automation with Tapir': 'Communauté – l\'automatisation d\'Archicad avec Tapir',
    'Ask on Discord, report a bug, test a fix in your Archicad: how Tapir\'s community and its bots turn a report into a release, and how the project grew.':
        'Posez vos questions sur Discord, signalez un bug, testez un correctif dans votre Archicad : comment la communauté de Tapir et ses bots transforment un signalement en nouvelle version, et comment le projet a grandi.',
    '<a href="../">Tapir</a> / Community': '<a href="../">Tapir</a> / Communauté',
    'Ask. Report. <em>Test.</em>': 'Demander. Signaler. <em>Tester.</em>',
    'Architects, developers and the maintainers in one place. Bugs and ideas become fixes that someone with your Archicad version tests before they ship.':
        'Architectes, développeurs et mainteneurs au même endroit. Les bugs et les idées deviennent des correctifs que quelqu\'un ayant votre version d\'Archicad teste avant leur publication.',

    // Where to find us
    'Where to find us': 'Où nous trouver',
    'Ask anything': 'Posez toutes vos questions',
    'Architects, developers and the maintainers, in one server. Show what you made and talk with the people who build Tapir.':
        'Architectes, développeurs et mainteneurs, sur un seul serveur. Montrez ce que vous avez réalisé et discutez avec les personnes qui développent Tapir.',
    'Join Discord': 'Rejoindre Discord',
    'Learn the API': 'Apprendre l\'API',
    'A video series from the first command to full workflows.': 'Une série de vidéos, de la première commande aux workflows complets.',
    'Watch the tutorials': 'Voir les tutoriels',
    'AI assistants': 'Assistants IA',
    'Talk to your model': 'Parlez à votre maquette',
    'MCP servers let Claude and other assistants read and change a project through Tapir.':
        'Les serveurs MCP permettent à Claude et à d\'autres assistants de lire et de modifier un projet via Tapir.',

    // How the bots work
    'From report to release': 'Du signalement à la version',
    'How the bots <em>work.</em>': 'Comment travaillent <em>les bots.</em>',
    'Most fixes are written by bots and checked by people. The maintainers set the rules and can stop any change; you decide whether a fix works in your Archicad.':
        'La plupart des correctifs sont écrits par des bots et vérifiés par des personnes. Les mainteneurs fixent les règles et peuvent arrêter toute modification ; c\'est vous qui décidez si un correctif fonctionne dans votre Archicad.',
    '<b>01</b><strong>Post it</strong><span>Describe the bug or idea on Discord or GitHub. A GitHub account is not needed on Discord.</span>':
        '<b>01</b><strong>Publiez-le</strong><span>Décrivez le bug ou l\'idée sur Discord ou GitHub. Sur Discord, aucun compte GitHub n\'est nécessaire.</span>',
    '<b>02</b><strong>An issue</strong><span>Every half hour, Discord posts that read like a bug report or a feature request become GitHub issues, and the bot replies with the link.</span>':
        '<b>02</b><strong>Une issue</strong><span>Toutes les demi-heures, les messages Discord qui ressemblent à un signalement de bug ou à une demande de fonctionnalité deviennent des issues GitHub, et le bot répond avec le lien.</span>',
    '<b>03</b><strong>A draft fix</strong><span>The issue bot answers, asks for what it is missing, or writes the fix and opens a pull request. It is built for every Archicad version and reviewed.</span>':
        '<b>03</b><strong>Un projet de correctif</strong><span>Le bot des issues répond, demande ce qui lui manque, ou écrit le correctif et ouvre une pull request. Celle-ci est compilée pour chaque version d\'Archicad et relue.</span>',
    '<b>04</b><strong>You test</strong><span>A comment on the pull request links the built add-on and asks the person who reported it to try it.</span>':
        '<b>04</b><strong>Vous testez</strong><span>Un commentaire sur la pull request donne le lien vers l\'add-on compilé et demande à la personne qui a signalé le problème de l\'essayer.</span>',
    '<b>05</b><strong>Released</strong><span>Green and reviewed, it is merged three days later unless a maintainer stops it, and ships in Monday\'s <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/releases">release</a>.</span>':
        '<b>05</b><strong>Publié</strong><span>Au vert et relue, elle est fusionnée trois jours plus tard, sauf si un mainteneur l\'arrête, et sort dans la <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/releases">version</a> du lundi.</span>',

    // Report and test
    'Report and test': 'Signaler et tester',
    'Report': 'Signaler',
    'How to report a bug': 'Comment signaler un bug',
    'Post it on Discord or open an issue on GitHub; both reach the same place. A report that can be fixed in one go says:':
        'Publiez-le sur Discord ou ouvrez une issue sur GitHub ; les deux arrivent au même endroit. Un signalement qui peut être corrigé d\'un coup indique :',
    'your Archicad version and build, and its language (for example 29.0.0 (3100) INT);':
        'votre version et votre build d\'Archicad, ainsi que sa langue (par exemple 29.0.0 (3100) INT) ;',
    'your Tapir version (the <em>About Tapir</em> dialog shows it);':
        'votre version de Tapir (la boîte de dialogue <em>About Tapir</em> l\'affiche) ;',
    'what you did: the script or Grasshopper definition, or the command and the JSON you sent;':
        'ce que vous avez fait : le script ou la définition Grasshopper, ou la commande et le JSON envoyés ;',
    'what you expected and what happened instead, with the error message;':
        'ce que vous attendiez et ce qui s\'est passé à la place, avec le message d\'erreur ;',
    'whether the project is a Teamwork project, and which libraries are loaded, when it matters.':
        'si le projet est un projet Teamwork, et quelles bibliothèques sont chargées, quand cela compte.',
    '<strong class="callout-label"><span aria-hidden="true">⚠</span> Public</strong> Issues and Discord channels are public. Leave out client names, project names and file paths you don\'t want to share.':
        '<strong class="callout-label"><span aria-hidden="true">⚠</span> Public</strong> Les issues et les salons Discord sont publics. N\'y mettez pas les noms de clients, les noms de projets ni les chemins de fichiers que vous ne voulez pas partager.',
    'Open an issue on GitHub': 'Ouvrir une issue sur GitHub',
    'Post on Discord': 'Publier sur Discord',
    'Test': 'Tester',
    'How to test a fix': 'Comment tester un correctif',
    'A fix is only known to work once someone has run it in Archicad. The bots can build Tapir but cannot run Archicad, so your test is what matters most.':
        'On ne sait qu\'un correctif fonctionne qu\'une fois que quelqu\'un l\'a lancé dans Archicad. Les bots peuvent compiler Tapir mais pas lancer Archicad : votre test est donc ce qui compte le plus.',
    'Open the pull request linked from your issue. A comment on it lists the downloads, one per Archicad version and platform. Downloading needs a GitHub login.':
        'Ouvrez la pull request liée à votre issue. Un commentaire y liste les téléchargements, un par version d\'Archicad et par plateforme. Le téléchargement nécessite une connexion à GitHub.',
    'Unzip the download for your Archicad version. On macOS it holds another zip with the <code>.bundle</code>.':
        'Décompressez le téléchargement pour votre version d\'Archicad. Sur macOS, il contient un autre zip avec le <code>.bundle</code>.',
    'In Archicad, open <em>Options &gt; Add-On Manager &gt; Edit List of Available Add-Ons</em>, remove or disable the installed Tapir, then <em>Add</em> the <code>.apx</code> (Windows) or <code>.bundle</code> (macOS) and click OK.':
        'Dans Archicad, ouvrez <em>Options &gt; Add-On Manager &gt; Edit List of Available Add-Ons</em>, retirez ou désactivez le Tapir installé, puis ajoutez avec <em>Add</em> le <code>.apx</code> (Windows) ou le <code>.bundle</code> (macOS) et cliquez sur OK.',
    'Do what went wrong before, in a copy of your project.':
        'Refaites ce qui ne fonctionnait pas, dans une copie de votre projet.',
    'Say on the pull request whether it works and fixes your case. Afterwards, put your installed Tapir back the same way.':
        'Dites sur la pull request si cela fonctionne et corrige votre cas. Ensuite, remettez votre Tapir installé de la même façon.',
    'Testing often? The <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/blob/main/tools/community-test-runner/GUIDE.md">community test runner</a> tests open pull requests on your own Archicad while it is closed, leaves your Tapir in place and posts the result. It runs on Windows and needs Python and a GitHub account.':
        'Vous testez souvent ? Le <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/blob/main/tools/community-test-runner/GUIDE.md">community test runner</a> teste les pull requests ouvertes sur votre propre Archicad pendant qu\'il est fermé, laisse votre Tapir en place et publie le résultat. Il fonctionne sous Windows et nécessite Python et un compte GitHub.',

    // Momentum (community.js fills in the numbers)
    '{sign}{diff} in {days} days': '{sign}{diff} en {days} jours',
    'Momentum': 'Dynamique',
    'Commits': 'Commits',
    'Discord messages': 'Messages Discord',
    'Discord members <span class="change"></span>': 'Membres Discord <span class="change"></span>',
    'People taking part, last 12 months': 'Personnes participantes, 12 derniers mois',
    'Top contributors': 'Principaux contributeurs',
    'Counted: commits (co-authors included), issues they reported that a merged fix closed, and pull requests they tested in Archicad (passed or failed).':
        'Sont comptés : les commits (co-auteurs compris), les issues signalées qu\'un correctif fusionné a fermées, et les pull requests testées dans Archicad (réussies ou non).',
    '{count} commit': '{count} commit',
    '{count} commits': '{count} commits',
    '{count} report': '{count} signalement',
    '{count} reports': '{count} signalements',
    '{count} test': '{count} test',
    '{count} tests': '{count} tests',

    // Growth
    'Commits, issues and pull requests, and Discord messages per month, stacked, with the number of Discord members as a line, {first} to {last}. The table below lists every number.':
        'Commits, issues et pull requests, et messages Discord par mois, empilés, avec le nombre de membres Discord en ligne, de {first} à {last}. Le tableau ci-dessous donne tous les chiffres.',
    'Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec': 'janv. févr. mars avr. mai juin juil. août sept. oct. nov. déc.',
    'Growth': 'Croissance',
    'Growth since <em>2022.</em>': 'La croissance depuis <em>2022.</em>',
    'Everything that happens around Tapir each month, from the first commit at Enzyme to today. Only numbers are collected, never the text of a message.':
        'Tout ce qui se passe autour de Tapir chaque mois, du premier commit chez Enzyme à aujourd\'hui. Seuls des chiffres sont collectés, jamais le texte d\'un message.',
    '<i class="swatch layer-github"></i>Issues and pull requests': '<i class="swatch layer-github"></i>Issues et pull requests',
    '<i class="swatch layer-discordMessages"></i>Discord messages': '<i class="swatch layer-discordMessages"></i>Messages Discord',
    '<i class="swatch swatch-line"></i>Discord members, right scale': '<i class="swatch swatch-line"></i>Membres Discord, échelle de droite',
    'Per month, stacked, on a square-root scale so the early years stay visible. Commits by people, without merges and bots; Discord messages include threads and forum posts. Discord members are the people in the server at the end of each month, counted by when they joined; people who later left are not included.':
        'Par mois, empilés, sur une échelle en racine carrée pour que les premières années restent visibles. Les commits faits par des personnes, sans les merges ni les bots ; les messages Discord comprennent les fils et les publications de forum. Les membres Discord sont les personnes présentes sur le serveur à la fin de chaque mois, comptées selon leur date d\'arrivée ; celles qui sont parties depuis ne sont pas comptées.',
    'Show the numbers as a table': 'Afficher les chiffres sous forme de tableau',
    'Month': 'Mois',
    'Issues opened': 'Issues ouvertes',
    'Pull requests merged': 'Pull requests fusionnées',
    'Discord members': 'Membres Discord',

    // Help build it
    'Help build it': 'Participez',
    'Made a script your office uses? Share it, and any office can install it from the palette.':
        'Vous avez écrit un script que votre agence utilise ? Partagez-le, et toute agence pourra l\'installer depuis la palette.',
    'How to share': 'Comment partager',
    'Ask for a feature': 'Demander une fonctionnalité',
    'How features are requested, and what is being asked for now.': 'Comment les fonctionnalités sont demandées, et ce qui est demandé en ce moment.',
    'See the roadmap': 'Voir la feuille de route',
    'Code': 'Code',
    'Write a command': 'Écrire une commande',
    'The add-on is C++ on the Archicad API. The development guide shows how to add a command.':
        'L\'add-on est écrit en C++ sur l\'API d\'Archicad. Le guide de développement montre comment ajouter une commande.',
    'Read the guide': 'Lire le guide',
    'Get involved': 'Engagez-vous',
    'We\'re building a motivated, diverse community around Archicad automation.':
        'Nous construisons une communauté motivée et diverse autour de l\'automatisation d\'Archicad.',
    'Say hello on Discord, and tell us what you automate in Archicad.':
        'Venez dire bonjour sur Discord, et racontez-nous ce que vous automatisez dans Archicad.',
    'Join the Discord': 'Rejoindre le Discord'
});
