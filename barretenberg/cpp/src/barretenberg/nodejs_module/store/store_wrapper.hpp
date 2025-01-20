#pragma once

#include "barretenberg/messaging/dispatcher.hpp"
#include "barretenberg/messaging/header.hpp"
#include "barretenberg/nodejs_module/store/store_message.hpp"
#include "barretenberg/nodejs_module/util/message_processor.hpp"
#include <cstdint>
#include <map>
#include <mutex>
#include <napi.h>

namespace bb::nodejs::store {

struct CursorData {
    std::string current;
    bool reverse;
};
/**
 * @brief Manages the interaction between the JavaScript runtime and the LMDB instance.
 */
class StoreWrapper : public Napi::ObjectWrap<StoreWrapper> {
  public:
    StoreWrapper(const Napi::CallbackInfo&);

    /**
     * @brief The only instance method exposed to JavaScript. Takes a msgpack Message and returns a Promise
     */
    Napi::Value call(const Napi::CallbackInfo&);

    static Napi::Function get_class(Napi::Env env);

  private:
    // coarse thread safety for dummy implementation. This will be handled by LMDB
    std::mutex _mutex;

    bb::nodejs::AsyncMessageProcessor _msg_processor;

    std::map<std::string, std::vector<std::byte>> _data;
    std::map<std::string, std::set<std::vector<std::byte>>> _index_data;

    uint64_t _next_cursor = 1;
    std::map<uint64_t, CursorData> _cursors;

    BoolResponse set(const EntryRequest& req);
    GetResponse get(const KeyRequest& req);
    BoolResponse remove(const KeyRequest& req);
    BoolResponse has(const KeyRequest& req);

    CursorStartResponse start_cursor(const CursorStartRequest& req);
    CursorAdvanceResponse advance_cursor(const CursorRequest& req);
    BoolResponse close_cursor(const CursorRequest& req);

    BoolResponse index_add(const EntryRequest& req);
    IndexGetResponse index_get(const KeyRequest& req);
    BoolResponse index_remove(const EntryRequest& req);
    BoolResponse index_remove_key(const KeyRequest& req);
    BoolResponse index_has(const EntryRequest& req);
    BoolResponse index_has_key(const KeyRequest& req);

    IndexCursorAdvanceResponse advance_index_cursor(const CursorRequest& req);
};

} // namespace bb::nodejs::store
