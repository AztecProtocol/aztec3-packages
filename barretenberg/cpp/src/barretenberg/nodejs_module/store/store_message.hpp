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
    HAS,
    REMOVE,

    INDEX_GET,
    INDEX_ADD,
    INDEX_REMOVE,
    INDEX_REMOVE_KEY,
    INDEX_HAS,
    INDEX_HAS_KEY,

    CURSOR_START,
    CURSOR_ADVANCE,
    CURSOR_CLOSE,

    INDEX_CURSOR_ADVANCE,
};

struct KeyRequest {
    std::string key;
    MSGPACK_FIELDS(key);
};

struct GetResponse {
    std::optional<std::vector<std::byte>> value;
    MSGPACK_FIELDS(value);
};

struct EntryRequest {
    std::string key;
    std::vector<std::byte> value;
    MSGPACK_FIELDS(key, value);
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

struct CursorRequest {
    uint64_t cursor;
    MSGPACK_FIELDS(cursor);
};

struct CursorAdvanceResponse {
    std::string key;
    std::vector<std::byte> value;
    bool done;
    MSGPACK_FIELDS(key, value, done);
};

struct IndexGetResponse {
    std::vector<std::vector<std::byte>> values;
    MSGPACK_FIELDS(values);
};

struct IndexCursorAdvanceResponse {
    std::string key;
    std::vector<std::vector<std::byte>> values;
    bool done;
    MSGPACK_FIELDS(key, values, done);
};

struct BoolResponse {
    bool ok;
    MSGPACK_FIELDS(ok);
};

} // namespace bb::nodejs::store

MSGPACK_ADD_ENUM(bb::nodejs::store::StoreMessageType)
