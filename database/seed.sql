-- =============================================================================
-- Traffic Management System — Seed Data
-- =============================================================================
-- Run AFTER schema.sql:
--   psql -U postgres -d traffic_db -f database/seed.sql
-- =============================================================================

-- Owners
INSERT INTO owners (full_name, email, phone) VALUES
('Alex Carter',      'alex.carter@example.com',    '+1-555-0101'),
('Bob Williams',     'bob.williams@example.com',   '+1-555-0102'),
('Carol Davis',      'carol.davis@example.com',    '+1-555-0103'),
('David Martinez',   'david.martinez@example.com', '+1-555-0104'),
('Emma Wilson',      'emma.wilson@example.com',    '+1-555-0105');

-- Junctions
-- Seed locations are real intersections in Leipzig, Germany so the map view
-- renders something meaningful out of the box. Coordinates are WGS84
-- decimal degrees.
INSERT INTO junctions (name, location, speed_limit_kmh, latitude, longitude) VALUES
('Junction 1 - Augustusplatz',     'Augustusplatz (Opernhaus / Gewandhaus)',     50, 51.3387, 12.3811),
('Junction 2 - Hauptbahnhof',      'Willy-Brandt-Platz (Hauptbahnhof)',          50, 51.3447, 12.3826),
('Junction 3 - Bayerischer Bahnhof','Bayrischer Platz / Nuernberger Strasse',    50, 51.3270, 12.3850),
('Junction 4 - Plagwitz',          'Karl-Heine-Strasse / Erich-Zeigner-Allee',   50, 51.3290, 12.3380),
('Junction 5 - Connewitz Kreuz',   'Wolfgang-Heinze-Strasse / Bornaische Str.',  50, 51.3060, 12.3760);

-- Driver profiles (simple notification preferences + typical route)
INSERT INTO driver_profiles (owner_id, home_junction_id, typical_route, notification_opt_in) VALUES
(1, 1, '{1,2,3}', TRUE),
(2, 2, '{2,1,4}', TRUE),
(3, 3, '{3,5,1}', FALSE),
(4, 4, '{4,1,2}', TRUE),
(5, 5, '{5,3,2}', TRUE);

-- Road segments — directed graph used by the route-guidance algorithm.
-- Each undirected road is stored as two rows so traversal works either way.
-- Distances are approximate driving distances between the Leipzig junctions
-- above; speed limits reflect typical inner-Leipzig posted limits.
INSERT INTO road_segments (from_junction_id, to_junction_id, name, distance_km, speed_limit_kmh) VALUES
(1, 2, 'Goethestrasse',        0.7, 50),
(2, 1, 'Goethestrasse',        0.7, 50),
(1, 3, 'Windmuehlenstrasse',   1.3, 50),
(3, 1, 'Windmuehlenstrasse',   1.3, 50),
(1, 4, 'Jahnallee',            3.2, 50),
(4, 1, 'Jahnallee',            3.2, 50),
(2, 3, 'Innerer Ring (Ost)',   2.0, 50),
(3, 2, 'Innerer Ring (Ost)',   2.0, 50),
(3, 5, 'Karl-Liebknecht-Str.', 2.5, 50),
(5, 3, 'Karl-Liebknecht-Str.', 2.5, 50),
(4, 5, 'Antonienstrasse',      3.0, 50),
(5, 4, 'Antonienstrasse',      3.0, 50),
(1, 5, 'Harkortstr./KarLi',    3.7, 50),
(5, 1, 'Harkortstr./KarLi',    3.7, 50);

-- Vehicles
INSERT INTO vehicles (number_plate, vehicle_type, owner_id) VALUES
('AB12CDE', 'CAR',        1),
('XY99ZZZ', 'TRUCK',      2),
('LM34NOP', 'MOTORCYCLE', 3),
('QR56STU', 'CAR',        4),
('VW78XYZ', 'BUS',        5);

-- Emergency vehicles — W15: added plate column; include plates so junction
-- cameras can automatically detect emergency vehicles by number plate.
-- At least one vehicle of vehicle_type='EMERGENCY' must also exist in the
-- vehicles table for feature-2 plate matching demo. We seed two plates here.
INSERT INTO emergency_vehicles (type, identifier, plate, is_active) VALUES
('AMBULANCE', 'AMB-001',  'EM01AMB', TRUE),
('FIRE',      'FIRE-001', 'EM02FIR', TRUE),
('POLICE',    'POL-001',  NULL,      TRUE);

-- Seed an EMERGENCY-type vehicle in the main vehicles table so the
-- junction logVehicle auto-detects it by vehicleType as well.
INSERT INTO vehicles (number_plate, vehicle_type, owner_id) VALUES
('EM01AMB', 'EMERGENCY', NULL);

INSERT INTO emergency_events (
    emergency_vehicle_id, start_junction_id, target_junction_id,
    status, last_route, estimated_minutes
) VALUES
(1, 2, 5, 'ACTIVE', '{2,3,5}', 9.0);

-- Junction signals — W15: one NORMAL row per seeded junction.
-- These are managed by the backend; seed them as NORMAL.
INSERT INTO junction_signals (junction_id, state) VALUES
(1, 'NORMAL'),
(2, 'NORMAL'),
(3, 'NORMAL'),
(4, 'NORMAL'),
(5, 'NORMAL');

-- Violation types — W13: include auto_notify and requires_review flags.
-- Different flags demonstrate per-type configurable actions:
--   SPEEDING   : default (auto_notify=true, requires_review=false)
--   PARKING    : auto_notify=false (quiet offence, no immediate email)
--   RED_LIGHT  : requires_review=true (serious — needs manual sign-off before fine)
--   UNREGISTERED: auto_notify=true, requires_review=false
--   RECKLESS   : auto_notify=true, requires_review=true (most serious)
INSERT INTO violation_types (name, code, base_fine, description, auto_notify, requires_review) VALUES
('Speeding',            'SPEEDING',     150.00, 'Vehicle detected exceeding the posted speed limit',  TRUE,  FALSE),
('Illegal Parking',     'PARKING',       80.00, 'Vehicle parked in a prohibited zone',               FALSE, FALSE),
('Running Red Light',   'RED_LIGHT',    200.00, 'Vehicle passed through a red traffic signal',        TRUE,  TRUE),
('Unregistered Vehicle','UNREGISTERED', 300.00, 'Vehicle not found in the registration database',    TRUE,  FALSE),
('Reckless Driving',    'RECKLESS',     500.00, 'Dangerous or reckless vehicle operation',           TRUE,  TRUE);

-- Route alternatives (congestion diversions) — kept for backwards compat
INSERT INTO route_alternatives (from_junction_id, to_junction_id, via_description, estimated_time_minutes) VALUES
(1, 2, 'Take Park Ave north to Elm St, then join North Blvd eastbound',    12),
(2, 1, 'Take Ring Rd south to Oak St, then Main St westbound',             14),
(1, 3, 'Take River Lane east — bypasses City Centre entirely',             18),
(3, 4, 'Take Bridge St west, then Market Ave south to West Market',        10),
(4, 5, 'Take Commercial Rd south and join Terminal Blvd',                  15),
(2, 3, 'Take Elm St east directly to East Bridge',                          9),
(5, 1, 'Take Terminal Blvd north, join Ring Rd, then south via Main St',   20);

-- =============================================================================
-- Sample plate logs (spread over the past 3 hours for dashboard data)
-- =============================================================================
INSERT INTO plate_logs (number_plate, vehicle_id, junction_id, speed_kmh, detected_at, status, event_type) VALUES
('AB12CDE', 1, 1, 48.0,  NOW() - INTERVAL '3 hours',           'REGISTERED',   'PASSAGE'),
('XY99ZZZ', 2, 1, 55.0,  NOW() - INTERVAL '2 hours 50 minutes','REGISTERED',   'PASSAGE'),
('LM34NOP', 3, 2, 62.0,  NOW() - INTERVAL '2 hours 40 minutes','REGISTERED',   'PASSAGE'),
('AB12CDE', 1, 2, 78.0,  NOW() - INTERVAL '2 hours 30 minutes','REGISTERED',   'SPEEDING'),
('QR56STU', 4, 3, 72.0,  NOW() - INTERVAL '2 hours 20 minutes','REGISTERED',   'PASSAGE'),
('VW78XYZ', 5, 4, 47.0,  NOW() - INTERVAL '2 hours 10 minutes','REGISTERED',   'PASSAGE'),
('LM34NOP', 3, 3, 88.0,  NOW() - INTERVAL '2 hours',           'REGISTERED',   'SPEEDING'),
('ZZ00UNK', NULL, 1, NULL,NOW() - INTERVAL '1 hour 50 minutes','UNREGISTERED', 'PASSAGE'),
('AB12CDE', 1, 1, NULL,  NOW() - INTERVAL '1 hour 40 minutes', 'REGISTERED',   'RED_LIGHT'),
('QR56STU', 4, 2, NULL,  NOW() - INTERVAL '1 hour 30 minutes', 'REGISTERED',   'PARKING'),
('XY99ZZZ', 2, 5, 58.0,  NOW() - INTERVAL '1 hour 20 minutes', 'REGISTERED',   'PASSAGE'),
('VW78XYZ', 5, 5, 61.0,  NOW() - INTERVAL '1 hour 10 minutes', 'REGISTERED',   'PASSAGE'),
('AB12CDE', 1, 4, 52.0,  NOW() - INTERVAL '1 hour',            'REGISTERED',   'PASSAGE'),
('LM34NOP', 3, 1, 49.0,  NOW() - INTERVAL '50 minutes',        'REGISTERED',   'PASSAGE'),
('QR56STU', 4, 1, 51.0,  NOW() - INTERVAL '40 minutes',        'REGISTERED',   'PASSAGE'),
('XY99ZZZ', 2, 2, 63.0,  NOW() - INTERVAL '30 minutes',        'REGISTERED',   'PASSAGE'),
('VW78XYZ', 5, 3, 69.0,  NOW() - INTERVAL '20 minutes',        'REGISTERED',   'PASSAGE'),
('AB12CDE', 1, 3, 48.0,  NOW() - INTERVAL '10 minutes',        'REGISTERED',   'PASSAGE'),
('LM34NOP', 3, 4, NULL,  NOW() - INTERVAL '5 minutes',         'REGISTERED',   'PARKING');

-- Extra plate logs for junction 1 — pushes 24-hour count above 30 (SEVERE threshold)
INSERT INTO plate_logs (number_plate, vehicle_id, junction_id, speed_kmh, detected_at, status, event_type) VALUES
('AB12CDE', 1, 1, 47.0, NOW() - INTERVAL '23 hours',           'REGISTERED', 'PASSAGE'),
('XY99ZZZ', 2, 1, 52.0, NOW() - INTERVAL '22 hours 30 minutes','REGISTERED', 'PASSAGE'),
('LM34NOP', 3, 1, 49.0, NOW() - INTERVAL '22 hours',           'REGISTERED', 'PASSAGE'),
('QR56STU', 4, 1, 51.0, NOW() - INTERVAL '21 hours 30 minutes','REGISTERED', 'PASSAGE'),
('VW78XYZ', 5, 1, 48.0, NOW() - INTERVAL '21 hours',           'REGISTERED', 'PASSAGE'),
('AB12CDE', 1, 1, 53.0, NOW() - INTERVAL '20 hours',           'REGISTERED', 'PASSAGE'),
('XY99ZZZ', 2, 1, 50.0, NOW() - INTERVAL '19 hours 30 minutes','REGISTERED', 'PASSAGE'),
('LM34NOP', 3, 1, 46.0, NOW() - INTERVAL '19 hours',           'REGISTERED', 'PASSAGE'),
('QR56STU', 4, 1, 54.0, NOW() - INTERVAL '18 hours',           'REGISTERED', 'PASSAGE'),
('VW78XYZ', 5, 1, 47.0, NOW() - INTERVAL '17 hours 30 minutes','REGISTERED', 'PASSAGE'),
('AB12CDE', 1, 1, 49.0, NOW() - INTERVAL '17 hours',           'REGISTERED', 'PASSAGE'),
('XY99ZZZ', 2, 1, 51.0, NOW() - INTERVAL '16 hours',           'REGISTERED', 'PASSAGE'),
('LM34NOP', 3, 1, 48.0, NOW() - INTERVAL '15 hours 30 minutes','REGISTERED', 'PASSAGE'),
('QR56STU', 4, 1, 52.0, NOW() - INTERVAL '15 hours',           'REGISTERED', 'PASSAGE'),
('VW78XYZ', 5, 1, 50.0, NOW() - INTERVAL '14 hours',           'REGISTERED', 'PASSAGE'),
('AB12CDE', 1, 1, 47.0, NOW() - INTERVAL '13 hours',           'REGISTERED', 'PASSAGE'),
('XY99ZZZ', 2, 1, 53.0, NOW() - INTERVAL '12 hours 30 minutes','REGISTERED', 'PASSAGE'),
('LM34NOP', 3, 1, 49.0, NOW() - INTERVAL '12 hours',           'REGISTERED', 'PASSAGE'),
('QR56STU', 4, 1, 51.0, NOW() - INTERVAL '11 hours',           'REGISTERED', 'PASSAGE'),
('VW78XYZ', 5, 1, 48.0, NOW() - INTERVAL '10 hours 30 minutes','REGISTERED', 'PASSAGE'),
('AB12CDE', 1, 1, 50.0, NOW() - INTERVAL '10 hours',           'REGISTERED', 'PASSAGE'),
('XY99ZZZ', 2, 1, 52.0, NOW() - INTERVAL '9 hours',            'REGISTERED', 'PASSAGE'),
('LM34NOP', 3, 1, 47.0, NOW() - INTERVAL '8 hours 30 minutes', 'REGISTERED', 'PASSAGE'),
('QR56STU', 4, 1, 54.0, NOW() - INTERVAL '8 hours',            'REGISTERED', 'PASSAGE');

-- Sample violations (auto-detected from the logs above)
INSERT INTO violations (vehicle_id, junction_id, violation_type_id, plate_log_id, detected_at, severity, evidence_note, status) VALUES
(1, 2, 1, 4,  NOW() - INTERVAL '2 hours 30 minutes', 'HIGH',     'Speed: 78 km/h in 50 km/h zone',    'OPEN'),
(3, 3, 1, 7,  NOW() - INTERVAL '2 hours',            'HIGH',     'Speed: 88 km/h in 50 km/h zone',    'OPEN'),
(1, 1, 3, 9,  NOW() - INTERVAL '1 hour 40 minutes',  'CRITICAL', 'Passed red light at Junction 1',    'REVIEW'),
(4, 2, 2, 10, NOW() - INTERVAL '1 hour 30 minutes',  'MEDIUM',   'Parked in no-parking zone',         'OPEN'),
(3, 4, 2, 19, NOW() - INTERVAL '5 minutes',          'MEDIUM',   'Parked in no-parking zone',         'OPEN');

-- Sample fines (note: violation 3 is REVIEW so no fine yet)
INSERT INTO fines (violation_id, amount, status, issued_at, due_date) VALUES
(1, 225.00, 'PENDING', NOW() - INTERVAL '2 hours 25 minutes', NOW() + INTERVAL '30 days'),
(2, 225.00, 'PENDING', NOW() - INTERVAL '1 hour 55 minutes',  NOW() + INTERVAL '30 days'),
(4, 120.00, 'PAID',    NOW() - INTERVAL '1 hour 25 minutes',  NOW() + INTERVAL '30 days'),
(5, 120.00, 'PENDING', NOW() - INTERVAL '2 minutes',          NOW() + INTERVAL '30 days');

-- Recent congestion records (past 6 hours, used by live dashboard)
INSERT INTO congestion_records (junction_id, time_window_start, time_window_end, vehicle_count, congestion_level, recorded_at) VALUES
(1, NOW() - INTERVAL '6 hours', NOW() - INTERVAL '5 hours', 12, 'MODERATE', NOW() - INTERVAL '5 hours'),
(1, NOW() - INTERVAL '5 hours', NOW() - INTERVAL '4 hours', 19, 'MODERATE', NOW() - INTERVAL '4 hours'),
(1, NOW() - INTERVAL '4 hours', NOW() - INTERVAL '3 hours', 28, 'HIGH',     NOW() - INTERVAL '3 hours'),
(1, NOW() - INTERVAL '3 hours', NOW() - INTERVAL '2 hours', 35, 'SEVERE',   NOW() - INTERVAL '2 hours'),
(1, NOW() - INTERVAL '2 hours', NOW() - INTERVAL '1 hour',  22, 'HIGH',     NOW() - INTERVAL '1 hour'),
(1, NOW() - INTERVAL '1 hour',  NOW(),                       8, 'LOW',      NOW()),
(2, NOW() - INTERVAL '6 hours', NOW() - INTERVAL '5 hours', 8,  'LOW',      NOW() - INTERVAL '5 hours'),
(2, NOW() - INTERVAL '5 hours', NOW() - INTERVAL '4 hours', 14, 'MODERATE', NOW() - INTERVAL '4 hours'),
(2, NOW() - INTERVAL '4 hours', NOW() - INTERVAL '3 hours', 18, 'MODERATE', NOW() - INTERVAL '3 hours'),
(2, NOW() - INTERVAL '3 hours', NOW() - INTERVAL '2 hours', 25, 'HIGH',     NOW() - INTERVAL '2 hours'),
(3, NOW() - INTERVAL '4 hours', NOW() - INTERVAL '3 hours', 6,  'LOW',      NOW() - INTERVAL '3 hours'),
(3, NOW() - INTERVAL '3 hours', NOW() - INTERVAL '2 hours', 11, 'MODERATE', NOW() - INTERVAL '2 hours'),
(4, NOW() - INTERVAL '4 hours', NOW() - INTERVAL '3 hours', 22, 'HIGH',     NOW() - INTERVAL '3 hours'),
(4, NOW() - INTERVAL '3 hours', NOW() - INTERVAL '2 hours', 31, 'SEVERE',   NOW() - INTERVAL '2 hours'),
(4, NOW() - INTERVAL '2 hours', NOW() - INTERVAL '1 hour',  17, 'MODERATE', NOW() - INTERVAL '1 hour'),
(5, NOW() - INTERVAL '3 hours', NOW() - INTERVAL '2 hours', 9,  'LOW',      NOW() - INTERVAL '2 hours'),
(5, NOW() - INTERVAL '2 hours', NOW() - INTERVAL '1 hour',  16, 'MODERATE', NOW() - INTERVAL '1 hour');

-- Historical hourly pattern (yesterday) — gives the hourly profile chart real rush-hour shape
-- Junction 1 - City Centre (busiest junction)
INSERT INTO congestion_records (junction_id, time_window_start, time_window_end, vehicle_count, congestion_level, recorded_at) VALUES
(1, CURRENT_DATE - 1 + TIME '06:00', CURRENT_DATE - 1 + TIME '07:00',  4, 'LOW',      CURRENT_DATE - 1 + TIME '07:00'),
(1, CURRENT_DATE - 1 + TIME '07:00', CURRENT_DATE - 1 + TIME '08:00', 14, 'MODERATE', CURRENT_DATE - 1 + TIME '08:00'),
(1, CURRENT_DATE - 1 + TIME '08:00', CURRENT_DATE - 1 + TIME '09:00', 28, 'HIGH',     CURRENT_DATE - 1 + TIME '09:00'),
(1, CURRENT_DATE - 1 + TIME '09:00', CURRENT_DATE - 1 + TIME '10:00', 22, 'HIGH',     CURRENT_DATE - 1 + TIME '10:00'),
(1, CURRENT_DATE - 1 + TIME '10:00', CURRENT_DATE - 1 + TIME '11:00',  9, 'LOW',      CURRENT_DATE - 1 + TIME '11:00'),
(1, CURRENT_DATE - 1 + TIME '11:00', CURRENT_DATE - 1 + TIME '12:00',  7, 'LOW',      CURRENT_DATE - 1 + TIME '12:00'),
(1, CURRENT_DATE - 1 + TIME '12:00', CURRENT_DATE - 1 + TIME '13:00', 15, 'MODERATE', CURRENT_DATE - 1 + TIME '13:00'),
(1, CURRENT_DATE - 1 + TIME '13:00', CURRENT_DATE - 1 + TIME '14:00', 12, 'MODERATE', CURRENT_DATE - 1 + TIME '14:00'),
(1, CURRENT_DATE - 1 + TIME '14:00', CURRENT_DATE - 1 + TIME '15:00',  7, 'LOW',      CURRENT_DATE - 1 + TIME '15:00'),
(1, CURRENT_DATE - 1 + TIME '15:00', CURRENT_DATE - 1 + TIME '16:00',  6, 'LOW',      CURRENT_DATE - 1 + TIME '16:00'),
(1, CURRENT_DATE - 1 + TIME '16:00', CURRENT_DATE - 1 + TIME '17:00', 16, 'MODERATE', CURRENT_DATE - 1 + TIME '17:00'),
(1, CURRENT_DATE - 1 + TIME '17:00', CURRENT_DATE - 1 + TIME '18:00', 27, 'HIGH',     CURRENT_DATE - 1 + TIME '18:00'),
(1, CURRENT_DATE - 1 + TIME '18:00', CURRENT_DATE - 1 + TIME '19:00', 38, 'SEVERE',   CURRENT_DATE - 1 + TIME '19:00'),
(1, CURRENT_DATE - 1 + TIME '19:00', CURRENT_DATE - 1 + TIME '20:00', 24, 'HIGH',     CURRENT_DATE - 1 + TIME '20:00'),
(1, CURRENT_DATE - 1 + TIME '20:00', CURRENT_DATE - 1 + TIME '21:00', 11, 'MODERATE', CURRENT_DATE - 1 + TIME '21:00'),
(1, CURRENT_DATE - 1 + TIME '21:00', CURRENT_DATE - 1 + TIME '22:00',  6, 'LOW',      CURRENT_DATE - 1 + TIME '22:00'),
(1, CURRENT_DATE - 1 + TIME '22:00', CURRENT_DATE - 1 + TIME '23:00',  3, 'LOW',      CURRENT_DATE - 1 + TIME '23:00');

-- Junction 2 - North Gate
INSERT INTO congestion_records (junction_id, time_window_start, time_window_end, vehicle_count, congestion_level, recorded_at) VALUES
(2, CURRENT_DATE - 1 + TIME '07:00', CURRENT_DATE - 1 + TIME '08:00', 10, 'MODERATE', CURRENT_DATE - 1 + TIME '08:00'),
(2, CURRENT_DATE - 1 + TIME '08:00', CURRENT_DATE - 1 + TIME '09:00', 18, 'MODERATE', CURRENT_DATE - 1 + TIME '09:00'),
(2, CURRENT_DATE - 1 + TIME '09:00', CURRENT_DATE - 1 + TIME '10:00', 15, 'MODERATE', CURRENT_DATE - 1 + TIME '10:00'),
(2, CURRENT_DATE - 1 + TIME '12:00', CURRENT_DATE - 1 + TIME '13:00',  9, 'LOW',      CURRENT_DATE - 1 + TIME '13:00'),
(2, CURRENT_DATE - 1 + TIME '17:00', CURRENT_DATE - 1 + TIME '18:00', 21, 'HIGH',     CURRENT_DATE - 1 + TIME '18:00'),
(2, CURRENT_DATE - 1 + TIME '18:00', CURRENT_DATE - 1 + TIME '19:00', 26, 'HIGH',     CURRENT_DATE - 1 + TIME '19:00'),
(2, CURRENT_DATE - 1 + TIME '19:00', CURRENT_DATE - 1 + TIME '20:00', 16, 'MODERATE', CURRENT_DATE - 1 + TIME '20:00');

-- Junction 4 - West Market (second-busiest)
INSERT INTO congestion_records (junction_id, time_window_start, time_window_end, vehicle_count, congestion_level, recorded_at) VALUES
(4, CURRENT_DATE - 1 + TIME '08:00', CURRENT_DATE - 1 + TIME '09:00', 20, 'HIGH',     CURRENT_DATE - 1 + TIME '09:00'),
(4, CURRENT_DATE - 1 + TIME '09:00', CURRENT_DATE - 1 + TIME '10:00', 17, 'MODERATE', CURRENT_DATE - 1 + TIME '10:00'),
(4, CURRENT_DATE - 1 + TIME '17:00', CURRENT_DATE - 1 + TIME '18:00', 24, 'HIGH',     CURRENT_DATE - 1 + TIME '18:00'),
(4, CURRENT_DATE - 1 + TIME '18:00', CURRENT_DATE - 1 + TIME '19:00', 33, 'SEVERE',   CURRENT_DATE - 1 + TIME '19:00'),
(4, CURRENT_DATE - 1 + TIME '19:00', CURRENT_DATE - 1 + TIME '20:00', 18, 'MODERATE', CURRENT_DATE - 1 + TIME '20:00');
