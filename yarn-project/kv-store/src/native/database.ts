import { MessageHeader, TypedMessage } from '@aztec/foundation/message';
import { MessageChannel, instantiateLmdb } from '@aztec/native';

import { DatabaseMessageType, DatabaseRequest, DatabaseResponse } from './message.js';

export class Database {
  private channel: MessageChannel;
  private id = 1;
  private constructor() {
    this.channel = instantiateLmdb();
  }

  public async new() {
    const db = new Database();
    await db.init();
    return db;
  }

  public async get(dbName: string, key: string): Promise<Buffer | undefined> {
    const response = await this.sendMessage(DatabaseMessageType.GET, { dbName, key });
    return response.value;
  }

  public async set(dbName: string, key: string, value: Buffer): Promise<void> {
    await this.sendMessage(DatabaseMessageType.SET, { dbName, key, value });
  }

  private async init() {
    await this.sendMessage(DatabaseMessageType.OPEN_DATABASE, { dbName: 'data' });
    await this.sendMessage(DatabaseMessageType.OPEN_DATABASE, { dbName: 'index' });
  }

  private async sendMessage<T extends DatabaseMessageType>(
    messageType: T,
    requestBody: DatabaseRequest[T],
  ): Promise<DatabaseResponse[T]> {
    const messageId = this.id++;
    const msg = new TypedMessage(messageType, new MessageHeader({ messageId }), requestBody);
    const { response } = await this.channel.sendMessage<T, DatabaseRequest[T], DatabaseResponse[T]>(msg);
    return response.value;
  }
}
