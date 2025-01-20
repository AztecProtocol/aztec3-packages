import { expect } from 'chai';

import { Database } from './database.js';

describe.only('Database', () => {
  let db: Database;

  beforeEach(async () => {
    db = await Database.new();
  });

  it('returns undefined for unknown keys', async () => {
    expect(await db.get('foo')).to.be.undefined;
    expect((await db.getIndex('foo')).length).to.eq(0);
  });

  it('gets & sets', async () => {
    await db.set('foo', Buffer.from('bar'));
    expect((await db.get('foo'))!.toString('utf-8')).to.eq('bar');
  });

  it('gets & sets index', async () => {
    await db.addToIndex('foo', Buffer.from('bar'));
    await db.addToIndex('foo', Buffer.from('baz'));
    expect((await db.getIndex('foo'))!.map(x => x.toString('utf-8'))).to.deep.eq(['bar', 'baz']);
  });

  it('sets index twice', async () => {
    await db.addToIndex('foo', Buffer.from('bar'));
    await db.addToIndex('foo', Buffer.from('bar'));
    expect((await db.getIndex('foo'))!.map(x => x.toString('utf-8'))).to.deep.eq(['bar']);
  });

  it('iterates over keys', async () => {
    await db.set('1', Buffer.from('foo'));
    await db.set('2', Buffer.from('bar'));
    await db.set('3', Buffer.from('baz'));

    const entries: any[] = [];
    for await (const [key, value] of db.iterate('1')) {
      entries.push([key, value.toString('utf-8')]);
    }

    expect(entries).to.deep.equal([
      ['1', 'foo'],
      ['2', 'bar'],
      ['3', 'baz'],
    ]);
  });

  it('iterates over index', async () => {
    await db.addToIndex('1', Buffer.from('foo'));
    await db.addToIndex('1', Buffer.from('bar'));
    await db.addToIndex('1', Buffer.from('baz'));
    await db.addToIndex('2', Buffer.from('quux'));

    const entries: any[] = [];
    for await (const [key, value] of db.iterateIndex('1')) {
      entries.push([key, value.toString('utf-8')]);
    }

    expect(entries).to.deep.equal([
      ['1', 'bar'],
      ['1', 'baz'],
      ['1', 'foo'],
      ['2', 'quux'],
    ]);
  });
});
