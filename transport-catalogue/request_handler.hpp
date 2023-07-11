#pragma once
#include <deque>
#include <memory>
#include <vector>

#include "transport_catalogue.hpp"

namespace request_handler {

struct BusQuery {
    std::string_view bus_name;
    std::vector<std::string_view> stop_names;
};

struct StopQuery {
    std::string_view stop_name;
    geo::Coordinates coordinates;
    std::unordered_map<std::string_view, int> distances;
};

class RequestHandler {
public:
    RequestHandler(std::unique_ptr<transport_catalogue::TransportCatalogue>&& catalogue);

private:
    std::shared_ptr<transport_catalogue::TransportCatalogue> catalogue_;
    
    std::deque<std::string> raw_queries_;

    std::vector<BusQuery> bus_input_queries_;

    std::vector<StopQuery> stop_input_queries_;
};

namespace tests {

} // namespace request_handler::tests

} // namespace request_handler

// Класс RequestHandler играет роль Фасада, упрощающего взаимодействие JSON reader-а
// с другими подсистемами приложения.
// См. паттерн проектирования Фасад: https://ru.wikipedia.org/wiki/Фасад_(шаблон_проектирования)
/*
class RequestHandler {
public:
    // MapRenderer понадобится в следующей части итогового проекта
    RequestHandler(const TransportCatalogue& db, const renderer::MapRenderer& renderer);

    // Возвращает информацию о маршруте (запрос Bus)
    std::optional<BusStat> GetBusStat(const std::string_view& bus_name) const;

    // Возвращает маршруты, проходящие через
    const std::unordered_set<BusPtr>* GetBusesByStop(const std::string_view& stop_name) const;

    // Этот метод будет нужен в следующей части итогового проекта
    svg::Document RenderMap() const;

private:
    // RequestHandler использует агрегацию объектов "Транспортный Справочник" и "Визуализатор Карты"
    const TransportCatalogue& db_;
    const renderer::MapRenderer& renderer_;
};
*/