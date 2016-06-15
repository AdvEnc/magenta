// Copyright 2016 The Fuchsia Authors
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

#pragma once

#include <stdint.h>

#include <kernel/mutex.h>

#include <utils/array.h>
#include <utils/intrusive_double_list.h>
#include <utils/ref_counted.h>

#include <magenta/waiter.h>

class Handle;

struct MessagePacket {
    MessagePacket* next;
    MessagePacket* prev;
    utils::Array<uint8_t> data;
    utils::Array<Handle*> handles;

    ~MessagePacket();

    void ReturnHandles();

    MessagePacket* list_prev() {
        return prev;
    }
    const MessagePacket* list_prev() const {
        return prev;
    }
    MessagePacket* list_next() {
        return next;
    }
    const MessagePacket* list_next() const {
        return next;
    }
    void list_set_prev(MessagePacket* prv) {
        prev = prv;
    }
    void list_set_next(MessagePacket* nxt) {
        next = nxt;
    }
};

class MessagePipe : public utils::RefCounted<MessagePipe> {
public:
    MessagePipe();
    ~MessagePipe();

    void OnDispatcherDestruction(size_t side);

    status_t Read(size_t side, utils::unique_ptr<MessagePacket>* msg);
    status_t Write(size_t side, utils::unique_ptr<MessagePacket> msg);

    Waiter* GetWaiter(size_t side);

private:
    bool dispatcher_alive_[2];
    utils::DoublyLinkedList<MessagePacket> messages_[2];
    // This lock protects |dispatcher_alive_| and |messages_|.
    mutex_t lock_;
    Waiter waiter_[2];
};