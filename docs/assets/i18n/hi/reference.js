/* हिन्दी: the heads, headings and introductions of the reference pages (archicad-addon/, api-coverage/,
   gh-coverage/), which stay mostly English (data-i18n-partial). See ../README.md. */
(window.TAPIR_I18N = window.TAPIR_I18N || []).push ({
    // The command reference (archicad-addon/)
    'Archicad JSON API commands reference | Tapir': 'Archicad JSON API कमांड संदर्भ | Tapir',
    'Every JSON command the Tapir add-on adds to Archicad, with its parameters, its response and a Python example, and how to call one from Python.':
        'Tapir add-on, Archicad में जो JSON कमांड जोड़ता है, उनमें से हर एक, उसके parameters, उसके response और एक Python उदाहरण के साथ, और Python से कोई कमांड call कैसे करें।',
    '<a href="../">Tapir</a> / <a href="../docs/">Docs</a> / Commands': '<a href="../">Tapir</a> / <a href="../docs/">दस्तावेज़</a> / कमांड',
    'Tapir <em>commands.</em>': 'Tapir <em>कमांड।</em>',
    'The JSON commands the Tapir add-on registers in Archicad, in the <code>TapirCommand</code> namespace, next to Graphisoft\'s official commands. Each command shows its parameters, its response and an example.':
        'वे JSON कमांड जो Tapir add-on, Archicad में <code>TapirCommand</code> namespace में, Graphisoft के official कमांड के साथ register करता है। हर कमांड अपने parameters, अपना response और एक उदाहरण दिखाता है।',
    'New to Tapir? <a href="../start/">Get started</a> shows how to install it and run a first script. Looking for something no command does? <a href="../limits/">What the Archicad API can\'t do</a> lists what Graphisoft\'s API does not offer, with what to do instead.':
        'Tapir में नए हैं? <a href="../start/">शुरू करें</a> में बताया गया है कि इसे install कैसे करें और पहली script कैसे चलाएँ। कुछ ऐसा ढूँढ रहे हैं जो कोई command नहीं करता? <a href="../limits/">Archicad API क्या नहीं कर सकता</a> बताता है कि Graphisoft का API क्या नहीं देता, और उसकी जगह क्या करें।',
    'Calling a <em>command.</em>': 'कमांड <em>call करना।</em>',
    'Archicad answers JSON commands on <code>http://127.0.0.1:19723</code> while a project is open (the next Archicads opened use 19724, 19725 and so on). A Tapir command is sent through Archicad\'s <code>API.ExecuteAddOnCommand</code> command.':
        'जब तक कोई project खुला है, Archicad <code>http://127.0.0.1:19723</code> पर JSON कमांड का जवाब देता है (बाद में खोले गए Archicad 19724, 19725 आदि इस्तेमाल करते हैं)। Tapir कमांड, Archicad के <code>API.ExecuteAddOnCommand</code> कमांड के ज़रिए भेजा जाता है।',
    'Python standard library': 'Python standard library',
    'Graphisoft\'s <a href="https://pypi.org/project/archicad/"><code>archicad</code> package</a>':
        'Graphisoft का <a href="https://pypi.org/project/archicad/"><code>archicad</code> package</a>',
    'The examples of the commands below use <code>aclib</code>, the small helper in <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/tree/main/archicad-addon/Examples">archicad-addon/Examples</a> that sends commands the same way as the standard-library script.':
        'नीचे के कमांड के उदाहरण <code>aclib</code> इस्तेमाल करते हैं, जो <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/tree/main/archicad-addon/Examples">archicad-addon/Examples</a> में एक छोटा helper है और कमांड उसी तरह भेजता है जैसे standard library वाली script।',
    'All <em>commands.</em>': 'सभी <em>कमांड।</em>',

    // The two coverage pages
    'Overview': 'सारांश',
    'By group': 'Group के अनुसार',
    'Checklist': 'Checklist',

    // The Archicad API coverage (api-coverage/)
    'Archicad API coverage checklist | Tapir': 'Archicad API कवरेज checklist | Tapir',
    'Which parts of Graphisoft\'s Archicad API the Tapir add-on uses, item by item, kept up to date from every issue, pull request and push to the repository.':
        'Tapir add-on, Graphisoft के Archicad API के कौन-से हिस्से इस्तेमाल करता है, एक-एक item करके, repository के हर issue, pull request और push के साथ अपडेट।',
    '<a href="../">Tapir</a> / <a href="../docs/">Docs</a> / API coverage': '<a href="../">Tapir</a> / <a href="../docs/">दस्तावेज़</a> / API कवरेज',
    'Archicad API <em>coverage.</em>': 'Archicad API <em>कवरेज।</em>',
    'What Tapir covers of Graphisoft\'s Archicad API, item by item.': 'Tapir, Graphisoft के Archicad API का क्या-क्या cover करता है, एक-एक item करके।',
    'Which parts of the Archicad API (<span id="archicad-range">every supported Archicad version</span>: C functions with their older names and command ids, and the C++ API) the <a href="https://github.com/ENZYME-APD/tapir-archicad-automation">Tapir Add-On</a> uses, updated within minutes of every issue and push to main, and within the hour of every pull request. See also the <a href="../archicad-addon/">command reference</a>.':
        '<a href="https://github.com/ENZYME-APD/tapir-archicad-automation">Tapir Add-On</a>, Archicad API (<span id="archicad-range">हर supported Archicad version</span>: C functions अपने पुराने नामों और command ids के साथ, और C++ API) के कौन-से हिस्से इस्तेमाल करता है। हर issue और main पर हर push के कुछ मिनटों में, और हर pull request के एक घंटे के भीतर अपडेट होता है। <a href="../archicad-addon/">कमांड संदर्भ</a> भी देखें।',
    'By part of the API': 'API के हिस्से के अनुसार',

    // The Grasshopper coverage (gh-coverage/)
    'Grasshopper coverage of the Tapir commands | Tapir': 'Tapir कमांड का Grasshopper कवरेज | Tapir',
    'Which Tapir commands for Archicad the components of the Tapir Grasshopper plugin call, and whether they cover each command\'s parameters or still miss some.':
        'Tapir Grasshopper plugin के components, Archicad के लिए कौन-से Tapir कमांड call करते हैं, और क्या वे हर कमांड के parameters cover करते हैं या अभी कुछ छूट जाते हैं।',
    '<a href="../">Tapir</a> / <a href="../docs/">Docs</a> / Grasshopper coverage': '<a href="../">Tapir</a> / <a href="../docs/">दस्तावेज़</a> / Grasshopper कवरेज',
    'Grasshopper <em>coverage.</em>': 'Grasshopper <em>कवरेज।</em>',
    'Which Tapir commands the Grasshopper components call, and the parameters they still miss.':
        'Grasshopper components कौन-से Tapir कमांड call करते हैं, और कौन-से parameters अभी बाकी हैं।',
    'Which JSON commands of the <a href="https://github.com/ENZYME-APD/tapir-archicad-automation">Tapir Add-On</a> a component of the Tapir Grasshopper plugin calls, and whether its components cover the command\'s parameters, updated within minutes of every issue and push to main, and within the hour of every pull request. This is a static check of the plugin\'s code: it shows which parameters the components name, not that each one works. See also the <a href="../archicad-addon/">command reference</a> and the <a href="../api-coverage/">Archicad API coverage</a>.':
        'Tapir Grasshopper plugin का कोई component <a href="https://github.com/ENZYME-APD/tapir-archicad-automation">Tapir Add-On</a> के कौन-से JSON कमांड call करता है, और क्या उसके components कमांड के parameters cover करते हैं। हर issue और main पर हर push के कुछ मिनटों में, और हर pull request के एक घंटे के भीतर अपडेट होता है। यह plugin के code की static जाँच है: यह दिखाती है कि components कौन-से parameters लेते हैं, यह नहीं कि हर एक काम करता है। <a href="../archicad-addon/">कमांड संदर्भ</a> और <a href="../api-coverage/">Archicad API कवरेज</a> भी देखें।',
    'Parameters of the used commands': 'इस्तेमाल हुए कमांड के parameters'
});
