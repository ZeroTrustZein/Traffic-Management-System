#pragma once
#include <string>
#include <vector>
#include "DatabaseConnection.h"
#include "../Models.h"

// W13/W14/W15: writes and reads the notification_logs audit trail.
// Django still SENDS emails; Crow records them here so they are queryable.
class NotificationRepository {
public:
    // Create a notification log entry.
    // ownerId = 0 is treated as NULL (system/junction-level alert with no specific owner).
    int create(int ownerId,
               const std::string& category,
               const std::string& subject,
               const std::string& message,
               const std::string& status = "SENT");

    // Return the most recent limit notification log entries.
    std::vector<NotificationLog> findAll(int limit = 100);
};
