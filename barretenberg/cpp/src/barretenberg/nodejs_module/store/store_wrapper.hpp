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

    EmptyResponse set(const SetRequest& req);
    GetResponse get(const GetRequest& req);
    EmptyResponse remove(const RemoveRequest& req);

    CursorStartResponse start_cursor(const CursorStartRequest& req);
    CursorAdvanceResponse advance_cursor(const CursorAdvanceRequest& req);
    EmptyResponse close_cursor(const CursorCloseRequest& req);

    EmptyResponse index_add(const IndexAddRequest& req);
    IndexGetResponse index_get(const IndexGetRequest& req);
    EmptyResponse index_remove(const IndexRemoveRequest& req);
    EmptyResponse index_remove_key(const IndexRemoveKeyRequest& req);

    IndexCursorAdvanceResponse advance_index_cursor(const IndexCursorAdvanceRequest& req);
};

} // namespace bb::nodejs::store
