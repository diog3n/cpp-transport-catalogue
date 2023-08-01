#include "domain.hpp"

bool domain::BusCompare::operator() (const BusPtr lb, const BusPtr rb) const {
    return lb->name < rb->name;
}