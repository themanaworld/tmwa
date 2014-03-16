#ifndef TMWA_MAP_GRFIO_HPP
#define TMWA_MAP_GRFIO_HPP

# include <cstdint>

# include <vector>

# include "../mmo/mmo.hpp"

bool load_resnametable(ZString filename);

/// Load a resource into memory, subject to data/resnametable.txt.
/// Normally, resourcename is xxx-y.gat and the file is xxx-y.wlk.
/// Currently there is exactly one .wlk per .gat, but multiples are fine.
std::vector<uint8_t> grfio_reads(MapName resourcename);

#endif // TMWA_MAP_GRFIO_HPP
