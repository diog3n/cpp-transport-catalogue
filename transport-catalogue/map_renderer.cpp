#include "map_renderer.hpp"

namespace renderer {

namespace util {

bool IsZero(double value) {
    return std::abs(value) < EPSILON;
}

} // namespace renderer::util



} // namespace renderer