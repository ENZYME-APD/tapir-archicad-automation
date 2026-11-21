// Offline tests of the /triage Worker: node --test worker.test.js
import test from 'node:test';
import assert from 'node:assert/strict';
import worker, { handleInteraction, isAllowed, parseOptions, verifyDiscordRequest } from './worker.js';

const subtle = crypto.subtle;
const keyPair = await subtle.generateKey({ name: 'Ed25519' }, true, ['sign', 'verify']);
const publicKeyHex = Buffer.from(await subtle.exportKey('raw', keyPair.publicKey)).toString('hex');

async function signed(body, { timestamp = String(Math.floor(Date.now() / 1000)), tamper = false } = {}) {
  const text = typeof body === 'string' ? body : JSON.stringify(body);
  const signature = Buffer.from(await subtle.sign('Ed25519', keyPair.privateKey,
    new TextEncoder().encode(timestamp + text))).toString('hex');
  return new Request('https://worker.example/', {
    method: 'POST',
    headers: { 'x-signature-ed25519': signature, 'x-signature-timestamp': timestamp },
    body: tamper ? text + ' ' : text,
  });
}

const ENV = { DISCORD_PUBLIC_KEY: publicKeyHex, GITHUB_TOKEN: 'ghp_test', ALLOWED_ROLE_IDS: '111,222', ALLOWED_USER_IDS: '999', DISCORD_GUILD_ID: 'g1' };

function command(options = [], { roles = ['111'], userId = '5', guild = 'g1' } = {}) {
  return { type: 2, application_id: 'app', token: 'tok', guild_id: guild,
    member: { roles, user: { id: userId } }, data: { name: 'triage', options } };
}

function recorder(status = 204) {
  const calls = [];
  const fetchImpl = async (url, init) => { calls.push({ url, init }); return { status: url.includes('github.com') ? status : 200 }; };
  const deferred = [];
  return { calls, fetchImpl, deferred, defer: (p) => deferred.push(p) };
}

async function run(interaction, env = ENV, status = 204) {
  const r = recorder(status);
  const response = await handleInteraction(interaction, env, { fetchImpl: r.fetchImpl, defer: r.defer });
  await Promise.all(r.deferred);
  return { response: await response.json(), calls: r.calls };
}

test('a signed ping is answered with a pong', async () => {
  const response = await worker.fetch(await signed({ type: 1 }), ENV, { waitUntil() {} });
  assert.equal(response.status, 200);
  assert.deepEqual(await response.json(), { type: 1 });
});

test('a tampered body, a wrong key and a stale timestamp are refused', async () => {
  assert.equal((await worker.fetch(await signed({ type: 1 }, { tamper: true }), ENV, {})).status, 401);
  const otherKey = await subtle.generateKey({ name: 'Ed25519' }, true, ['sign', 'verify']);
  const otherHex = Buffer.from(await subtle.exportKey('raw', otherKey.publicKey)).toString('hex');
  assert.equal((await worker.fetch(await signed({ type: 1 }), { ...ENV, DISCORD_PUBLIC_KEY: otherHex }, {})).status, 401);
  const old = String(Math.floor(Date.now() / 1000) - 3600);
  assert.equal((await worker.fetch(await signed({ type: 1 }, { timestamp: old }), ENV, {})).status, 401);
});

test('missing or malformed signature headers and keys are refused, not thrown', async () => {
  const empty = new Headers();
  assert.equal(await verifyDiscordRequest(empty, '{}', publicKeyHex), false);
  const bad = new Headers({ 'x-signature-ed25519': 'zz', 'x-signature-timestamp': '1' });
  assert.equal(await verifyDiscordRequest(bad, '{}', publicKeyHex), false);
  const request = await signed({ type: 1 });
  assert.equal(await verifyDiscordRequest(request.headers, JSON.stringify({ type: 1 }), 'not-hex'), false);
  assert.equal((await worker.fetch(new Request('https://w/', { method: 'GET' }), ENV, {})).status, 405);
});

test('an allowed role starts the workflow with the right request', async () => {
  const { response, calls } = await run(command());
  assert.equal(response.type, 5);
  assert.equal(response.data.flags, 64);
  const github = calls.find((c) => c.url.includes('api.github.com'));
  assert.equal(github.url, 'https://api.github.com/repos/ENZYME-APD/tapir-archicad-automation/actions/workflows/discord_issue_bot.yml/dispatches');
  assert.equal(github.init.headers.Authorization, 'Bearer ghp_test');
  assert.deepEqual(JSON.parse(github.init.body), { ref: 'main', inputs: { dry_run: 'false' } });
  const followUp = calls.find((c) => c.url.includes('discord.com'));
  assert.match(followUp.url, /webhooks\/app\/tok\/messages\/@original$/);
  assert.match(JSON.parse(followUp.init.body).content, /^Started a scan/);
});

test('an allowed user id works without a role; dry run and lookback are passed on', async () => {
  const { calls } = await run(command(
    [{ name: 'dry_run', value: true }, { name: 'lookback_minutes', value: 4320 }],
    { roles: [], userId: '999' }));
  const body = JSON.parse(calls.find((c) => c.url.includes('github.com')).init.body);
  assert.deepEqual(body.inputs, { dry_run: 'true', lookback_minutes: '4320' });
  assert.match(JSON.parse(calls.find((c) => c.url.includes('discord.com')).init.body).content,
    /^Started a dry run of the last 4320 minutes/);
});

test('nobody is allowed without a configured role or user (fails closed)', async () => {
  const env = { ...ENV, ALLOWED_ROLE_IDS: '', ALLOWED_USER_IDS: '' };
  assert.equal(isAllowed(command(), env), false);
  const { response, calls } = await run(command(), env);
  assert.equal(response.type, 4);
  assert.match(response.data.content, /not allowed/);
  assert.equal(calls.length, 0);
});

test('a person without an allowed role or id never reaches GitHub', async () => {
  const { response, calls } = await run(command([], { roles: ['333'], userId: '7' }));
  assert.match(response.data.content, /not allowed/);
  assert.equal(calls.length, 0);
});

test('another server is refused when a server id is configured', async () => {
  const { response, calls } = await run(command([], { guild: 'other' }));
  assert.match(response.data.content, /only works on the Tapir server/);
  assert.equal(calls.length, 0);
});

test('a lookback out of range is refused before anything is sent', async () => {
  for (const value of [0, -5, 10081, 1.5, '60']) {
    assert.ok(parseOptions(command([{ name: 'lookback_minutes', value }])).error, String(value));
  }
  const { response, calls } = await run(command([{ name: 'lookback_minutes', value: 99999 }]));
  assert.match(response.data.content, /from 1 to 10080/);
  assert.equal(calls.length, 0);
});

test('an unknown command is not dispatched', async () => {
  const interaction = command();
  interaction.data.name = 'other';
  const { response, calls } = await run(interaction);
  assert.equal(response.data.content, 'Unknown command.');
  assert.equal(calls.length, 0);
});

test('a GitHub failure is reported without the token', async () => {
  for (const [status, text] of [[401, /refused the token/], [404, /did not find/], [422, /rejected/], [500, /answered 500/]]) {
    const { calls } = await run(command(), ENV, status);
    const content = JSON.parse(calls.find((c) => c.url.includes('discord.com')).init.body).content;
    assert.match(content, /did not start/);
    assert.match(content, text);
    assert.ok(!content.includes('ghp_test'));
  }
});

test('an unreachable GitHub is reported', async () => {
  const calls = [];
  const deferred = [];
  const fetchImpl = async (url, init) => {
    calls.push({ url, init });
    if (url.includes('github.com')) throw new Error('network down');
    return { status: 200 };
  };
  await handleInteraction(command(), ENV, { fetchImpl, defer: (p) => deferred.push(p) });
  await Promise.all(deferred);
  assert.match(JSON.parse(calls.at(-1).init.body).content, /could not be reached/);
});

test('repository, branch and workflow can be set by variables', async () => {
  const env = { ...ENV, GITHUB_REPOSITORY: 'me/fork', GITHUB_REF: 'dev', WORKFLOW_FILE: 'bot.yml' };
  const { calls } = await run(command(), env);
  const github = calls.find((c) => c.url.includes('github.com'));
  assert.equal(github.url, 'https://api.github.com/repos/me/fork/actions/workflows/bot.yml/dispatches');
  assert.equal(JSON.parse(github.init.body).ref, 'dev');
});
