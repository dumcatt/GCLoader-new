#pragma once
#include "Patches/GameVersion/VersionedPlan.h"
namespace gc::game_compatibility {
// real_cabinet_io omits the emulated-dongle, dongle-transmit, and COM-port
// patches so the game performs its native dongle and COM checks. Requires an
// unpatched executable image.
[[nodiscard]] std::expected<game_version::FeaturePlan, game_version::PlanError> BuildGameCompatibilityPlan(
    game_version::GameBuild, game_version::GameImageVariant, bool real_cabinet_io) noexcept;
}
