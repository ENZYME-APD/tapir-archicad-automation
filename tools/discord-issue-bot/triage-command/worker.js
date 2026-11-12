// The /triage slash command of the Discord issue bot.
//
// Discord sends a slash command to an HTTPS endpoint; GitHub Actions cannot
// listen for it. This Cloudflare Worker is that endpoint. It checks Discord's
// signature, checks that the person is allowed, and starts the "Discord Issue
// Bot" workflow with workflow_dispatch - nothing else. It never reads
// messages, never creates issues and never sees the Discord bot token.
//
// Secrets: DISCORD_PUBLIC_KEY (the application's public key), GITHUB_TOKEN (a
// fine-grained token with "Actions: read and write" on this one repository).
// Variables: ALLOWED_ROLE_IDS and/or ALLOWED_USER_IDS (comma separated; with
// neither set nobody is allowed), DISCORD_GUILD_ID (optional), and optionally
// GITHUB_REPOSITORY, GITHUB_REF, WORKFLOW_FILE. Setup: README.md.

const DEFAULT_REPOSITORY = 'ENZYME-APD/tapir-archicad-automation';
const DEFAULT_REF = 'main';
const DEFAULT_WORKFLOW = 'discord_issue_bot.yml';
const MAX_LOOKBACK_MINUTES = 10080; // one week
const MAX_SIGNATURE_AGE_SECONDS = 300;
const EPHEMERAL = 64; // only the person who ran the command sees the reply

const PING = 1;
const APPLICATION_COMMAND = 2;
const PONG = 1;
const CHANNEL_MESSAGE = 4;
const DEFERRED_CHANNEL_MESSAGE = 5;

export function hexToBytes(hex) {
  if (typeof hex !== 'string' || hex.length % 2 !== 0 || !/^[0-9a-fA-F]*$/.test(hex)) {
    return null;
  }
  const bytes = new Uint8Array(hex.length / 2);
  for (let i = 0; i < bytes.length; i++) {
    bytes[i] = parseInt(hex.slice(i * 2, i * 2 + 2), 16);
  }
  return bytes;
}

// Node (and tests) name the algorithm Ed25519, Workers NODE-ED25519.
async function importPublicKey(publicKeyHex) {
  const raw = hexToBytes(publicKeyHex);
  if (!raw || raw.length !== 32) return null;
  for (const algorithm of [{ name: 'Ed25519' }, { name: 'NODE-ED25519', namedCurve: 'NODE-ED25519' }]) {
    try {
      const key = await crypto.subtle.importKey('raw', raw, algorithm, false, ['verify']);
      return { key, algorithm };
    } catch (error) {
      // try the next name
    }
  }
  return null;
}

// True only for a request Discord signed with the application's key, not
// older than MAX_SIGNATURE_AGE_SECONDS (so a captured request cannot be replayed later).
export async function verifyDiscordRequest(headers, bodyText, publicKeyHex, nowSeconds) {
  const signature = hexToBytes(headers.get('x-signature-ed25519') || '');
  const timestamp = headers.get('x-signature-timestamp') || '';
  if (!signature || signature.length !== 64 || !/^\d+$/.test(timestamp)) return false;
  const now = nowSeconds ?? Math.floor(Date.now() / 1000);
  if (Math.abs(now - Number(timestamp)) > MAX_SIGNATURE_AGE_SECONDS) return false;
  const imported = await importPublicKey(publicKeyHex);
  if (!imported) return false;
  try {
    return await crypto.subtle.verify(imported.algorithm, imported.key, signature,
      new TextEncoder().encode(timestamp + bodyText));
  } catch (error) {
    return false;
  }
}

function idList(value) {
  return String(value || '').split(',').map((part) => part.trim()).filter(Boolean);
}

// Fails closed: nobody is allowed until a role or user is listed.
export function isAllowed(interaction, env) {
  const roles = idList(env.ALLOWED_ROLE_IDS);
  const users = idList(env.ALLOWED_USER_IDS);
  const userId = interaction.member?.user?.id ?? interaction.user?.id;
  if (userId && users.includes(userId)) return true;
  const memberRoles = interaction.member?.roles ?? [];
  return roles.some((role) => memberRoles.includes(role));
}

export function parseOptions(interaction) {
  const options = {};
  for (const option of interaction.data?.options ?? []) options[option.name] = option.value;
  const dryRun = options.dry_run === true;
  let lookback = null;
  if (options.lookback_minutes !== undefined) {
    const value = options.lookback_minutes;
    if (!Number.isInteger(value) || value < 1 || value > MAX_LOOKBACK_MINUTES) {
      return { error: `lookback_minutes must be a whole number from 1 to ${MAX_LOOKBACK_MINUTES}.` };
    }
    lookback = value;
  }
  return { dryRun, lookback };
}

function reply(content, type = CHANNEL_MESSAGE) {
  return Response.json({ type, data: { content, flags: EPHEMERAL, allowed_mentions: { parse: [] } } });
}

export async function dispatchWorkflow(env, { dryRun, lookback }, fetchImpl = fetch) {
  const repository = env.GITHUB_REPOSITORY || DEFAULT_REPOSITORY;
  const workflow = env.WORKFLOW_FILE || DEFAULT_WORKFLOW;
  const inputs = { dry_run: dryRun ? 'true' : 'false' };
  if (lookback !== null) inputs.lookback_minutes = String(lookback);
  const response = await fetchImpl(
    `https://api.github.com/repos/${repository}/actions/workflows/${workflow}/dispatches`,
    {
      method: 'POST',
      headers: {
        Authorization: `Bearer ${env.GITHUB_TOKEN}`,
        Accept: 'application/vnd.github+json',
        'X-GitHub-Api-Version': '2022-11-28',
        'User-Agent': 'tapir-discord-triage-command',
        'Content-Type': 'application/json',
      },
      body: JSON.stringify({ ref: env.GITHUB_REF || DEFAULT_REF, inputs }),
    },
  );
  return response.status; // 204 on success
}

function describeFailure(status) {
  if (status === 401 || status === 403) return 'GitHub refused the token (it needs "Actions: read and write" on the repository).';
  if (status === 404) return 'GitHub did not find the workflow or the repository (check the token and the names).';
  if (status === 422) return 'GitHub rejected the request (the branch or the workflow inputs).';
  return `GitHub answered ${status}.`;
}

async function followUp(interaction, content, env, fetchImpl) {
  const url = `https://discord.com/api/v10/webhooks/${interaction.application_id}/${interaction.token}/messages/@original`;
  try {
    await fetchImpl(url, {
      method: 'PATCH',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ content, allowed_mentions: { parse: [] } }),
    });
  } catch (error) {
    // the run was started or it was not; Discord just keeps showing "thinking"
  }
}

// Returns the Response for Discord; `defer(promise)` keeps the Worker alive for the
// work that continues after the immediate answer.
export async function handleInteraction(interaction, env, { fetchImpl = fetch, defer = () => {} } = {}) {
  if (interaction.type === PING) return Response.json({ type: PONG });
  if (interaction.type !== APPLICATION_COMMAND || interaction.data?.name !== 'triage') {
    return reply('Unknown command.');
  }
  if (env.DISCORD_GUILD_ID && interaction.guild_id !== env.DISCORD_GUILD_ID) {
    return reply('This command only works on the Tapir server.');
  }
  if (!isAllowed(interaction, env)) {
    return reply('You are not allowed to run /triage.');
  }
  const parsed = parseOptions(interaction);
  if (parsed.error) return reply(parsed.error);

  defer((async () => {
    let message;
    try {
      const status = await dispatchWorkflow(env, parsed, fetchImpl);
      if (status === 204) {
        message = (parsed.dryRun ? 'Started a dry run' : 'Started a scan')
          + (parsed.lookback !== null ? ` of the last ${parsed.lookback} minutes` : '')
          + ' of the Discord channels. Issues it files are announced in the channel they came from.';
      } else {
        message = `The scan did not start. ${describeFailure(status)}`;
      }
    } catch (error) {
      message = 'The scan did not start: GitHub could not be reached.';
    }
    await followUp(interaction, message, env, fetchImpl);
  })());
  return Response.json({ type: DEFERRED_CHANNEL_MESSAGE, data: { flags: EPHEMERAL } });
}

export default {
  async fetch(request, env, ctx) {
    if (request.method !== 'POST') return new Response('POST only', { status: 405 });
    const bodyText = await request.text();
    if (!(await verifyDiscordRequest(request.headers, bodyText, env.DISCORD_PUBLIC_KEY))) {
      return new Response('Bad request signature', { status: 401 });
    }
    let interaction;
    try {
      interaction = JSON.parse(bodyText);
    } catch (error) {
      return new Response('Bad JSON', { status: 400 });
    }
    return handleInteraction(interaction, env, { defer: (promise) => ctx.waitUntil(promise) });
  },
};
