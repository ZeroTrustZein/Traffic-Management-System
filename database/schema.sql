-- =============================================================================
-- Traffic Management System — Full Database Schema
-- =============================================================================
-- Run: psql -U postgres -d traffic_db -f database/schema.sql
-- =============================================================================

-- Drop in reverse dependency order (safe re-run)
DROP TABLE IF EXISTS junction_signals     CASCADE;
DROP TABLE IF EXISTS notification_logs   CASCADE;
DROP TABLE IF EXISTS emergency_events    CASCADE;
DROP TABLE IF EXISTS emergency_vehicles  CASCADE;
DROP TABLE IF EXISTS driver_profiles     CASCADE;
DROP TABLE IF EXISTS road_segments       CASCADE;
DROP TABLE IF EXISTS route_alternatives  CASCADE;
DROP TABLE IF EXISTS congestion_records  CASCADE;
DROP TABLE IF EXISTS fines               CASCADE;
DROP TABLE IF EXISTS violations          CASCADE;
DROP TABLE IF EXISTS violation_types     CASCADE;
DROP TABLE IF EXISTS plate_logs          CASCADE;
DROP TABLE IF EXISTS vehicles            CASCADE;
DROP TABLE IF EXISTS junctions           CASCADE;
DROP TABLE IF EXISTS owners              CASCADE;

-- =============================================================================
-- OWNERS
-- =============================================================================
CREATE TABLE owners (
    id         SERIAL       PRIMARY KEY,
    full_name  VARCHAR(100) NOT NULL,
    email      VARCHAR(200) UNIQUE NOT NULL,
    phone      VARCHAR(20),
    created_at TIMESTAMP    NOT NULL DEFAULT CURRENT_TIMESTAMP
);

-- =============================================================================
-- JUNCTIONS — city road junctions with cameras
-- =============================================================================
CREATE TABLE junctions (
    id               SERIAL          PRIMARY KEY,
    name             VARCHAR(100)    NOT NULL,
    location         VARCHAR(200)    NOT NULL DEFAULT '',
    speed_limit_kmh  INTEGER         NOT NULL DEFAULT 60,
    -- Geographic coordinates for map visualisation (WGS84 decimal degrees).
    -- NULL/0 means coordinates have not been set yet.
    latitude         NUMERIC(10, 7),
    longitude        NUMERIC(10, 7),
    is_active        BOOLEAN         NOT NULL DEFAULT TRUE
);

-- =============================================================================
-- DRIVER PROFILES — notification preferences and typical route
-- =============================================================================
CREATE TABLE driver_profiles (
    id                  SERIAL    PRIMARY KEY,
    owner_id             INTEGER   NOT NULL REFERENCES owners(id) ON DELETE CASCADE,
    home_junction_id     INTEGER   REFERENCES junctions(id) ON DELETE SET NULL,
    typical_route        INTEGER[] NOT NULL DEFAULT '{}',
    notification_opt_in  BOOLEAN   NOT NULL DEFAULT TRUE,
    created_at           TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
);

-- =============================================================================
-- ROAD SEGMENTS — directed edges between junctions
-- -----------------------------------------------------------------------------
-- Used by the route-guidance algorithm to build a graph of the city. Each row
-- represents one direction of travel; bidirectional roads need two rows.
-- =============================================================================
CREATE TABLE road_segments (
    id                SERIAL        PRIMARY KEY,
    from_junction_id  INTEGER       NOT NULL REFERENCES junctions(id) ON DELETE CASCADE,
    to_junction_id    INTEGER       NOT NULL REFERENCES junctions(id) ON DELETE CASCADE,
    name              VARCHAR(100)  NOT NULL DEFAULT '',
    distance_km       NUMERIC(6, 2) NOT NULL DEFAULT 1.0,
    speed_limit_kmh   INTEGER       NOT NULL DEFAULT 50,
    CHECK (from_junction_id <> to_junction_id)
);

-- =============================================================================
-- VEHICLES
-- =============================================================================
CREATE TABLE vehicles (
    id            SERIAL      PRIMARY KEY,
    number_plate  VARCHAR(10) UNIQUE NOT NULL,
    vehicle_type  VARCHAR(20) NOT NULL DEFAULT 'CAR',
    owner_id      INTEGER     REFERENCES owners(id) ON DELETE SET NULL,
    is_active     BOOLEAN     NOT NULL DEFAULT TRUE,
    created_at    TIMESTAMP   NOT NULL DEFAULT CURRENT_TIMESTAMP
);

-- =============================================================================
-- EMERGENCY VEHICLES
-- W15: Added plate column for automatic junction detection
-- =============================================================================
CREATE TABLE emergency_vehicles (
    id          SERIAL       PRIMARY KEY,
    type        VARCHAR(30)  NOT NULL,
    identifier  VARCHAR(50)  UNIQUE NOT NULL,
    plate       VARCHAR(20)  NULL,
    is_active   BOOLEAN      NOT NULL DEFAULT TRUE,
    created_at  TIMESTAMP    NOT NULL DEFAULT CURRENT_TIMESTAMP
);

-- =============================================================================
-- EMERGENCY EVENTS — active dispatches with a computed route
-- =============================================================================
CREATE TABLE emergency_events (
    id                   SERIAL      PRIMARY KEY,
    emergency_vehicle_id  INTEGER     NOT NULL REFERENCES emergency_vehicles(id) ON DELETE CASCADE,
    start_junction_id     INTEGER     NOT NULL REFERENCES junctions(id),
    target_junction_id    INTEGER     NOT NULL REFERENCES junctions(id),
    status               VARCHAR(20) NOT NULL DEFAULT 'ACTIVE',
    started_at            TIMESTAMP   NOT NULL DEFAULT CURRENT_TIMESTAMP,
    last_route            INTEGER[]   NOT NULL DEFAULT '{}',
    estimated_minutes     NUMERIC(10,2)
);

-- =============================================================================
-- JUNCTION SIGNALS — W15 traffic-signal control per junction
-- =============================================================================
CREATE TABLE junction_signals (
    junction_id         INTEGER     PRIMARY KEY REFERENCES junctions(id) ON DELETE CASCADE,
    state               VARCHAR(20) NOT NULL DEFAULT 'NORMAL',
    -- NORMAL | GREEN_PRIORITY
    emergency_event_id  INTEGER     NULL REFERENCES emergency_events(id) ON DELETE SET NULL,
    updated_at          TIMESTAMP   NOT NULL DEFAULT NOW()
);

-- =============================================================================
-- PLATE LOGS — every detection event at a junction
-- =============================================================================
CREATE TABLE plate_logs (
    id            SERIAL          PRIMARY KEY,
    number_plate  VARCHAR(10)     NOT NULL,
    vehicle_id    INTEGER         REFERENCES vehicles(id) ON DELETE SET NULL,
    junction_id   INTEGER         NOT NULL REFERENCES junctions(id),
    speed_kmh     NUMERIC(6,2),
    detected_at   TIMESTAMP       NOT NULL DEFAULT CURRENT_TIMESTAMP,
    status        VARCHAR(20)     NOT NULL DEFAULT 'REGISTERED',
    -- REGISTERED | UNREGISTERED
    event_type    VARCHAR(30)     NOT NULL DEFAULT 'PASSAGE'
    -- PASSAGE | SPEEDING | RED_LIGHT | PARKING
);

-- =============================================================================
-- VIOLATION TYPES — configurable catalogue
-- W13: Added auto_notify and requires_review for per-type configurable actions
-- =============================================================================
CREATE TABLE violation_types (
    id                  SERIAL          PRIMARY KEY,
    name                VARCHAR(100)    NOT NULL,
    code                VARCHAR(20)     UNIQUE NOT NULL,
    base_fine           NUMERIC(10,2)   NOT NULL,
    description         TEXT,
    is_active           BOOLEAN         NOT NULL DEFAULT TRUE,
    -- Per-severity fine multipliers — configurable without recompiling the backend
    multiplier_low      NUMERIC(4,2)    NOT NULL DEFAULT 1.00,
    multiplier_medium   NUMERIC(4,2)    NOT NULL DEFAULT 1.50,
    multiplier_high     NUMERIC(4,2)    NOT NULL DEFAULT 2.00,
    multiplier_critical NUMERIC(4,2)    NOT NULL DEFAULT 3.00,
    -- W13: Per-type configurable actions
    auto_notify         BOOLEAN         NOT NULL DEFAULT TRUE,
    requires_review     BOOLEAN         NOT NULL DEFAULT FALSE
);

-- =============================================================================
-- VIOLATIONS — one per detected offence
-- =============================================================================
CREATE TABLE violations (
    id                SERIAL      PRIMARY KEY,
    vehicle_id        INTEGER     NOT NULL REFERENCES vehicles(id),
    junction_id       INTEGER     REFERENCES junctions(id),
    violation_type_id INTEGER     NOT NULL REFERENCES violation_types(id),
    plate_log_id      INTEGER     REFERENCES plate_logs(id),
    detected_at       TIMESTAMP   NOT NULL DEFAULT CURRENT_TIMESTAMP,
    severity          VARCHAR(20) NOT NULL DEFAULT 'MEDIUM',
    -- LOW | MEDIUM | HIGH | CRITICAL
    evidence_note     TEXT,
    status            VARCHAR(20) NOT NULL DEFAULT 'OPEN'
    -- OPEN | REVIEW | PAID | CANCELLED | CONTESTED
);

-- =============================================================================
-- FINES — one-to-one with violations
-- =============================================================================
CREATE TABLE fines (
    id           SERIAL          PRIMARY KEY,
    violation_id INTEGER         UNIQUE NOT NULL REFERENCES violations(id),
    amount       NUMERIC(10,2)   NOT NULL,
    status       VARCHAR(20)     NOT NULL DEFAULT 'PENDING',
    -- PENDING | PAID | CANCELLED | OVERDUE
    issued_at    TIMESTAMP       NOT NULL DEFAULT CURRENT_TIMESTAMP,
    due_date     TIMESTAMP,
    paid_at      TIMESTAMP
);

-- =============================================================================
-- CONGESTION RECORDS — analysed traffic windows
-- =============================================================================
CREATE TABLE congestion_records (
    id                SERIAL      PRIMARY KEY,
    junction_id       INTEGER     NOT NULL REFERENCES junctions(id),
    time_window_start TIMESTAMP   NOT NULL,
    time_window_end   TIMESTAMP   NOT NULL,
    vehicle_count     INTEGER     NOT NULL DEFAULT 0,
    congestion_level  VARCHAR(20) NOT NULL DEFAULT 'LOW',
    -- LOW | MODERATE | HIGH | SEVERE
    recorded_at       TIMESTAMP   NOT NULL DEFAULT CURRENT_TIMESTAMP
);

-- =============================================================================
-- ROUTE ALTERNATIVES — pre-seeded reference data (kept for backwards compat)
-- =============================================================================
CREATE TABLE route_alternatives (
    id                       SERIAL  PRIMARY KEY,
    from_junction_id         INTEGER REFERENCES junctions(id),
    to_junction_id           INTEGER REFERENCES junctions(id),
    via_description          TEXT    NOT NULL,
    estimated_time_minutes   INTEGER
);

-- =============================================================================
-- NOTIFICATION LOGS — W13/W14/W15 auditable notification trail
-- =============================================================================
CREATE TABLE notification_logs (
    id         SERIAL       PRIMARY KEY,
    owner_id   INTEGER      REFERENCES owners(id),
    category   VARCHAR(50)  NOT NULL,
    -- FINE_ISSUED | CONGESTION_ALERT | EMERGENCY_ALERT | EMERGENCY_DETECTED | PAYMENT_CONFIRM
    subject    VARCHAR(200),
    message    TEXT         NOT NULL,
    sent_at    TIMESTAMP    NOT NULL DEFAULT CURRENT_TIMESTAMP,
    status     VARCHAR(20)  NOT NULL DEFAULT 'PENDING'
    -- PENDING | SENT | FAILED
);

-- =============================================================================
-- INDEXES
-- =============================================================================
CREATE INDEX idx_plate_logs_junction    ON plate_logs(junction_id);
CREATE INDEX idx_plate_logs_detected_at ON plate_logs(detected_at);
CREATE INDEX idx_plate_logs_plate       ON plate_logs(number_plate);
CREATE INDEX idx_violations_vehicle     ON violations(vehicle_id);
CREATE INDEX idx_violations_detected_at ON violations(detected_at);
CREATE INDEX idx_fines_status           ON fines(status);
CREATE INDEX idx_congestion_junction    ON congestion_records(junction_id);
CREATE INDEX idx_congestion_time        ON congestion_records(time_window_start);
CREATE INDEX idx_road_segments_from     ON road_segments(from_junction_id);
CREATE INDEX idx_road_segments_to       ON road_segments(to_junction_id);
CREATE INDEX idx_driver_profiles_owner  ON driver_profiles(owner_id);
CREATE INDEX idx_emergency_events_status ON emergency_events(status);
CREATE INDEX idx_notification_logs_sent ON notification_logs(sent_at DESC);
CREATE INDEX idx_em_vehicles_plate      ON emergency_vehicles(plate);
