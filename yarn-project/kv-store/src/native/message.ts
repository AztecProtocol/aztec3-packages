export enum DatabaseMessageType {
  GET = 100,
  SET,
  REMOVE,

  INDEX_GET,
  INDEX_ADD,
  INDEX_REMOVE,
  INDEX_REMOVE_KEY,

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

export type DatabaseRequest = {
  [DatabaseMessageType.GET]: JustKeyRequest;
  [DatabaseMessageType.SET]: EntryRequest;
  [DatabaseMessageType.REMOVE]: JustKeyRequest;

  [DatabaseMessageType.INDEX_GET]: JustKeyRequest;
  [DatabaseMessageType.INDEX_ADD]: EntryRequest;
  [DatabaseMessageType.INDEX_REMOVE]: EntryRequest;
  [DatabaseMessageType.INDEX_REMOVE_KEY]: JustKeyRequest;

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

interface EmptyResponse {
  ok: true;
}

export type DatabaseResponse = {
  [DatabaseMessageType.GET]: GetResponse;
  [DatabaseMessageType.SET]: EmptyResponse;
  [DatabaseMessageType.REMOVE]: EmptyResponse;

  [DatabaseMessageType.INDEX_GET]: IndexGetResponse;
  [DatabaseMessageType.INDEX_ADD]: EmptyResponse;
  [DatabaseMessageType.INDEX_REMOVE]: EmptyResponse;
  [DatabaseMessageType.INDEX_REMOVE_KEY]: EmptyResponse;

  [DatabaseMessageType.CURSOR_START]: CursorResponse;
  [DatabaseMessageType.CURSOR_ADVANCE]: CursorAdvanceResponse;
  [DatabaseMessageType.CURSOR_CLOSE]: EmptyResponse;

  [DatabaseMessageType.INDEX_CURSOR_ADVANCE]: IndexCursorAdvanceResponse;
};
