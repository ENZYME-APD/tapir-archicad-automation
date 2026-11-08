/* Italiano: the Roadmap page (roadmap/index.html). See ../README.md.
   Command names, the label enhancement and the names of the issue form and of the old list stay English. */
(window.TAPIR_I18N = window.TAPIR_I18N || []).push ({
    'Tapir roadmap – Archicad automation': 'Roadmap di Tapir – automazione di Archicad',
    'How to ask for a feature in Tapir, the Archicad add-on, what people are asking for now, what shipped from the old list and which ideas are still open.':
        'Come chiedere una funzionalità in Tapir, l\'add-on per Archicad, cosa si chiede adesso, cosa è uscito dalla vecchia lista e quali idee sono ancora aperte.',
    '<a href="../">Tapir</a> / <a href="../community/">Community</a> / Roadmap':
        '<a href="../">Tapir</a> / <a href="../community/">Community</a> / Roadmap',
    'What\'s <em>next.</em>': 'Cosa <em>arriva.</em>',
    'Tapir grows from requests. Most new commands, components and fixes start as a message from someone who needed them in their own work.':
        'Tapir cresce dalle richieste. La maggior parte dei nuovi comandi, componenti e correzioni nasce dal messaggio di qualcuno che ne aveva bisogno nel proprio lavoro.',

    // How to ask for a feature
    'How to ask for a <em>feature.</em>': 'Come chiedere una <em>funzionalità.</em>',
    'Four steps from "I need this" to a Monday release.': 'Quattro passi da «Mi serve questo» a una versione pubblicata di lunedì.',
    'Say what you want to do': 'Di\' cosa vuoi fare',
    'Post it on Discord or open an issue on GitHub. Describe the task in Archicad, not only the command: "number the doors along a line", "read the area of every zone". A Discord message that reads like a feature request becomes a GitHub issue by itself, labelled <code>enhancement</code>.':
        'Scrivilo su Discord o apri una issue su GitHub. Descrivi il compito in Archicad, non solo il comando: «numerare le porte lungo una linea», «leggere la superficie di ogni zona». Un messaggio Discord che sembra una richiesta di funzionalità diventa da solo una issue su GitHub, con l\'etichetta <code>enhancement</code>.',
    'Asking for a part of the Archicad API?': 'Ti serve una parte dell\'API di Archicad?',
    'The <a href="../api-coverage/">API coverage</a> page lists every function of the Archicad API and whether Tapir has a command for it. Use the <em>Archicad API request</em> issue form to ask for one that is missing.':
        'La pagina <a href="../api-coverage/">Copertura dell’API</a> elenca ogni funzione dell\'API di Archicad e indica se Tapir ha un comando per essa. Usa il modulo di issue <em>Archicad API request</em> per chiederne una che manca.',
    'The issue bot picks it up': 'Il bot delle issue la prende in carico',
    'A clear, wanted request is built like a fix: the bot writes it and opens a pull request, and you are asked to try it. A large one is split into parts. When the design is open, the bot describes the choices and a maintainer decides.':
        'Una richiesta chiara e voluta viene realizzata come una correzione: il bot la scrive e apre una pull request, e ti viene chiesto di provarla. Una richiesta grande viene divisa in parti. Quando il progetto è aperto, il bot descrive le scelte e decide un maintainer.',
    'It ships on a Monday': 'Esce di lunedì',
    'Merged changes are released every Monday. The release notes list them, and the add-on offers to update itself.':
        'Le modifiche unite vengono pubblicate ogni lunedì. Le note di rilascio le elencano, e l\'add-on si offre di aggiornarsi da solo.',

    // What is being asked for now
    'What is being asked for now': 'Cosa viene chiesto adesso',
    'Requests': 'Richieste',
    'Open feature requests': 'Richieste di funzionalità aperte',
    'Every open issue labelled <code>enhancement</code>. Add a comment or a reaction to one you need too.':
        'Ogni issue aperta con l\'etichetta <code>enhancement</code>. Aggiungi un commento o una reazione a quella che serve anche a te.',
    'See the requests': 'Vedi le richieste',
    'Plan': 'Piano',
    'Project board': 'Bacheca del progetto',
    'The maintainers\' board of planned and ongoing work.': 'La bacheca dei maintainer con il lavoro previsto e in corso.',
    'Open the board': 'Apri la bacheca',
    'In progress': 'In corso',
    'Open pull requests': 'Pull request aperte',
    'Changes being built, reviewed and tested right now.': 'Modifiche che in questo momento vengono sviluppate, revisionate e testate.',
    'See the pull requests': 'Vedi le pull request',
    'Shipped': 'Uscito',
    'Releases': 'Versioni',
    'Every release, with its notes and downloads. The <span class="nowrap">command reference</span> shows the version each command came in.':
        'Ogni versione, con le sue note e i download. Il <span class="nowrap">riferimento dei comandi</span> mostra la versione in cui è arrivato ogni comando.',
    'See the releases': 'Vedi le versioni',

    // Recently shipped
    'Recently shipped': 'Uscito di recente',
    'Already <em>here.</em>': 'Già <em>qui.</em>',
    'Asked for on the old site\'s "What\'s cooking" list, and now part of Tapir.':
        'Chiesto nella lista «What\'s cooking» del vecchio sito, e ora parte di Tapir.',
    'Stairs, roofs and curtain walls': 'Scale, coperture e facciate continue',
    'Read and change them: <a href="../archicad-addon/#ModifyStairs"><code>ModifyStairs</code></a>, <a href="../archicad-addon/#ModifyRoofs"><code>ModifyRoofs</code></a> and <a href="../archicad-addon/#ModifyCurtainWalls"><code>ModifyCurtainWalls</code></a>.':
        'Leggile e modificale: <a href="../archicad-addon/#ModifyStairs"><code>ModifyStairs</code></a>, <a href="../archicad-addon/#ModifyRoofs"><code>ModifyRoofs</code></a> e <a href="../archicad-addon/#ModifyCurtainWalls"><code>ModifyCurtainWalls</code></a>.',
    'Building materials and composites': 'Materiali da costruzione e strutture composte',
    'Create them from a script: <a href="../archicad-addon/#CreateBuildingMaterials"><code>CreateBuildingMaterials</code></a> and <a href="../archicad-addon/#CreateComposites"><code>CreateComposites</code></a>.':
        'Creali da uno script: <a href="../archicad-addon/#CreateBuildingMaterials"><code>CreateBuildingMaterials</code></a> e <a href="../archicad-addon/#CreateComposites"><code>CreateComposites</code></a>.',
    'Dialogs from Python': 'Finestre di dialogo da Python',
    'A script shows its own window in Archicad: <a href="../archicad-addon/#ShowScriptUI"><code>ShowScriptUI</code></a> and <a href="../archicad-addon/#GetScriptUIResult"><code>GetScriptUIResult</code></a>.':
        'Uno script mostra la propria finestra in Archicad: <a href="../archicad-addon/#ShowScriptUI"><code>ShowScriptUI</code></a> e <a href="../archicad-addon/#GetScriptUIResult"><code>GetScriptUIResult</code></a>.',
    '2D detailing': 'Dettagli 2D',
    'Lines, fills and text: <a href="../archicad-addon/#CreateLineElements"><code>CreateLineElements</code></a>, <a href="../archicad-addon/#CreateHatches"><code>CreateHatches</code></a> and <a href="../archicad-addon/#CreateTexts"><code>CreateTexts</code></a>.':
        'Linee, retini e testi: <a href="../archicad-addon/#CreateLineElements"><code>CreateLineElements</code></a>, <a href="../archicad-addon/#CreateHatches"><code>CreateHatches</code></a> e <a href="../archicad-addon/#CreateTexts"><code>CreateTexts</code></a>.',
    'Your own properties': 'Le tue proprietà',
    'Add property definitions from a script: <a href="../archicad-addon/#CreatePropertyDefinitions"><code>CreatePropertyDefinitions</code></a>.':
        'Aggiungi definizioni di proprietà da uno script: <a href="../archicad-addon/#CreatePropertyDefinitions"><code>CreatePropertyDefinitions</code></a>.',
    'Built-in scripts': 'Script inclusi',
    'Ready to run from the palette, from numbering to a project health check. <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/tree/main/builtin-scripts">See them on GitHub</a>.':
        'Pronti da eseguire dalla palette, dalla numerazione al controllo dello stato del progetto. <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/tree/main/builtin-scripts">Vedili su GitHub</a>.',

    // Ideas
    'Ideas': 'Idee',
    'Still <em>open.</em>': 'Ancora <em>aperte.</em>',
    'Two ideas from the old list that nobody has asked for in an issue yet. Want one? Open the issue.':
        'Due idee della vecchia lista che nessuno ha ancora chiesto in una issue. Ne vuoi una? Apri la issue.',
    'Idea': 'Idea',
    'Clearer error messages in Python': 'Messaggi di errore più chiari in Python',
    'Tracebacks and command errors that say what went wrong and how to fix it, so a script is easier to troubleshoot.':
        'Traceback ed errori dei comandi che dicono cosa è andato storto e come correggerlo, così è più facile trovare il problema in uno script.',
    'Ask for it': 'Chiedila',
    'Tapir scripts in CI': 'Script di Tapir in CI',
    'Running Tapir scripts automatically, for example in a GitHub Actions pipeline.':
        'Eseguire script di Tapir in automatico, per esempio in una pipeline di GitHub Actions.',

    // Next step
    'Next step': 'Passo successivo',
    'Missing something in your work?': 'Ti manca qualcosa nel tuo lavoro?',
    'Tell us what you want to automate': 'Dicci cosa vuoi automatizzare',
    'Open an issue, or describe it on Discord and the bot files it for you.':
        'Apri una issue, oppure descrivilo su Discord e il bot la apre per te.',
    'Request a feature on GitHub': 'Chiedi una funzionalità su GitHub',
    'Ask on Discord': 'Chiedi su Discord'
});
