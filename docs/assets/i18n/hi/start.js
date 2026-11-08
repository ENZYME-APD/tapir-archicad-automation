/* हिन्दी: the Get started page (start/index.html). See ../README.md.
   The menu, palette and button names of Archicad and Tapir stay English, as the add-on shows them. */
(window.TAPIR_I18N = window.TAPIR_I18N || []).push ({
    'Automate Archicad with Python – get started | Tapir': 'Python से Archicad automate करें – शुरुआत करें | Tapir',
    'Install Tapir, run a script from the Tapir palette in Archicad, add more scripts, write a first Python script and install the Grasshopper plugin.':
        'Tapir इंस्टॉल करें, Archicad में Tapir palette से एक script चलाएँ, और scripts जोड़ें, पहली Python script लिखें और Grasshopper plugin इंस्टॉल करें।',
    '<a href="../">Tapir</a> / Get started': '<a href="../">Tapir</a> / शुरू करें',
    'Get <em>started.</em>': '<em>शुरू</em> करें।',
    'From download to your first script in about ten minutes. Steps 1 to 3 need no programming.':
        'डाउनलोड से आपकी पहली script तक, लगभग दस मिनट में। चरण 1 से 3 के लिए programming नहीं चाहिए।',
    '<strong class="callout-label"><span aria-hidden="true">▶</span> Video</strong> Prefer watching? The <a href="../tutorials/">Tapir video tutorials</a> walk through installing Tapir and using it.':
        '<strong class="callout-label"><span aria-hidden="true">▶</span> वीडियो</strong> देखकर सीखना पसंद है? <a href="../tutorials/">Tapir के वीडियो ट्यूटोरियल</a> Tapir को इंस्टॉल करना और इस्तेमाल करना दिखाते हैं (अंग्रेज़ी में)।',

    // 1. Install
    'Install Tapir': 'Tapir इंस्टॉल करें',
    '<strong>With the installer (recommended).</strong> It finds every Archicad version on the computer and installs the matching add-on into each, so Archicad loads it on its next start.':
        '<strong>Installer से (सुझाया गया तरीका)।</strong> यह कंप्यूटर पर हर Archicad संस्करण ढूँढता है और हर एक में उससे मेल खाता add-on इंस्टॉल करता है, ताकि Archicad अगली बार शुरू होने पर उसे load कर ले।',
    'Download <span class="os">WINDOWS</span>': 'डाउनलोड <span class="os">WINDOWS</span>',
    'Download <span class="os">MACOS</span>': 'डाउनलोड <span class="os">MACOS</span>',
    '<strong>Windows:</strong> the installer asks for administrator rights, because it copies the add-on into the Archicad folder under Program Files.':
        '<strong>Windows:</strong> installer administrator अधिकार माँगता है, क्योंकि वह add-on को Program Files के अंदर Archicad folder में कॉपी करता है।',
    '<strong>macOS:</strong> unzip the download and open the app.':
        '<strong>macOS:</strong> डाउनलोड की गई फ़ाइल को unzip करें और app खोलें।',
    '<strong>Prefer to install by hand?</strong> Download the <code>TapirAddOn_AC&lt;version&gt;_Win.apx</code> or <code>TapirAddOn_AC&lt;version&gt;_Mac.zip</code> file for your Archicad version from the <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/releases/latest">latest release</a>, then in Archicad: <em>Options &gt; Add-On Manager &gt; Edit List of Available Add-Ons &gt; Add</em>, browse to the file and click <em>OK</em>.':
        '<strong>ख़ुद इंस्टॉल करना चाहते हैं?</strong> <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/releases/latest">नवीनतम release</a> से अपने Archicad संस्करण के लिए <code>TapirAddOn_AC&lt;version&gt;_Win.apx</code> या <code>TapirAddOn_AC&lt;version&gt;_Mac.zip</code> फ़ाइल डाउनलोड करें, फिर Archicad में: <em>Options &gt; Add-On Manager &gt; Edit List of Available Add-Ons &gt; Add</em>, फ़ाइल चुनें और <em>OK</em> पर क्लिक करें।',
    'Tapir keeps itself up to date: it tells you when a new version is out and installs it for you.':
        'Tapir ख़ुद को अपडेट रखता है: नया संस्करण आने पर यह आपको बताता है और उसे आपके लिए इंस्टॉल कर देता है।',

    // 2. Run a script
    'Run your first script': 'अपनी पहली script चलाएँ',
    'Tapir adds a <em>Tapir</em> menu to Archicad. Choose <em>Tapir Palette</em> from it, pick a script in the palette\'s list and click <em>Run Script</em>. The built-in scripts are there from the start: try <em>ProjectHealthCheck</em>, which lists elements without an ID, zones with the same number, unclassified elements and more. It only reads the model.':
        'Tapir, Archicad में एक <em>Tapir</em> menu जोड़ता है। उसमें से <em>Tapir Palette</em> चुनें, palette की सूची में एक script चुनें और <em>Run Script</em> पर क्लिक करें। Built-in scripts शुरू से मौजूद हैं: <em>ProjectHealthCheck</em> आज़माएँ, जो बिना ID वाले elements, एक ही नंबर वाले zones, अवर्गीकृत elements और बहुत कुछ की सूची बनाती है। यह मॉडल को सिर्फ़ पढ़ती है।',
    'Tapir runs the scripts with <a href="https://docs.astral.sh/uv/">uv</a>, which brings the Python and the packages each script needs. If uv is not installed yet, Tapir offers to install it the first time you run a script.':
        'Tapir scripts को <a href="https://docs.astral.sh/uv/">uv</a> से चलाता है, जो हर script के लिए ज़रूरी Python और packages ले आता है। अगर uv अभी इंस्टॉल नहीं है, तो पहली बार script चलाने पर Tapir उसे इंस्टॉल करने का विकल्प देता है।',
    '<em>Manage Shortcuts</em> on the palette puts up to six scripts in the Tapir menu, as <em>Run Script Shortcut 1</em> to <em>6</em>.':
        'Palette पर <em>Manage Shortcuts</em> से छह तक scripts Tapir menu में <em>Run Script Shortcut 1</em> से <em>6</em> के रूप में रखी जा सकती हैं।',

    // 3. More scripts
    'Add more scripts': 'और scripts जोड़ें',
    'The <a href="../scripts/">Scripts</a> page collects scripts shared by the community, each one reviewed before it is listed. The Tapir palette is getting a window to find and install them in one click.':
        '<a href="../scripts/">Scripts</a> पेज पर समुदाय द्वारा शेयर की गई scripts हैं, हर एक सूची में आने से पहले जाँची जाती है। Tapir palette में जल्द ही एक window आएगी जिससे इन्हें एक क्लिक में ढूँढकर इंस्टॉल किया जा सकेगा।',
    '<strong class="callout-label"><span aria-hidden="true">ⓘ</span> Until then</strong> If your palette has no such window yet, add a script by hand, as below.':
        '<strong class="callout-label"><span aria-hidden="true">ⓘ</span> तब तक</strong> अगर आपके palette में अभी ऐसी window नहीं है, तो नीचे बताए अनुसार script ख़ुद जोड़ें।',
    'Download the scripts you want into a folder, then tell Tapir about that folder in its configuration file, <code>Documents/Tapir/config.json</code> in your user folder (Tapir writes it the first time it runs). Add the folder to its <code>localFolders</code> list, keeping the rest of the file as it is:':
        'अपनी चाही scripts एक folder में डाउनलोड करें, फिर Tapir की configuration फ़ाइल में उस folder का नाम दें, जो आपके user folder में <code>Documents/Tapir/config.json</code> है (Tapir इसे पहली बार चलने पर लिखता है)। Folder को उसकी <code>localFolders</code> सूची में जोड़ें, और फ़ाइल का बाक़ी हिस्सा जैसा है वैसा ही रहने दें:',
    '<code>path</code> is the folder (on macOS for example <code>/Users/me/Documents/Tapir scripts</code>). In JSON, a Windows path\'s backslashes are doubled.':
        '<code>path</code> वह folder है (macOS पर उदाहरण के लिए <code>/Users/me/Documents/Tapir scripts</code>)। JSON में Windows path के backslashes दो बार लिखे जाते हैं।',
    '<code>displayName</code> is the name the palette shows for the folder\'s scripts; without it, the folder\'s name is used.':
        '<code>displayName</code> वह नाम है जो palette उस folder की scripts के लिए दिखाता है; इसके बिना folder का नाम इस्तेमाल होता है।',
    'Optionally, <code>includePattern</code> and <code>excludePattern</code> are regular expressions that choose which files of the folder the palette lists.':
        'वैकल्पिक रूप से, <code>includePattern</code> और <code>excludePattern</code> regular expressions हैं जो तय करते हैं कि palette folder की कौन-सी फ़ाइलें दिखाए।',
    'The palette lists the folder\'s scripts next to the built-in ones the next time it loads its scripts.':
        'अगली बार scripts load करने पर palette उस folder की scripts को built-in scripts के साथ दिखाता है।',

    // 4. Python
    'Write a script of your own': 'अपनी ख़ुद की script लिखें',
    'Archicad answers JSON commands on <code>http://127.0.0.1:19723</code> while a project is open (with several Archicads open, the next ones use 19724, 19725 and so on). Tapir\'s commands are called through Archicad\'s <code>API.ExecuteAddOnCommand</code> command, in the <code>TapirCommand</code> namespace. Python\'s standard library is enough; there are no packages to install.':
        'प्रोजेक्ट खुला रहने तक Archicad <code>http://127.0.0.1:19723</code> पर JSON कमांड का जवाब देता है (कई Archicad खुले हों तो अगले वाले 19724, 19725 इत्यादि इस्तेमाल करते हैं)। Tapir के कमांड Archicad के <code>API.ExecuteAddOnCommand</code> कमांड से, <code>TapirCommand</code> namespace में, बुलाए जाते हैं। Python की standard library काफ़ी है; कोई package इंस्टॉल नहीं करना है।',
    'Graphisoft\'s <a href="https://pypi.org/project/archicad/"><code>archicad</code> package</a> sends the same commands and finds the port by itself, if you prefer it. Put your script in a <a href="#marketplace">local folder</a> to run it from the Tapir palette.':
        'चाहें तो Graphisoft का <a href="https://pypi.org/project/archicad/"><code>archicad</code> पैकेज</a> भी यही कमांड भेजता है और port ख़ुद ढूँढ लेता है। अपनी script को Tapir palette से चलाने के लिए उसे एक <a href="#marketplace">लोकल फ़ोल्डर</a> में रखें।',
    '<a href="../archicad-addon/">All commands, with schemas and examples</a>':
        '<a href="../archicad-addon/">सभी कमांड, schemas और उदाहरणों के साथ</a>',

    // 5. Grasshopper
    'Use it from Grasshopper': 'Grasshopper से इस्तेमाल करें',
    'In Rhino, open the Package Manager (the <code>PackageManager</code> command, or <em>Tools &gt; Package Manager</em>), search for <span class="mono">tapir</span>, install it and restart Rhino. Its components are on the <em>Tapir</em> tab in Grasshopper and work with the Archicad that runs Tapir. The Walls, Doors, Windows and Railings components read and change Archicad elements with the names you know from Archicad\'s settings dialogs.':
        'Rhino में Package Manager खोलें (<code>PackageManager</code> कमांड, या <em>Tools &gt; Package Manager</em>), <span class="mono">tapir</span> खोजें, उसे इंस्टॉल करें और Rhino दोबारा शुरू करें। इसके components Grasshopper में <em>Tapir</em> tab पर हैं और उस Archicad के साथ काम करते हैं जिसमें Tapir चल रहा है। Walls, Doors, Windows और Railings components, Archicad elements को उन्हीं नामों से पढ़ते और बदलते हैं जो आप Archicad के settings dialogs से जानते हैं।',
    '<a href="../grasshopper/">The Grasshopper guide</a>': '<a href="../grasshopper/">Grasshopper गाइड</a>',

    // Next step
    'Next step': 'अगला क़दम',
    'Find a script': 'एक script ढूँढें',
    'Scripts shared by architects, checked and adapted for any office.':
        'आर्किटेक्ट्स द्वारा शेयर की गई scripts, जाँची हुई और किसी भी ऑफ़िस के लिए ढाली गई।',
    'Browse scripts': 'Scripts देखें',
    'The command reference': 'कमांड संदर्भ',
    'Every Tapir command with its parameters, its response and a Python example.':
        'हर Tapir कमांड, अपने parameters, अपने response और एक Python उदाहरण के साथ।',
    'Open the reference': 'संदर्भ खोलें',
    'Stuck?': 'कहीं अटक गए?',
    'Ask on Discord': 'Discord पर पूछें',
    'Architects, developers and the maintainers answer there. Or open an issue on GitHub.':
        'वहाँ आर्किटेक्ट्स, डेवलपर्स और maintainers जवाब देते हैं। या GitHub पर एक issue खोलें।',
    'Join Discord': 'Discord से जुड़ें'
});
