#pragma once
#include <cstdint>

// The abstract input vocabulary the app understands (ADR-0001). Every board's input
// driver, whatever its hardware, lowers its raw signals to these events; the single
// search-and-browse UI consumes only these, so a keyboard board gets type-to-search and a
// button/encoder board gets scroll-to-browse with no separate UI code path.
//
// This header is pure: no Arduino, no hardware, host-compilable and host-tested. The
// device drivers that PRODUCE these events (keyboard, button/encoder) are separate,
// device-only translation units (see LEDGER); they depend on this type, not the reverse.

namespace tp {

enum class InputKind : uint8_t {
  None,    // nothing pending this poll (see InputSource::poll)
  Char,    // a character was typed; `ch` carries it
  Nav,     // a browse step; `nav` carries the direction
  Select,  // commit/confirm (open the focused result, accept a letter)
  Back,    // cancel/ascend (clear a letter, leave the detail view)
};

enum class NavDir : uint8_t { Up, Down };

// One input event. Only the member named by `kind` is meaningful: `ch` for Char, `nav`
// for Nav, neither for None/Select/Back. Construct through the named factories below so an
// event is always well-formed (e.g. a Nav can never exist without a direction); the
// members are left public for cheap value copies, but direct aggregate init is a smell.
//
// `ch` is a single byte on purpose: the reference keyboards (and the Latin-only language
// set, ADR-0001) are ASCII, and the query path folds diacritics away anyway (ADR-0002), so
// the user types "agua" to find "água". A board that can emit multi-byte characters is a
// future extension, recorded as a decision then — not speculative generality now (KISS).
struct InputEvent {
  InputKind kind = InputKind::None;
  char ch = '\0';
  NavDir nav = NavDir::Up;

  static constexpr InputEvent none() { return {}; }
  static constexpr InputEvent character(char typed) { return {InputKind::Char, typed, NavDir::Up}; }
  static constexpr InputEvent navigate(NavDir dir) { return {InputKind::Nav, '\0', dir}; }
  static constexpr InputEvent select() { return {InputKind::Select, '\0', NavDir::Up}; }
  static constexpr InputEvent back() { return {InputKind::Back, '\0', NavDir::Up}; }

  constexpr bool isNone() const { return kind == InputKind::None; }
};

}  // namespace tp
