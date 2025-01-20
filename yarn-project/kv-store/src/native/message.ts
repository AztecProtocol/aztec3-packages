export enum DatabaseMessageType {
  GET = 100,
  SET,
  REMOVE,
  HAS,
  BATCH,

  INDEX_GET,
  INDEX_ADD,
  INDEX_REMOVE,
  INDEX_REMOVE_KEY,
  INDEX_HAS,
  INDEX_HAS_KEY,
  INDEX_BATCH,

  CURSOR_START,
  CURSOR_ADVANCE,
  CURSOR_CLOSE,

  INDEX_CURSOR_ADVANCE,
}

interface JustKeyRequest {
  key: string;
}

interface EntryRequest {
  key: string;
  value: Buffer;
}

interface CursorStartRequest {
  key: string;
  reverse: boolean;
}

interface JustCursorRequest {
  cursor: number;
}

interface BatchRequest {
  set: Map<string, Buffer>;
  remove: string[];
}

interface IndexBatchRequest {
  add: Map<string, Buffer[]>;
  remove: Map<string, Buffer[]>;
  removeKey: string[];
}

export type DatabaseRequest = {
  [DatabaseMessageType.GET]: JustKeyRequest;
  [DatabaseMessageType.SET]: EntryRequest;
  [DatabaseMessageType.REMOVE]: JustKeyRequest;
  [DatabaseMessageType.HAS]: JustKeyRequest;
  [DatabaseMessageType.BATCH]: BatchRequest;

  [DatabaseMessageType.INDEX_GET]: JustKeyRequest;
  [DatabaseMessageType.INDEX_ADD]: EntryRequest;
  [DatabaseMessageType.INDEX_REMOVE]: EntryRequest;
  [DatabaseMessageType.INDEX_REMOVE_KEY]: JustKeyRequest;
  [DatabaseMessageType.INDEX_HAS]: EntryRequest;
  [DatabaseMessageType.INDEX_HAS_KEY]: JustKeyRequest;
  [DatabaseMessageType.INDEX_BATCH]: IndexBatchRequest;

  [DatabaseMessageType.CURSOR_START]: CursorStartRequest;
  [DatabaseMessageType.CURSOR_ADVANCE]: JustCursorRequest;
  [DatabaseMessageType.CURSOR_CLOSE]: JustCursorRequest;
  [DatabaseMessageType.INDEX_CURSOR_ADVANCE]: JustCursorRequest;
};

interface GetResponse {
  value: Buffer | null;
}

interface IndexGetResponse {
  values: Array<Buffer>;
}

interface CursorResponse {
  cursor: number;
}

interface CursorAdvanceResponse {
  key: string;
  value: Buffer;
  done: boolean;
}

interface IndexCursorAdvanceResponse {
  key: string;
  values: Array<Buffer>;
  done: boolean;
}

interface BoolResponse {
  ok: true;
}

export type DatabaseResponse = {
  [DatabaseMessageType.GET]: GetResponse;
  [DatabaseMessageType.SET]: BoolResponse;
  [DatabaseMessageType.REMOVE]: BoolResponse;
  [DatabaseMessageType.HAS]: BoolResponse;
  [DatabaseMessageType.BATCH]: BoolResponse;

  [DatabaseMessageType.INDEX_GET]: IndexGetResponse;
  [DatabaseMessageType.INDEX_ADD]: BoolResponse;
  [DatabaseMessageType.INDEX_REMOVE]: BoolResponse;
  [DatabaseMessageType.INDEX_REMOVE_KEY]: BoolResponse;
  [DatabaseMessageType.INDEX_HAS]: BoolResponse;
  [DatabaseMessageType.INDEX_HAS_KEY]: BoolResponse;
  [DatabaseMessageType.INDEX_BATCH]: BoolResponse;

  [DatabaseMessageType.CURSOR_START]: CursorResponse;
  [DatabaseMessageType.CURSOR_ADVANCE]: CursorAdvanceResponse;
  [DatabaseMessageType.CURSOR_CLOSE]: BoolResponse;

  [DatabaseMessageType.INDEX_CURSOR_ADVANCE]: IndexCursorAdvanceResponse;
};
