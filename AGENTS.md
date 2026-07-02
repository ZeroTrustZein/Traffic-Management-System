<!-- AGENTS.md — instructions for AI coding agents working on this repo -->
# Agents Guide — Traffic Management System

Purpose: give a concise, actionable summary so an AI coding agent can be productive immediately in this repository.

- Checklist for the agent
  - Read the "big picture": `README.md` (Architecture). Crow C++ backend is the domain service; Django is a pure HTTP gateway/proxy.
  - Start the dev environment (docker or local build) and verify endpoints on :8080 (Crow) and :8000 (Django).
  - Follow the repository pattern in `backend/src/DataAccessLayer` and make logic changes in `backend/src/MicroserviceLayer`.

1) Big picture (what matters)
  - Architecture: Browser → Django Gateway (:8000) → Crow C++ REST server (:8080) → PostgreSQL (:5432).
    - See `README.md` and `backend/src/main.cpp` (route registrations) for concrete endpoints.
  - Single source of truth: Crow is the only service that talks to PostgreSQL. Django proxies and sends email only.
  - Controller vs Repository: business logic lives in `MicroserviceLayer/*Controller.cpp`, SQL lives in `DataAccessLayer/*Repository.*`.

2) How to run / debug (explicit commands & gotchas)
  - Docker (recommended, reproducible): from repo root

      docker compose up --build

    - Web UI: http://localhost:8000  Crow API: http://localhost:8080
    - DB auto-init from `database/schema.sql` + `database/seed.sql` on first run.
  - Local Windows native backend build (PowerShell): open `backend/` and run

      .\start-server.ps1

    - Script uses CMake + vcpkg toolchain. It sets `DATABASE_URL` interactively; `main.cpp` will throw if `DATABASE_URL` is missing.
  - Run C++ unit tests: CMake target `TrafficManagementBackendTests` is registered. After building locally run `ctest -C Release` or execute the test binary in `build/Release`.
  - Useful logs: `docker compose logs django` (email output), backend stdout prints "[API] Traffic Management Crow backend running on http://localhost:8080".

3) Project-specific patterns & conventions
  - Repository pattern: Each DB table has a single repository file (e.g. `VehicleRepository.*`, `OwnerRepository.*`) that owns SQL and mapping.
  - Controllers are thin orchestrators: they validate input (validators live under `MicroserviceLayer`), call repositories, compose `Response` objects.
  - Validators and business rules are plain C++ classes (e.g. `VehicleValidator.cpp`, `PlateValidator.cpp`, `ViolationRules.cpp`, `TrafficRules.cpp`). Unit tests assert their behaviour under `backend/tests/RulesTests.cpp`.
  - Django gateway uses `gateway_app/utils.py::CrowHttpClient` to call Crow. It expects JSON and gracefully maps connection errors to (503, {error}). Modify gateway calls via this client.
  - API key protection: check `gateway_app/middleware.py` for how `/api/` proxy endpoints are protected by `X-API-Key`.

4) Integration & external dependencies
  - C++ dependencies built via vcpkg (see `backend/vcpkg.json` and `docker/Dockerfile.crow`). Crow and libpqxx are used.
  - PostgreSQL is expected on `:5432` (containerised by docker-compose). Crow connects using `DATABASE_URL` env var (format shown in `backend/start-server.ps1`).
  - Django handles email (`gateway_app/email_service.py`) and uses console backend by default for dev.

5) Where to look for common changes (examples)
  - Add/change REST endpoints: modify `backend/src/main.cpp` (routes) and the corresponding `MicroserviceLayer/*Controller`.
  - Change schema or seed data: `database/schema.sql` and `database/seed.sql`.
  - Update domain models: `backend/src/Models.h`.
  - Tune congestion/violation logic: `backend/src/MicroserviceLayer/ViolationRules.cpp` and `TrafficRules.cpp` with tests under `backend/tests/RulesTests.cpp`.

6) Quick example requests
  - Register a vehicle (POST /api/vehicles): body (from README)

      { "number_plate": "AB12CDE", "vehicle_type": "CAR", "owner_name": "John Doe", "owner_email": "john@example.com", "owner_phone": "+60123456789" }

  - Detect violations (POST /api/violations/detect):

      { "junction_id": 2, "hours_back": 24 }

  - Crow error on missing DB: `main.cpp` throws with message starting "DATABASE_URL is not set. Example: host=localhost ..."

7) Safety for automated edits (rules for AI agents)
  - Do not change `gateway/` to query the DB directly. Preserve proxy pattern — change only if intentional and coordinated.
  - Keep SQL inside `DataAccessLayer` files. Avoid scattering SQL across controllers.
  - When altering business rules, update `backend/tests/RulesTests.cpp` to assert behaviour before committing.

References (high-value files to open first)
  - `README.md` (architecture + API reference)
  - `backend/src/main.cpp` (routes)
  - `backend/src/Models.h` (domain model)
  - `backend/src/DataAccessLayer/*` and `backend/src/MicroserviceLayer/*`
  - `backend/CMakeLists.txt`, `backend/start-server.ps1`, `docker/Dockerfile.crow`
  - `gateway/gateway_app/utils.py`, `gateway/gateway_app/middleware.py`, `gateway/gateway_app/email_service.py`

End of AGENTS.md

