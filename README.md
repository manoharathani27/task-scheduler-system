# Distributed Task Scheduler

A production-grade distributed task scheduler featuring a **C++ scheduling engine** with min-heap priority queue and 5 design patterns, a **Node.js REST API**, **GCP Pub/Sub** for reliable task delivery, and a **GitHub Actions CI/CD pipeline** deploying to **GCP Cloud Run**.

---

## Architecture

```
User/Service
     │
     ▼
┌─────────────────────┐
│  Node.js REST API   │  ← Create, pause, resume, delete tasks
│  (GCP Cloud Run)    │
└────────┬────────────┘
         │ Publish to
         ▼
┌─────────────────────┐
│  GCP Pub/Sub Topic  │  ← At-least-once message delivery
└────────┬────────────┘
         │ Subscribe
    ┌────┴─────┐
    ▼          ▼
┌────────┐ ┌──────────────────┐
│ Worker │ │ C++ Engine       │
│(Node)  │ │ Min-Heap         │  ← Picks next task O(1)
│Cloud   │ │ Scheduler        │  ← Reschedules O(log n)
│Run     │ │ (GCP Cloud Run)  │
└────────┘ └──────────────────┘
         │
         ▼
┌─────────────────────┐
│  PostgreSQL          │  ← Job registry + execution logs
│  (GCP Cloud SQL)    │
└─────────────────────┘
```

---

## Why Each Technology

| Technology | Why We Use It |
|---|---|
| **C++ Min-Heap** | O(1) next-task lookup, O(log n) insert — fastest possible scheduler |
| **GCP Pub/Sub** | At-least-once delivery — no task lost even if worker crashes |
| **GCP Cloud Run** | Serverless containers — auto-scales, pay per request |
| **GCP Cloud SQL** | Managed PostgreSQL — automatic backups and failover |
| **GCP Artifact Registry** | Private Docker registry — faster pulls from Cloud Run |
| **GCP Secret Manager** | Secure secrets — never in code or env files |
| **GitHub Actions** | CI/CD — automated test → build → deploy on every push |
| **Docker Multi-Stage** | Smaller images — faster Cloud Run cold starts |

---

## Design Patterns (C++ Engine)

| Pattern | Where | Why |
|---|---|---|
| **Singleton** | MinHeapScheduler, TaskRegistry | One shared instance — prevents duplicate execution |
| **Strategy** | HttpCallbackStrategy, PubSubStrategy | Swap execution method without changing scheduler |
| **Observer** | DatabaseObserver, LogObserver | Decouple task completion from side effects |
| **Factory** | TaskFactory | Complex task creation with validation in one place |
| **Command** | ExecuteTaskCommand | Encapsulate execution for retry, undo, logging |

---

## High Level Design Concepts

**At-least-once delivery** — GCP Pub/Sub redelivers if worker doesn't ACK within deadline. Worker uses idempotency keys (executionId) to skip duplicates.

**Fault tolerance** — If C++ engine crashes, it reloads pending tasks from PostgreSQL on restart. No task is lost.

**Horizontal scaling** — Multiple worker instances subscribe to the same Pub/Sub subscription. Messages are load-balanced across workers automatically.

**Idempotency** — Every task execution gets a unique `executionId = taskId + timestamp`. Database has a unique constraint on it — even if a message is delivered twice, the second insert fails silently.

---

## Project Structure

```
task-scheduler/
├── cpp-engine/                  ← C++ scheduling engine
│   ├── include/
│   │   ├── core/
│   │   │   ├── Task.h           ← Core domain entity
│   │   │   ├── CronParser.h     ← Cron expression parser
│   │   │   └── MinHeapScheduler.h
│   │   └── patterns/
│   │       └── DesignPatterns.h ← All 5 patterns declared
│   ├── src/
│   │   ├── core/
│   │   │   ├── CronParser.cpp
│   │   │   └── MinHeapScheduler.cpp
│   │   ├── patterns/
│   │   │   └── DesignPatterns.cpp
│   │   └── main.cpp
│   └── CMakeLists.txt
├── api/                         ← Node.js REST API
│   └── src/
│       ├── config/              ← DB, PubSub, Logger
│       ├── controllers/         ← Auth, Task, Execution
│       ├── middleware/          ← JWT auth
│       ├── models/              ← Task, ExecutionLog, User
│       └── routes/
├── worker/                      ← GCP Pub/Sub subscriber
│   └── src/worker.js
├── infrastructure/
│   ├── docker/                  ← Dockerfiles (API, Worker, C++)
│   └── gcp/setup-gcp.sh        ← One-time GCP setup script
├── .github/workflows/
│   └── deploy.yml               ← CI/CD pipeline
└── docker-compose.yml           ← Local development
```

---

## Getting Started

### Prerequisites
- Node.js v18+, Docker, GCP Account, `gcloud` CLI

### 1. Clone
```bash
git clone https://github.com/manoharathani27/task-scheduler.git
cd task-scheduler
```

### 2. Setup GCP (run once)
```bash
chmod +x infrastructure/gcp/setup-gcp.sh
./infrastructure/gcp/setup-gcp.sh YOUR_GCP_PROJECT_ID
```

### 3. Configure environment
```bash
cp .env.example .env
# Fill in GCP_PROJECT_ID, DB credentials, JWT_SECRET
```

### 4. Run locally
```bash
docker-compose up --build
```

### 5. Setup GitHub Secrets for CI/CD
In your GitHub repo → Settings → Secrets → Actions:
```
GCP_PROJECT_ID          = your-project-id
GCP_SERVICE_ACCOUNT     = github-actions-sa@your-project.iam.gserviceaccount.com
GCP_WORKLOAD_IDENTITY_PROVIDER = projects/NUMBER/locations/global/workloadIdentityPools/...
```

### 6. Deploy
```bash
git push origin main
# GitHub Actions auto-deploys to GCP Cloud Run
```

---

## API Reference

### Auth
```
POST /api/auth/register    Register user
POST /api/auth/login       Login, get JWT
```

### Tasks
```
POST   /api/tasks          Create task
GET    /api/tasks          List tasks
GET    /api/tasks/:id      Get task + recent executions
PUT    /api/tasks/:id      Update task
POST   /api/tasks/:id/pause   Pause task
POST   /api/tasks/:id/resume  Resume task
DELETE /api/tasks/:id      Delete task
```

### Executions
```
GET  /api/executions           List execution logs
GET  /api/executions/:id       Get execution detail
POST /api/executions/record    Record execution result (internal)
```

---

## Example Request

```json
POST /api/tasks
Authorization: Bearer <token>

{
  "name": "Daily Report",
  "cronExpression": "0 9 * * 1-5",
  "executionStrategy": "HTTP_CALLBACK",
  "callbackUrl": "https://your-service.com/reports/generate",
  "payload": { "type": "daily", "format": "pdf" },
  "maxRetries": 3
}
```

---

## CI/CD Pipeline

```
Push to main
     │
     ├── test-api (Jest + PostgreSQL)
     ├── build-cpp (cmake + make)
     │
     └── deploy (after both pass)
           ├── Build Docker images
           ├── Push to Artifact Registry
           ├── Deploy API → Cloud Run
           ├── Deploy Worker → Cloud Run
           └── Deploy C++ Engine → Cloud Run
```

---

## Author

**Manohar Mahadev Athani**
- GitHub: [@manoharathani27](https://github.com/manoharathani27)
- LinkedIn: [manoharathani](https://linkedin.com/in/manoharathani)
