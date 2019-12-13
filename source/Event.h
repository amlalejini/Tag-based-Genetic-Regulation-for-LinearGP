
#ifndef _CUSTOM_EVENTS_H
#define _CUSTOM_EVENTS_H

// Standard includes
#include <ostream>
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

#endif