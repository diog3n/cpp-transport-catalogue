#include "domain.hpp"

bool domain::BusCompare::operator() (const Bus* lb, const Bus* rb) const {
    return lb->name < rb->name;
}