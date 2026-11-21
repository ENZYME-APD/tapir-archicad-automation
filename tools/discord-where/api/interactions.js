// The /where slash command of the Tapir Discord: "/where place: Maribor" puts the person's town
// on the Community page's map (https://archicad-api.com/community/#user-map).
//
// Discord sends each use of the command here (the app's Interactions Endpoint URL). The answer is
// a public message in the channel, which is also the record: tools/generate_user_map.py reads the
// app's /where answers every night and places the town in **bold** for the person who used the
// command. So nothing is stored here, and a person's latest /where wins.
// "/where place: remove" takes them off the map.
//
// DISCORD_PUBLIC_KEY (the app's Public Key, Developer Portal > General Information) checks that a
// request comes from Discord; without it every request is refused.
//
// Discord needs an answer within 3 s, and a cold start plus the OpenStreetMap lookup can take
// longer. So the command is answered at once with "thinking…" (a deferred answer), and the town
// is filled in afterwards by editing that answer (waitUntil keeps the function alive for it).

import { waitUntil } from '@vercel/functions';

const MAP_URL = 'https://archicad-api.com/community/#user-map';
const REMOVED = 'Removed from the Tapir map.';
const NOMINATIM = 'https://nominatim.openstreetmap.org/search';
const USER_AGENT = 'TapirWhereCommand (https://github.com/ENZYME-APD/tapir-archicad-automation, 1.0)';

function hexToBytes (hex) {
    return new Uint8Array ((hex.match (/../g) || []).map ((b) => parseInt (b, 16)));
}

async function verify (request, body) {
    const key = (process.env.DISCORD_PUBLIC_KEY || '').trim ();
    const signature = request.headers.get ('x-signature-ed25519') || '';
    const timestamp = request.headers.get ('x-signature-timestamp') || '';
    if (!key || !signature || !timestamp) {
        return false;
    }
    try {
        const publicKey = await crypto.subtle.importKey ('raw', hexToBytes (key), { name: 'Ed25519' }, false, ['verify']);
        return await crypto.subtle.verify ('Ed25519', publicKey, hexToBytes (signature), new TextEncoder ().encode (timestamp + body));
    } catch (e) {
        return false;
    }
}

// What a person typed, safe to show in bold: no markdown, mentions or line breaks.
function cleanPlace (text) {
    return String (text || '').replace (/[*_~|`<>@#\\\n\r]+/g, ' ').replace (/\s+/g, ' ').trim ().slice (0, 60);
}

// The town as OpenStreetMap knows it ("Maribor", "Slovenia"), or null, within 5 s.
async function lookUp (place) {
    try {
        const params = new URLSearchParams ({ q: place, format: 'jsonv2', limit: '1', addressdetails: '1', 'accept-language': 'en' });
        const response = await fetch (`${NOMINATIM}?${params}`, { headers: { 'User-Agent': USER_AGENT }, signal: AbortSignal.timeout (5000) });
        const hit = response.ok ? (await response.json ())[0] : null;
        const address = (hit && hit.address) || {};
        if (!hit || !address.country) {
            return null;
        }
        const town = address.city || address.town || address.village || address.municipality || address.county || address.state;
        return { town: cleanPlace (town || ''), country: cleanPlace (address.country) };
    } catch (e) {
        return null;
    }
}

// The words of the answer: the town in **bold**, which tools/generate_user_map.py reads.
async function answer (userId, place) {
    if (/^(remove|delete|none|off)$/i.test (place)) {
        return `🗑️ <@${userId}>: ${REMOVED}`;
    }
    const found = await lookUp (place);
    if (found && found.town) {
        const country = found.country && found.country !== found.town ? `, ${found.country}` : '';
        return `📍 <@${userId}> is in **${found.town}**${country}. On the Tapir map tonight: <${MAP_URL}>`;
    }
    return `📍 <@${userId}> is in **${place}**. On the Tapir map tonight, if OpenStreetMap knows it: <${MAP_URL}>`;
}

// Replaces the "thinking…" answer with the real one. allowed_mentions: none, so naming the
// person never pings them.
async function finish (interaction, userId, place) {
    const content = await answer (userId, place);
    const url = `https://discord.com/api/v10/webhooks/${interaction.application_id}/${interaction.token}/messages/@original`;
    const response = await fetch (url, {
        method: 'PATCH',
        headers: { 'Content-Type': 'application/json', 'User-Agent': USER_AGENT },
        body: JSON.stringify ({ content, allowed_mentions: { parse: [] } }),
    });
    console.log (`where: answered (${response.status})`);
}

export async function POST (request) {
    const started = Date.now ();
    const body = await request.text ();
    if (!(await verify (request, body))) {
        console.log ('where: refused a request with a bad or missing signature');
        return new Response ('Bad signature', { status: 401 });
    }
    const interaction = JSON.parse (body);
    console.log (`where: interaction type ${interaction.type} ${interaction.data?.name || ''} after ${Date.now () - started} ms`);
    if (interaction.type === 1) { // Discord's PING when the endpoint URL is saved
        return Response.json ({ type: 1 });
    }
    if (interaction.type !== 2 || interaction.data?.name !== 'where') {
        return Response.json ({ type: 4, data: { content: 'Unknown command.', flags: 64 } });
    }
    const user = (interaction.member && interaction.member.user) || interaction.user || {};
    const option = (interaction.data.options || []).find ((o) => o.name === 'place');
    const place = cleanPlace (option && option.value);
    if (!place) {
        return Response.json ({ type: 4, data: { flags: 64, content: 'Type your town, for example `/where place: Maribor, Slovenia`.' } });
    }
    waitUntil (finish (interaction, user.id, place).catch ((error) => console.log (`where: could not answer: ${error}`)));
    return Response.json ({ type: 5 }); // "Tapir Data Scraper is thinking…", public
}

export function GET () {
    return new Response ('The Tapir /where command. Discord posts here.', { status: 200 });
}
