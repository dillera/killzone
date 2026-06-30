const net = require('net');
const { once } = require('events');
const World = require('../src/world');
const TcpServer = require('../src/tcp_server');

function buildJoinPacket(name) {
  const nameBuf = Buffer.from(name);
  return Buffer.concat([
    Buffer.from([0x01, nameBuf.length]),
    nameBuf
  ]);
}

function parseJoinResponse(buf) {
  let offset = 0;
  const type = buf.readUInt8(offset++);
  const idLen = buf.readUInt8(offset++);
  const id = buf.slice(offset, offset + idLen).toString();
  offset += idLen;
  const x = buf.readUInt8(offset++);
  const y = buf.readUInt8(offset++);
  const health = buf.readUInt8(offset++);
  const verLen = buf.readUInt8(offset++);
  const version = buf.slice(offset, offset + verLen).toString();

  return { type, idLen, id, x, y, health, verLen, version, totalLen: 1 + 1 + idLen + 1 + 1 + 1 + 1 + verLen };
}

function parseMoveResponse(buf) {
  let offset = 0;
  const type = buf.readUInt8(offset++);
  const x = buf.readUInt8(offset++);
  const y = buf.readUInt8(offset++);
  const health = buf.readUInt8(offset++);
  const collision = buf.readUInt8(offset++);
  const msgLen = buf.readUInt8(offset++);
  const message = buf.slice(offset, offset + msgLen).toString();
  offset += msgLen;
  const loserIdLen = buf.readUInt8(offset++);
  const loserId = buf.slice(offset, offset + loserIdLen).toString();
  offset += loserIdLen;

  return {
    type,
    x,
    y,
    health,
    collision,
    msgLen,
    message,
    loserIdLen,
    loserId,
    totalLen: offset
  };
}

function parseStateResponse(buf) {
  let offset = 0;
  const type = buf.readUInt8(offset++);
  const count = buf.readUInt8(offset++);
  const ticksLow = buf.readUInt8(offset++);
  const ticksHigh = buf.readUInt8(offset++);
  const msgLen = buf.readUInt8(offset++);
  const message = buf.slice(offset, offset + msgLen).toString();
  offset += msgLen;
  const entities = [];
  for (let i = 0; i < count; i++) {
    const typeChar = String.fromCharCode(buf.readUInt8(offset++));
    const x = buf.readUInt8(offset++);
    const y = buf.readUInt8(offset++);
    entities.push({ typeChar, x, y });
  }
  return {
    type,
    count,
    ticks: (ticksHigh << 8) | ticksLow,
    msgLen,
    message,
    entities,
    totalLen: offset
  };
}

function waitForData(socket, timeoutMs = 500) {
  return new Promise((resolve, reject) => {
    const timer = setTimeout(() => {
      cleanup();
      reject(new Error(`Timed out waiting for socket data (${timeoutMs}ms)`));
    }, timeoutMs);

    const onData = (chunk) => {
      cleanup();
      resolve(chunk);
    };

    const onError = (err) => {
      cleanup();
      reject(err);
    };

    function cleanup() {
      clearTimeout(timer);
      socket.off('data', onData);
      socket.off('error', onError);
    }

    socket.on('data', onData);
    socket.on('error', onError);
  });
}

function waitForNoData(socket, timeoutMs = 150) {
  return new Promise((resolve, reject) => {
    const onData = () => {
      cleanup();
      reject(new Error('Expected no data, but received a packet'));
    };

    const onError = (err) => {
      cleanup();
      reject(err);
    };

    const timer = setTimeout(() => {
      cleanup();
      resolve();
    }, timeoutMs);

    function cleanup() {
      clearTimeout(timer);
      socket.off('data', onData);
      socket.off('error', onError);
    }

    socket.on('data', onData);
    socket.on('error', onError);
  });
}

async function createServerAndClient() {
  const world = new World(40, 20);
  const tcpServer = new TcpServer(world, 0);
  tcpServer.start();
  await once(tcpServer.server, 'listening');
  const { port } = tcpServer.server.address();
  const client = net.createConnection({ port, host: '127.0.0.1' });
  await once(client, 'connect');
  return { world, tcpServer, client };
}

describe('TCP Server Protocol', () => {
  const sockets = [];
  const servers = [];
  let logSpy;

  beforeAll(() => {
    logSpy = jest.spyOn(console, 'log').mockImplementation(() => {});
  });

  afterAll(() => {
    if (logSpy) {
      logSpy.mockRestore();
    }
  });

  afterEach(async () => {
    for (const socket of sockets.splice(0)) {
      await new Promise((resolve) => {
        if (socket.destroyed) {
          resolve();
          return;
        }
        const timer = setTimeout(() => {
          if (!socket.destroyed) {
            socket.destroy();
          }
          resolve();
        }, 250);
        socket.once('close', () => {
          clearTimeout(timer);
          resolve();
        });
        socket.end();
      });
    }
    await Promise.all(servers.splice(0).map((srv) => new Promise((resolve) => srv.close(resolve))));
  });

  test('reassembles fragmented join packets and returns a valid join response', async () => {
    const { world, tcpServer, client } = await createServerAndClient();
    sockets.push(client);
    servers.push(tcpServer.server);

    const joinPacket = buildJoinPacket('FragUser');
    client.write(joinPacket.slice(0, 1));
    client.write(joinPacket.slice(1));

    const joinRespRaw = await waitForData(client);
    const joinResp = parseJoinResponse(joinRespRaw);

    expect(joinResp.type).toBe(0x01);
    expect(joinResp.idLen).toBeGreaterThan(0);
    expect(joinResp.id).toMatch(/^player_/);
    expect(joinResp.health).toBe(100);
    expect(joinResp.verLen).toBeGreaterThan(0);
    expect(joinResp.version.length).toBe(joinResp.verLen);
    expect(world.getPlayerCount()).toBe(1);
  });

  test('parses coalesced client commands (move + state) from one TCP data event', async () => {
    const { world, tcpServer, client } = await createServerAndClient();
    sockets.push(client);
    servers.push(tcpServer.server);

    client.write(buildJoinPacket('ComboUser'));
    const joinRespRaw = await waitForData(client);
    const joinResp = parseJoinResponse(joinRespRaw);
    expect(joinResp.type).toBe(0x01);

    world.clearKillMessage(); // keep state packet deterministic
    client.write(Buffer.from([0x02, 'u'.charCodeAt(0), 0x03]));

    let combined = await waitForData(client);
    if (combined.length < 7) {
      combined = Buffer.concat([combined, await waitForData(client)]);
    }

    const moveResp = parseMoveResponse(combined);
    expect(moveResp.type).toBe(0x02);
    expect(moveResp.loserIdLen).toBe(0);
    expect(moveResp.totalLen).toBe(7);

    let stateBuf = combined.slice(moveResp.totalLen);
    if (stateBuf.length < 5) {
      stateBuf = Buffer.concat([stateBuf, await waitForData(client)]);
    }
    const stateResp = parseStateResponse(stateBuf);
    expect(stateResp.type).toBe(0x03);
    expect(stateResp.count).toBeGreaterThanOrEqual(1);
  });

  test('includes loser_id in move response and blocks dead socket from further moves', async () => {
    const world = new World(40, 20);
    const tcpServer = new TcpServer(world, 0);
    tcpServer.start();
    await once(tcpServer.server, 'listening');
    servers.push(tcpServer.server);

    const { port } = tcpServer.server.address();
    const attackerClient = net.createConnection({ port, host: '127.0.0.1' });
    const defenderClient = net.createConnection({ port, host: '127.0.0.1' });
    await Promise.all([once(attackerClient, 'connect'), once(defenderClient, 'connect')]);
    sockets.push(attackerClient, defenderClient);

    attackerClient.write(buildJoinPacket('Attacker'));
    const attackerJoin = parseJoinResponse(await waitForData(attackerClient));
    defenderClient.write(buildJoinPacket('Defender'));
    const defenderJoin = parseJoinResponse(await waitForData(defenderClient));

    const attacker = world.getPlayer(attackerJoin.id);
    const defender = world.getPlayer(defenderJoin.id);
    attacker.setPosition(1, 1);
    defender.setPosition(2, 1);

    const oldRandom = Math.random;
    Math.random = () => 0.99; // Force attacker to lose all equal-weight rounds
    try {
      attackerClient.write(Buffer.from([0x02, 'r'.charCodeAt(0)]));
      const moveRespRaw = await waitForData(attackerClient);
      const moveResp = parseMoveResponse(moveRespRaw);

      expect(moveResp.type).toBe(0x02);
      expect(moveResp.collision).toBe(1);
      expect(moveResp.loserIdLen).toBeGreaterThan(0);
      expect(moveResp.loserId).toBe(attackerJoin.id);
      expect(world.getPlayer(attackerJoin.id)).toBeNull();

      attackerClient.write(Buffer.from([0x02, 'r'.charCodeAt(0)]));
      await waitForNoData(attackerClient, 200);
    } finally {
      Math.random = oldRandom;
    }
  });

  test('skips unknown packet bytes and still processes following valid packets', async () => {
    const { world, tcpServer, client } = await createServerAndClient();
    sockets.push(client);
    servers.push(tcpServer.server);

    const payload = Buffer.concat([Buffer.from([0x7f]), buildJoinPacket('RecoveryUser')]);
    client.write(payload);

    const joinRespRaw = await waitForData(client);
    const joinResp = parseJoinResponse(joinRespRaw);
    expect(joinResp.type).toBe(0x01);
    expect(joinResp.id).toMatch(/^player_/);
    expect(world.getPlayerCount()).toBe(1);
  });

  test('rejects oversized join name length by closing socket', async () => {
    const { tcpServer, client } = await createServerAndClient();
    sockets.push(client);
    servers.push(tcpServer.server);

    // Declares name length 32 (max allowed is 31)
    client.write(Buffer.from([0x01, 0x20]));

    await expect(Promise.race([
      once(client, 'close').then(() => 'closed'),
      waitForData(client, 250).then(() => 'data')
    ])).resolves.toBe('closed');
  });
});
