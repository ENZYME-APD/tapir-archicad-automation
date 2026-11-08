/* Français: the Share page (scripts/share/index.html). See ../README.md.
   Channel, label, file and code names, and the palette's Share command, stay English. */
(window.TAPIR_I18N = window.TAPIR_I18N || []).push ({
    'Share an Archicad script | Tapir': 'Partager un script Archicad | Tapir',
    'Share an Archicad script you wrote: send it on Discord or as a pull request, and it is checked, adapted to work in any office and listed with your name.':
        'Partagez un script Archicad que vous avez écrit : envoyez-le sur Discord ou en pull request, et il est vérifié, adapté pour fonctionner dans toute agence et publié avec votre nom.',
    '<a href="../../">Tapir</a> / <a href="../">Scripts</a> / Share': '<a href="../../">Tapir</a> / <a href="../">Scripts</a> / Partager',
    'Share a script <em>from Archicad.</em>': 'Partagez un script <em>depuis Archicad.</em>',
    'No GitHub account, no setup. Your name stays on it, and everyone who uses Tapir can install it.':
        'Sans compte GitHub, sans configuration. Votre nom y reste attaché, et tous les utilisateurs de Tapir peuvent l\'installer.',

    // What happens
    'What happens': 'Comment ça se passe',
    '<b aria-hidden="true">1</b><strong>Send it</strong><span>Post the script in <code>#share-a-script</code> on Discord, with a few words on what it does. The Tapir palette is getting a Share button that does the same: you see the whole file before anything is sent. It arrives as a GitHub issue labelled <code>script-share</code>.</span>':
        '<b aria-hidden="true">1</b><strong>Envoyez-le</strong><span>Publiez le script dans <code>#share-a-script</code> sur Discord, avec quelques mots sur ce qu\'il fait. La palette Tapir va recevoir un bouton Share qui fait la même chose : vous voyez le fichier entier avant que quoi que ce soit ne soit envoyé. Il arrive sous forme d\'issue GitHub avec le label <code>script-share</code>.</span>',
    '<b aria-hidden="true">2</b><strong>Adapted for any office</strong><span>Tapir\'s bot, an AI, opens a pull request: hard-coded layers, properties and paths become settings, risky code is removed, and it works in every Archicad language, in metric and imperial.</span>':
        '<b aria-hidden="true">2</b><strong>Adapté à toute agence</strong><span>Le bot de Tapir, une IA, ouvre une pull request : les calques, propriétés et chemins écrits en dur deviennent des réglages, le code risqué est retiré, et le script fonctionne dans toutes les langues d\'Archicad, en métrique comme en impérial.</span>',
    '<b aria-hidden="true">3</b><strong>You confirm, it\'s listed</strong><span>Try the adapted version and say it still does what you meant. Once the pull request is reviewed and merged, it appears on the <a href="../">Scripts</a> page with your name.</span>':
        '<b aria-hidden="true">3</b><strong>Vous confirmez, il est publié</strong><span>Essayez la version adaptée et dites si elle fait toujours ce que vous vouliez. Une fois la pull request relue et fusionnée, il apparaît sur la page <a href="../">Scripts</a> avec votre nom.</span>',

    // Three ways
    'Three ways to send it': 'Trois façons de l\'envoyer',
    'Coming to the palette': 'Bientôt dans la palette',
    'From the Tapir palette': 'Depuis la palette Tapir',
    'Right-click your script and choose Share: add a few words about it and send. Until your palette has it, use Discord.':
        'Faites un clic droit sur votre script et choisissez Share : ajoutez quelques mots et envoyez. Tant que votre palette ne l\'a pas, utilisez Discord.',
    'Easiest today': 'Le plus simple aujourd\'hui',
    'On Discord': 'Sur Discord',
    'Post the script in the <code>#share-a-script</code> channel, with a few words on what it does and which Archicad version you use.':
        'Publiez le script dans le canal <code>#share-a-script</code>, avec quelques mots sur ce qu\'il fait et la version d\'Archicad que vous utilisez.',
    'Join Discord': 'Rejoindre Discord',
    'If you use GitHub': 'Si vous utilisez GitHub',
    'As a pull request': 'En pull request',
    'Add a folder <code>marketplace/&lt;name&gt;/</code> with the script, a <code>listing.json</code> and a README, as the marketplace README describes.':
        'Ajoutez un dossier <code>marketplace/&lt;name&gt;/</code> avec le script, un <code>listing.json</code> et un README, comme le décrit le README de la marketplace.',
    'The marketplace folder': 'Le dossier de la marketplace',

    // The small print
    'The small print': 'Les petits caractères',
    'What is sent': 'Ce qui est envoyé',
    'Only the script': 'Seulement le script',
    'The file, your one-line description, the name to credit, and your Archicad and Tapir versions. Nothing from the open project.':
        'Le fichier, votre description d\'une ligne, le nom à créditer, et vos versions d\'Archicad et de Tapir. Rien du projet ouvert.',
    'Free for everyone': 'Libre pour tous',
    'Every shared script is free and open source. Unless you choose another open-source licence, yours is published under the <a class="inline" href="https://opensource.org/license/mit">MIT licence</a>: anyone may use, change and share it, as long as they keep your name and the licence with it. It comes without warranty. You stay its author, and its page credits you.':
        'Chaque script partagé est gratuit et open source. Sauf si vous choisissez une autre licence open source, le vôtre est publié sous <a class="inline" href="https://opensource.org/license/mit">licence MIT</a> : chacun peut l\'utiliser, le modifier et le partager, à condition de conserver votre nom et la licence. Il est fourni sans garantie. Vous en restez l\'auteur, et sa page vous crédite.',
    'Only share a script you wrote, or one whose author agreed to share it.':
        'Ne partagez qu\'un script que vous avez écrit, ou dont l\'auteur a accepté le partage.',

    // What the AI changes
    'What the AI changes': 'Ce que l\'IA modifie',
    'What the AI <em>changes.</em>': 'Ce que l\'IA <em>modifie.</em>',
    'It keeps what your script does. It changes how, so the script is safe and works in other offices, and lists every change in plain words in the pull request.':
        'Elle conserve ce que fait votre script. Elle change la manière de le faire, pour que le script soit sûr et fonctionne dans d\'autres agences, et liste chaque modification en termes simples dans la pull request.',
    'Made safe': 'Sécurisé',
    'Removed, or flagged for a maintainer': 'Retiré, ou signalé à un mainteneur',
    'connections to anything other than Archicad on your own computer;':
        'les connexions à autre chose qu\'Archicad sur votre propre ordinateur ;',
    'running other programs or code built from text (<code>subprocess</code>, <code>os.system</code>, <code>eval</code>, <code>exec</code>);':
        'le lancement d\'autres programmes ou de code construit à partir de texte (<code>subprocess</code>, <code>os.system</code>, <code>eval</code>, <code>exec</code>) ;',
    'installing packages while it runs (<code>pip install</code>);':
        'l\'installation de paquets pendant l\'exécution (<code>pip install</code>) ;',
    'deleting or overwriting files, and writing anywhere but the project\'s folder and <code>Documents/Tapir</code>;':
        'la suppression ou l\'écrasement de fichiers, et l\'écriture ailleurs que dans le dossier du projet et <code>Documents/Tapir</code> ;',
    'passwords, tokens and keys;': 'les mots de passe, jetons et clés ;',
    'user names, <code>C:\\Users\\…</code> paths, and project, client and office names, in the code and its comments.':
        'les noms d\'utilisateur, les chemins <code>C:\\Users\\…</code>, et les noms de projets, de clients et d\'agences, dans le code et ses commentaires.',
    'Made to fit other offices': 'Adapté aux autres agences',
    'Settings instead of your names': 'Des réglages au lieu de vos noms',
    'Property names, layers, stories, paths and classification codes you typed in become settings at the top of the script, or fields of its window.':
        'Les noms de propriétés, calques, niveaux, chemins et codes de classification saisis en dur deviennent des réglages en tête du script, ou des champs de sa fenêtre.',
    'Built-in properties are found by their ID, so the script works in every language of Archicad.':
        'Les propriétés intégrées sont retrouvées par leur ID, pour que le script fonctionne dans toutes les langues d\'Archicad.',
    'Lengths and areas work in metric and imperial units; paths work on Windows and macOS.':
        'Les longueurs et surfaces fonctionnent en unités métriques et impériales ; les chemins sous Windows et macOS.',
    'Made safe to run': 'Sûr à exécuter',
    'No surprises': 'Pas de surprises',
    'It says what it is about to change, and does nothing, saying so, when nothing fits.':
        'Il annonce ce qu\'il va modifier, et ne fait rien, en le signalant, quand rien ne correspond.',
    'It makes its changes in one Undo step where it can.':
        'Il fait ses modifications en une seule étape d\'annulation quand c\'est possible.',
    'When a property or element type it needs is missing, it shows a clear message instead of crashing.':
        'Quand une propriété ou un type d\'élément dont il a besoin manque, il affiche un message clair au lieu de planter.',
    'Described': 'Documenté',
    'A README in plain words': 'Un README en termes simples',
    'What it does, how to use it, its settings, what it changes, its limits, and <em>Original by</em> you.':
        'Ce qu\'il fait, comment l\'utiliser, ses réglages, ce qu\'il modifie, ses limites, et <em>Original by</em> avec votre nom.',
    'Its category, the Archicad and Tapir versions it works with, and whether it changes the model, writes files or only reads.':
        'Sa catégorie, les versions d\'Archicad et de Tapir avec lesquelles il fonctionne, et s\'il modifie le modèle, écrit des fichiers ou ne fait que lire.',

    // Review
    'Review': 'Relecture',
    'Before it is merged': 'Avant la fusion',
    'What the review checks': 'Ce que vérifie la relecture',
    'Every pull request is reviewed before it is merged, like any other code in Tapir. The review checks that:':
        'Chaque pull request est relue avant d\'être fusionnée, comme tout autre code de Tapir. La relecture vérifie que :',
    'nothing on the <em>Made safe</em> list is left in, unless a maintainer agreed to it;':
        'rien de la liste <em>Sécurisé</em> ne reste, sauf accord d\'un mainteneur ;',
    'the script still does what you wrote it for, and you confirmed that;':
        'le script fait toujours ce pour quoi vous l\'avez écrit, et vous l\'avez confirmé ;',
    'the folder has the files and fields the <a class="inline" href="https://github.com/ENZYME-APD/tapir-archicad-automation/blob/main/marketplace/README.md">marketplace format</a> asks for;':
        'le dossier contient les fichiers et champs que demande le <a class="inline" href="https://github.com/ENZYME-APD/tapir-archicad-automation/blob/main/marketplace/README.md">format de la marketplace</a> ;',
    'the README is accurate, and the screenshots show no client, project or people.':
        'le README est exact, et les captures d\'écran ne montrent ni client, ni projet, ni personne.',
    'Next step': 'Étape suivante',
    'Post your script': 'Publiez votre script',
    'Share it in <code>#share-a-script</code> on Discord, and we take it from there.':
        'Partagez-le dans <code>#share-a-script</code> sur Discord, et nous nous occupons du reste.',
    'Browse scripts': 'Parcourir les scripts'
});
