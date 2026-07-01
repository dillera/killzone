describe('logger', () => {
  let stdoutWriteSpy;
  let stderrWriteSpy;

  beforeEach(() => {
    jest.resetModules();
    stdoutWriteSpy = jest.spyOn(process.stdout, 'write').mockImplementation(() => true);
    stderrWriteSpy = jest.spyOn(process.stderr, 'write').mockImplementation(() => true);
  });

  afterEach(() => {
    stdoutWriteSpy.mockRestore();
    stderrWriteSpy.mockRestore();
    delete process.env.LOG_LEVEL;
  });

  test('defaults to info level', () => {
    const logger = require('../src/logger');
    expect(logger.getLevel()).toBe('info');
  });

  test('setLevel changes the current level (case-insensitive)', () => {
    const logger = require('../src/logger');
    logger.setLevel('DEBUG');
    expect(logger.getLevel()).toBe('debug');
    logger.setLevel('warn');
    expect(logger.getLevel()).toBe('warn');
  });

  test('setLevel ignores invalid level names', () => {
    const logger = require('../src/logger');
    logger.setLevel('info');
    logger.setLevel('not-a-level');
    expect(logger.getLevel()).toBe('info');
  });

  test('at warn level, info and debug are suppressed but warn/error are written', () => {
    const logger = require('../src/logger');
    logger.setLevel('warn');

    logger.debug('debug message');
    logger.info('info message');
    logger.warn('warn message');
    logger.error('error message');

    expect(stdoutWriteSpy).not.toHaveBeenCalled();
    expect(stderrWriteSpy).toHaveBeenCalledTimes(2);
    expect(stderrWriteSpy.mock.calls[0][0]).toContain('[WARN] warn message');
    expect(stderrWriteSpy.mock.calls[1][0]).toContain('[ERROR] error message');
  });

  test('at debug level, all messages are written to the correct stream', () => {
    const logger = require('../src/logger');
    logger.setLevel('debug');

    logger.debug('d');
    logger.info('i');
    logger.warn('w');
    logger.error('e');

    expect(stdoutWriteSpy).toHaveBeenCalledTimes(2); // debug + info
    expect(stderrWriteSpy).toHaveBeenCalledTimes(2); // warn + error
  });

  test('log lines include an ISO-8601 timestamp and level tag', () => {
    const logger = require('../src/logger');
    logger.setLevel('info');
    logger.info('hello world');

    expect(stdoutWriteSpy).toHaveBeenCalledTimes(1);
    const line = stdoutWriteSpy.mock.calls[0][0];
    expect(line).toMatch(/^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}\.\d{3}Z \[INFO\] hello world\n$/);
  });

  test('never throws even if something goes wrong while writing', () => {
    const logger = require('../src/logger');
    stdoutWriteSpy.mockImplementation(() => {
      throw new Error('boom');
    });
    stderrWriteSpy.mockImplementation(() => {
      throw new Error('boom');
    });

    expect(() => logger.info('should not throw')).not.toThrow();
    expect(() => logger.error('should not throw')).not.toThrow();
    expect(() => logger.warn('should not throw')).not.toThrow();
    expect(() => logger.debug('should not throw')).not.toThrow();
  });

  test('LOG_LEVEL env var (case-insensitive) sets the initial level', () => {
    process.env.LOG_LEVEL = 'ERROR';
    const logger = require('../src/logger');
    expect(logger.getLevel()).toBe('error');
  });
});
