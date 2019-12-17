
#ifndef _CUSTOM_EVENTS_H
#define _CUSTOM_EVENTS_H

// Standard includes
#include <ostream>
#include <unordered_map>
// Empirical includes
#include "tools/BitSet.h"
// SignalGP includes
#include "hardware/SignalGP/impls/SignalGPLinearFunctionsProgram.h"

/// Custom Event type!
template<size_t W>
struct Event : public sgp::BaseEvent {
  using tag_t = emp::BitSet<W>;
  tag_t tag;

  Event(size_t _id, tag_t _tag)
    : BaseEvent(_id), tag(_tag) { ; }

  tag_t & GetTag() { return tag; }
  const tag_t & GetTag() const { return tag; }

  void Print(std::ostream & os) const {
    os << "{id:" << GetID() << ",tag:";
    tag.Print(os);
    os << "}";
  }
};

/// Custom Event type!
template<size_t W>
struct MessageEvent : public Event<W> {
  using tag_t = typename Event<W>::tag_t;
  using data_t = std::unordered_map<int, double>;
  data_t data;

  MessageEvent(size_t _id, tag_t _tag, const data_t & _data=data_t())
    : Event<W>(_id, _tag), data(_data) { ; }

  data_t & GetData() { return data; }
  const data_t & GetData() const { return data; }
};

#endif