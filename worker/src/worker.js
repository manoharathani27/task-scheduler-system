const express = require('express');
const app = express();
const PORT = process.env.PORT || 8080;
require('dotenv').config();
const { PubSub } = require('@google-cloud/pubsub');
const axios = require('axios');
const winston = require('winston');

/**
 * WHY A SEPARATE WORKER SERVICE:
 * The worker handles actual task execution independently from the API.
 * Benefits:
 * 1. Scale workers independently — 10 workers can process tasks in parallel
 * 2. If a task takes 30 seconds, it doesn't block the API
 * 3. Workers can be deployed in different regions (GCP Cloud Run)
 *
 * WHY PUBSUB SUBSCRIPTION:
 * Workers pull messages from Pub/Sub. If worker crashes mid-execution,
 * Pub/Sub redelivers the message to another worker — no task lost.
 * This is "at-least-once" delivery guarantee.
 */

const logger = winston.createLogger({
  level: 'info',
  format: winston.format.combine(winston.format.timestamp(), winston.format.simple()),
  transports: [new winston.transports.Console()]
});

const pubSubClient = new PubSub({ projectId: process.env.GCP_PROJECT_ID });
const SUBSCRIPTION_NAME = process.env.GCP_PUBSUB_SUBSCRIPTION || 'task-scheduler-subscription';
const API_BASE_URL = process.env.API_BASE_URL;

// Track processed executionIds in memory for short-term dedup
// WHY: Pub/Sub can redeliver same message within seconds.
// This in-memory set prevents duplicate execution in that window.
// Long-term dedup is handled by DB unique constraint on executionId.
const processedExecutions = new Set();

async function executeHttpCallback(task) {
  const startTime = Date.now();
  try {
    logger.info(`Executing HTTP callback for task ${task.taskId} → ${task.callbackUrl}`);
    const response = await axios.post(task.callbackUrl, task.payload || {}, {
      headers: {
        'Content-Type': 'application/json',
        'X-Task-Id': task.taskId,
        'X-Execution-Id': task.executionId  // Idempotency header for receivers
      },
      timeout: 30000  // 30 second timeout
    });

    const durationMs = Date.now() - startTime;
    logger.info(`HTTP callback succeeded for ${task.taskId} in ${durationMs}ms`);

    // Report success back to API
    await reportExecution({
      taskId: task.taskId,
      executionId: task.executionId,
      status: 'SUCCESS',
      durationMs
    });

    return true;
  } catch (err) {
    const durationMs = Date.now() - startTime;
    logger.error(`HTTP callback failed for ${task.taskId}: ${err.message}`);

    await reportExecution({
      taskId: task.taskId,
      executionId: task.executionId,
      status: 'FAILED',
      durationMs,
      errorMessage: err.message
    });

    return false;
  }
}

async function reportExecution(data) {
  try {
    await axios.post(`${API_BASE_URL}/api/executions/record`, data);
  } catch (err) {
    logger.error(`Failed to report execution: ${err.message}`);
  }
}

async function processMessage(message) {
  let taskData;
  try {
    taskData = JSON.parse(message.data.toString());
  } catch {
    logger.error('Failed to parse Pub/Sub message');
    message.ack();  // Ack to prevent redelivery of malformed messages
    return;
  }

  const { action, taskId, executionId } = taskData;
  logger.info(`Received message — Action: ${action} | TaskId: ${taskId}`);

  // Idempotency check — skip if already processed
  if (executionId && processedExecutions.has(executionId)) {
    logger.info(`Duplicate execution detected: ${executionId} — skipping`);
    message.ack();
    return;
  }

  if (executionId) processedExecutions.add(executionId);

  // Handle different actions
  if (action === 'EXECUTE' || action === 'REGISTER') {
    if (taskData.strategy === 'HTTP_CALLBACK' && taskData.callbackUrl) {
      await executeHttpCallback(taskData);
    } else {
      // For PUBSUB strategy, this IS the execution — log it
      logger.info(`PubSub task executed: ${taskId} | Payload: ${JSON.stringify(taskData.payload)}`);
      await reportExecution({ taskId, executionId, status: 'SUCCESS', durationMs: 0 });
    }
  } else if (action === 'PAUSE') {
    logger.info(`Task paused: ${taskId}`);
  } else if (action === 'RESUME') {
    logger.info(`Task resumed: ${taskId}`);
  } else if (action === 'DELETE') {
    logger.info(`Task deleted: ${taskId}`);
  }

  // ACK — tells Pub/Sub this message was handled successfully
  // WHY ACK: Without ack, Pub/Sub redelivers after ackDeadline (default 10-600s)
  message.ack();

  // Cleanup old entries from in-memory set (prevent memory leak)
  if (processedExecutions.size > 10000) {
    const entries = Array.from(processedExecutions);
    entries.slice(0, 5000).forEach(e => processedExecutions.delete(e));
  }
}

async function startWorker() {
  const subscription = pubSubClient.subscription(SUBSCRIPTION_NAME);

  subscription.on('message', processMessage);

  subscription.on('error', (err) => {
    logger.error(`Subscription error: ${err.message}`);
  });

  logger.info(`Worker listening on subscription: ${SUBSCRIPTION_NAME}`);
  logger.info('Waiting for tasks...');
}

startWorker().catch(err => {
  logger.error('Worker failed to start:', err);
  process.exit(1);
});

// Graceful shutdown
process.on('SIGTERM', () => {
  logger.info('Worker shutting down gracefully');
  process.exit(0);
});
