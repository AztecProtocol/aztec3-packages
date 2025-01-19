#include "barretenberg/nodejs_module/store/store_wrapper.hpp"
#include "barretenberg/nodejs_module/store/store_message.hpp"
#include "napi.h"
#include <iterator>
#include <stdexcept>

using namespace bb::nodejs;
using namespace bb::nodejs::store;

StoreWrapper::StoreWrapper(const Napi::CallbackInfo& info)
    : ObjectWrap(info)
{
    Napi::Env env = info.Env();

    size_t data_dir_index = 0;
    std::string data_dir;
    if (info.Length() > data_dir_index && info[data_dir_index].IsString()) {
        data_dir = info[data_dir_index].As<Napi::String>();
    } else {
        throw Napi::TypeError::New(env, "Directory needs to be a string");
    }

    _msg_processor.register_handler(StoreMessageType::SET, this, &StoreWrapper::set);
    _msg_processor.register_handler(StoreMessageType::REMOVE, this, &StoreWrapper::remove);
    _msg_processor.register_handler(StoreMessageType::GET, this, &StoreWrapper::get);

    _msg_processor.register_handler(StoreMessageType::CURSOR_START, this, &StoreWrapper::start_cursor);
    _msg_processor.register_handler(StoreMessageType::CURSOR_ADVANCE, this, &StoreWrapper::advance_cursor);
    _msg_processor.register_handler(StoreMessageType::CURSOR_CLOSE, this, &StoreWrapper::close_cursor);

    _msg_processor.register_handler(StoreMessageType::INDEX_ADD, this, &StoreWrapper::index_add);
    _msg_processor.register_handler(StoreMessageType::INDEX_REMOVE, this, &StoreWrapper::index_remove);
    _msg_processor.register_handler(StoreMessageType::INDEX_REMOVE_KEY, this, &StoreWrapper::index_remove_key);
    _msg_processor.register_handler(StoreMessageType::INDEX_GET, this, &StoreWrapper::index_get);

    _msg_processor.register_handler(StoreMessageType::INDEX_CURSOR_ADVANCE, this, &StoreWrapper::advance_index_cursor);
}

Napi::Value StoreWrapper::call(const Napi::CallbackInfo& info)
{
    return _msg_processor.process_message(info);
}

Napi::Function StoreWrapper::get_class(Napi::Env env)
{
    return DefineClass(env,
                       "Store",
                       {
                           StoreWrapper::InstanceMethod("call", &StoreWrapper::call),
                       });
}

EmptyResponse StoreWrapper::set(const SetRequest& req)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _data[req.key] = req.value;
    return { true };
}

EmptyResponse StoreWrapper::remove(const RemoveRequest& req)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _data.erase(req.key);
    return { true };
}

GetResponse StoreWrapper::get(const GetRequest& req)
{
    std::lock_guard<std::mutex> lock(_mutex);
    auto it = _data.find(req.key);
    if (it == _data.end()) {
        return { std::nullopt };
    }
    return { (*it).second };
}

CursorStartResponse StoreWrapper::start_cursor(const CursorStartRequest& req)
{
    std::lock_guard<std::mutex> lock(_mutex);
    uint64_t cursor = _next_cursor++;
    _cursors[cursor] = { req.key, req.reverse.value_or(false) };
    return { cursor };
}

EmptyResponse StoreWrapper::close_cursor(const CursorCloseRequest& req)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _cursors.erase(req.cursor);
    return { true };
}

CursorAdvanceResponse StoreWrapper::advance_cursor(const CursorAdvanceRequest& req)
{
    std::lock_guard<std::mutex> lock(_mutex);
    auto it = _cursors.find(req.cursor);
    if (it == _cursors.end()) {
        throw std::runtime_error("Cursor does not exist");
    }

    auto& cursor = (*it).second;

    std::string key = cursor.current;
    auto data_it = _data.find(key);
    if (data_it == _data.end()) {
        throw std::runtime_error("Data does not exist");
    }
    std::vector<std::byte> value = (*data_it).second;
    bool done = false;

    std::string next;
    if (cursor.reverse) {
        data_it--;
    } else {
        data_it++;
    }
    if (data_it == _data.end()) {
        done = true;
    } else {
        next = (*data_it).first;
    }

    cursor.current = next;

    return { key, value, done };
}

EmptyResponse StoreWrapper::index_add(const IndexAddRequest& req)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _index_data[req.key].insert(req.value);
    return { true };
}

EmptyResponse StoreWrapper::index_remove(const IndexRemoveRequest& req)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _index_data[req.key].erase(req.value);
    return { true };
}

EmptyResponse StoreWrapper::index_remove_key(const IndexRemoveKeyRequest& req)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _index_data.erase(req.key);
    return { true };
}

IndexGetResponse StoreWrapper::index_get(const IndexGetRequest& req)
{
    std::lock_guard<std::mutex> lock(_mutex);
    std::vector<std::vector<std::byte>> values;
    std::copy(_index_data[req.key].begin(), _index_data[req.key].end(), std::back_inserter(values));
    return { values };
}

IndexCursorAdvanceResponse StoreWrapper::advance_index_cursor(const IndexCursorAdvanceRequest& req)
{
    std::lock_guard<std::mutex> lock(_mutex);
    auto it = _cursors.find(req.cursor);
    if (it == _cursors.end()) {
        throw std::runtime_error("Cursor does not exist");
    }

    auto& cursor = (*it).second;

    std::string key = cursor.current;
    auto data_it = _index_data.find(key);
    if (data_it == _index_data.end()) {
        throw std::runtime_error("Data does not exist");
    }
    std::vector<std::vector<std::byte>> values;
    std::copy((*data_it).second.begin(), (*data_it).second.end(), std::back_inserter(values));
    bool done = false;

    std::string next;
    if (cursor.reverse) {
        data_it--;
    } else {
        data_it++;
    }

    if (data_it == _index_data.end()) {
        done = true;
    } else {
        next = (*data_it).first;
    }

    cursor.current = next;

    return { key, values, done };
}
