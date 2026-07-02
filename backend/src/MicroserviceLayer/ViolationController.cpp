#include "ViolationController.h"
#include "Response.h"
#include "ViolationRules.h"
#include "../Models.h"
#include <algorithm>
#include <map>

static const std::map<EventType, std::string> kEventToCode = {
    {EventType::SPEEDING,  "SPEEDING"},
    {EventType::RED_LIGHT, "RED_LIGHT"},
    {EventType::PARKING,   "PARKING"},
};

crow::response ViolationController::detectViolations(const crow::request& req) {
    auto body = crow::json::load(req.body);
    if (!body)
        return jsonError(400, "Request body must be valid JSON");

    // Numeric fields must be JSON numbers, so bad input returns 400 not a 500.
    if (body.has("junction_id") && body["junction_id"].t() != crow::json::type::Number)
        return jsonFieldError(400, "junction_id", "junction_id must be a number");
    if (body.has("hours_back") && body["hours_back"].t() != crow::json::type::Number)
        return jsonFieldError(400, "hours_back", "hours_back must be a number");

    const int junctionId = body.has("junction_id") ? body["junction_id"].i() : 0;
    const int hoursBack  = body.has("hours_back")  ? body["hours_back"].i()  : 1;

    if (junctionId <= 0)
        return jsonFieldError(400, "junction_id", "junction_id is required and must be positive");
    if (hoursBack < 1 || hoursBack > 168)
        return jsonFieldError(400, "hours_back", "hours_back must be between 1 and 168");

    try {
        auto junction = junctionRepo_.findById(junctionId);
        if (!junction)
            return jsonError(404, "Junction " + std::to_string(junctionId) + " not found");

        auto logs = logRepo_.findUnprocessedViolationLogs(junctionId, hoursBack);

        std::vector<crow::json::wvalue> created;
        for (const auto& log : logs) {
            if (log.vehicleId == 0) continue;  // skip unregistered (no vehicle to attach to)

            const auto it = kEventToCode.find(log.eventType);
            if (it == kEventToCode.end()) continue;

            auto vtype = typeRepo_.findByCode(it->second);
            if (!vtype) continue;

            const std::string severity = ViolationRules::severityForEvent(
                toString(log.eventType), log.speedKmh, log.hasSpeed, junction->speedLimitKmh);

            std::string note;
            if (log.eventType == EventType::SPEEDING && log.hasSpeed) {
                note = "Speed: " + std::to_string(static_cast<int>(log.speedKmh))
                     + " km/h in " + std::to_string(junction->speedLimitKmh) + " km/h zone";
            } else if (log.eventType == EventType::RED_LIGHT) {
                note = "Vehicle passed red light at " + junction->name;
            } else if (log.eventType == EventType::PARKING) {
                note = "Vehicle parked in prohibited zone near " + junction->name;
            }

            // W4: determine violation status based on requires_review flag
            const std::string violationStatus = vtype->requiresReview ? "REVIEW" : "OPEN";

            // W9: check repeat-offender status BEFORE creating the new violation
            const int priorCount = violationRepo_.countByVehicle(log.vehicleId);
            const bool isRepeatOffender = ViolationRules::isRepeatOffender(priorCount);

            const int violationId = violationRepo_.save(
                log.vehicleId, junctionId, vtype->id, log.id,
                severity, note, violationStatus);

            crow::json::wvalue entry;
            entry["violation_id"]       = violationId;
            entry["plate_log_id"]       = log.id;
            entry["number_plate"]       = log.numberPlate;
            entry["violation"]          = vtype->name;
            entry["severity"]           = severity;
            entry["status"]             = violationStatus;
            entry["is_repeat_offender"] = isRepeatOffender;

            // W4: only auto-issue fine if NOT requires_review
            if (!vtype->requiresReview) {
                double amount = vtype->baseFine * ViolationRules::multiplierForSeverity(
                    severity, vtype->multLow, vtype->multMedium, vtype->multHigh, vtype->multCritical);

                // W9: escalate fine amount for repeat offenders
                if (isRepeatOffender) {
                    amount *= ViolationRules::repeatOffenderMultiplier();
                }

                const int fineId = fineRepo_.save(violationId, amount);
                entry["fine_id"]    = fineId;
                entry["fine_amount"] = amount;

                // W3/W4: write notification log only if auto_notify is true
                if (vtype->autoNotify) {
                    // Get vehicle's owner_id for the notification
                    auto vehicle = vehicleRepo_.findById(log.vehicleId);
                    const int ownerId = vehicle ? vehicle->ownerId : 0;
                    notifRepo_.create(
                        ownerId,
                        "FINE_ISSUED",
                        "Fine issued for " + vtype->name,
                        "A fine of " + std::to_string(amount) +
                        " has been issued for vehicle " + log.numberPlate +
                        " for " + vtype->name + " at junction " + junction->name +
                        (isRepeatOffender ? " (repeat offender surcharge applied)" : "") + ".",
                        "SENT"
                    );
                }
            } else {
                // requires_review: fine deferred; no notification yet
                entry["fine_id"]    = nullptr;
                entry["fine_amount"] = nullptr;
                entry["review_note"] = "Violation requires manual review before fine is issued";
            }

            created.push_back(std::move(entry));
        }

        crow::json::wvalue res;
        res["junction_id"]        = junctionId;
        res["violations_created"] = static_cast<int>(created.size());
        res["results"]            = std::move(created);
        return jsonResponse(200, std::move(res));

    } catch (const std::exception&) {
        return jsonServerError();
    }
}

crow::response ViolationController::listViolations(const crow::request&) {
    try {
        auto violations = violationRepo_.findAll(100);

        std::vector<crow::json::wvalue> list;
        list.reserve(violations.size());
        for (const auto& v : violations) {
            crow::json::wvalue entry;
            entry["id"]                = v.id;
            entry["vehicle_id"]        = v.vehicleId;
            entry["junction_id"]       = v.junctionId;
            entry["violation_type_id"] = v.violationTypeId;
            entry["detected_at"]       = v.detectedAt;
            entry["severity"]          = toString(v.severity);
            entry["evidence_note"]     = v.evidenceNote;
            entry["status"]            = v.status;
            list.push_back(std::move(entry));
        }

        crow::json::wvalue res;
        res["count"]      = static_cast<int>(violations.size());
        res["violations"] = std::move(list);
        return jsonResponse(200, std::move(res));

    } catch (const std::exception&) {
        return jsonServerError();
    }
}

crow::response ViolationController::getViolation(const crow::request&, int id) {
    try {
        auto v = violationRepo_.findById(id);
        if (!v) return jsonError(404, "Violation " + std::to_string(id) + " not found");

        auto fine = fineRepo_.findByViolationId(id);

        crow::json::wvalue res;
        res["id"]                = v->id;
        res["vehicle_id"]        = v->vehicleId;
        res["junction_id"]       = v->junctionId;
        res["violation_type_id"] = v->violationTypeId;
        res["detected_at"]       = v->detectedAt;
        res["severity"]          = toString(v->severity);
        res["evidence_note"]     = v->evidenceNote;
        res["status"]            = v->status;
        if (fine) {
            crow::json::wvalue fineJson;
            fineJson["id"]        = fine->id;
            fineJson["amount"]    = fine->amount;
            fineJson["status"]    = toString(fine->status);
            fineJson["issued_at"] = fine->issuedAt;
            res["fine"] = std::move(fineJson);
        }
        return jsonResponse(200, std::move(res));

    } catch (const std::exception&) {
        return jsonServerError();
    }
}

crow::response ViolationController::listViolationTypes(const crow::request&) {
    try {
        auto types = typeRepo_.findAll();

        std::vector<crow::json::wvalue> list;
        list.reserve(types.size());
        for (const auto& t : types) {
            crow::json::wvalue entry;
            entry["id"]                  = t.id;
            entry["name"]                = t.name;
            entry["code"]                = t.code;
            entry["base_fine"]           = t.baseFine;
            entry["description"]         = t.description;
            entry["is_active"]           = t.isActive;
            entry["multiplier_low"]      = t.multLow;
            entry["multiplier_medium"]   = t.multMedium;
            entry["multiplier_high"]     = t.multHigh;
            entry["multiplier_critical"] = t.multCritical;
            entry["auto_notify"]         = t.autoNotify;    // W13
            entry["requires_review"]     = t.requiresReview; // W13
            list.push_back(std::move(entry));
        }

        crow::json::wvalue res;
        res["count"] = static_cast<int>(types.size());
        res["types"] = std::move(list);
        return jsonResponse(200, std::move(res));

    } catch (const std::exception&) {
        return jsonServerError();
    }
}

crow::response ViolationController::issueFine(const crow::request& req, int violationId) {
    try {
        auto v = violationRepo_.findById(violationId);
        if (!v) return jsonError(404, "Violation not found");

        auto existing = fineRepo_.findByViolationId(violationId);
        if (existing)
            return jsonError(409, "A fine for violation " + std::to_string(violationId) + " already exists");

        auto vtype = typeRepo_.findById(v->violationTypeId);
        if (!vtype) return jsonError(500, "Violation type not found");

        double amount = vtype->baseFine * ViolationRules::multiplierForSeverity(
            toString(v->severity), vtype->multLow, vtype->multMedium, vtype->multHigh, vtype->multCritical);

        // Allow override from request body
        auto body = crow::json::load(req.body);
        if (body && body.has("amount") && body["amount"].t() == crow::json::type::Number)
            amount = body["amount"].d();

        // W9: check repeat-offender status (count existing violations for this vehicle)
        const int totalViolationCount = violationRepo_.countByVehicle(v->vehicleId);
        const int priorCount = std::max(0, totalViolationCount - 1);
        const bool isRepeatOffender = ViolationRules::isRepeatOffender(priorCount);
        if (isRepeatOffender && !(body && body.has("amount"))) {
            // Only escalate if caller didn't provide an explicit override amount
            amount *= ViolationRules::repeatOffenderMultiplier();
        }

        const int fineId = fineRepo_.save(violationId, amount);

        // Mark violation as OPEN if it was in REVIEW
        if (v->status == "REVIEW") {
            violationRepo_.updateStatus(violationId, "OPEN");
        }

        // W3: write notification log
        if (vtype->autoNotify) {
            auto vehicle = vehicleRepo_.findById(v->vehicleId);
            const int ownerId = vehicle ? vehicle->ownerId : 0;
            notifRepo_.create(
                ownerId,
                "FINE_ISSUED",
                "Fine issued for " + vtype->name,
                "A fine of " + std::to_string(amount) +
                " has been manually issued for violation #" + std::to_string(violationId) +
                (isRepeatOffender ? " (repeat offender surcharge applied)" : "") + ".",
                "SENT"
            );
        }

        crow::json::wvalue res;
        res["fine_id"]           = fineId;
        res["violation_id"]      = violationId;
        res["amount"]            = amount;
        res["status"]            = "PENDING";
        res["is_repeat_offender"] = isRepeatOffender;
        return jsonResponse(201, std::move(res));

    } catch (const std::exception&) {
        return jsonServerError();
    }
}

