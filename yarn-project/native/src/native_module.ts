import bindings from 'bindings';

import { MessageChannel, MessageReceiver } from './message_channel.js';

export interface NativeClass {
  new (...args: unknown[]): MessageReceiver;
}

const nativeModule: Record<string, NativeClass> = bindings('nodejs_module');

function instantiate(klassName: string, ...args: unknown[]): MessageChannel {
  const inst = new nativeModule[klassName](...args);
  return new MessageChannel(inst);
}

export { type MessageChannel };
export const instantiateWorldState = instantiate.bind(null, 'WorldState');
export const instantiateLmdb = instantiate.bind(null, 'Lmdb');
