#pragma once
#include "barretenberg/messaging/header.hpp"
#include "barretenberg/serialize/msgpack.hpp"
#include "msgpack/adaptor/define_decl.hpp"
#include <cstdint>
#include <optional>
#include <string>

namespace bb::nodejs::store {

using namespace bb::messaging;

enum StoreMessageType {
    GET = FIRST_APP_MSG_TYPE,
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
};

struct GetRequest {
    std::string key;
    MSGPACK_FIELDS(key);
};

struct GetResponse {
    std::optional<std::vector<std::byte>> value;
    MSGPACK_FIELDS(value);
};

struct SetRequest {
    std::string key;
    std::vector<std::byte> value;
    MSGPACK_FIELDS(key, value);
};

struct RemoveRequest {
    std::string key;
    MSGPACK_FIELDS(key);
};

struct CursorStartRequest {
    std::string key;
    std::optional<bool> reverse;
    MSGPACK_FIELDS(key, reverse);
};

struct CursorStartResponse {
    uint64_t cursor;
    MSGPACK_FIELDS(cursor);
};

struct CursorAdvanceRequest {
    uint64_t cursor;
    MSGPACK_FIELDS(cursor);
};

struct CursorAdvanceResponse {
    std::string key;
    std::vector<std::byte> value;
    bool done;
    MSGPACK_FIELDS(key, value, done);
};

struct CursorCloseRequest {
    uint64_t cursor;
    MSGPACK_FIELDS(cursor);
};

struct IndexGetRequest {
    std::string key;
    MSGPACK_FIELDS(key);
};

struct IndexGetResponse {
    std::vector<std::vector<std::byte>> values;
    MSGPACK_FIELDS(values);
};

struct IndexAddRequest {
    std::string key;
    std::vector<std::byte> value;
    MSGPACK_FIELDS(key, value);
};

struct IndexRemoveRequest {
    std::string key;
    std::vector<std::byte> value;
    MSGPACK_FIELDS(key, value);
};

struct IndexRemoveKeyRequest {
    std::string key;
    MSGPACK_FIELDS(key);
};

struct IndexCursorAdvanceRequest {
    uint64_t cursor;
    MSGPACK_FIELDS(cursor);
};

struct IndexCursorAdvanceResponse {
    std::string key;
    std::vector<std::vector<std::byte>> values;
    bool done;
    MSGPACK_FIELDS(key, values, done);
};

struct EmptyResponse {
    bool ok;
    MSGPACK_FIELDS(ok);
};

} // namespace bb::nodejs::store

MSGPACK_ADD_ENUM(bb::nodejs::store::StoreMessageType)
