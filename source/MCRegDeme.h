#ifndef _MC_REG_DEME_H
#define _MC_REG_DEME_H

// Empirical includes
#include "tools/math.h"
// SignalGP includes
#include "hardware/SignalGP/impls/SignalGPLinearFunctionsProgram.h"

/// Base class for toroidal demes!
/// - Note, this base class implements neighbor lookups, but does not manage the deme cells (that's
///   for derived classes).
template<typename HW_MEMORY_MODEL_T,
         typename HW_TAG_T,
         typename HW_INST_ARG_T,
         typename HW_MATCHBIN_T>
class MCRegDeme {

public:

  enum Facing { N=0, NE=1, E=2, SE=3, S=4, SW=5, W=6, NW=7 };                   ///< All possible directions
  static constexpr Facing Dir[] {Facing::N, Facing::NE, Facing::E, Facing::SE,  ///< Array of possible directions
                                Facing::S, Facing::SW, Facing::W, Facing::NW};
  static constexpr size_t NUM_DIRECTIONS = 8;                                   ///< Number of neighbors each board space has.

  /// Custom hardware component for SignalGP
  struct CellHardware {
    size_t cell_id=0;
    bool active=false;
    int response=-1;
    Facing cell_facing=Facing::N;

    /// Reset everything except cell id
    void Reset() {
      response=-1;
      active=false;
      cell_facing=Facing::N;
    }

    size_t GetCellID() const { return cell_id; }
    void SetCellID(size_t id) { cell_id = id; }

    bool IsActive() const { return active; }

    Facing GetFacing() const { return cell_facing; }
    void SetFacing(Facing facing) { cell_facing = facing; }

    /// Rotate cell facing clockwise
    Facing RotateCW(int rot=1) {
      const int cur_dir = (int)cell_facing;
      const size_t new_dir = (size_t)emp::Mod(cur_dir+rot, (int)MCRegDeme::NUM_DIRECTIONS);
      emp_assert(new_dir < MCRegDeme::NUM_DIRECTIONS);
      cell_facing = MCRegDeme::Dir[new_dir];
    }

    /// Rotate cell facing counter-clockwise
    Facing RotateCCW(int rot=1) {
      const int cur_dir = (int)cell_facing;
      const size_t new_dir = (size_t)emp::Mod(cur_dir-rot, (int)MCRegDeme::NUM_DIRECTIONS);
      emp_assert(new_dir < MCRegDeme::NUM_DIRECTIONS);
      cell_facing = MCRegDeme::Dir[new_dir];
    }
  };

  using hardware_t = sgp::LinearFunctionsProgramSignalGP<HW_MEMORY_MODEL_T,
                                                         HW_INST_ARG_T,
                                                         HW_TAG_T,
                                                         HW_MATCHBIN_T,
                                                         CellHardware>;
  using event_lib_t = typename hardware_t::event_lib_t;
  using inst_lib_t = typename hardware_t::inst_lib_t;

protected:
  size_t width;   ///< Width of deme grid
  size_t height;  ///< Height of deme grid
  emp::Random & random;
  emp::vector<size_t> neighbor_lookup; ///< Lookup table for neighbors
  emp::vector<hardware_t> cells;       ///< Toroidal grid of hardware units
  emp::vector<size_t> cell_schedule;   ///< Order to execute cells

  /// Build neighbor lookup (according to current width and height)
  void BuildNeighborLookup();

  /// Calculate the neighbor ID of the cell (specified by id) in a particular direction
  size_t CalcNeighbor(size_t id, Facing dir) const;

public:
  MCRegDeme(size_t _width, size_t _height, emp::Random & _random,
            inst_lib_t & ilib, event_lib_t & elib)
    : width(_width),
      height(_height),
      random(_random)
  {
    for (size_t i = 0; i < width*height; ++i) {
      cells.emplace_back(random, ilib, elib);
      cells.back().GetCustomComponent().SetCellID(i);

    }
    BuildNeighborLookup();
  }

  /// Get cell capacity of deme
  size_t GetSize() const { return cells.size(); }

  /// Given cell ID, return x coordinate
  size_t GetCellX(size_t id) const { return id % width; }

  /// Given cell ID, return y coordinate
  size_t GetCellY(size_t id) const { return id / width; }

  /// Given x,y coordinate, return cell ID
  size_t GetCellID(size_t x, size_t y) const { return (y * width) + x; }

  /// Get cell at position ID (outsource bounds checking to emp::vector)
  hardware_t & GetCell(size_t id) { return cells[id]; }

  /// Get const cell at position ID (outsource bounds checking to emp::vector)
  const hardware_t & GetCell(size_t id) const { return cells[id]; }

  emp::vector<hardware_t> & GetCells() { return cells; }

  // todo - Advance (by a number of steps)
  // todo - SingleAdvance()
  // todo - Configure Hardware function

  /// Given a Facing direction, return a representative string (useful for debugging)
  std::string FacingToStr(Facing dir) const;
  /// Print the deme's neighbor map! (useful for debugging)
  void PrintNeighborMap(std::ostream & os=std::cout) const;
};

template<typename HW_MEMORY_MODEL_T,typename HW_TAG_T,typename HW_INST_ARG_T,typename HW_MATCHBIN_T>
void MCRegDeme<HW_MEMORY_MODEL_T,HW_TAG_T,HW_INST_ARG_T,HW_MATCHBIN_T>::BuildNeighborLookup()  {
  const size_t num_cells = width * height;
  neighbor_lookup.resize(num_cells * NUM_DIRECTIONS);
  for (size_t i = 0; i < num_cells; ++i) {
    for (size_t d = 0; d < NUM_DIRECTIONS; ++d) {
      neighbor_lookup[i*NUM_DIRECTIONS + d] = CalcNeighbor(i, Dir[d]);
    }
  }
}

template<typename HW_MEMORY_MODEL_T,typename HW_TAG_T,typename HW_INST_ARG_T,typename HW_MATCHBIN_T>
size_t MCRegDeme<HW_MEMORY_MODEL_T,HW_TAG_T,HW_INST_ARG_T,HW_MATCHBIN_T>::CalcNeighbor(size_t id, Facing dir) const {
  int facing_y = (int)GetCellY(id);
  int facing_x = (int)GetCellX(id);
  switch (dir) {
    case Facing::N:
      facing_y = emp::Mod(facing_y + 1, (int)height);
      break;
    case Facing::NE:
      facing_x = emp::Mod(facing_x + 1, (int)width);
      facing_y = emp::Mod(facing_y + 1, (int)height);
      break;
    case Facing::E:
      facing_x = emp::Mod(facing_x + 1, (int)width);
      break;
    case Facing::SE:
      facing_x = emp::Mod(facing_x + 1, (int)width);
      facing_y = emp::Mod(facing_y - 1, (int)height);
      break;
    case Facing::S:
      facing_y = emp::Mod(facing_y - 1, (int)height);
      break;
    case Facing::SW:
      facing_x = emp::Mod(facing_x - 1, (int)width);
      facing_y = emp::Mod(facing_y - 1, (int)height);
      break;
    case Facing::W:
      facing_x = emp::Mod(facing_x - 1, (int)width);
      break;
    case Facing::NW:
      facing_x = emp::Mod(facing_x - 1, (int)width);
      facing_y = emp::Mod(facing_y + 1, (int)height);
      break;
    default:
      emp_assert(false, "Invalid direction!");
      break;
  }
  return GetCellID(facing_x, facing_y);
}


/// Given a Facing direction, return a representative string (useful for debugging)
template<typename HW_MEMORY_MODEL_T,typename HW_TAG_T,typename HW_INST_ARG_T,typename HW_MATCHBIN_T>
std::string MCRegDeme<HW_MEMORY_MODEL_T,HW_TAG_T,HW_INST_ARG_T,HW_MATCHBIN_T>::FacingToStr(Facing dir) const {
  switch (dir) {
    case Facing::N: return "N";
    case Facing::NE: return "NE";
    case Facing::E: return "E";
    case Facing::SE: return "SE";
    case Facing::S: return "S";
    case Facing::SW: return "SW";
    case Facing::W: return "W";
    case Facing::NW: return "NW";
    default:
      emp_assert(false, "Invalid direction!");
      return "";
  }
}


/// Print the deme's neighbor map! (useful for debugging)
template<typename HW_MEMORY_MODEL_T,typename HW_TAG_T,typename HW_INST_ARG_T,typename HW_MATCHBIN_T>
void MCRegDeme<HW_MEMORY_MODEL_T,HW_TAG_T,HW_INST_ARG_T,HW_MATCHBIN_T>::PrintNeighborMap(std::ostream & os /*= std::cout*/) const {
  const size_t num_cells = width * height;
  for (size_t i = 0; i < num_cells; ++i) {
    os << i << " (" << GetCellX(i) << ", " << GetCellY(i) << "): " << std::endl;
    for (size_t d = 0; d < NUM_DIRECTIONS; ++d) {
      const size_t neighbor_id = neighbor_lookup[i*NUM_DIRECTIONS + d];
      os << "  " << FacingStr(Dir[d]) << "(" << d << "): " << neighbor_id << "(" << GetCellX(neighbor_id) << ", " << GetCellY(neighbor_id) << ")" << std::endl;
    }
  }
}

#endif