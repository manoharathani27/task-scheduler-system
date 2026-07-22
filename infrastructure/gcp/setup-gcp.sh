#!/bin/bash
# WHY THIS SCRIPT:
# Sets up all GCP resources needed for the project.
# Run this ONCE before first deployment.
# Everything is reproducible — delete and recreate anytime.

set -e  # Exit on any error

PROJECT_ID=$1
REGION="asia-south1"

if [ -z "$PROJECT_ID" ]; then
  echo "Usage: ./setup-gcp.sh YOUR_PROJECT_ID"
  exit 1
fi

echo "Setting up GCP resources for project: $PROJECT_ID"

# Set project
gcloud config set project $PROJECT_ID

# Enable required APIs
# WHY: GCP APIs are disabled by default for security.
# We enable only what we need.
echo "Enabling GCP APIs..."
gcloud services enable \
  run.googleapis.com \
  pubsub.googleapis.com \
  artifactregistry.googleapis.com \
  sqladmin.googleapis.com \
  secretmanager.googleapis.com \
  cloudbuild.googleapis.com

# Create Artifact Registry repository
# WHY: Private Docker image registry inside GCP
echo "Creating Artifact Registry..."
gcloud artifacts repositories create task-scheduler \
  --repository-format=docker \
  --location=$REGION \
  --description="Task Scheduler Docker images" 2>/dev/null || echo "Registry already exists"

# Create Pub/Sub topic
# WHY: Message bus between API, C++ engine, and workers
echo "Creating Pub/Sub topic..."
gcloud pubsub topics create task-scheduler-topic 2>/dev/null || echo "Topic already exists"

# Create Pub/Sub subscription
# WHY: Workers subscribe here to receive task messages
echo "Creating Pub/Sub subscription..."
gcloud pubsub subscriptions create task-scheduler-subscription \
  --topic=task-scheduler-topic \
  --ack-deadline=60 \
  --message-retention-duration=7d 2>/dev/null || echo "Subscription already exists"

# Create Cloud SQL (PostgreSQL) instance
# WHY CLOUD SQL: Managed PostgreSQL — automatic backups, failover, patching
echo "Creating Cloud SQL instance (this takes ~5 minutes)..."
gcloud sql instances create task-scheduler-db \
  --database-version=POSTGRES_15 \
  --tier=db-f1-micro \
  --region=$REGION \
  --storage-type=SSD \
  --storage-size=10GB 2>/dev/null || echo "SQL instance already exists"

# Create database
gcloud sql databases create task_scheduler_db \
  --instance=task-scheduler-db 2>/dev/null || echo "Database already exists"

# Store secrets in Secret Manager
# WHY SECRET MANAGER: Never store secrets in code or env vars.
# Cloud Run pulls secrets at runtime from Secret Manager.
echo "Creating secrets in Secret Manager..."
echo -n "your-db-password" | gcloud secrets create db-password \
  --data-file=- 2>/dev/null || echo "Secret already exists"

echo -n "your-jwt-secret-key-change-this" | gcloud secrets create jwt-secret \
  --data-file=- 2>/dev/null || echo "Secret already exists"

# Create service account for GitHub Actions
# WHY: Least privilege — GitHub Actions only gets permissions it needs
echo "Creating service account for CI/CD..."
gcloud iam service-accounts create github-actions-sa \
  --display-name="GitHub Actions Service Account" 2>/dev/null || echo "SA already exists"

SA_EMAIL="github-actions-sa@${PROJECT_ID}.iam.gserviceaccount.com"

# Grant necessary roles
gcloud projects add-iam-policy-binding $PROJECT_ID \
  --member="serviceAccount:${SA_EMAIL}" \
  --role="roles/run.admin"

gcloud projects add-iam-policy-binding $PROJECT_ID \
  --member="serviceAccount:${SA_EMAIL}" \
  --role="roles/artifactregistry.writer"

gcloud projects add-iam-policy-binding $PROJECT_ID \
  --member="serviceAccount:${SA_EMAIL}" \
  --role="roles/iam.serviceAccountUser"

echo ""
echo "✅ GCP setup complete!"
echo ""
echo "Next steps:"
echo "1. Set these GitHub Secrets in your repo:"
echo "   GCP_PROJECT_ID = $PROJECT_ID"
echo "   GCP_SERVICE_ACCOUNT = ${SA_EMAIL}"
echo "   GCP_WORKLOAD_IDENTITY_PROVIDER = (see Workload Identity Federation docs)"
echo ""
echo "2. Copy .env.example to .env and fill in your values"
echo "3. Push to main branch — CI/CD will deploy automatically!"
