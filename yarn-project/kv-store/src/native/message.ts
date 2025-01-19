export enum DatabaseMessageType {
  OPEN_DATABASE = 100,
  GET,
  SET,
  CLOSE_DATABASE,
}

interface WithDatabaseName {
  dbName: string;
}

interface GetKeyRequest extends WithDatabaseName {
  key: string;
}

interface GetKeyResponse {
  value: Buffer | undefined;
}

interface SetKeyRequest extends WithDatabaseName {
  key: string;
  value: Buffer;
}

interface EmptyResponse {
  ok: true;
}

export type DatabaseRequest = {
  [DatabaseMessageType.OPEN_DATABASE]: WithDatabaseName;

  [DatabaseMessageType.GET]: GetKeyRequest;
  [DatabaseMessageType.SET]: SetKeyRequest;

  [DatabaseMessageType.CLOSE_DATABASE]: WithDatabaseName;
};

export type DatabaseResponse = {
  [DatabaseMessageType.OPEN_DATABASE]: EmptyResponse;

  [DatabaseMessageType.GET]: GetKeyResponse;
  [DatabaseMessageType.SET]: EmptyResponse;

  [DatabaseMessageType.CLOSE_DATABASE]: EmptyResponse;
};
