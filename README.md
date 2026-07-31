# Traffic Management System

Submission-ready Programming Clinic project for automated traffic management using a layered architecture:
`client -> Django gateway -> Crow C++ backend -> PostgreSQL`.

The project is backend-first. The Django web interface is kept only as a minimal optional demo surface; the primary submission path is the Django gateway API.

**Stack:** C++ (Crow) · Django · PostgreSQL · Docker Compose

## Scope And Objectives

This repository implements Project 3 for `LZPMPC005L - Programming Clinic`, covering four weekly capabilities:

| Week | Capability |
|------|------------|
| `12` | Vehicle registration and plate recognition/logging |
| `13` | Traffic violation detection and fine issuing |
| `14` | Traffic flow analysis, congestion detection, and route advice |
| `15` | Emergency vehicle prioritisation and route clearing |

**Project objective:** provide JSON-based backend features through a Django gateway, backed by a separate SQL server, using a simple deployment path and clear validation.

## Functional Requirements

- Register vehicles with owner details and validate number plates.
- Log plate observations at junctions and preserve unregistered detections.
- Detect speeding, red-light, and parking violations from logged traffic events.
- Issue, pay, and cancel fines with configurable severity-based pricing.
- Analyse flow per junction, classify congestion, and predict near-term traffic levels.
- Recommend alternative routes and congestion-aware routes across the road network.
- Detect emergency vehicles, prioritise their route, and notify affected drivers.
- Send notification emails through Django in development and production-friendly modes.

## Non-Functional Requirements

- **Architecture:** Django is a thin gateway; Crow owns domain logic; PostgreSQL is the single source of truth.
- **Validation:** write operations reject malformed input with structured `400` responses.
- **Security:** configuration is environment-driven and SQL is parameterised.
- **Deployability:** `docker compose up --build` is the primary run path.
- **Testability:** backend business rules and Django gateway behaviour are covered by automated tests.
- **Maintainability:** SQL stays in the data access layer; business logic stays in service/controller classes.

## Architecture

```text
Client / Postman / Minimal Web UI
              |
              v  HTTP :8000
Django Gateway
  - API proxy
  - optional minimal web pages
  - email notifications
              |
              v  HTTP/JSON :8080
Crow C++ Backend
  - validation
  - business rules
  - repository-based data access
              |
              v
PostgreSQL :5432
```

**Key rule:** Django does not access PostgreSQL directly for domain data. All domain reads and writes flow through Crow.

## Technologies And Design Decisions

| Layer | Technology | Notes |
|------|------------|-------|
| Gateway | Django 5.2.15 | API proxy, optional minimal UI, email handling |
| Backend | Crow + C++17 + libpqxx | REST API and business logic |
| Database | PostgreSQL 15 | schema and seed data loaded automatically in Docker |
| Deployment | Docker Compose | primary lecturer-friendly run path |

Important design decisions:

- **Gateway-first access:** the official API path is `http://localhost:8000/api/...`
- **Optional UI only:** the frontend is deliberately small and not required to use the system
- **Repository pattern:** SQL stays in `backend/src/DataAccessLayer`
- **Email in Django:** notification delivery stays in Django rather than C++
- **Simple prediction logic:** congestion prediction uses a moving-average approach, not ML

## Quick Start

### Prerequisites

- Docker Desktop or Docker Engine with Compose v2

### Run The Project

```bash
git clone <repo-url>
cd <project-folder>
cp .env.example .env
docker compose up --build
```

Replace the placeholder secret values in `.env` before starting the stack.
If you prefer Windows automation, `.\start-all.ps1` copies `.env.example` and generates the local secret values for you.

| Service | URL | Purpose |
|--------|-----|---------|
| Django gateway API | `http://localhost:8000/api` | primary project interface |
| Optional Django UI | `http://localhost:8000` | lightweight demo only |
| Crow backend | `http://localhost:8080` | direct backend debug access |
| PostgreSQL | `localhost:5432` | database |

The database initialises automatically from `database/schema.sql` and `database/seed.sql`.

**First build note:** the Crow image compiles C++ dependencies with `vcpkg`, so the first build can take several minutes.

### Stop And Reset

```bash
docker compose down
docker compose down -v
```

### Windows Helper

You can also use:

```powershell
.\start-all.ps1
```

This still uses Docker Compose and remains a convenience wrapper only.

## Environment Variables

Copy `.env.example` to `.env`. The file contains safe placeholders, not live shared secrets.

| Variable | Default | Purpose |
|---------|---------|---------|
| `POSTGRES_DB` | `traffic_db` | database name |
| `POSTGRES_USER` | `postgres` | database user |
| `POSTGRES_PASSWORD` | generated locally | database password |
| `DJANGO_SECRET_KEY` | generated locally or replaced manually | Django secret key |
| `DJANGO_DEBUG` | `False` | Django debug mode |
| `ALLOWED_HOSTS` | `localhost,127.0.0.1` | allowed hosts |
| `DJANGO_API_KEY` | generated locally or replaced manually | API key for `/api/...` gateway access |
| `DEFAULT_FROM_EMAIL` | `traffic-dept@example.com` | sender address |
| `GOOGLE_MAPS_API_KEY` | empty | optional; default map rendering uses OSM/Leaflet |

For gateway API requests, send the key stored in your local `.env` file:

```text
X-API-Key: <your DJANGO_API_KEY value>
```

## Repository Structure

```text
project-root/
├── backend/
│   ├── src/
│   │   ├── DataAccessLayer/
│   │   ├── MicroserviceLayer/
│   │   ├── Models.h
│   │   ├── ValidationResult.h
│   │   └── main.cpp
│   ├── tests/RulesTests.cpp
│   ├── CMakeLists.txt
│   ├── start-server.ps1
│   └── vcpkg.json
├── gateway/
│   ├── gateway/
│   ├── gateway_app/
│   │   ├── api_urls.py
│   │   ├── api_views.py
│   │   ├── email_service.py
│   │   ├── middleware.py
│   │   ├── tests.py
│   │   ├── utils.py
│   │   ├── web_urls.py
│   │   └── web_views.py
│   ├── templates/
│   ├── manage.py
│   └── requirements.txt
├── database/
│   ├── schema.sql
│   └── seed.sql
├── docker/
│   ├── Dockerfile.crow
│   └── Dockerfile.django
├── docs/
│   ├── week1/
│   ├── week2/
│   ├── week3/
│   ├── week4/
├── postman/
├── docker-compose.yml
├── .env.example
├── .gitignore
└── README.md
```

## Weekly Deliverables In This Repo

- `docs/week1/` contains Week 12 UML artefacts
- `docs/week2/` contains Week 13 UML artefacts
- `docs/week3/` contains Week 14 UML artefacts
- `docs/week4/` contains Week 15 UML artefacts and the integrated class diagram

## Assessment Walkthrough

For lecturer marking, the fastest local validation path is:

1. Start the stack with `docker compose up --build`.
2. Open Postman with `postman/Traffic_Management_Collection.json` and `postman/Traffic_Management_Environment.json`.
3. Set `django_api_key` in the Postman environment to the `DJANGO_API_KEY` stored in your local `.env`.
4. Run the API requests through the Django gateway in this order:
   - Week 12: register a vehicle, list vehicles, log a plate at a junction
   - Week 13: detect a violation, inspect the created fine, pay or cancel a fine
   - Week 14: analyse traffic, inspect congestion records, view route recommendations
   - Week 15: list emergency vehicles, create an emergency event, inspect affected drivers
5. Optionally open the Django pages for a lightweight visual walkthrough only.

## API Summary

Use the Django gateway as the primary interface:

- `POST /api/vehicles`
- `GET /api/vehicles`
- `GET /api/vehicles/:plate`
- `POST /api/junctions`
- `GET /api/junctions`
- `POST /api/junctions/:id/log`
- `GET /api/junctions/:id/logs`
- `GET /api/plate-logs`
- `POST /api/violations/detect`
- `GET /api/violations`
- `GET /api/violations/:id`
- `GET /api/violation-types`
- `POST /api/violations/:id/fine`
- `GET /api/fines`
- `GET /api/fines/:id`
- `POST /api/fines/:id/pay`
- `POST /api/fines/:id/cancel`
- `POST /api/traffic/analyze`
- `GET /api/traffic/congestion`
- `GET /api/traffic/flow`
- `GET /api/traffic/flow/hourly`
- `GET /api/traffic/congestion-prone`
- `GET /api/traffic/predict/:junction_id`
- `GET /api/traffic/routes/:junction_id`
- `POST /api/traffic/route`
- `POST /api/traffic/tour`
- `GET /api/traffic/network`
- `GET /api/emergency/vehicles`
- `POST /api/emergency/vehicles`
- `POST /api/emergency/events`
- `GET /api/emergency/events/:id`
- `GET /api/emergency/events/:id/affected-drivers`
- `POST /api/emergency/events/:id/resolve`
- `GET /api/signals`
- `GET /api/notifications`

## User Guidance

Recommended usage for a marker:

1. Start the stack with Docker Compose.
2. Use Postman or curl against `http://localhost:8000/api/...` with the `X-API-Key` header from your local `.env`.
3. Verify the weekly flows in order:
   - vehicle registration and plate logging
   - violation detection and fines
   - congestion analysis and routing
   - emergency prioritisation and affected-driver notifications
4. Use the optional Django pages only if you want a visual walkthrough.

## Testing And Validation

### Backend Unit Tests

```bash
cd backend
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

These cover pure business rules such as validation, violation severity, and congestion classification.

### Django Gateway Tests

```bash
cd gateway
python manage.py test
```

These cover the web layer and the gateway API proxy path.

### Postman Validation

Import:

- `postman/Traffic_Management_Collection.json`
- `postman/Traffic_Management_Environment.json`

The collection is prepared for the Django gateway path; run the end-to-end workflow folder in sequence.
If you use the Django gateway path, include the `X-API-Key` header from your local `.env`.

### Manual Endpoint Validation

Use Postman or curl against the gateway API and include the `X-API-Key` value from your local `.env`, then verify the weekly flows in this order:

1. register a vehicle
2. log a number plate at a junction
3. detect a violation and issue a fine
4. pay or cancel a fine
5. analyse congestion and inspect route suggestions
6. create an emergency event and inspect affected drivers

## Database Model Summary

Main tables currently implemented:

- `owners`
- `driver_profiles`
- `junctions`
- `road_segments`
- `vehicles`
- `emergency_vehicles`
- `emergency_events`
- `junction_signals`
- `plate_logs`
- `violation_types`
- `violations`
- `fines`
- `congestion_records`
- `route_alternatives`
- `notification_logs`

The full authoritative schema is in `database/schema.sql`.

## UML And Documentation

Submission-relevant artefacts currently stored in the repo:

- weekly UML PNG folders under `docs/week1` to `docs/week4`
- integrated class diagram PNG in `docs/week4`
- Postman collection and environment in `postman/`

## Optional Web UI

The Django UI is intentionally minimal and outside the core project scope. It remains in the repo for demonstration only.

Available optional pages:

- `/`
- `/vehicles/`
- `/vehicles/register/`
- `/vehicles/<plate>/`
- `/plate-logs/`
- `/violations/`
- `/fines/`
- `/traffic/flow/`
- `/traffic/congestion/`
- `/traffic/map/`
- `/traffic/route/`
- `/emergency/`

## Known Limitations

- No real ANPR hardware integration; plate observations are simulated through API or form input.
- Congestion prediction uses a simple moving-average approach rather than a learning model.
- The optional UI is not the primary assessed surface and is intentionally lightweight.
- Advanced route-planning helpers are internal implementation support and do not change the submission-facing use cases.

## Submission Notes

- Primary lecturer run path: Docker Compose
- Primary lecturer interaction path: Django gateway API on port `8000`
- Optional demo surface: minimal Django pages
