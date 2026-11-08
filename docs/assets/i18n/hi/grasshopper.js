/* हिन्दी: the Grasshopper page (grasshopper/index.html). See ../README.md.
   Component, input and button names stay English, as Grasshopper shows them. */
(window.TAPIR_I18N = window.TAPIR_I18N || []).push ({
    'Archicad Grasshopper live connection | Tapir': 'Archicad और Grasshopper का live connection | Tapir',
    'Connect Grasshopper live to a running Archicad with the Tapir plugin: read, change and create Archicad elements from Rhino geometry, without writing code.':
        'Tapir plugin से Grasshopper को चल रहे Archicad से live जोड़ें: Rhino geometry से Archicad elements पढ़ें, बदलें और बनाएँ, बिना code लिखे।',
    '<a href="../">Tapir</a> / <a href="../docs/">Docs</a> / Grasshopper':
        '<a href="../">Tapir</a> / <a href="../docs/">दस्तावेज़</a> / Grasshopper',
    'Grasshopper, <em>visually.</em>': 'Grasshopper, <em>दृश्य रूप में।</em>',
    'The Tapir plugin connects Grasshopper to a running Archicad: read elements and their settings, change them, and create new ones from Rhino geometry, without writing code.':
        'Tapir plugin, Grasshopper को चल रहे Archicad से जोड़ता है: elements और उनकी settings पढ़ें, उन्हें बदलें, और Rhino geometry से नए elements बनाएँ, बिना code लिखे।',

    // What the components do
    'What the components <em>do.</em>': 'Components <em>क्या करते हैं।</em>',
    'Each component sends a Tapir command to Archicad and gives you the answer as Grasshopper data. You find them on the <strong>Tapir</strong> tab, grouped as in Archicad.':
        'हर component, Archicad को एक Tapir कमांड भेजता है और जवाब आपको Grasshopper data के रूप में देता है। ये <strong>Tapir</strong> tab पर मिलते हैं, Archicad की तरह समूहों में।',
    '<b>READ</b><strong>Elements and their data</strong><span>Elements by type, story or selection; their properties, classifications, settings and geometry.</span>':
        '<b>पढ़ें</b><strong>Elements और उनका data</strong><span>Type, story या selection के अनुसार elements; उनकी properties, classifications, settings और geometry।</span>',
    '<b>CHANGE</b><strong>What you pass in</strong><span>Property values, classifications, layers, IDs and tool settings of the elements you pass in.</span>':
        '<b>बदलें</b><strong>जो आप देते हैं</strong><span>आपके दिए गए elements की property values, classifications, layers, IDs और tool settings।</span>',
    '<b>CREATE</b><strong>From Rhino geometry</strong><span>Walls, doors, windows, railings, level dimensions and more, from points and curves.</span>':
        '<b>बनाएँ</b><strong>Rhino geometry से</strong><span>Points और curves से walls, doors, windows, railings, level dimensions और बहुत कुछ।</span>',
    '<b>PROJECT</b><strong>The rest of the file</strong><span>Stories, attributes, navigator views, layouts, issues, favorites and libraries.</span>':
        '<b>प्रोजेक्ट</b><strong>फ़ाइल का बाक़ी हिस्सा</strong><span>Stories, attributes, navigator views, layouts, issues, favorites और libraries।</span>',
    'Which Tapir commands have a component, and which settings each covers, is on the <a href="../gh-coverage/">Grasshopper coverage</a> page.':
        'किन Tapir कमांड का component है, और हर एक कौन-सी settings cover करता है, यह <a href="../gh-coverage/">Grasshopper कवरेज</a> पेज पर है।',

    // Tool components
    'Tool components': 'Tool के components',
    'One per Archicad <em>tool.</em>': 'हर Archicad tool के लिए <em>एक।</em>',
    'A tool component works like the Archicad tool of the same name. One JSON input and one JSON output per panel of its settings dialog, keyed by the labels you see there, such as <code>Wall Height</code>.':
        'एक tool component उसी नाम के Archicad tool की तरह काम करता है। उसके settings dialog के हर panel के लिए एक JSON input और एक JSON output, जिनकी keys वही labels हैं जो आप वहाँ देखते हैं, जैसे <code>Wall Height</code>।',
    'Placed by <code>StartPoint</code> and <code>EndPoint</code>.':
        '<code>StartPoint</code> और <code>EndPoint</code> से रखे जाते हैं।',
    'Placed in an <code>OwnerWall</code> at a <code>CenterOffset</code>.':
        'एक <code>OwnerWall</code> में, एक <code>CenterOffset</code> पर रखे जाते हैं।',
    'Placed along a <code>ReferenceLine</code>.': 'एक <code>ReferenceLine</code> के साथ-साथ रखे जाते हैं।',
    'Placed at a <code>Position</code>, optionally on a parent element.':
        'एक <code>Position</code> पर रखे जाते हैं, चाहें तो किसी parent element पर।',
    '<b>REFRESH</b><strong>Reads</strong><span>Reads the elements in <code>ElementGuid</code> and fills every output.</span>':
        '<b>REFRESH</b><strong>पढ़ता है</strong><span><code>ElementGuid</code> के elements पढ़ता है और हर output भरता है।</span>',
    '<b>EXECUTE</b><strong>Writes</strong><span>Writes the given settings to those elements. Only the keys you give are written.</span>':
        '<b>EXECUTE</b><strong>लिखता है</strong><span>दी गई settings उन elements में लिखता है। सिर्फ़ वही keys लिखी जाती हैं जो आप देते हैं।</span>',
    '<b>CREATE</b><strong>Makes new elements</strong><span>From the geometry inputs, starting from the tool\'s defaults or from a <code>FavoriteName</code>.</span>':
        '<b>CREATE</b><strong>नए elements बनाता है</strong><span>Geometry inputs से, tool के defaults या किसी <code>FavoriteName</code> से शुरू करके।</span>',
    'Nothing is written to Archicad until you press Execute or Create. A JSON output read by Refresh can be fed straight back into the same input. Archicad has no command to change railings or level dimensions after they are placed, so for those Execute only writes the ID, layer and draw order.':
        'जब तक आप Execute या Create नहीं दबाते, Archicad में कुछ नहीं लिखा जाता। Refresh से पढ़ा गया JSON output सीधे उसी input में वापस दिया जा सकता है। Railings या level dimensions रखने के बाद उन्हें बदलने का कोई कमांड Archicad में नहीं है, इसलिए उनके लिए Execute सिर्फ़ ID, layer और draw order लिखता है।',
    '<strong class="callout-label"><span aria-hidden="true">ⓘ</span> Note</strong> Walls remembers the walls its Create made, in the <code>.gh</code> file. The next Create updates them instead of making new ones. Turn this off, or forget them, in the component\'s menu.':
        '<strong class="callout-label"><span aria-hidden="true">ⓘ</span> ध्यान दें</strong> Walls अपने Create से बनी walls को <code>.gh</code> फ़ाइल में याद रखता है। अगला Create नई walls बनाने के बजाय उन्हीं को अपडेट करता है। इसे component के menu में बंद करें, या उन्हें भुला दें।',

    // Install
    'Install': 'इंस्टॉल करें',
    'Install the Tapir Add-On in Archicad': 'Archicad में Tapir Add-On इंस्टॉल करें',
    'The components talk to Archicad through the Add-On. <a href="../start/#install">Get started</a> shows how.':
        'Components, Add-On के ज़रिए Archicad से बात करते हैं। <a href="../start/#install">शुरू करें</a> में तरीका दिया गया है।',
    'Install the <span class="mono">tapir</span> package in Rhino':
        'Rhino में <span class="mono">tapir</span> package इंस्टॉल करें',
    'In Rhino 7 or 8, open <em>Tools &gt; Package Manager</em> (or type <code>PackageManager</code>), search for <strong>tapir</strong> and click Install. Restart Rhino.':
        'Rhino 7 या 8 में <em>Tools &gt; Package Manager</em> खोलें (या <code>PackageManager</code> टाइप करें), <strong>tapir</strong> खोजें और Install पर क्लिक करें। Rhino को फिर से शुरू करें।',
    'Open Archicad, then Grasshopper': 'पहले Archicad खोलें, फिर Grasshopper',
    'With a project open in Archicad, start Grasshopper. The <strong>Tapir</strong> tab appears in its toolbar. With more than one Archicad open, give the port of the one you want to the <strong>ConnectArchicad</strong> component (<em>Tapir &gt; General</em>); the first Archicad uses 19723.':
        'Archicad में एक प्रोजेक्ट खुला हो, तब Grasshopper शुरू करें। उसके toolbar में <strong>Tapir</strong> tab दिखता है। एक से ज़्यादा Archicad खुले हों, तो जिसे चाहते हैं उसका port <strong>ConnectArchicad</strong> component (<em>Tapir &gt; General</em>) को दें; पहला Archicad 19723 इस्तेमाल करता है।',

    // A first definition
    'A first definition': 'पहली definition',
    'One <em>wall.</em>': 'एक <em>wall।</em>',
    'Build this in an empty Grasshopper canvas, with a project open in Archicad:':
        'Archicad में एक प्रोजेक्ट खुला रखकर, एक खाली Grasshopper canvas पर यह बनाएँ:',
    'Place two <strong>Point</strong> parameters and set them to 0,0,0 and 5,0,0. Archicad reads the numbers as metres.':
        'दो <strong>Point</strong> parameters रखें और उन्हें 0,0,0 और 5,0,0 पर सेट करें। Archicad इन संख्याओं को मीटर में पढ़ता है।',
    'Place the <strong>Walls</strong> component from <em>Tapir &gt; Design Tools</em>. Connect the points to <code>StartPoint</code> and <code>EndPoint</code>.':
        '<em>Tapir &gt; Design Tools</em> से <strong>Walls</strong> component रखें। Points को <code>StartPoint</code> और <code>EndPoint</code> से जोड़ें।',
    'Place a <strong>Panel</strong> with the text on the right and connect it to <code>GeometryAndPositioning_JSON</code>.':
        'दाईं ओर वाले text के साथ एक <strong>Panel</strong> रखें और उसे <code>GeometryAndPositioning_JSON</code> से जोड़ें।',
    'Press <strong>Create</strong>. A 5 m wall appears in Archicad, 3 m high and 30 cm thick.':
        '<strong>Create</strong> दबाएँ। Archicad में 5 m लंबी, 3 m ऊँची और 30 cm मोटी एक wall दिखती है।',
    'Move a point and press Create again: the same wall moves. Change the panel and press Create: the same wall changes.':
        'एक point हिलाएँ और फिर से Create दबाएँ: वही wall खिसक जाती है। Panel बदलें और Create दबाएँ: वही wall बदल जाती है।',
    'Press <strong>Refresh</strong> to see every setting of the wall in the JSON outputs. Copy a key from there to set it the next time.':
        'Wall की हर setting JSON outputs में देखने के लिए <strong>Refresh</strong> दबाएँ। अगली बार उसे सेट करने के लिए वहाँ से एक key कॉपी करें।',

    // Next step
    'Next step': 'अगला क़दम',
    'Install the Add-On': 'Add-On इंस्टॉल करें',
    'Install Tapir and the <span class="mono">tapir</span> package, then try the wall above.':
        'Tapir और <span class="mono">tapir</span> package इंस्टॉल करें, फिर ऊपर वाली wall आज़माएँ।',
    'Get started': 'शुरू करें',
    'Coverage': 'कवरेज',
    'Which commands have a component, and which inputs each still lacks.':
        'किन कमांड का component है, और हर एक में अभी कौन-से inputs बाक़ी हैं।',
    'See the coverage': 'कवरेज देखें',
    'Videos': 'वीडियो',
    'Watch it work': 'इसे काम करते देखें',
    'The Archicad and Grasshopper tutorials.': 'Archicad और Grasshopper के ट्यूटोरियल।',
    'Watch the tutorials': 'ट्यूटोरियल देखें'
});
