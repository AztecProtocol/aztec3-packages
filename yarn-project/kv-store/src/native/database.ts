import { MessageHeader, TypedMessage } from '@aztec/foundation/message';
import { MessageChannel, instantiateStore } from '@aztec/native';

import { DatabaseMessageType, DatabaseRequest, DatabaseResponse } from './message.js';

class TypeSafeMessageChannel {
  private id = 1;

  constructor(private channel: MessageChannel) {}

  public async sendMessage<T extends DatabaseMessageType>(
    messageType: T,
    requestBody: DatabaseRequest[T],
  ): Promise<DatabaseResponse[T]> {
    const messageId = this.id++;
    const msg = new TypedMessage(messageType, new MessageHeader({ messageId }), requestBody);
    const { response } = await this.channel.sendMessage<T, DatabaseRequest[T], DatabaseResponse[T]>(msg);
    return response.value;
  }
}

export class Database {
  private channel: TypeSafeMessageChannel;
  private constructor() {
    this.channel = new TypeSafeMessageChannel(instantiateStore(''));
  }

  public static async new() {
    const db = new Database();
    return db;
  }

  public async get(key: string): Promise<Buffer | undefined> {
    const response = await this.channel.sendMessage(DatabaseMessageType.GET, { key });
    return response.value ?? undefined;
  }

  public async has(key: string): Promise<boolean> {
    const response = await this.channel.sendMessage(DatabaseMessageType.HAS, { key });
    return response.ok;
  }

  public async set(key: string, value: Buffer): Promise<void> {
    await this.channel.sendMessage(DatabaseMessageType.SET, { key, value });
  }

  public async remove(key: string): Promise<void> {
    await this.channel.sendMessage(DatabaseMessageType.REMOVE, { key });
  }

  public async getIndex(key: string): Promise<Array<Buffer>> {
    const response = await this.channel.sendMessage(DatabaseMessageType.INDEX_GET, { key });
    return response.values;
  }

  public async addToIndex(key: string, value: Buffer): Promise<void> {
    await this.channel.sendMessage(DatabaseMessageType.INDEX_ADD, { key, value });
  }

  public async removeFromIndex(key: string, value: Buffer): Promise<void> {
    await this.channel.sendMessage(DatabaseMessageType.INDEX_REMOVE, { key, value });
  }

  public async removeKeyFromIndex(key: string): Promise<void> {
    await this.channel.sendMessage(DatabaseMessageType.INDEX_REMOVE_KEY, { key });
  }

  public async indexHasEntry(key: string, value: Buffer): Promise<boolean> {
    const response = await this.channel.sendMessage(DatabaseMessageType.INDEX_HAS, { key, value });
    return response.ok;
  }

  public async indexHasKey(key: string): Promise<boolean> {
    const response = await this.channel.sendMessage(DatabaseMessageType.INDEX_HAS_KEY, { key });
    return response.ok;
  }

  public iterate(key: string, reverse = false): AsyncIterable<[string, Buffer]> {
    return new StoreIterator(this.channel, key, reverse);
  }

  public iterateIndex(key: string, reverse = false): AsyncIterable<[string, Buffer]> {
    return new IndexIterator(this.channel, key, reverse);
  }
}

class StoreIterator implements AsyncIterable<[string, Buffer]> {
  constructor(private channel: TypeSafeMessageChannel, private key: string, private reverse: boolean) {}

  async *[Symbol.asyncIterator](): AsyncIterator<[string, Buffer]> {
    const { cursor } = await this.channel.sendMessage(DatabaseMessageType.CURSOR_START, {
      key: this.key,
      reverse: this.reverse,
    });

    let done = false;
    try {
      do {
        const it = await this.channel.sendMessage(DatabaseMessageType.CURSOR_ADVANCE, { cursor });
        done = it.done;
        yield [it.key, it.value];
      } while (!done);
    } finally {
      this.channel.sendMessage(DatabaseMessageType.CURSOR_CLOSE, { cursor });
    }
  }
}

class IndexIterator implements AsyncIterable<[string, Buffer]> {
  constructor(private channel: TypeSafeMessageChannel, private key: string, private reverse: boolean) {}

  async *[Symbol.asyncIterator](): AsyncIterator<[string, Buffer]> {
    const { cursor } = await this.channel.sendMessage(DatabaseMessageType.CURSOR_START, {
      key: this.key,
      reverse: this.reverse,
    });

    let done = false;
    try {
      do {
        const it = await this.channel.sendMessage(DatabaseMessageType.INDEX_CURSOR_ADVANCE, { cursor });
        done = it.done;

        if (this.reverse) {
          it.values.reverse();
        }

        for (const value of it.values) {
          yield [it.key, value];
        }
      } while (!done);
    } finally {
      this.channel.sendMessage(DatabaseMessageType.CURSOR_CLOSE, { cursor });
    }
  }
}
