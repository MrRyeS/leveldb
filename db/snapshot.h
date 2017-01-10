// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef STORAGE_LEVELDB_DB_SNAPSHOT_H_
#define STORAGE_LEVELDB_DB_SNAPSHOT_H_

#include "db/dbformat.h"
#include "leveldb/db.h"

namespace leveldb {

class SnapshotList;

// Snapshot 存储的是immutable数据，对外提供的访问方法是线程安全的
//
// leveldb 实现快照的原理关键就在于那个sequence number。每当插入一条记录时，
// 都会插入一个独一无二的序列号，而且这个序列号是递增的。所以当插入两条记录
// 的键值一样时，只能通过序列号来区分哪条记录是最新的，因为系统返回的是最新
// 的。而快照SnapShot类的实现原理就是，当调用函数获取一个快照时，就获取目前
// 的sequence number，当读取数据时，只读取小于等于这个序列号的记录，这样就可
// 以读取这个快照时间点之前的数据了。
//
// Snapshots are kept in a doubly-linked list in the DB.
// Each SnapshotImpl corresponds to a particular sequence number.
class SnapshotImpl : public Snapshot {
 public:
  SequenceNumber number_;  // const after creation

 private:
  friend class SnapshotList;

  // SnapshotImpl is kept in a doubly-linked circular list
  SnapshotImpl* prev_;
  SnapshotImpl* next_;

  SnapshotList* list_;                 // just for sanity checks
};

// DB中持有全部Snapshot的链表.
// DBImpl::GetSnapshot()时在链表中加入一个节点
// DBImpl::ReleaseSnapshot()时释放
class SnapshotList {
 public:
  SnapshotList() {
    list_.prev_ = &list_;
    list_.next_ = &list_;
  }

  bool empty() const { return list_.next_ == &list_; }
  SnapshotImpl* oldest() const { assert(!empty()); return list_.next_; }
  SnapshotImpl* newest() const { assert(!empty()); return list_.prev_; }

  const SnapshotImpl* New(SequenceNumber seq) {
    SnapshotImpl* s = new SnapshotImpl;
    s->number_ = seq;
    s->list_ = this;
    s->next_ = &list_;
    s->prev_ = list_.prev_;
    s->prev_->next_ = s;
    s->next_->prev_ = s;
    return s;
  }

  void Delete(const SnapshotImpl* s) {
    assert(s->list_ == this);
    s->prev_->next_ = s->next_;
    s->next_->prev_ = s->prev_;
    delete s;
  }

 private:
  // Dummy head of doubly-linked list of snapshots
  SnapshotImpl list_;
};

}  // namespace leveldb

#endif  // STORAGE_LEVELDB_DB_SNAPSHOT_H_
