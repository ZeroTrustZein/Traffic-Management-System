#include "FineController.h"
#include "Response.h"
#include "../Models.h"

crow::response FineController::listFines(const crow::request&) {
    try {
        auto fines = fineRepo_.findAll(100);

        std::vector<crow::json::wvalue> list;
        list.reserve(fines.size());
        for (const auto& f : fines) {
            crow::json::wvalue entry;
            entry["id"]           = f.id;
            entry["violation_id"] = f.violationId;
            entry["amount"]       = f.amount;
            entry["status"]       = toString(f.status);
            entry["issued_at"]    = f.issuedAt;
            entry["due_date"]     = f.dueDate;
            if (!f.paidAt.empty()) entry["paid_at"] = f.paidAt;

            // Enrich with vehicle/owner info
            auto v = violationRepo_.findById(f.violationId);
            if (v) {
                auto vehicle = vehicleRepo_.findById(v->vehicleId);
                if (vehicle) {
                    crow::json::wvalue vInfo;
                    vInfo["number_plate"] = vehicle->numberPlate;
                    vInfo["vehicle_type"] = toString(vehicle->vehicleType);
                    entry["vehicle"] = std::move(vInfo);

                    if (vehicle->ownerId > 0) {
                        auto owner = ownerRepo_.findById(vehicle->ownerId);
                        if (owner) {
                            crow::json::wvalue oInfo;
                            oInfo["full_name"] = owner->fullName;
                            oInfo["email"]     = owner->email;
                            entry["owner"] = std::move(oInfo);
                        }
                    }
                }
            }
            list.push_back(std::move(entry));
        }

        crow::json::wvalue res;
        res["count"] = static_cast<int>(fines.size());
        res["fines"] = std::move(list);
        return jsonResponse(200, std::move(res));

    } catch (const std::exception&) {
        return jsonServerError();
    }
}

crow::response FineController::getFine(const crow::request&, int id) {
    try {
        auto f = fineRepo_.findById(id);
        if (!f) return jsonError(404, "Fine " + std::to_string(id) + " not found");

        crow::json::wvalue res;
        res["id"]           = f->id;
        res["violation_id"] = f->violationId;
        res["amount"]       = f->amount;
        res["status"]       = toString(f->status);
        res["issued_at"]    = f->issuedAt;
        res["due_date"]     = f->dueDate;
        if (!f->paidAt.empty()) res["paid_at"] = f->paidAt;
        return jsonResponse(200, std::move(res));

    } catch (const std::exception&) {
        return jsonServerError();
    }
}

crow::response FineController::payFine(const crow::request&, int id) {
    try {
        auto f = fineRepo_.findById(id);
        if (!f) return jsonError(404, "Fine not found");
        if (f->status == FineStatus::PAID)
            return jsonError(409, "Fine is already paid");
        if (f->status == FineStatus::CANCELLED)
            return jsonError(409, "Fine is cancelled and cannot be paid");

        fineRepo_.updateStatus(id, "PAID");
        violationRepo_.updateStatus(f->violationId, "PAID");

        crow::json::wvalue res;
        res["fine_id"] = id;
        res["status"]  = "PAID";
        res["message"] = "Fine marked as paid";
        return jsonResponse(200, std::move(res));

    } catch (const std::exception&) {
        return jsonServerError();
    }
}

crow::response FineController::cancelFine(const crow::request&, int id) {
    try {
        auto f = fineRepo_.findById(id);
        if (!f) return jsonError(404, "Fine not found");
        if (f->status == FineStatus::PAID)
            return jsonError(409, "Paid fine cannot be cancelled");

        fineRepo_.updateStatus(id, "CANCELLED");
        violationRepo_.updateStatus(f->violationId, "CANCELLED");

        crow::json::wvalue res;
        res["fine_id"] = id;
        res["status"]  = "CANCELLED";
        res["message"] = "Fine cancelled";
        return jsonResponse(200, std::move(res));

    } catch (const std::exception&) {
        return jsonServerError();
    }
}

