# 🚀 Distributed Task Scheduler

![CI/CD](https://github.com/manoharathani27/task-scheduler-system/actions/workflows/deploy.yml/badge.svg)
![C++](https://img.shields.io/badge/C++-17-blue)
![Node.js](https://img.shields.io/badge/Node.js-20-green)
![PostgreSQL](https://img.shields.io/badge/PostgreSQL-15-blue)
![Docker](https://img.shields.io/badge/Docker-Ready-2496ED)
![Google Cloud](https://img.shields.io/badge/Google%20Cloud-Cloud%20Run-orange)
![License](https://img.shields.io/badge/license-MIT-green)

A **production-grade distributed task scheduler** built using **C++**, **Node.js**, and **Google Cloud Platform**.

The system combines a **high-performance C++ scheduling engine**, **Node.js microservices**, **Google Cloud Pub/Sub**, **Cloud SQL**, and a fully automated **GitHub Actions CI/CD pipeline** to provide reliable, scalable, fault-tolerant task scheduling.

---

# 📌 Features

- JWT Authentication
- RESTful Task Management API
- Cron Expression Scheduling
- High-performance C++ Min Heap Scheduler
- HTTP Callback Execution
- Google Cloud Pub/Sub Integration
- Retry Mechanism
- Idempotent Task Execution
- Execution History
- PostgreSQL Storage
- Google Cloud SQL Integration
- Dockerized Microservices
- Google Cloud Run Deployment
- Secret Manager Integration
- GitHub Actions CI/CD
- Production Ready Architecture

---

# 🏗 System Architecture

```
                        +----------------------+
                        |      Client/User     |
                        +----------+-----------+
                                   |
                                   |
                           REST API Requests
                                   |
                                   ▼
                   +-----------------------------+
                   |      Node.js REST API       |
                   |      Google Cloud Run       |
                   +-------------+---------------+
                                 |
                                 |
                           Publish Tasks
                                 |
                                 ▼
                   +-----------------------------+
                   |      Google Pub/Sub         |
                   +-------------+---------------+
                                 |
                     Pull Messages
                                 |
             +-------------------+------------------+
             |                                      |
             ▼                                      ▼
+--------------------------+          +-----------------------------+
|     Worker Service       |          |     C++ Scheduling Engine   |
|    Google Cloud Run      |          |     Min Heap Scheduler      |
+-------------+------------+          +-------------+---------------+
              |                                      |
              +-------------------+------------------+
                                  |
                                  ▼
                    +------------------------------+
                    | PostgreSQL (Cloud SQL)       |
                    | Task Registry & Execution    |
                    +------------------------------+
```

---

# ☁️ Cloud Architecture

```
Developer
     │
     ▼
GitHub Repository
     │
     ▼
GitHub Actions
     │
     ▼
Artifact Registry
     │
     ├────────────► API (Cloud Run)
     │                     │
     │                     ▼
     │                Cloud SQL
     │
     └────────────► Worker (Cloud Run)
                           │
                           ▼
                       Pub/Sub
```

---

# 💻 Tech Stack

## Backend

- Node.js
- Express.js
- Sequelize ORM
- JWT Authentication
- PostgreSQL

## Scheduler Engine

- C++17
- STL
- Min Heap Priority Queue
- Custom Cron Parser
- Object-Oriented Design

## Cloud

- Google Cloud Run
- Google Cloud SQL
- Google Pub/Sub
- Google Secret Manager
- Google Artifact Registry

## DevOps

- Docker
- Docker Compose
- GitHub Actions
- Workload Identity Federation

---

# 🎯 Design Patterns Used

| Pattern | Purpose |
|----------|---------|
| Singleton | Scheduler & Registry instance management |
| Strategy | HTTP Callback / PubSub execution |
| Observer | Logging & Database notifications |
| Factory | Task creation and validation |
| Command | Task execution, retry and undo |

---

# ⚡ High Level Design

### Min Heap Scheduler

- O(1) next task lookup
- O(log n) insertion
- O(log n) deletion

---

### At-Least-Once Delivery

Google Pub/Sub guarantees messages are delivered even if workers crash.

---

### Idempotency

Each execution receives

```
executionId = taskId + timestamp
```

Duplicate executions are safely ignored.

---

### Horizontal Scaling

Multiple Worker instances subscribe to the same Pub/Sub subscription.

Cloud Run automatically scales based on traffic.

---

### Fault Tolerance

If a worker crashes

- Task remains inside Pub/Sub
- Another worker automatically processes it

No task is lost.

---

# 🔐 Security

- JWT Authentication
- Password Hashing
- Google Secret Manager
- Workload Identity Federation
- Secrets never stored inside source code
- Secure Cloud SQL Connection

---

# 📊 Performance

- O(1) Task Peek
- O(log n) Scheduling
- Automatic Cloud Run Scaling
- Production Ready Docker Images
- Reliable Pub/Sub Messaging
- Efficient Connection Pooling

---

# 📂 Project Structure

```text
task-scheduler/
│
├── api/
│   ├── src/
│   │   ├── config/
│   │   ├── controllers/
│   │   ├── middleware/
│   │   ├── models/
│   │   ├── routes/
│   │   └── app.js
│   ├── package.json
│   └── Dockerfile
│
├── worker/
│   ├── src/
│   │   └── worker.js
│   ├── package.json
│   └── Dockerfile
│
├── cpp-engine/
│   ├── include/
│   ├── src/
│   ├── CMakeLists.txt
│   └── build/
│
├── infrastructure/
│   ├── docker/
│   └── gcp/
│
├── .github/
│   └── workflows/
│       └── deploy.yml
│
├── docker-compose.yml
└── README.md
```

---

# 🚀 Getting Started

## Prerequisites

- Node.js 20+
- Docker
- Google Cloud SDK
- Git
- CMake
- C++17 Compiler

---

## Clone Repository

```bash
git clone https://github.com/manoharathani27/task-scheduler-system.git

cd task-scheduler-system
```

---

## Install Dependencies

### API

```bash
cd api

npm install
```

---

### Worker

```bash
cd worker

npm install
```

---

### C++ Engine

```bash
cd cpp-engine

mkdir build

cd build

cmake ..

make
```

---

## Run Locally

```bash
docker-compose up --build
```

---

# ☁️ Google Cloud Services Used

- Cloud Run
- Cloud SQL
- Pub/Sub
- Artifact Registry
- Secret Manager
- IAM
- Workload Identity Federation

---

# ⚙️ CI/CD Pipeline

```
Push to main
      │
      ▼
GitHub Actions
      │
      ├──────── Test API
      │
      ├──────── Build C++ Engine
      │
      ├──────── Build Docker Images
      │
      ├──────── Push Images to Artifact Registry
      │
      ├──────── Deploy API to Cloud Run
      │
      └──────── Deploy Worker to Cloud Run
```

---

# 📚 REST API

## Authentication

| Method | Endpoint |
|----------|---------------------------|
| POST | /api/auth/register |
| POST | /api/auth/login |

---

## Tasks

| Method | Endpoint |
|----------|-------------------------|
| POST | /api/tasks |
| GET | /api/tasks |
| GET | /api/tasks/:id |
| PUT | /api/tasks/:id |
| DELETE | /api/tasks/:id |
| POST | /api/tasks/:id/pause |
| POST | /api/tasks/:id/resume |

---

## Executions

| Method | Endpoint |
|----------|--------------------------------|
| GET | /api/executions |
| GET | /api/executions/:id |
| POST | /api/executions/record |

---

# 📝 Example Request

```http
POST /api/tasks
Authorization: Bearer <JWT_TOKEN>

{
  "name": "Daily Report",
  "cronExpression": "0 9 * * 1-5",
  "executionStrategy": "HTTP_CALLBACK",
  "callbackUrl": "https://example.com/report",
  "payload": {
      "type":"daily",
      "format":"pdf"
  },
  "maxRetries":3
}
```

---

# 📈 Future Improvements

- Kubernetes Deployment
- Redis Distributed Lock
- Dead Letter Queue
- Prometheus Monitoring
- Grafana Dashboard
- Multi-region Scheduling
- Terraform Infrastructure

---

# 🌟 Highlights

- High-performance C++ scheduling engine
- Distributed microservices architecture
- Google Cloud native deployment
- Automated CI/CD using GitHub Actions
- Dockerized services
- Fault-tolerant task execution
- Production-ready architecture
- Five GoF Design Patterns implemented

---

# 👨‍💻 Author

**Manohar Mahadev Athani**

GitHub: https://github.com/manoharathani27

LinkedIn: https://www.linkedin.com/in/manoharathani

---

# 📄 License

MIT License
