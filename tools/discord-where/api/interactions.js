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

// The town as OpenStreetMap knows it ("Maribor", "Slovenia"), or null; never longer than ~1.5 s,
// as Discord needs the answer within 3 s.
async function lookUp (place) {
    try {
        const params = new URLSearchParams ({ q: place, format: 'jsonv2', limit: '1', addressdetails: '1', 'accept-language': 'en' });
        const response = await fetch (`${NOMINATIM}?${params}`, { headers: { 'User-Agent': USER_AGENT }, signal: AbortSignal.timeout (1500) });
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

function reply (content) {
    // allowed_mentions: none, so naming the person never pings them.
    return Response.json ({ type: 4, data: { content, allowed_mentions: { parse: [] } } });
}

export async function POST (request) {
    const body = await request.text ();
    if (!(await verify (request, body))) {
        return new Response ('Bad signature', { status: 401 });
    }
    const interaction = JSON.parse (body);
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
    if (/^(remove|delete|none|off)$/i.test (place)) {
        return reply (`🗑️ <@${user.id}>: ${REMOVED}`);
    }
    const found = await lookUp (place);
    if (found && found.town) {
        const country = found.country && found.country !== found.town ? `, ${found.country}` : '';
        return reply (`📍 <@${user.id}> is in **${found.town}**${country}. On the Tapir map tonight: <${MAP_URL}>`);
    }
    return reply (`📍 <@${user.id}> is in **${place}**. On the Tapir map tonight, if OpenStreetMap knows it: <${MAP_URL}>`);
}

export function GET () {
    return new Response ('The Tapir /where command. Discord posts here.', { status: 200 });
}
