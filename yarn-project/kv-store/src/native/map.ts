import { Range } from '../interfaces/common.js';
import { type AztecAsyncMap } from '../interfaces/map.js';

export class DBMap implements AztecAsyncMap<string, Buffer> {
  /**
   * Sets the value at the given key.
   * @param key - The key to set the value at
   * @param val - The value to set
   */
  set(key: string, val: Buffer): Promise<void> {
    return Promise.resolve();
  }

  /**
   * Sets the value at the given key if it does not already exist.
   * @param key - The key to set the value at
   * @param val - The value to set
   */
  setIfNotExists(key: string, val: Buffer): Promise<boolean> {
    return Promise.resolve(false);
  }

  /**
   * Deletes the value at the given key.
   * @param key - The key to delete the value at
   */
  delete(key: string): Promise<void> {
    return Promise.resolve();
  }

  getAsync(key: string): Promise<Buffer> {
    return Promise.resolve(Buffer.from(key));
  }

  hasAsync(key: string): Promise<boolean> {
    return Promise.resolve(false);
  }

  /**
   * Iterates over the map's key-value entries in the key's natural order
   * @param range - The range of keys to iterate over
   */
  async *entriesAsync(_range?: Range<string>): AsyncIterableIterator<[string, Buffer]> {
    const entries: [string, Buffer][] = [];
    for (const entry of entries) {
      yield entry;
    }
  }

  /**
   * Iterates over the map's values in the key's natural order
   * @param range - The range of keys to iterate over
   */
  async *valuesAsync(_range?: Range<string>): AsyncIterableIterator<Buffer> {
    for await (const [_, value] of this.entriesAsync()) {
      yield value;
    }
  }

  /**
   * Iterates over the map's keys in the key's natural order
   * @param range - The range of keys to iterate over
   */
  async *keysAsync(_range?: Range<string>): AsyncIterableIterator<string> {
    for await (const [key, _] of this.entriesAsync()) {
      yield key;
    }
  }
}
