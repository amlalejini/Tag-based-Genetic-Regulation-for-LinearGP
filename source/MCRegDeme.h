#ifndef _MC_REG_DEME_H
#define _MC_REG_DEME_H

#include <ostream>
// Empirical includes
#include "base/vector.h"
#include "tools/math.h"
#include "tools/random_utils.h"
#include "tools/vector_utils.h"
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
    bool new_born=false;
    int response=-1;
    Facing cell_facing=Facing::N;

    /// Reset everything except cell id
    void Reset() {
      response=-1;
      active=false;
      cell_facing=Facing::N;
      new_born=false;
    }

    size_t GetCellID() const { return cell_id; }
    void SetCellID(size_t id) { cell_id = id; }

    int GetResponse() const { return response; }
    void SetResponse(int val) { response = val; }

    bool IsActive() const { return active; }
    void SetActive() { active = true; }
    void SetInactive() { active = false; }

    Facing GetFacing() const { return cell_facing; }
    void SetFacing(Facing facing) { cell_facing = facing; }

    bool IsNewBorn() const { return new_born; }

    /// Rotate cell facing clockwise
    Facing RotateCW(int rot=1) {
      const int cur_dir = (int)cell_facing;
      const size_t new_dir = (size_t)emp::Mod(cur_dir+rot, (int)MCRegDeme::NUM_DIRECTIONS);
      emp_assert(new_dir < MCRegDeme::NUM_DIRECTIONS);
      cell_facing = MCRegDeme::Dir[new_dir];
      return cell_facing;
    }

    /// Rotate cell facing counter-clockwise
    Facing RotateCCW(int rot=1) {
      const int cur_dir = (int)cell_facing;
      const size_t new_dir = (size_t)emp::Mod(cur_dir-rot, (int)MCRegDeme::NUM_DIRECTIONS);
      emp_assert(new_dir < MCRegDeme::NUM_DIRECTIONS);
      cell_facing = MCRegDeme::Dir[new_dir];
      return cell_facing;
    }
  };

  using hardware_t = sgp::LinearFunctionsProgramSignalGP<HW_MEMORY_MODEL_T,
                                                         HW_TAG_T,
                                                         HW_INST_ARG_T,
                                                         HW_MATCHBIN_T,
                                                         CellHardware>;
  using event_lib_t = typename hardware_t::event_lib_t;
  using inst_lib_t = typename hardware_t::inst_lib_t;
  using program_t = typename hardware_t::program_t;

protected:
  size_t width;   ///< Width of deme grid
  size_t height;  ///< Height of deme grid
  emp::Random & random;
  emp::vector<size_t> neighbor_lookup; ///< Lookup table for neighbors
  emp::vector<hardware_t> cells;       ///< Toroidal grid of hardware units
  emp::vector<size_t> cell_schedule;   ///< Order to execute cells

  emp::Signal<void(hardware_t &)> on_propagule_activate_sig;              ///< Triggered when a cell is activated as a propagule
  emp::Signal<void(hardware_t & /*offspring hw*/, hardware_t & /*parent hw*/)> on_repro_activate_sig;     ///< Triggered when a cell is activated after reproduction event

  /// Build neighbor lookup (according to current width and height)
  void BuildNeighborLookup();

  /// Calculate the neighbor ID of the cell (specified by id) in a particular direction
  size_t CalcNeighbor(size_t id, Facing dir) const;

  void ActivatePropaguleClumpy(const program_t & prog, size_t prop_size);
  void ActivatePropaguleRandom(const program_t & prog, size_t prop_size);

  /// Activate cell and schedule (if it is going from an inactive => active state)
  void ActivateCell(size_t cell_id, const program_t & prog) {
    emp_assert( (IsActive(cell_id) && emp::Has(cell_schedule, cell_id)) || (!IsActive(cell_id) && !emp::Has(cell_schedule, cell_id)) );
    cells[cell_id].SetProgram(prog);
    if (IsActive(cell_id)) {
      cells[cell_id].GetCustomComponent().Reset();
    } else {
      cell_schedule.emplace_back(cell_id);
    }
    cells[cell_id].GetCustomComponent().SetActive();
  }

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

  /// Reset cellular hardware for each cell in deme.
  void ResetCells() {
    for (hardware_t & cell : cells) {
      cell.Reset();
      cell.GetCustomComponent().Reset();
      emp_assert(cell.ValidateThreadState());
    }
    cell_schedule.clear();
  }

  // todo - Configure Hardware function
  void ConfigureCells(size_t max_active_threads, size_t max_thread_capacity) {
    for (hardware_t & cell : cells) {
      cell.SetActiveThreadLimit(max_active_threads);
      cell.SetThreadCapacity(max_thread_capacity);
      emp_assert(cell.ValidateThreadState());
    }
  }

  /// Activate deme by activating propagule cells
  /// Note, this will reset deme state
  void ActivatePropagule(const program_t & prog,
                         size_t prop_size=1,
                         bool clumpy=true)
  {
    ResetCells(); // Reset
    // Todo - if prop_size == deme_size, don't do anything complicated!
    if (clumpy) ActivatePropaguleClumpy(prog, prop_size);
    else ActivatePropaguleRandom(prog, prop_size);
  }

  /// Get cell capacity of deme
  size_t GetSize() const { return cells.size(); }

  /// Given cell ID, return x coordinate
  size_t GetCellX(size_t id) const { return id % width; }

  /// Given cell ID, return y coordinate
  size_t GetCellY(size_t id) const { return id / width; }

  /// Given x,y coordinate, return cell ID
  size_t GetCellID(size_t x, size_t y) const { return (y * width) + x; }

  /// Given a cell ID and facing (of that cell), return the appropriate neighboring cell ID
  size_t GetNeighboringCellID(size_t id, Facing dir) const { return neighbor_lookup[id*NUM_DIRECTIONS + (size_t)dir]; }

  /// Get cell at position ID (outsource bounds checking to emp::vector)
  hardware_t & GetCell(size_t id) { return cells[id]; }

  /// Get const cell at position ID (outsource bounds checking to emp::vector)
  const hardware_t & GetCell(size_t id) const { return cells[id]; }

  emp::vector<hardware_t> & GetCells() { return cells; }

  /// Is a cell at a particular location active?
  bool IsActive(size_t id) const { return cells[id].GetCustomComponent().IsActive(); }

  // todo - SingleAdvance()
  bool SingleAdvance() {
    // Advance cells in a random order.
    emp::Shuffle(random, cell_schedule);
    for (size_t i = 0; i < cell_schedule.size(); ++i) {
      const size_t cell_id = cell_schedule[i];
      emp_assert(IsActive(cell_id));
      hardware_t & cell_hw = cells[cell_id];
      // If this cell was just 'born', don't run it. Update birth status (so it will run next time)
      if (cell_hw.GetCustomComponent().IsNewBorn()) {
        cell_hw.GetCustomComponent().new_born = false;
        continue;
      }
      cell_hw.SingleProcess();
    }
    bool executing = false;
    for (size_t cell_id : cell_schedule) {
      hardware_t & cell_hw = cells[cell_id];
      if (cell_hw.GetCustomComponent().IsNewBorn()) cell_hw.GetCustomComponent().new_born = false;
      executing = (bool)cell_hw.GetNumActiveThreads() || (bool)cell_hw.GetNumPendingThreads() || (bool)cell_hw.GetNumQueuedEvents();
    }
    return executing;
  }

  /// Do reproduction from offspring cell id ==> parent cell id
  void DoReproduction(size_t offspring_cell_id, size_t parent_cell_id) {
    emp_assert(IsActive(parent_cell_id));
    // Activate offspring cell with program of parent cell.
    ActivateCell(offspring_cell_id, cells[parent_cell_id].GetProgram());
    // Set offspring to new_born status (it will not execute this step)
    cells[offspring_cell_id].GetCustomComponent().new_born = true;
    // Assert that offspring is indeed active now
    emp_assert(IsActive(offspring_cell_id));
    // Trigger on repro signal
    on_repro_activate_sig.Trigger(cells[offspring_cell_id], cells[parent_cell_id]);
  }

  void OnPropaguleActivate(const std::function<void(hardware_t &)> & fun) {
    on_propagule_activate_sig.AddAction(fun);
  }

  void OnReproActivate(const std::function<void(hardware_t & /*offspring hw*/, hardware_t & /*parent hw*/)> & fun) {
    on_repro_activate_sig.AddAction(fun);
  }

  /// Given a Facing direction, return a representative string (useful for debugging)
  std::string FacingToStr(Facing dir) const;

  /// Print the deme's neighbor map! (useful for debugging)
  void PrintNeighborMap(std::ostream & os=std::cout) const;

  /// Print which cells are active in deme (useful for debugging)
  void PrintActive(std::ostream & os=std::cout) {
    os << "-- Deme Active/Inactive --\n";
    for (size_t y = 0; y < height; ++y) {
      for (size_t x = 0; x < width; ++x) {
        os << (int)IsActive(GetCellID(x,y)) << " ";
      } os << "\n";
    }
  }

  void PrintResponses(std::ostream & os=std::cout) {
    os << "-- Deme Responses --\n";
    for (size_t y = 0; y < height; ++y) {
      for (size_t x = 0; x < width; ++x) {
        const int resp = GetCell(GetCellID(x,y)).GetCustomComponent().GetResponse();
        if (resp < 0) {
          os << " N ";
        } else {
          os << " " << resp << " ";
        }
      } os << "\n";
    }
  }
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

template<typename HW_MEMORY_MODEL_T,typename HW_TAG_T,typename HW_INST_ARG_T,typename HW_MATCHBIN_T>
void MCRegDeme<HW_MEMORY_MODEL_T,HW_TAG_T,HW_INST_ARG_T,HW_MATCHBIN_T>::ActivatePropaguleClumpy(const program_t & prog,
                                                                                                size_t prop_size)
{
  emp_assert(prop_size < cells.size(), "Cannot activate propagule with more cells than there is space.", prop_size, cells.size());
  // size_t cell_id = random.GetUInt(0, cells.size());  // Where should we start clump activate?
  size_t cell_id = (width * height) / 2;
  emp_assert(cell_id < cells.size());
  size_t prop_cnt = 0;                               // Track the number of cells activated so far.
  Facing dir = Facing::N;
  while (prop_cnt < prop_size) {
    if (!IsActive(cell_id)) {
      prop_cnt += 1;
      // TODO activate propagule!
      // Load program
      ActivateCell(cell_id, prog);
      emp_assert(IsActive(cell_id));
      on_propagule_activate_sig.Trigger(cells[cell_id]);
    } else {
      Facing r_dir = Dir[(dir + 2) % NUM_DIRECTIONS]; // Want to use 90 degree turns here (thus, +2 instead of +1)
      size_t r_id = GetNeighboringCellID(cell_id, r_dir);
      if (!IsActive(r_id)) {
        dir = r_dir; cell_id = r_id;
      } else {
        cell_id = GetNeighboringCellID(cell_id, dir);
      }
    }
  }
}

template<typename HW_MEMORY_MODEL_T,typename HW_TAG_T,typename HW_INST_ARG_T,typename HW_MATCHBIN_T>
void MCRegDeme<HW_MEMORY_MODEL_T,HW_TAG_T,HW_INST_ARG_T,HW_MATCHBIN_T>::ActivatePropaguleRandom(const program_t & prog,
                                                                                                size_t prop_size)
{
  // Active deme randomly
  emp::vector<size_t> prop_ids(GetSize());
  for (size_t i = 0; i < GetSize(); ++i) prop_ids[i] = i;
  emp::Shuffle(random, prop_ids);
  prop_ids.resize(prop_size);
  for (size_t id : prop_ids) {
    // Activate cell @ id
    ActivateCell(id, prog);
    emp_assert(IsActive(id));
    on_propagule_activate_sig.Trigger(cells[id]);
  }
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