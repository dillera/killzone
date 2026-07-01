/**
 * Minimal dependency-free logger.
 *
 * Levels (severity order, most-to-least severe): error, warn, info, debug.
 * Current level is read from process.env.LOG_LEVEL (case-insensitive),
 * default 'info'. Messages below the current level are suppressed.
 *
 * Writes error/warn to stderr, info/debug to stdout, via process.std*.write
 * so output is line-oriented and flushes promptly under journald.
 */

const LEVELS = { error: 0, warn: 1, info: 2, debug: 3 };

function normalizeLevel(name) {
    if (typeof name !== 'string') return null;
    const lower = name.toLowerCase();
    return Object.prototype.hasOwnProperty.call(LEVELS, lower) ? lower : null;
}

let currentLevel = normalizeLevel(process.env.LOG_LEVEL) || 'info';

function setLevel(name) {
    const normalized = normalizeLevel(name);
    if (normalized) {
        currentLevel = normalized;
    }
}

function getLevel() {
    return currentLevel;
}

function write(level, msg) {
    try {
        if (LEVELS[level] > LEVELS[currentLevel]) {
            return;
        }
        const timestamp = new Date().toISOString();
        const line = `${timestamp} [${level.toUpperCase()}] ${msg}`;
        if (level === 'error' || level === 'warn') {
            process.stderr.write(line + '\n');
        } else {
            process.stdout.write(line + '\n');
        }
    } catch (e) {
        // Never let logging crash the server.
    }
}

module.exports = {
    error: (msg) => write('error', msg),
    warn: (msg) => write('warn', msg),
    info: (msg) => write('info', msg),
    debug: (msg) => write('debug', msg),
    setLevel,
    getLevel,
};
