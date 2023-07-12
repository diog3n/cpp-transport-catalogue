#pragma once
#include <algorithm>
#include <deque>
#include <memory>
#include <vector>

#include "transport_catalogue.hpp"

namespace request_handler {

class RequestHandler {
public:
    RequestHandler();

    RequestHandler(const transport_catalogue::TransportCatalogue& tc);

    RequestHandler(std::unique_ptr<transport_catalogue::TransportCatalogue>&& tc_ptr);

    void ExecuteInputQueries();

    void ExecuteOutputQueries();

    void AddStopInputQuery();
    
    void AddBusInputQuery();

    void AddStopOutputQuery();

    void AddBusOutputQuery();

private:

    std::shared_ptr<transport_catalogue::TransportCatalogue> catalogue_ptr_;

    std::vector<domain::BusInputQuery> bus_input_queries_;

    std::vector<domain::StopInputQuery> stop_input_queries_;

    std::vector<domain::StopOutputQuery> stop_output_queries_;

    std::vector<domain::BusOutputQuery> bus_output_queries_;
};

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